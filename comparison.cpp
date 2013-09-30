// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "comparison.h"
#include <stdexcept>
#include <numeric>
#include <zen/perf.h>
#include <zen/scope_guard.h>
#include <zen/process_priority.h>
#include <zen/symlink_target.h>
#include <zen/format_unit.h>
#include "algorithm.h"
#include "lib/parallel_scan.h"
#include "lib/resolve_path.h"
#include "lib/dir_exist_async.h"
#include "lib/binary.h"
#include "lib/cmp_filetime.h"
#include "lib/status_handler_impl.h"
#include "lib/parallel_scan.h"

using namespace zen;


std::vector<FolderPairCfg> zen::extractCompareCfg(const MainConfiguration& mainCfg)
{
    //merge first and additional pairs
    std::vector<FolderPairEnh> allPairs;
    allPairs.push_back(mainCfg.firstPair);
    allPairs.insert(allPairs.end(),
                    mainCfg.additionalPairs.begin(), //add additional pairs
                    mainCfg.additionalPairs.end());

    std::vector<FolderPairCfg> output;
    std::transform(allPairs.begin(), allPairs.end(), std::back_inserter(output),
                   [&](const FolderPairEnh& enhPair) -> FolderPairCfg
    {
        const Zstring leftDirFmt  = getFormattedDirectoryName(enhPair.leftDirectory);  //ensure they end with FILE_NAME_SEPARATOR and replace macros
        const Zstring rightDirFmt = getFormattedDirectoryName(enhPair.rightDirectory); //

        return FolderPairCfg(leftDirFmt, rightDirFmt,

        enhPair.altCmpConfig.get() ? enhPair.altCmpConfig->compareVar     : mainCfg.cmpConfig.compareVar,
        enhPair.altCmpConfig.get() ? enhPair.altCmpConfig->handleSymlinks : mainCfg.cmpConfig.handleSymlinks,

        normalizeFilters(mainCfg.globalFilter, enhPair.localFilter),

        enhPair.altSyncConfig.get() ? enhPair.altSyncConfig->directionCfg : mainCfg.syncCfg.directionCfg);
    });
    return output;
}

//------------------------------------------------------------------------------------------
namespace
{
void checkForIncompleteInput(const std::vector<FolderPairCfg>& folderPairsForm, bool& warningInputFieldEmpty, ProcessCallback& callback)
{
    bool havePartialPair = false;
    bool haveFullPair    = false;

    std::for_each(folderPairsForm.begin(), folderPairsForm.end(),
                  [&](const FolderPairCfg& fpCfg)
    {
        if (fpCfg.leftDirectoryFmt.empty() != fpCfg.rightDirectoryFmt.empty())
            havePartialPair = true;
        else if (!fpCfg.leftDirectoryFmt.empty())
            haveFullPair = true;
    });

    if (havePartialPair == haveFullPair) //error if: all empty or exist both full and partial pairs -> support single-dir scenario
        callback.reportWarning(_("A folder input field is empty.") + L" \n\n" +
                               _("The corresponding folder will be considered as empty."), warningInputFieldEmpty);
}

std::set<Zstring, LessFilename> determineExistentDirs(const std::set<Zstring, LessFilename>& dirnames,
                                                      bool allowUserInteraction,
                                                      ProcessCallback& callback)
{
    std::set<Zstring, LessFilename> dirsEx;

    warn_static("retry klappt nicht für [] + env vars!")

    tryReportingError([&]
    {
        std::set<Zstring, LessFilename> missing;
        dirsEx = getExistingDirsUpdating(dirnames, missing, allowUserInteraction, callback); //check *all* directories on each try!
        if (!missing.empty())
        {
            std::wstring msg = _("Cannot find the following folders:") + L"\n";
            for (const Zstring& dirname : missing)
                msg += std::wstring(L"\n") + dirname;
            throw FileError(msg, _("You can ignore this error to consider each folder as empty. The folders then will be created automatically during synchronization."));
        }
    }, callback);

    return dirsEx;
}


//check whether one side is subdirectory of other side (folder pair wise!)
//similar check if one directory is read/written by multiple pairs not before beginning of synchronization
void checkFolderDependency(const std::vector<FolderPairCfg>& folderPairsFmt, bool& warningDependentFolders, ProcessCallback& callback) //returns warning message, empty if all ok
{
    std::vector<std::pair<Zstring, Zstring>> dependentDirs;

    auto dependentDir = [](const Zstring& lhs, const Zstring& rhs)
    {
        return EqualFilename()(Zstring(lhs.c_str(), std::min(lhs.length(), rhs.length())), //note: this is NOT an equivalence relation!
                               Zstring(rhs.c_str(), std::min(lhs.length(), rhs.length())));
    };

    for (const FolderPairCfg& fpCfg : folderPairsFmt)
        if (!fpCfg.leftDirectoryFmt.empty() && !fpCfg.rightDirectoryFmt.empty()) //empty folders names may be accepted by user
        {
            if (dependentDir(fpCfg.leftDirectoryFmt, fpCfg.rightDirectoryFmt)) //test wheter leftDirectory begins with rightDirectory or the other way round
                dependentDirs.push_back(std::make_pair(fpCfg.leftDirectoryFmt, fpCfg.rightDirectoryFmt));
        }

    if (!dependentDirs.empty())
    {
        std::wstring warningMsg = _("The following folders have dependent paths. Be careful when setting up synchronization rules:");
        for (auto it = dependentDirs.begin(); it != dependentDirs.end(); ++it)
            warningMsg += std::wstring(L"\n\n") +
                          it->first + L"\n" +
                          it->second;

        callback.reportWarning(warningMsg, warningDependentFolders);
    }
}


class CmpCallbackImpl : public CompareCallback
{
public:
    CmpCallbackImpl(ProcessCallback& pc, Int64& bytesReported) :
        pc_(pc),
        bytesReported_(bytesReported) {}

    virtual void updateCompareStatus(Int64 bytesDelta)
    {
        //inform about the (differential) processed amount of data
        pc_.updateProcessedData(0, bytesDelta); //throw()! -> ensure client and service provider are in sync!
        bytesReported_ += bytesDelta;           //

        pc_.requestUiRefresh(); //may throw
    }

private:
    ProcessCallback& pc_;
    Int64& bytesReported_;
};


bool filesHaveSameContentUpdating(const Zstring& filename1, const Zstring& filename2, Int64 expectedBytesToCmp, ProcessCallback& pc) //throw FileError
{
    Int64 bytesReported; //amount of bytes that have been compared and communicated to status handler

    //error = unexpected increase of total workload
    zen::ScopeGuard guardStatistics = zen::makeGuard([&] { pc.updateTotalData(0, bytesReported); });

    CmpCallbackImpl callback(pc, bytesReported);
    bool sameContent = filesHaveSameContent(filename1, filename2, callback); //throw FileError

    guardStatistics.dismiss();

    //update statistics to consider the real amount of data processed: consider short-cut behavior if first bytes differ!
    if (bytesReported != expectedBytesToCmp)
        pc.updateTotalData(0, bytesReported - expectedBytesToCmp);

    return sameContent;
}

//#############################################################################################################################

class ComparisonBuffer
{
public:
    ComparisonBuffer(const std::set<DirectoryKey>& keysToRead, size_t fileTimeTol, ProcessCallback& callback);

    //create comparison result table and fill category except for files existing on both sides: undefinedFiles and undefinedLinks are appended!
    std::shared_ptr<BaseDirPair> compareByTimeSize(const FolderPairCfg& fpConfig) const;
    std::list<std::shared_ptr<BaseDirPair>> compareByContent(const std::vector<FolderPairCfg>& workLoad) const;

private:
    ComparisonBuffer(const ComparisonBuffer&);
    ComparisonBuffer& operator=(const ComparisonBuffer&);

    std::shared_ptr<BaseDirPair> performComparison(const FolderPairCfg& fpCfg,
                                                   std::vector<FilePair*>& undefinedFiles,
                                                   std::vector<SymlinkPair*>& undefinedLinks) const;

    std::map<DirectoryKey, DirectoryValue> directoryBuffer; //contains only *existing* directories
    const size_t fileTimeTolerance;
    ProcessCallback& callback_;
};


ComparisonBuffer::ComparisonBuffer(const std::set<DirectoryKey>& keysToRead,
                                   size_t fileTimeTol,
                                   ProcessCallback& callback) :
    fileTimeTolerance(fileTimeTol),
    callback_(callback)
{
    class CbImpl : public FillBufferCallback
    {
    public:
        CbImpl(ProcessCallback& pcb) :
            callback_(pcb),
            itemsReported(0) {}

        virtual void reportStatus(const std::wstring& statusMsg, int itemsTotal)
        {
            callback_.updateProcessedData(itemsTotal - itemsReported, 0); //processed bytes are reported in subfunctions!
            itemsReported = itemsTotal;

            callback_.reportStatus(statusMsg); //may throw
            //callback_.requestUiRefresh(); //already called by reportStatus()
        }

        virtual HandleError reportError(const std::wstring& msg, size_t retryNumber)
        {
            switch (callback_.reportError(msg, retryNumber))
            {
                case ProcessCallback::IGNORE_ERROR:
                    return ON_ERROR_IGNORE;

                case ProcessCallback::RETRY:
                    return ON_ERROR_RETRY;
            }

            assert(false);
            return ON_ERROR_IGNORE;
        }

    private:
        ProcessCallback& callback_;
        int itemsReported;
    } cb(callback);

    fillBuffer(keysToRead, //in
               directoryBuffer, //out
               cb,
               UI_UPDATE_INTERVAL / 2); //every ~50 ms
}


//--------------------assemble conflict descriptions---------------------------

//const wchar_t arrowLeft [] = L"\u2190";
//const wchar_t arrowRight[] = L"\u2192"; unicode arrows -> too small
const wchar_t arrowLeft [] = L"<--";
const wchar_t arrowRight[] = L"-->";


//check for very old dates or date2s in the future
std::wstring getConflictInvalidDate(const Zstring& fileNameFull, Int64 utcTime)
{
    return replaceCpy(_("File %x has an invalid date."), L"%x", fmtFileName(fileNameFull)) + L"\n" +
           _("Date:") + L" " + utcToLocalTimeString(utcTime);
}


//check for changed files with same modification date
std::wstring getConflictSameDateDiffSize(const FilePair& fileObj)
{
    return replaceCpy(_("Files %x have the same date but a different size."), L"%x", fmtFileName(fileObj.getObjRelativeName())) + L"\n" +
           L"    " + arrowLeft  + L" " + _("Date:") + L" " + utcToLocalTimeString(fileObj.getLastWriteTime<LEFT_SIDE >()) + L"    " + _("Size:") + L" " + toGuiString(fileObj.getFileSize<LEFT_SIDE>()) + L"\n" +
           L"    " + arrowRight + L" " + _("Date:") + L" " + utcToLocalTimeString(fileObj.getLastWriteTime<RIGHT_SIDE>()) + L"    " + _("Size:") + L" " + toGuiString(fileObj.getFileSize<RIGHT_SIDE>());
}


inline
std::wstring getDescrDiffMetaShortnameCase(const FileSystemObject& fsObj)
{
    return _("Items differ in attributes only") + L"\n" +
           L"    " + arrowLeft  + L" " + fmtFileName(fsObj.getShortName<LEFT_SIDE >()) + L"\n" +
           L"    " + arrowRight + L" " + fmtFileName(fsObj.getShortName<RIGHT_SIDE>());
}


template <class FileOrLinkPair> inline
std::wstring getDescrDiffMetaDate(const FileOrLinkPair& fileObj)
{
    return _("Items differ in attributes only") + L"\n" +
           L"    " + arrowLeft  + L" " + _("Date:") + L" " + utcToLocalTimeString(fileObj.template getLastWriteTime<LEFT_SIDE >()) + L"\n" +
           L"    " + arrowRight + L" " + _("Date:") + L" " + utcToLocalTimeString(fileObj.template getLastWriteTime<RIGHT_SIDE>());
}

//-----------------------------------------------------------------------------

void categorizeSymlinkByTime(SymlinkPair& linkObj, size_t fileTimeTolerance)
{
    //categorize symlinks that exist on both sides
    switch (CmpFileTime::getResult(linkObj.getLastWriteTime<LEFT_SIDE>(),
                                   linkObj.getLastWriteTime<RIGHT_SIDE>(), fileTimeTolerance))
    {
        case CmpFileTime::TIME_EQUAL:
            //Caveat:
            //1. SYMLINK_EQUAL may only be set if short names match in case: InSyncDir's mapping tables use short name as a key! see db_file.cpp
            //2. harmonize with "bool stillInSync()" in algorithm.cpp

            if (linkObj.getShortName<LEFT_SIDE>() == linkObj.getShortName<RIGHT_SIDE>())
                linkObj.setCategory<FILE_EQUAL>();
            else
                linkObj.setCategoryDiffMetadata(getDescrDiffMetaShortnameCase(linkObj));
            break;

        case CmpFileTime::TIME_LEFT_NEWER:
            linkObj.setCategory<FILE_LEFT_NEWER>();
            break;

        case CmpFileTime::TIME_RIGHT_NEWER:
            linkObj.setCategory<FILE_RIGHT_NEWER>();
            break;

        case CmpFileTime::TIME_LEFT_INVALID:
            linkObj.setCategoryConflict(getConflictInvalidDate(linkObj.getFullName<LEFT_SIDE>(), linkObj.getLastWriteTime<LEFT_SIDE>()));
            break;

        case CmpFileTime::TIME_RIGHT_INVALID:
            linkObj.setCategoryConflict(getConflictInvalidDate(linkObj.getFullName<RIGHT_SIDE>(), linkObj.getLastWriteTime<RIGHT_SIDE>()));
            break;
    }
}


std::shared_ptr<BaseDirPair> ComparisonBuffer::compareByTimeSize(const FolderPairCfg& fpConfig) const
{
    //do basis scan and retrieve files existing on both sides as "compareCandidates"
    std::vector<FilePair*> uncategorizedFiles;
    std::vector<SymlinkPair*> uncategorizedLinks;
    std::shared_ptr<BaseDirPair> output = performComparison(fpConfig, uncategorizedFiles, uncategorizedLinks);

    //finish symlink categorization
    std::for_each(uncategorizedLinks.begin(), uncategorizedLinks.end(),
    [&](SymlinkPair* linkObj) { categorizeSymlinkByTime(*linkObj, fileTimeTolerance); });

    //categorize files that exist on both sides
    std::for_each(uncategorizedFiles.begin(), uncategorizedFiles.end(),
                  [&](FilePair* fileObj)
    {
        switch (CmpFileTime::getResult(fileObj->getLastWriteTime<LEFT_SIDE>(),
                                       fileObj->getLastWriteTime<RIGHT_SIDE>(), fileTimeTolerance))
        {
            case CmpFileTime::TIME_EQUAL:
                //Caveat:
                //1. FILE_EQUAL may only be set if short names match in case: InSyncDir's mapping tables use short name as a key! see db_file.cpp
                //2. FILE_EQUAL is expected to mean identical file sizes! See InSyncFile
                //3. harmonize with "bool stillInSync()" in algorithm.cpp, FilePair::syncTo() in file_hierarchy.cpp
                if (fileObj->getFileSize<LEFT_SIDE>() == fileObj->getFileSize<RIGHT_SIDE>())
                {
                    if (fileObj->getShortName<LEFT_SIDE>() == fileObj->getShortName<RIGHT_SIDE>())
                        fileObj->setCategory<FILE_EQUAL>();
                    else
                        fileObj->setCategoryDiffMetadata(getDescrDiffMetaShortnameCase(*fileObj));
                }
                else
                    fileObj->setCategoryConflict(getConflictSameDateDiffSize(*fileObj)); //same date, different filesize
                break;

            case CmpFileTime::TIME_LEFT_NEWER:
                fileObj->setCategory<FILE_LEFT_NEWER>();
                break;

            case CmpFileTime::TIME_RIGHT_NEWER:
                fileObj->setCategory<FILE_RIGHT_NEWER>();
                break;

            case CmpFileTime::TIME_LEFT_INVALID:
                fileObj->setCategoryConflict(getConflictInvalidDate(fileObj->getFullName<LEFT_SIDE>(), fileObj->getLastWriteTime<LEFT_SIDE>()));
                break;

            case CmpFileTime::TIME_RIGHT_INVALID:
                fileObj->setCategoryConflict(getConflictInvalidDate(fileObj->getFullName<RIGHT_SIDE>(), fileObj->getLastWriteTime<RIGHT_SIDE>()));
                break;
        }
    });
    return output;
}


void categorizeSymlinkByContent(SymlinkPair& linkObj, size_t fileTimeTolerance, ProcessCallback& callback)
{
    //categorize symlinks that exist on both sides
    Zstring targetPathRawL;
    Zstring targetPathRawR;
    Opt<std::wstring> errMsg = tryReportingError([&]
    {
        callback.reportStatus(replaceCpy(_("Resolving symbolic link %x"), L"%x", fmtFileName(linkObj.getFullName<LEFT_SIDE>())));
        targetPathRawL = getSymlinkTargetRaw(linkObj.getFullName<LEFT_SIDE>()); //throw FileError

        callback.reportStatus(replaceCpy(_("Resolving symbolic link %x"), L"%x", fmtFileName(linkObj.getFullName<RIGHT_SIDE>())));
        targetPathRawR = getSymlinkTargetRaw(linkObj.getFullName<RIGHT_SIDE>()); //throw FileError
    }, callback);

    if (errMsg)
        linkObj.setCategoryConflict(*errMsg);
    else
    {
        if (targetPathRawL == targetPathRawR
#ifdef ZEN_WIN //type of symbolic link is relevant for Windows only
            &&
            dirExists(linkObj.getFullName<LEFT_SIDE >()) == //check if dir-symlink
            dirExists(linkObj.getFullName<RIGHT_SIDE>())    //
#endif
           )
        {
            //Caveat:
            //1. SYMLINK_EQUAL may only be set if short names match in case: InSyncDir's mapping tables use short name as a key! see db_file.cpp
            //2. harmonize with "bool stillInSync()" in algorithm.cpp, FilePair::syncTo() in file_hierarchy.cpp

            //symlinks have same "content"
            if (linkObj.getShortName<LEFT_SIDE>() != linkObj.getShortName<RIGHT_SIDE>())
                linkObj.setCategoryDiffMetadata(getDescrDiffMetaShortnameCase(linkObj));
            else if (CmpFileTime::getResult(linkObj.getLastWriteTime<LEFT_SIDE>(),
                                            linkObj.getLastWriteTime<RIGHT_SIDE>(), fileTimeTolerance) != CmpFileTime::TIME_EQUAL)
                linkObj.setCategoryDiffMetadata(getDescrDiffMetaDate(linkObj));
            else
                linkObj.setCategory<FILE_EQUAL>();
        }
        else
            linkObj.setCategory<FILE_DIFFERENT>();
    }
}


std::list<std::shared_ptr<BaseDirPair>> ComparisonBuffer::compareByContent(const std::vector<FolderPairCfg>& workLoad) const
{
    std::list<std::shared_ptr<BaseDirPair>> output;
    if (workLoad.empty())
        return output;

    //PERF_START;
    std::vector<FilePair*> undefinedFiles;

    //process one folder pair after each other
    for (auto it = workLoad.begin(); it != workLoad.end(); ++it)
    {
        std::vector<SymlinkPair*> uncategorizedLinks;
        //do basis scan and retrieve candidates for binary comparison (files existing on both sides)

        output.push_back(performComparison(*it, undefinedFiles, uncategorizedLinks));

        //finish symlink categorization
        std::for_each(uncategorizedLinks.begin(), uncategorizedLinks.end(),
        [&](SymlinkPair* linkObj) { categorizeSymlinkByContent(*linkObj, fileTimeTolerance, callback_); });
    }

    //finish categorization...
    std::vector<FilePair*> filesToCompareBytewise;

    //content comparison of file content happens AFTER finding corresponding files
    //in order to separate into two processes (scanning and comparing)

    std::for_each(undefinedFiles.begin(), undefinedFiles.end(),
                  [&](FilePair* fileObj)
    {
        //pre-check: files have different content if they have a different filesize (must not be FILE_EQUAL: see InSyncFile)
        if (fileObj->getFileSize<LEFT_SIDE>() != fileObj->getFileSize<RIGHT_SIDE>())
            fileObj->setCategory<FILE_DIFFERENT>();
        else
            filesToCompareBytewise.push_back(fileObj);
    });

    const size_t objectsTotal = filesToCompareBytewise.size();
    const UInt64 bytesTotal   =  //left and right filesizes are equal
        std::accumulate(filesToCompareBytewise.begin(), filesToCompareBytewise.end(), static_cast<UInt64>(0),
    [](UInt64 sum, FilePair* fsObj) { return sum + fsObj->getFileSize<LEFT_SIDE>(); });

    callback_.initNewPhase(static_cast<int>(objectsTotal),
                           to<Int64>(bytesTotal),
                           ProcessCallback::PHASE_COMPARING_CONTENT);

    const std::wstring txtComparingContentOfFiles = _("Comparing content of files %x");

    //compare files (that have same size) bytewise...
    std::for_each(filesToCompareBytewise.begin(), filesToCompareBytewise.end(),
                  [&](FilePair* fileObj)
    {
        callback_.reportStatus(replaceCpy(txtComparingContentOfFiles, L"%x", fmtFileName(fileObj->getObjRelativeName()), false));

        //check files that exist in left and right model but have different content

        bool haveSameContent = false;
        Opt<std::wstring> errMsg = tryReportingError([&]
        {
            haveSameContent = filesHaveSameContentUpdating(fileObj->getFullName<LEFT_SIDE>(), //throw FileError
            fileObj->getFullName<RIGHT_SIDE>(),
            to<Int64>(fileObj->getFileSize<LEFT_SIDE>()),
            callback_);

            callback_.updateProcessedData(1, 0); //processed bytes are reported in subfunctions!
            callback_.requestUiRefresh(); //may throw
        }, callback_);

        if (errMsg)
            fileObj->setCategoryConflict(*errMsg);
        else
        {
            if (haveSameContent)
            {
                //Caveat:
                //1. FILE_EQUAL may only be set if short names match in case: InSyncDir's mapping tables use short name as a key! see db_file.cpp
                //2. FILE_EQUAL is expected to mean identical file sizes! See InSyncFile
                //3. harmonize with "bool stillInSync()" in algorithm.cpp, FilePair::syncTo() in file_hierarchy.cpp
                if (fileObj->getShortName<LEFT_SIDE>() != fileObj->getShortName<RIGHT_SIDE>())
                    fileObj->setCategoryDiffMetadata(getDescrDiffMetaShortnameCase(*fileObj));
                else if (CmpFileTime::getResult(fileObj->getLastWriteTime<LEFT_SIDE>(), fileObj->getLastWriteTime<RIGHT_SIDE>(), fileTimeTolerance) != CmpFileTime::TIME_EQUAL)
                    fileObj->setCategoryDiffMetadata(getDescrDiffMetaDate(*fileObj));
                else
                    fileObj->setCategory<FILE_EQUAL>();
            }
            else
                fileObj->setCategory<FILE_DIFFERENT>();
        }
    });
    return output;
}


class MergeSides
{
public:
    MergeSides(std::vector<FilePair*>& appendUndefinedFileOut,
               std::vector<SymlinkPair*>& appendUndefinedLinkOut) :
        appendUndefinedFile(appendUndefinedFileOut),
        appendUndefinedLink(appendUndefinedLinkOut) {}

    void execute(const DirContainer& leftSide, const DirContainer& rightSide, HierarchyObject& output);

private:
    template <SelectedSide side>
    void fillOneSide(const DirContainer& dirCont, HierarchyObject& output);

    std::vector<FilePair*>& appendUndefinedFile;
    std::vector<SymlinkPair*>& appendUndefinedLink;
};


template <SelectedSide side>
void MergeSides::fillOneSide(const DirContainer& dirCont, HierarchyObject& output)
{
    for (const auto& file : dirCont.files)
        output.addSubFile<side>(file.first, file.second);

    for (const auto& link : dirCont.links)
        output.addSubLink<side>(link.first, link.second);

    for (const auto& dir : dirCont.dirs)
    {
        DirPair& newDirMap = output.addSubDir<side>(dir.first);
        fillOneSide<side>(dir.second, newDirMap); //recurse
    }
}


//improve merge-perf by over 70% + more natural default sequence
template <class MapType, class ProcessLeftOnly, class ProcessRightOnly, class ProcessBoth> inline
void linearMerge(const MapType& mapLeft, const MapType& mapRight, ProcessLeftOnly lo, ProcessRightOnly ro, ProcessBoth bo)
{
    const auto lessVal = mapLeft.value_comp();

    auto iterLeft  = mapLeft .begin();
    auto iterRight = mapRight.begin();

    auto finishLeft  = [&] { std::for_each(iterLeft,  mapLeft .end(), lo); };
    auto finishRight = [&] { std::for_each(iterRight, mapRight.end(), ro); };

    if (iterLeft  == mapLeft .end()) return finishRight();
    if (iterRight == mapRight.end()) return finishLeft();

    for (;;)
        if (lessVal(*iterLeft, *iterRight))
        {
            lo(*iterLeft);
            if (++iterLeft == mapLeft.end())
                return finishRight();
        }
        else if (lessVal(*iterRight, *iterLeft))
        {
            ro(*iterRight);
            if (++iterRight == mapRight.end())
                return finishLeft();
        }
        else
        {
            bo(*iterLeft, *iterRight);
            ++iterLeft;  //
            ++iterRight; //increment BOTH before checking for end of range!
            if (iterLeft  == mapLeft .end()) return finishRight();
            if (iterRight == mapRight.end()) return finishLeft();
        }
}


void MergeSides::execute(const DirContainer& leftSide, const DirContainer& rightSide, HierarchyObject& output)
{
    //HierarchyObject::addSubFile() must NOT invalidate references used in "appendUndefined"!

    typedef const DirContainer::FileList::value_type FileData;

    linearMerge(leftSide.files, rightSide.files,
    [&](const FileData& fileLeft ) { output.addSubFile<LEFT_SIDE >(fileLeft .first, fileLeft .second); }, //left only
    [&](const FileData& fileRight) { output.addSubFile<RIGHT_SIDE>(fileRight.first, fileRight.second); }, //right only

    [&](const FileData& fileLeft, const FileData& fileRight) //both sides
    {
        FilePair& newEntry = output.addSubFile(fileLeft.first,
                                               fileLeft.second,
                                               FILE_EQUAL, //FILE_EQUAL is just a dummy-value here
                                               fileRight.first,
                                               fileRight.second);
        appendUndefinedFile.push_back(&newEntry);
    });

    //-----------------------------------------------------------------------------------------------
    typedef const DirContainer::LinkList::value_type LinkData;

    linearMerge(leftSide.links, rightSide.links,
    [&](const LinkData& linkLeft)  { output.addSubLink<LEFT_SIDE >(linkLeft.first, linkLeft.second);   }, //left only
    [&](const LinkData& linkRight) { output.addSubLink<RIGHT_SIDE>(linkRight.first, linkRight.second); }, //right only

    [&](const LinkData& linkLeft, const LinkData& linkRight) //both sides
    {
        SymlinkPair& newEntry = output.addSubLink(linkLeft.first,
                                                  linkLeft.second,
                                                  SYMLINK_EQUAL, //SYMLINK_EQUAL is just a dummy-value here
                                                  linkRight.first,
                                                  linkRight.second);
        appendUndefinedLink.push_back(&newEntry);
    });

    //-----------------------------------------------------------------------------------------------
    typedef const DirContainer::DirList::value_type DirData;

    linearMerge(leftSide.dirs, rightSide.dirs,
                [&](const DirData& dirLeft) //left only
    {
        DirPair& newDirMap = output.addSubDir<LEFT_SIDE>(dirLeft.first);
        this->fillOneSide<LEFT_SIDE>(dirLeft.second, newDirMap); //recurse into subdirectories
    },
    [&](const DirData& dirRight) //right only
    {
        DirPair& newDirMap = output.addSubDir<RIGHT_SIDE>(dirRight.first);
        this->fillOneSide<RIGHT_SIDE>(dirRight.second, newDirMap); //recurse into subdirectories
    },

    [&](const DirData& dirLeft, const DirData& dirRight) //both sides
    {
        DirPair& newDirMap = output.addSubDir(dirLeft.first, dirRight.first, DIR_EQUAL);
        if (dirLeft.first != dirRight.first)
            newDirMap.setCategoryDiffMetadata(getDescrDiffMetaShortnameCase(newDirMap));

        execute(dirLeft.second, dirRight.second, newDirMap); //recurse into subdirectories
    });
}

//mark excluded directories (see fillBuffer()) + remove superfluous excluded subdirectories
void removeFilteredDirs(HierarchyObject& hierObj, const HardFilter& filterProc)
{
    //process subdirs recursively
    for (DirPair& dirObj : hierObj.refSubDirs())
    {
        dirObj.setActive(filterProc.passDirFilter(dirObj.getObjRelativeName(), nullptr)); //subObjMightMatch is always true in this context!
        removeFilteredDirs(dirObj, filterProc);
    }

    //remove superfluous directories -> note: this does not invalidate "std::vector<FilePair*>& undefinedFiles", since we delete folders only
    //and there is no side-effect for memory positions of FilePair and SymlinkPair thanks to zen::FixedList!
    hierObj.refSubDirs().remove_if([](DirPair& dirObj)
    {
        return !dirObj.isActive() &&
               dirObj.refSubDirs ().empty() &&
               dirObj.refSubLinks().empty() &&
               dirObj.refSubFiles().empty();
    });
}


//create comparison result table and fill category except for files existing on both sides: undefinedFiles and undefinedLinks are appended!
std::shared_ptr<BaseDirPair> ComparisonBuffer::performComparison(const FolderPairCfg& fpCfg,
                                                                 std::vector<FilePair*>& undefinedFiles,
                                                                 std::vector<SymlinkPair*>& undefinedLinks) const
{
    callback_.reportStatus(_("Generating file list..."));
    callback_.forceUiRefresh();

    auto getDirValue = [&](const Zstring& dirnameFmt) -> const DirectoryValue*
    {
        auto it = directoryBuffer.find(DirectoryKey(dirnameFmt, fpCfg.filter.nameFilter, fpCfg.handleSymlinks));
        return it != directoryBuffer.end() ? &it->second : nullptr;
    };

    const DirectoryValue* bufValueLeft  = getDirValue(fpCfg.leftDirectoryFmt);
    const DirectoryValue* bufValueRight = getDirValue(fpCfg.rightDirectoryFmt);

    Zstring filterFailedRead;
    auto filterAddFailedDirReads = [&filterFailedRead](const std::set<Zstring>& failedDirReads) //exclude directory child items only!
    {
        //note: relDirPf is empty for base dir, otherwise postfixed! e.g. "subdir\"
        //an empty relDirPf is a pathological case at this point, since determineExistentDirs() already filtered missing base directories!
        std::for_each(failedDirReads .begin(), failedDirReads .end(), [&](const Zstring& relDirPf) { filterFailedRead += relDirPf + Zstr("?*\n"); });
    };
    auto filterAddFailedItemReads = [&filterFailedRead](const std::set<Zstring>& failedItemReads) //exclude item AND (potential) child items!
    {
        std::for_each(failedItemReads.begin(), failedItemReads.end(), [&](const Zstring& relItem ) { filterFailedRead += relItem + Zstr("\n"); });
    };

    if (bufValueLeft ) filterAddFailedDirReads(bufValueLeft ->failedDirReads);
    if (bufValueRight) filterAddFailedDirReads(bufValueRight->failedDirReads);

    if (bufValueLeft ) filterAddFailedItemReads(bufValueLeft ->failedItemReads);
    if (bufValueRight) filterAddFailedItemReads(bufValueRight->failedItemReads);

    std::shared_ptr<BaseDirPair> output = std::make_shared<BaseDirPair>(fpCfg.leftDirectoryFmt,
                                                                        bufValueLeft != nullptr, //dir existence must be checked only once: available iff buffer entry exists!
                                                                        fpCfg.rightDirectoryFmt,
                                                                        bufValueRight != nullptr,
                                                                        fpCfg.filter.nameFilter,
                                                                        fpCfg.compareVar,
                                                                        fileTimeTolerance);
    //PERF_START;
    MergeSides(undefinedFiles, undefinedLinks).execute(bufValueLeft  ? bufValueLeft ->dirCont : DirContainer(),
                                                       bufValueRight ? bufValueRight->dirCont : DirContainer(), *output);
    //PERF_STOP;

    //##################### in/exclude rows according to filtering #####################

    //attention: some excluded directories are still in the comparison result! (see include filter handling!)
    if (!fpCfg.filter.nameFilter->isNull())
        removeFilteredDirs(*output, *fpCfg.filter.nameFilter); //mark excluded directories (see fillBuffer()) + remove superfluous excluded subdirectories

    //apply soft filtering (hard filter already applied during traversal!)
    addSoftFiltering(*output, fpCfg.filter.timeSizeFilter);

    //handle (user-ignored) traversing errors: just uncheck them, no need to physically delete them from both sides
    if (!filterFailedRead.empty())
        addHardFiltering(*output, filterFailedRead);

    //##################################################################################
    return output;
}
}


void zen::compare(size_t fileTimeTolerance,
                  xmlAccess::OptionalDialogs& warnings,
                  bool allowUserInteraction,
                  bool runWithBackgroundPriority,
                  bool createDirLocks,
                  std::unique_ptr<LockHolder>& dirLocks,
                  const std::vector<FolderPairCfg>& cfgList,
                  FolderComparison& output,
                  ProcessCallback& callback)
{
    //specify process and resource handling priorities
    std::unique_ptr<ScheduleForBackgroundProcessing> backgroundPrio;
    if (runWithBackgroundPriority)
        try
        {
            backgroundPrio = make_unique<ScheduleForBackgroundProcessing>(); //throw FileError
        }
        catch (const FileError& e)
        {
            //not an error in this context
            callback.reportInfo(e.toString()); //may throw!
        }

    //prevent operating system going into sleep state
    std::unique_ptr<PreventStandby> noStandby;
    try
    {
        noStandby = make_unique<PreventStandby>(); //throw FileError
    }
    catch (const FileError& e)
    {
        //not an error in this context
        callback.reportInfo(e.toString()); //may throw!
    }

    //PERF_START;

    callback.reportInfo(_("Starting comparison")); //we want some indicator at the very beginning of the log to make sense of "total time"

    //init process: keep at beginning so that all gui elements are initialized properly
    callback.initNewPhase(-1, 0, ProcessCallback::PHASE_SCANNING); //it's not known how many files will be scanned => -1 objects

    //-------------------some basic checks:------------------------------------------

    checkForIncompleteInput(cfgList, warnings.warningInputFieldEmpty,  callback);
    checkFolderDependency  (cfgList, warnings.warningDependentFolders, callback);

    std::set<Zstring, LessFilename> dirnamesExisting;
    //list of directories that are *expected* to be existent (and need to be scanned)!
    //directory existence only checked *once* to avoid race conditions!
    {
        std::set<Zstring, LessFilename> dirnames;
        std::for_each(cfgList.begin(), cfgList.end(),
                      [&](const FolderPairCfg& fpCfg)
        {
            dirnames.insert(fpCfg.leftDirectoryFmt);
            dirnames.insert(fpCfg.rightDirectoryFmt);
        });
        dirnamesExisting = determineExistentDirs(dirnames, allowUserInteraction, callback);
    }

    auto dirAvailable = [&](const Zstring& dirnameFmt) { return dirnamesExisting.find(dirnameFmt) != dirnamesExisting.end(); };

    //-------------------end of basic checks------------------------------------------

    try
    {
        //lock (existing) directories before comparison
        if (createDirLocks)
            dirLocks = make_unique<LockHolder>(dirnamesExisting, warnings.warningDirectoryLockFailed, callback);

        //------------------- fill directory buffer ---------------------------------------------------
        std::set<DirectoryKey> dirsToRead;

        std::for_each(cfgList.begin(), cfgList.end(),
                      [&](const FolderPairCfg& fpCfg)
        {
            if (dirAvailable(fpCfg.leftDirectoryFmt)) //only request *currently existing* directories: at this point user is aware that non-ex + empty string are seen as empty folder!
                dirsToRead.insert(DirectoryKey(fpCfg.leftDirectoryFmt,  fpCfg.filter.nameFilter, fpCfg.handleSymlinks));
            if (dirAvailable(fpCfg.rightDirectoryFmt))
                dirsToRead.insert(DirectoryKey(fpCfg.rightDirectoryFmt, fpCfg.filter.nameFilter, fpCfg.handleSymlinks));
        });

        FolderComparison outputTmp; //write to output as a transaction!

        //reduce peak memory by restricting lifetime of ComparisonBuffer to have ended when loading potentially huge InSyncDir instance in redetermineSyncDirection()
        {
            //------------ traverse/read folders -----------------------------------------------------
            ComparisonBuffer cmpBuff(dirsToRead, fileTimeTolerance, callback);

            //process binary comparison in one block
            std::vector<FolderPairCfg> workLoadByContent;
            for (const FolderPairCfg& fpCfg : cfgList)
                switch (fpCfg.compareVar)
                {
                    case CMP_BY_TIME_SIZE:
                        break;
                    case CMP_BY_CONTENT:
                        workLoadByContent.push_back(fpCfg);
                        break;
                }
            std::list<std::shared_ptr<BaseDirPair>> outputByContent = cmpBuff.compareByContent(workLoadByContent);

            //write output in order
            for (const FolderPairCfg& fpCfg : cfgList)
                switch (fpCfg.compareVar)
                {
                    case CMP_BY_TIME_SIZE:
                        outputTmp.push_back(cmpBuff.compareByTimeSize(fpCfg));
                        break;
                    case CMP_BY_CONTENT:
                        assert(!outputByContent.empty());
                        if (!outputByContent.empty())
                        {
                            outputTmp.push_back(outputByContent.front());
                            outputByContent.pop_front();
                        }
                        break;
                }
        }

        assert(outputTmp.size() == cfgList.size());

        //--------- set initial sync-direction --------------------------------------------------

        for (auto j = begin(outputTmp); j != end(outputTmp); ++j)
        {
            const FolderPairCfg& fpCfg = cfgList[j - outputTmp.begin()];

            callback.reportStatus(_("Calculating sync directions..."));
            callback.forceUiRefresh();
            zen::redetermineSyncDirection(fpCfg.directionCfg, *j,
            [&](const std::wstring& warning) { callback.reportWarning(warning, warnings.warningDatabaseError); });
        }

        //only if everything was processed correctly output is written to!
        //note: output mustn't change during this process to be in sync with GUI grid view!!!
        outputTmp.swap(output);
    }
    catch (const std::bad_alloc& e)
    {
        callback.reportFatalError(_("Out of memory.") + L" " + utfCvrtTo<std::wstring>(e.what()));
    }
    catch (const std::exception& e)
    {
        callback.reportFatalError(utfCvrtTo<std::wstring>(e.what()));
    }
}
