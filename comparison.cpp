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
void checkForIncompleteInput(const std::vector<FolderPairCfg>& folderPairsForm, ProcessCallback& callback)
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

    tryReportingError([&]
    {
        if (havePartialPair == haveFullPair) //error if: all empty or exist both full and partial pairs -> support single-dir scenario
            throw FileError(_("A folder input field is empty.") + L" \n\n" +
            _("You can ignore this error to consider the folder as empty."));
    }, callback);
}


void determineExistentDirs(const std::set<Zstring, LessFilename>& dirnames,
                           std::set<Zstring, LessFilename>& dirnamesExisting,
                           bool allowUserInteraction,
                           ProcessCallback& callback)
{
    std::vector<Zstring> dirs(dirnames.begin(), dirnames.end());
    vector_remove_if(dirs, [](const Zstring& dir) { return dir.empty(); });



    warn_static("finish")
    /*
    //check existence of all directories in parallel! (avoid adding up search times if multiple network drives are not reachable)
    FixedList<boost::unique_future<bool>> asyncDirChecks;
    std::for_each(dirs.begin(), dirs.end(), [&](const Zstring& dirname)
    {
    asyncDirChecks.emplace_back(async([=]() -> bool
    {
    #ifdef FFS_WIN
        //1. login to network share, if necessary
        loginNetworkShare(dirname, allowUserInteraction);
    #endif
        //2. check dir existence
        return zen::dirExists(dirname);
    }));
    });

    auto timeMax = boost::get_system_time() + boost::posix_time::seconds(10); //limit total directory search time

    auto iterCheckDir = asyncDirChecks.begin();
    for (auto iter = dirs.begin(); iter != dirs.end(); ++iter, ++iterCheckDir)
    {
    const Zstring& dirname = *iter;
    callback.reportStatus(replaceCpy(_("Searching for folder %x..."), L"%x", fmtFileName(dirname), false));

    while (boost::get_system_time() < timeMax && !iterCheckDir->timed_wait(boost::posix_time::milliseconds(UI_UPDATE_INTERVAL)))
        callback.requestUiRefresh(); //may throw!

    //only (still) existing files should be included in the list
    if (iterCheckDir->is_ready() && iterCheckDir->get())
        dirnamesExisting.insert(dirname);
    else
    {

        switch (callback.reportError(replaceCpy(_("Cannot find folder %x."), L"%x", fmtFileName(dirname)) + L"\n\n" +
                _("You can ignore this error to consider the folder as empty."))) //may throw!
        {
            case ProcessCallback::IGNORE_ERROR:
                break;
            case ProcessCallback::RETRY:
                break; //continue with loop
        }


        if (tryReportingError([&]
    {
        if (!dirExistsUpdating(dirname, allowUserInteraction, callback))
                throw FileError();




        }, callback))
        dirnamesExisting.insert(dirname);
    }
    }
    */










    std::for_each(dirs.begin(), dirs.end(),
                  [&](const Zstring& dirname)
    {
        if (tryReportingError([&]
    {
        if (!dirExistsUpdating(dirname, allowUserInteraction, callback))
                throw FileError(replaceCpy(_("Cannot find folder %x."), L"%x", fmtFileName(dirname)) + L"\n\n" +
                _("You can ignore this error to consider the folder as empty."));
        }, callback))
        dirnamesExisting.insert(dirname);
    });
}


//check whether one side is subdirectory of other side (folder pair wise!)
//similar check if one directory is read/written by multiple pairs not before beginning of synchronization
std::wstring checkFolderDependency(const std::vector<FolderPairCfg>& folderPairsForm) //returns warning message, empty if all ok
{
    std::vector<std::pair<Zstring, Zstring>> dependentDirs;

    auto dependentDir = [](const Zstring& lhs, const Zstring& rhs)
    {
        return EqualFilename()(Zstring(lhs.c_str(), std::min(lhs.length(), rhs.length())), //note: this is NOT an equivalence relation!
                               Zstring(rhs.c_str(), std::min(lhs.length(), rhs.length())));
    };

    for (std::vector<FolderPairCfg>::const_iterator i = folderPairsForm.begin(); i != folderPairsForm.end(); ++i)
        if (!i->leftDirectoryFmt.empty() && !i->rightDirectoryFmt.empty()) //empty folders names may be accepted by user
        {
            if (dependentDir(i->leftDirectoryFmt, i->rightDirectoryFmt)) //test wheter leftDirectory begins with rightDirectory or the other way round
                dependentDirs.push_back(std::make_pair(i->leftDirectoryFmt, i->rightDirectoryFmt));
        }

    std::wstring warningMsg;

    if (!dependentDirs.empty())
    {
        warningMsg = _("Directories are dependent! Be careful when setting up synchronization rules:");
        for (auto i = dependentDirs.begin(); i != dependentDirs.end(); ++i)
            warningMsg += L"\n\n" +
                          fmtFileName(i->first) + L"\n" +
                          fmtFileName(i->second);
    }
    return warningMsg;
}


//callback implementation
class CmpCallbackImpl : public CompareCallback
{
public:
    CmpCallbackImpl(ProcessCallback& pc, UInt64& bytesReported) :
        pc_(pc),
        bytesReported_(bytesReported) {}

    virtual void updateCompareStatus(UInt64 totalBytes)
    {
        //inform about the (differential) processed amount of data
        pc_.updateProcessedData(0, to<Int64>(totalBytes) - to<Int64>(bytesReported_)); //throw()! -> this ensures client and service provider are in sync!
        bytesReported_ = totalBytes;                                                   //

        pc_.requestUiRefresh(); //may throw
    }

private:
    ProcessCallback& pc_;
    UInt64& bytesReported_;
};


bool filesHaveSameContentUpdating(const Zstring& filename1, const Zstring& filename2, UInt64 totalBytesToCmp, ProcessCallback& pc) //throw FileError
{
    UInt64 bytesReported; //amount of bytes that have been compared and communicated to status handler

    //in error situation: undo communication of processed amount of data
    zen::ScopeGuard guardStatistics = zen::makeGuard([&] { pc.updateProcessedData(0, -1 * to<Int64>(bytesReported)); });

    CmpCallbackImpl callback(pc, bytesReported);
    bool sameContent = filesHaveSameContent(filename1, filename2, callback); //throw FileError

    //inform about the (remaining) processed amount of data
    pc.updateProcessedData(0, to<Int64>(totalBytesToCmp) - to<Int64>(bytesReported));
    bytesReported = totalBytesToCmp;

    guardStatistics.dismiss();
    return sameContent;
}
}

//#############################################################################################################################

class ComparisonBuffer
{
public:
    ComparisonBuffer(const std::set<DirectoryKey>& keysToRead,
                     size_t fileTimeTol,
                     ProcessCallback& callback);

    //create comparison result table and fill category except for files existing on both sides: undefinedFiles and undefinedLinks are appended!
    void compareByTimeSize(const FolderPairCfg& fpConfig, BaseDirMapping& output);
    void compareByContent(std::vector<std::pair<FolderPairCfg, BaseDirMapping*>>& workLoad);

private:
    ComparisonBuffer(const ComparisonBuffer&);
    ComparisonBuffer& operator=(const ComparisonBuffer&);

    void performComparison(const FolderPairCfg& fpCfg,
                           BaseDirMapping& output,
                           std::vector<FileMapping*>& undefinedFiles,
                           std::vector<SymLinkMapping*>& undefinedLinks);

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
            callback_.updateProcessedData(itemsTotal - itemsReported, 0); //processed data is communicated in subfunctions!
            itemsReported = itemsTotal;

            callback_.reportStatus(statusMsg); //may throw
            //callback_.requestUiRefresh(); //already called by reportStatus()
        }

        virtual HandleError reportError(const std::wstring& msg)
        {
            switch (callback_.reportError(msg))
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
               UI_UPDATE_INTERVAL / 4); //every ~25 ms
}


void zen::compare(size_t fileTimeTolerance,
                  xmlAccess::OptionalDialogs& warnings,
                  bool allowUserInteraction,
                  bool runWithBackgroundPriority,
                  const std::vector<FolderPairCfg>& cfgList,
                  FolderComparison& output,
                  ProcessCallback& callback)
{
    //specify process and resource handling priorities
    std::unique_ptr<ScheduleForBackgroundProcessing> backgroundPrio;
    if (runWithBackgroundPriority)
        backgroundPrio.reset(new ScheduleForBackgroundProcessing);

    //prevent operating system going into sleep state
    PreventStandby dummy2;
    (void)dummy2;

    //PERF_START;

    callback.reportInfo(_("Start comparison")); //we want some indicator at the very beginning to make sense of "total time"

    //init process: keep at beginning so that all gui elements are initialized properly
    callback.initNewPhase(-1, 0, ProcessCallback::PHASE_SCANNING); //it's not known how many files will be scanned => -1 objects

    //-------------------some basic checks:------------------------------------------

    checkForIncompleteInput(cfgList, callback);


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
        determineExistentDirs(dirnames, dirnamesExisting, allowUserInteraction, callback);
    }
    auto dirAvailable = [&](const Zstring& dirnameFmt) { return dirnamesExisting.find(dirnameFmt) != dirnamesExisting.end(); };


    {
        //check if folders have dependencies
        std::wstring warningMessage = checkFolderDependency(cfgList);
        if (!warningMessage.empty())
            callback.reportWarning(warningMessage, warnings.warningDependentFolders);
    }

    //-------------------end of basic checks------------------------------------------

    try
    {
        //------------------- fill directory buffer ---------------------------------------------------
        std::set<DirectoryKey> keysToRead;

        std::for_each(cfgList.begin(), cfgList.end(),
                      [&](const FolderPairCfg& fpCfg)
        {
            if (dirAvailable(fpCfg.leftDirectoryFmt)) //only request *currently existing * directories: at this point user is aware that non-ex + empty string are seen as empty folder!
                keysToRead.insert(DirectoryKey(fpCfg.leftDirectoryFmt,  fpCfg.filter.nameFilter, fpCfg.handleSymlinks));
            if (dirAvailable(fpCfg.rightDirectoryFmt))
                keysToRead.insert(DirectoryKey(fpCfg.rightDirectoryFmt, fpCfg.filter.nameFilter, fpCfg.handleSymlinks));
        });

        ComparisonBuffer cmpBuff(keysToRead,
                                 fileTimeTolerance,
                                 callback);

        //------------ traverse/read folders -----------------------------------------------------

        FolderComparison output_tmp; //write to output not before END of process!

        //buffer "config"/"result of binary comparison" for latter processing as a single block
        std::vector<std::pair<FolderPairCfg, BaseDirMapping*>> workLoadBinary;

        std::for_each(cfgList.begin(), cfgList.end(),
                      [&](const FolderPairCfg& fpCfg)
        {
            //a pity VC11 screws up on std::make_shared with 7 arguments...
            output_tmp.push_back(std::shared_ptr<BaseDirMapping>(new BaseDirMapping(fpCfg.leftDirectoryFmt,
                                                                                    dirAvailable(fpCfg.leftDirectoryFmt),
                                                                                    fpCfg.rightDirectoryFmt,
                                                                                    dirAvailable(fpCfg.rightDirectoryFmt),
                                                                                    fpCfg.filter.nameFilter,
                                                                                    fpCfg.compareVar,
                                                                                    fileTimeTolerance)));
            switch (fpCfg.compareVar)
            {
                case CMP_BY_TIME_SIZE:
                    cmpBuff.compareByTimeSize(fpCfg, *output_tmp.back());
                    break;
                case CMP_BY_CONTENT:
                    workLoadBinary.push_back(std::make_pair(fpCfg, &*output_tmp.back()));
                    break;
            }
        });
        //process binary comparison in one block
        cmpBuff.compareByContent(workLoadBinary);

        assert(output_tmp.size() == cfgList.size());

        //--------- set initial sync-direction --------------------------------------------------

        for (auto j = begin(output_tmp); j != end(output_tmp); ++j)
        {
            const FolderPairCfg& fpCfg = cfgList[j - output_tmp.begin()];

            callback.reportStatus(_("Preparing synchronization..."));
            callback.forceUiRefresh();
            zen::redetermineSyncDirection(fpCfg.directionCfg, *j,
            [&](const std::wstring& warning) { callback.reportWarning(warning, warnings.warningDatabaseError); });
        }

        //only if everything was processed correctly output is written to!
        //note: output mustn't change during this process to be in sync with GUI grid view!!!
        output_tmp.swap(output);
    }
    catch (const std::bad_alloc& e)
    {
        callback.reportFatalError(_("Out of memory!") + L" " + utfCvrtTo<std::wstring>(e.what()));
    }
    catch (const std::exception& e)
    {
        callback.reportFatalError(utfCvrtTo<std::wstring>(e.what()));
    }
}

//--------------------assemble conflict descriptions---------------------------

namespace
{
//const wchar_t arrowLeft [] = L"\u2190";
//const wchar_t arrowRight[] = L"\u2192"; unicode arrows -> too small
const wchar_t arrowLeft [] = L"<--";
const wchar_t arrowRight[] = L"-->";


//check for very old dates or date2s in the future
std::wstring getConflictInvalidDate(const Zstring& fileNameFull, Int64 utcTime)
{
    return _("Conflict detected:") + L"\n" +
           replaceCpy(_("File %x has an invalid date!"), L"%x", fmtFileName(fileNameFull)) + L"\n" +
           _("Date:") + L" " + utcToLocalTimeString(utcTime);
}


//check for changed files with same modification date
std::wstring getConflictSameDateDiffSize(const FileMapping& fileObj)
{
    return _("Conflict detected:") + L"\n" +
           replaceCpy(_("Files %x have the same date but a different size!"), L"%x", fmtFileName(fileObj.getObjRelativeName())) + L"\n" +
           arrowLeft  + L"    " + _("Date:") + L" " + utcToLocalTimeString(fileObj.getLastWriteTime<LEFT_SIDE >()) + L"    " + _("Size:") + L" " + toGuiString(fileObj.getFileSize<LEFT_SIDE>()) + L"\n" +
           arrowRight + L"    " + _("Date:") + L" " + utcToLocalTimeString(fileObj.getLastWriteTime<RIGHT_SIDE>()) + L"    " + _("Size:") + L" " + toGuiString(fileObj.getFileSize<RIGHT_SIDE>());
}


inline
std::wstring getDescrDiffMetaShortnameCase(const FileSystemObject& fsObj)
{
    return _("Items differ in attributes only") + L"\n" +
           arrowLeft  + L"    " + fmtFileName(fsObj.getShortName<LEFT_SIDE >()) + L"\n" +
           arrowRight + L"    " + fmtFileName(fsObj.getShortName<RIGHT_SIDE>());
}


template <class FileOrLinkMapping> inline
std::wstring getDescrDiffMetaDate(const FileOrLinkMapping& fileObj)
{
    return _("Items differ in attributes only") + L"\n" +
           arrowLeft  + L"    " + _("Date:") + L" " + utcToLocalTimeString(fileObj.template getLastWriteTime<LEFT_SIDE >()) + L"\n" +
           arrowRight + L"    " + _("Date:") + L" " + utcToLocalTimeString(fileObj.template getLastWriteTime<RIGHT_SIDE>());
}
}

//-----------------------------------------------------------------------------

namespace
{
void categorizeSymlinkByTime(SymLinkMapping& linkObj, size_t fileTimeTolerance)
{
    //categorize symlinks that exist on both sides
    switch (CmpFileTime::getResult(linkObj.getLastWriteTime<LEFT_SIDE>(),
                                   linkObj.getLastWriteTime<RIGHT_SIDE>(), fileTimeTolerance))
    {
        case CmpFileTime::TIME_EQUAL:
            if (linkObj.getTargetPath<LEFT_SIDE>().empty())
                linkObj.setCategoryConflict(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(linkObj.getFullName<LEFT_SIDE>())));
            else if (linkObj.getTargetPath<RIGHT_SIDE>().empty())
                linkObj.setCategoryConflict(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(linkObj.getFullName<RIGHT_SIDE>())));

            else if (
#ifdef FFS_WIN //type of symbolic link is relevant for Windows only
                linkObj.getLinkType<LEFT_SIDE>() == linkObj.getLinkType<RIGHT_SIDE>() &&
#endif
                linkObj.getTargetPath<LEFT_SIDE>() == linkObj.getTargetPath<RIGHT_SIDE>()) //may both be empty if reading link content failed
            {
                //Caveat:
                //1. SYMLINK_EQUAL may only be set if short names match in case: InSyncDir's mapping tables use short name as a key! see db_file.cpp
                //2. harmonize with "bool stillInSync()" in algorithm.cpp

                if (linkObj.getShortName<LEFT_SIDE>() == linkObj.getShortName<RIGHT_SIDE>())
                    linkObj.setCategory<FILE_EQUAL>();
                else
                    linkObj.setCategoryDiffMetadata(getDescrDiffMetaShortnameCase(linkObj));
            }
            else
                linkObj.setCategoryConflict(_("Conflict detected:") + L"\n" +
                                            replaceCpy(_("Symbolic links %x have the same date but a different target."), L"%x", fmtFileName(linkObj.getObjRelativeName())));
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
}

void ComparisonBuffer::compareByTimeSize(const FolderPairCfg& fpConfig, BaseDirMapping& output)
{
    //do basis scan and retrieve files existing on both sides as "compareCandidates"
    std::vector<FileMapping*> uncategorizedFiles;
    std::vector<SymLinkMapping*> uncategorizedLinks;
    performComparison(fpConfig, output, uncategorizedFiles, uncategorizedLinks);

    //finish symlink categorization
    std::for_each(uncategorizedLinks.begin(), uncategorizedLinks.end(),
    [&](SymLinkMapping* linkMap) { categorizeSymlinkByTime(*linkMap, fileTimeTolerance); });

    //categorize files that exist on both sides
    std::for_each(uncategorizedFiles.begin(), uncategorizedFiles.end(),
                  [&](FileMapping* fileObj)
    {
        switch (CmpFileTime::getResult(fileObj->getLastWriteTime<LEFT_SIDE>(),
                                       fileObj->getLastWriteTime<RIGHT_SIDE>(), fileTimeTolerance))
        {
            case CmpFileTime::TIME_EQUAL:
                if (fileObj->getFileSize<LEFT_SIDE>() == fileObj->getFileSize<RIGHT_SIDE>())
                {
                    //Caveat:
                    //1. FILE_EQUAL may only be set if short names match in case: InSyncDir's mapping tables use short name as a key! see db_file.cpp
                    //2. harmonize with "bool stillInSync()" in algorithm.cpp

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
}

namespace
{
void categorizeSymlinkByContent(SymLinkMapping& linkObj, size_t fileTimeTolerance)
{
    //categorize symlinks that exist on both sides
    if (linkObj.getTargetPath<LEFT_SIDE>().empty())
        linkObj.setCategoryConflict(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(linkObj.getFullName<LEFT_SIDE>())));
    else if (linkObj.getTargetPath<RIGHT_SIDE>().empty())
        linkObj.setCategoryConflict(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(linkObj.getFullName<RIGHT_SIDE>())));
    //if one of these is empty it's handled like reading "file content" failed

    else if (
#ifdef FFS_WIN //type of symbolic link is relevant for Windows only
        linkObj.getLinkType<LEFT_SIDE>() == linkObj.getLinkType<RIGHT_SIDE>() &&
#endif
        linkObj.getTargetPath<LEFT_SIDE>() == linkObj.getTargetPath<RIGHT_SIDE>()) //may both be empty if determination failed!!!
    {
        //Caveat:
        //1. SYMLINK_EQUAL may only be set if short names match in case: InSyncDir's mapping tables use short name as a key! see db_file.cpp
        //2. harmonize with "bool stillInSync()" in algorithm.cpp

        //symlinks have same "content"
        if (linkObj.getShortName<LEFT_SIDE>() != linkObj.getShortName<RIGHT_SIDE>())
            linkObj.setCategoryDiffMetadata(getDescrDiffMetaShortnameCase(linkObj));
        else if (CmpFileTime::getResult(linkObj.getLastWriteTime<LEFT_SIDE>(), linkObj.getLastWriteTime<RIGHT_SIDE>(), fileTimeTolerance) != CmpFileTime::TIME_EQUAL)
            linkObj.setCategoryDiffMetadata(getDescrDiffMetaDate(linkObj));
        else
            linkObj.setCategory<FILE_EQUAL>();
    }
    else
        linkObj.setCategory<FILE_DIFFERENT>();
}
}

void ComparisonBuffer::compareByContent(std::vector<std::pair<FolderPairCfg, BaseDirMapping*>>& workLoad)
{
    if (workLoad.empty()) return;

    //PERF_START;
    std::vector<FileMapping*> undefinedFiles;

    //process one folder pair after each other
    for (auto workItem = workLoad.begin(); workItem != workLoad.end(); ++workItem)
    {
        std::vector<SymLinkMapping*> uncategorizedLinks;
        //do basis scan and retrieve candidates for binary comparison (files existing on both sides)

        performComparison(workItem->first, *workItem->second, undefinedFiles, uncategorizedLinks);

        //finish symlink categorization
        std::for_each(uncategorizedLinks.begin(), uncategorizedLinks.end(),
        [&](SymLinkMapping* linkMap) { categorizeSymlinkByContent(*linkMap, fileTimeTolerance); });
    }

    //finish categorization...
    std::vector<FileMapping*> filesToCompareBytewise;

    //content comparison of file content happens AFTER finding corresponding files
    //in order to separate into two processes (scanning and comparing)

    std::for_each(undefinedFiles.begin(), undefinedFiles.end(),
                  [&](FileMapping* fileObj)
    {
        //pre-check: files have different content if they have a different filesize
        if (fileObj->getFileSize<LEFT_SIDE>() != fileObj->getFileSize<RIGHT_SIDE>())
            fileObj->setCategory<FILE_DIFFERENT>();
        else
            filesToCompareBytewise.push_back(fileObj);
    });

    const size_t objectsTotal = filesToCompareBytewise.size();
    const UInt64 bytesTotal   =  //left and right filesizes are equal
        std::accumulate(filesToCompareBytewise.begin(), filesToCompareBytewise.end(), static_cast<UInt64>(0),
    [](UInt64 sum, FileMapping* fsObj) { return sum + fsObj->getFileSize<LEFT_SIDE>(); });

    callback_.initNewPhase(static_cast<int>(objectsTotal),
                           to<Int64>(bytesTotal),
                           ProcessCallback::PHASE_COMPARING_CONTENT);

    const std::wstring txtComparingContentOfFiles = replaceCpy(_("Comparing content of files %x"), L"%x", L"\n%x", false);

    //compare files (that have same size) bytewise...
    std::for_each(filesToCompareBytewise.begin(), filesToCompareBytewise.end(),
                  [&](FileMapping* fileObj)
    {
        callback_.reportStatus(replaceCpy(txtComparingContentOfFiles, L"%x", fmtFileName(fileObj->getObjRelativeName()), false));

        //check files that exist in left and right model but have different content

        if (!tryReportingError([&]
    {
        if (filesHaveSameContentUpdating(fileObj->getFullName<LEFT_SIDE>(), //throw FileError
            fileObj->getFullName<RIGHT_SIDE>(),
            fileObj->getFileSize<LEFT_SIDE >(),
            callback_))
            {
                //Caveat:
                //1. FILE_EQUAL may only be set if short names match in case: InSyncDir's mapping tables use short name as a key! see db_file.cpp
                //2. harmonize with "bool stillInSync()" in algorithm.cpp
                if (fileObj->getShortName<LEFT_SIDE>() != fileObj->getShortName<RIGHT_SIDE>())
                    fileObj->setCategoryDiffMetadata(getDescrDiffMetaShortnameCase(*fileObj));
                else if (CmpFileTime::getResult(fileObj->getLastWriteTime<LEFT_SIDE>(), fileObj->getLastWriteTime<RIGHT_SIDE>(), fileTimeTolerance) != CmpFileTime::TIME_EQUAL)
                    fileObj->setCategoryDiffMetadata(getDescrDiffMetaDate(*fileObj));
                else
                    fileObj->setCategory<FILE_EQUAL>();
            }
            else
                fileObj->setCategory<FILE_DIFFERENT>();

            callback_.updateProcessedData(1, 0); //processed data is communicated in subfunctions!

        }, callback_))
        fileObj->setCategoryConflict(_("Conflict detected:") + L"\n" + _("Comparing files by content failed."));

        callback_.requestUiRefresh(); //may throw
    });
}


namespace
{
class MergeSides
{
public:
    MergeSides(std::vector<FileMapping*>& appendUndefinedFileOut,
               std::vector<SymLinkMapping*>& appendUndefinedLinkOut) :
        appendUndefinedFile(appendUndefinedFileOut),
        appendUndefinedLink(appendUndefinedLinkOut) {}

    void execute(const DirContainer& leftSide, const DirContainer& rightSide, HierarchyObject& output);

private:
    template <SelectedSide side>
    void fillOneSide(const DirContainer& dirCont, HierarchyObject& output);

    std::vector<FileMapping*>& appendUndefinedFile;
    std::vector<SymLinkMapping*>& appendUndefinedLink;
};


template <SelectedSide side>
void MergeSides::fillOneSide(const DirContainer& dirCont, HierarchyObject& output)
{
    for (auto iter = dirCont.files.cbegin(); iter != dirCont.files.cend(); ++iter)
        output.addSubFile<side>(iter->first, iter->second);

    for (auto iter = dirCont.links.cbegin(); iter != dirCont.links.cend(); ++iter)
        output.addSubLink<side>(iter->first, iter->second);

    for (auto iter = dirCont.dirs.cbegin(); iter != dirCont.dirs.cend(); ++iter)
    {
        DirMapping& newDirMap = output.addSubDir<side>(iter->first);
        fillOneSide<side>(iter->second, newDirMap); //recurse
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
        FileMapping& newEntry = output.addSubFile(fileLeft.first,
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
        SymLinkMapping& newEntry = output.addSubLink(linkLeft.first,
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
        DirMapping& newDirMap = output.addSubDir<LEFT_SIDE>(dirLeft.first);
        this->fillOneSide<LEFT_SIDE>(dirLeft.second, newDirMap); //recurse into subdirectories
    },
    [&](const DirData& dirRight) //right only
    {
        DirMapping& newDirMap = output.addSubDir<RIGHT_SIDE>(dirRight.first);
        this->fillOneSide<RIGHT_SIDE>(dirRight.second, newDirMap); //recurse into subdirectories
    },

    [&](const DirData& dirLeft, const DirData& dirRight) //both sides
    {
        DirMapping& newDirMap = output.addSubDir(dirLeft.first, dirRight.first, DIR_EQUAL);
        if (dirLeft.first != dirRight.first)
            newDirMap.setCategoryDiffMetadata(getDescrDiffMetaShortnameCase(newDirMap));

        execute(dirLeft.second, dirRight.second, newDirMap); //recurse into subdirectories
    });
}

//mark excluded directories (see fillBuffer()) + remove superfluous excluded subdirectories
//note: this cannot be done while traversing directory, since both sides need to be taken into account, both for filtering AND removing subdirs!
void processFilteredDirs(HierarchyObject& hierObj, const HardFilter& filterProc)
{
    auto& subDirs = hierObj.refSubDirs();

    //process subdirs recursively
    std::for_each(subDirs.begin(), subDirs.end(),
                  [&](DirMapping& dirObj)
    {
        dirObj.setActive(filterProc.passDirFilter(dirObj.getObjRelativeName(), nullptr)); //subObjMightMatch is always true in this context!
        processFilteredDirs(dirObj, filterProc);
    });

    //remove superfluous directories -> note: this does not invalidate "std::vector<FileMapping*>& undefinedFiles", since we delete folders only
    //and there is no side-effect for memory positions of FileMapping and SymlinkMapping thanks to zen::FixedList!
    subDirs.remove_if([](DirMapping& dirObj)
    {
        return !dirObj.isActive() &&
               dirObj.refSubDirs ().empty() &&
               dirObj.refSubLinks().empty() &&
               dirObj.refSubFiles().empty();
    });
}
}


//create comparison result table and fill category except for files existing on both sides: undefinedFiles and undefinedLinks are appended!
void ComparisonBuffer::performComparison(const FolderPairCfg& fpCfg,
                                         BaseDirMapping& output,
                                         std::vector<FileMapping*>& undefinedFiles,
                                         std::vector<SymLinkMapping*>& undefinedLinks)
{
    assert(output.refSubDirs(). empty());
    assert(output.refSubLinks().empty());
    assert(output.refSubFiles().empty());

    //PERF_START;

    DirectoryValue emptyDummy;
    auto getDirValue = [&](const Zstring& dirnameFmt) -> const DirectoryValue&
    {
        auto iter = directoryBuffer.find(DirectoryKey(dirnameFmt, fpCfg.filter.nameFilter, fpCfg.handleSymlinks));
        return iter == directoryBuffer.end() ? emptyDummy : iter->second;
    };

    const DirectoryValue& bufValueLeft  = getDirValue(fpCfg.leftDirectoryFmt);
    const DirectoryValue& bufValueRight = getDirValue(fpCfg.rightDirectoryFmt);

    callback_.reportStatus(_("Generating file list..."));
    callback_.forceUiRefresh();

    //PERF_START;
    MergeSides(undefinedFiles, undefinedLinks).execute(bufValueLeft.dirCont, bufValueRight.dirCont, output);
    //PERF_STOP;

    //##################### in/exclude rows according to filtering #####################

    //attention: some excluded directories are still in the comparison result! (see include filter handling!)
    if (!fpCfg.filter.nameFilter->isNull())
        processFilteredDirs(output, *fpCfg.filter.nameFilter); //mark excluded directories (see fillBuffer()) + remove superfluous excluded subdirectories

    //apply soft filtering / hard filter already applied
    addSoftFiltering(output, fpCfg.filter.timeSizeFilter);

    //properly handle (user-ignored) traversing errors: just uncheck them, no need to physically delete them (<automatic> mode will be grateful)
    std::set<Zstring> failedReads;
    failedReads.insert(bufValueLeft .failedReads.begin(), bufValueLeft .failedReads.end());
    failedReads.insert(bufValueRight.failedReads.begin(), bufValueRight.failedReads.end());

    if (!failedReads.empty())
    {
        Zstring filterFailedRead;
        //exclude subfolders only
        std::for_each(failedReads.begin(), failedReads.end(),
        [&](const Zstring& relDirPf) { filterFailedRead += relDirPf + Zstr("?*\n"); });
        //note: relDirPf is empty for base dir, otherwise postfixed! e.g. "subdir\"

        addHardFiltering(output, filterFailedRead);
    }
    //##################################################################################
}
