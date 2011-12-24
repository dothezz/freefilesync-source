// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "comparison.h"
#include <stdexcept>
#include <zen/scope_guard.h>
#include <wx+/string_conv.h>
#include <wx+/format_unit.h>
#include "lib/parallel_scan.h"
#include "lib/resolve_path.h"
#include "lib/dir_exist_async.h"
#include "lib/binary.h"
#include "lib/cmp_filetime.h"
#include "algorithm.h"

#ifdef FFS_WIN
#include <zen/perf.h>
#endif

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

        return FolderPairCfg(leftDirFmt,
        rightDirFmt,

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
void checkForIncompleteInput(const std::vector<FolderPairCfg>& folderPairsForm, ProcessCallback& procCallback)
{
    int partiallyFilledPairs = 0;
    int totallyFilledPairs   = 0;

    std::for_each(folderPairsForm.begin(), folderPairsForm.end(),
                  [&](const FolderPairCfg& fpCfg)
    {
        if (fpCfg.leftDirectoryFmt.empty() != fpCfg.rightDirectoryFmt.empty())
            ++partiallyFilledPairs;

        if (!fpCfg.leftDirectoryFmt.empty() && !fpCfg.rightDirectoryFmt.empty())
            ++totallyFilledPairs;
    });

    //check for empty entries
    if ((totallyFilledPairs + partiallyFilledPairs == 0) || //all empty
        (partiallyFilledPairs > 0 && //partial entry is invalid
         !(totallyFilledPairs == 0 && partiallyFilledPairs == 1))) //exception: one partial pair okay: one-dir only scenario
    {
        while (true)
        {
            const std::wstring additionalInfo = _("You can ignore this error to consider the directory as empty.");
            const ProcessCallback::Response rv = procCallback.reportError(_("A directory input field is empty.") + L" \n\n" +
                                                                          + L"(" + additionalInfo + L")");
            if (rv == ProcessCallback::IGNORE_ERROR)
                break;
            else if (rv == ProcessCallback::RETRY)
                ;  //continue with loop
            else
                throw std::logic_error("Programming Error: Unknown return value! (1)");
        }
    }
}


void checkDirectoryExistence(const std::set<Zstring, LessFilename>& dirnames,
                             std::set<Zstring, LessFilename>& dirnamesExisting,
                             bool allowUserInteraction,
                             ProcessCallback& procCallback)
{
    std::for_each(dirnames.begin(), dirnames.end(),
                  [&](const Zstring& dirname)
    {
        if (!dirname.empty())
        {
            while (!dirExistsUpdating(dirname, allowUserInteraction, procCallback))
            {
                const std::wstring additionalInfo = _("You can ignore this error to consider the directory as empty.");
                std::wstring errorMessage = _("Directory does not exist:") + L"\n" + L"\"" + dirname + L"\"";
                ProcessCallback::Response rv = procCallback.reportError(errorMessage + L"\n\n" + additionalInfo /* + L" " + getLastErrorFormatted()*/);

                if (rv == ProcessCallback::IGNORE_ERROR)
                    return;
                else if (rv == ProcessCallback::RETRY)
                    ;  //continue with loop
                else
                    throw std::logic_error("Programming Error: Unknown return value! (2)");
            }

            dirnamesExisting.insert(dirname);
        }
    });
}


//check whether one side is subdirectory of other side (folder pair wise!)
//similar check if one directory is read/written by multiple pairs not before beginning of synchronization
std::wstring checkFolderDependency(const std::vector<FolderPairCfg>& folderPairsForm) //returns warning message, empty if all ok
{
    typedef std::vector<std::pair<std::wstring, std::wstring> > DirDirList;
    DirDirList dependentDirs;

    auto dependentDir = [](const Zstring& lhs, const Zstring& rhs)
    {
        return EqualFilename()(Zstring(lhs.c_str(), std::min(lhs.length(), rhs.length())), //note: this is NOT an equivalence relation!
                               Zstring(rhs.c_str(), std::min(lhs.length(), rhs.length())));
    };

    for (std::vector<FolderPairCfg>::const_iterator i = folderPairsForm.begin(); i != folderPairsForm.end(); ++i)
        if (!i->leftDirectoryFmt.empty() && !i->rightDirectoryFmt.empty()) //empty folders names may be accepted by user
        {
            if (dependentDir(i->leftDirectoryFmt, i->rightDirectoryFmt)) //test wheter leftDirectory begins with rightDirectory or the other way round
                dependentDirs.push_back(std::make_pair(utf8CvrtTo<std::wstring>(i->leftDirectoryFmt), utf8CvrtTo<std::wstring>(i->rightDirectoryFmt)));
        }

    std::wstring warningMsg;

    if (!dependentDirs.empty())
    {
        warningMsg = _("Directories are dependent! Be careful when setting up synchronization rules:");
        for (auto i = dependentDirs.begin(); i != dependentDirs.end(); ++i)
            warningMsg += std::wstring(L"\n\n") +
                          L"\"" + i->first  + L"\"\n" +
                          L"\"" + i->second + L"\"";
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

    virtual void updateCompareStatus(UInt64 totalBytesTransferred)
    {
        //inform about the (differential) processed amount of data
        pc_.updateProcessedData(0, to<Int64>(totalBytesTransferred) - to<Int64>(bytesReported_)); //throw()! -> this ensures client and service provider are in sync!
        bytesReported_ = totalBytesTransferred;                                                   //

        pc_.requestUiRefresh(); //may throw
    }

private:
    ProcessCallback& pc_;
    UInt64& bytesReported_;
};


bool filesHaveSameContentUpdating(const Zstring& filename1, const Zstring& filename2, UInt64 totalBytesToCmp, ProcessCallback& pc)
{
    UInt64 bytesReported; //amount of bytes that have been compared and communicated to status handler

    //in error situation: undo communication of processed amount of data
    zen::ScopeGuard guardStatistics = zen::makeGuard([&]() { pc.updateProcessedData(0, -1 * to<Int64>(bytesReported)); });

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

CompareProcess::CompareProcess(size_t fileTimeTol,
                               xmlAccess::OptionalDialogs& warnings,
                               bool allowUserInteraction,
                               bool runWithBackgroundPriority,
                               ProcessCallback& handler) :
    fileTimeTolerance(fileTimeTol),
    m_warnings(warnings),
    allowUserInteraction_(allowUserInteraction),
    procCallback(handler)
{
    if (runWithBackgroundPriority)
        procBackground.reset(new ScheduleForBackgroundProcessing);
}


void CompareProcess::startCompareProcess(const std::vector<FolderPairCfg>& cfgList, FolderComparison& output)
{
    //prevent shutdown while (binary) comparison is in progress
    PreventStandby dummy2;
    (void)dummy2;

    /*
    #ifdef NDEBUG
        wxLogNull noWxLogs; //hide wxWidgets log messages in release build
    #endif
    */
    //PERF_START;

    //init process: keep at beginning so that all gui elements are initialized properly
    procCallback.initNewProcess(-1, 0, ProcessCallback::PROCESS_SCANNING); //it's not known how many files will be scanned => -1 objects

    //-------------------some basic checks:------------------------------------------

    checkForIncompleteInput(cfgList, procCallback);


    std::set<Zstring, LessFilename> dirnamesExisting; //list of directories that are *expected* to be existent (and need to be scanned)!
    {
        std::set<Zstring, LessFilename> dirnames;
        std::for_each(cfgList.begin(), cfgList.end(),
                      [&](const FolderPairCfg& fpCfg)
        {
            dirnames.insert(fpCfg.leftDirectoryFmt);
            dirnames.insert(fpCfg.rightDirectoryFmt);
        });
        checkDirectoryExistence(dirnames, dirnamesExisting, allowUserInteraction_, procCallback);
    }
    auto dirAvailable = [&](const Zstring& dirnameFmt) { return dirnamesExisting.find(dirnameFmt) != dirnamesExisting.end(); };


    {
        //check if folders have dependencies
        std::wstring warningMessage = checkFolderDependency(cfgList);
        if (!warningMessage.empty())
            procCallback.reportWarning(warningMessage, m_warnings.warningDependentFolders);
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

        class CbImpl : public FillBufferCallback
        {
        public:
            CbImpl(ProcessCallback& pcb) :
                procCallback_(pcb),
                itemsReported(0) {}

            virtual void reportStatus(const std::wstring& statusMsg, int itemTotal)
            {
                procCallback_.updateProcessedData(itemTotal - itemsReported, 0); //processed data is communicated in subfunctions!
                itemsReported = itemTotal;

                procCallback_.reportStatus(statusMsg); //may throw
                //procCallback_.requestUiRefresh(); //already called by reportStatus()
            }

            virtual HandleError reportError(const std::wstring& errorText)
            {
                switch (procCallback_.reportError(errorText))
                {
                    case ProcessCallback::IGNORE_ERROR:
                        return TRAV_ERROR_IGNORE;

                    case ProcessCallback::RETRY:
                        return TRAV_ERROR_RETRY;
                }

                assert(false);
                return TRAV_ERROR_IGNORE;
            }

        private:
            ProcessCallback& procCallback_;
            int itemsReported;
        } cb(procCallback);

        fillBuffer(keysToRead, //in
                   directoryBuffer, //out
                   cb,
                   UI_UPDATE_INTERVAL / 4); //every ~25 ms
        //-------------------------------------------------------------------------------------------

        //traverse/process folders
        FolderComparison output_tmp; //write to output not before END of process!


        //buffer "config"/"result of binary comparison" for latter processing as a single block
        std::vector<std::pair<FolderPairCfg, BaseDirMapping*>> workLoadBinary;

        std::for_each(cfgList.begin(), cfgList.end(),
                      [&](const FolderPairCfg& fpCfg)
        {
            output_tmp.push_back(std::make_shared<BaseDirMapping>(fpCfg.leftDirectoryFmt,
                                                                  dirAvailable(fpCfg.leftDirectoryFmt),
                                                                  fpCfg.rightDirectoryFmt,
                                                                  dirAvailable(fpCfg.rightDirectoryFmt)));
            switch (fpCfg.compareVar)
            {
                case CMP_BY_TIME_SIZE:
                    compareByTimeSize(fpCfg, *output_tmp.back());
                    break;
                case CMP_BY_CONTENT:
                    workLoadBinary.push_back(std::make_pair(fpCfg, &*output_tmp.back()));
                    break;
            }
        });
        //process binary comparison in one block
        compareByContent(workLoadBinary);


        assert(output_tmp.size() == cfgList.size());

        for (auto j = begin(output_tmp); j != end(output_tmp); ++j)
        {
            const FolderPairCfg& fpCfg = cfgList[j - output_tmp.begin()];

            //set initial sync-direction
            class RedetermineCallback : public DeterminationProblem
            {
            public:
                RedetermineCallback(bool& warningSyncDatabase, ProcessCallback& procCallback) :
                    warningSyncDatabase_(warningSyncDatabase),
                    procCallback_(procCallback) {}

                virtual void reportWarning(const std::wstring& text)
                {
                    procCallback_.reportWarning(text, warningSyncDatabase_);
                }
            private:
                bool& warningSyncDatabase_;
                ProcessCallback& procCallback_;
            } redetCallback(m_warnings.warningSyncDatabase, procCallback);

            zen::redetermineSyncDirection(fpCfg.directionCfg, *j, &redetCallback);
        }

        //only if everything was processed correctly output is written to!
        //note: output mustn't change during this process to be in sync with GUI grid view!!!
        output_tmp.swap(output);
    }
    catch (const std::bad_alloc& e)
    {
        procCallback.reportFatalError(_("Memory allocation failed!") + L" " + utf8CvrtTo<std::wstring>(e.what()));
    }
    catch (const std::exception& e)
    {
        procCallback.reportFatalError(utf8CvrtTo<std::wstring>(e.what()));
    }
}

//--------------------assemble conflict descriptions---------------------------

//check for very old dates or date2s in the future
std::wstring getConflictInvalidDate(const Zstring& fileNameFull, Int64 utcTime)
{
    std::wstring msg = _("File %x has an invalid date!");
    replace(msg, L"%x", std::wstring(L"\"") + fileNameFull + L"\"");
    msg += L"\n\n" + _("Date") + L": " + utcToLocalTimeString(utcTime);
    return _("Conflict detected:") + L"\n" + msg;
}


namespace
{
//check for changed files with same modification date
std::wstring getConflictSameDateDiffSize(const FileMapping& fileObj)
{
    std::wstring msg = _("Files %x have the same date but a different size!");
    replace(msg, L"%x", std::wstring(L"\"") + fileObj.getRelativeName<LEFT_SIDE>() + L"\"");
    msg += L"\n\n";
    msg += L"<--    " + _("Date") + L": " + utcToLocalTimeString(fileObj.getLastWriteTime<LEFT_SIDE >()) + L"    " + _("Size") + L": " + toStringSep(fileObj.getFileSize<LEFT_SIDE>()) + L"\n";
    msg += L"-->    " + _("Date") + L": " + utcToLocalTimeString(fileObj.getLastWriteTime<RIGHT_SIDE>()) + L"    " + _("Size") + L": " + toStringSep(fileObj.getFileSize<RIGHT_SIDE>());
    return _("Conflict detected:") + L"\n" + msg;
}
}


//-----------------------------------------------------------------------------
void CompareProcess::categorizeSymlinkByTime(SymLinkMapping& linkObj) const
{
    const CmpFileTime timeCmp(fileTimeTolerance);

    //categorize symlinks that exist on both sides
    if ( //special handling: if symlinks have the same "content" they are seen as equal while other metadata is ignored
#ifdef FFS_WIN //type of symbolic link is relevant for Windows only
        linkObj.getLinkType<LEFT_SIDE>() == linkObj.getLinkType<RIGHT_SIDE>() &&
#endif
        !linkObj.getTargetPath<LEFT_SIDE>().empty() &&
        linkObj.getTargetPath<LEFT_SIDE>() == linkObj.getTargetPath<RIGHT_SIDE>())
    {
        //symlinks have same "content"
        if (linkObj.getShortName<LEFT_SIDE>() == linkObj.getShortName<RIGHT_SIDE>() &&
            timeCmp.getResult(linkObj.getLastWriteTime<LEFT_SIDE>(),
                              linkObj.getLastWriteTime<RIGHT_SIDE>()) == CmpFileTime::TIME_EQUAL)
            linkObj.setCategory<SYMLINK_EQUAL>();
        else
            linkObj.setCategory<SYMLINK_DIFFERENT_METADATA>();
        return;
    }

    switch (timeCmp.getResult(linkObj.getLastWriteTime<LEFT_SIDE>(),
                              linkObj.getLastWriteTime<RIGHT_SIDE>()))
    {
        case CmpFileTime::TIME_EQUAL:
            if (
#ifdef FFS_WIN //type of symbolic link is relevant for Windows only
                linkObj.getLinkType<LEFT_SIDE>() == linkObj.getLinkType<RIGHT_SIDE>() &&
#endif
                linkObj.getTargetPath<LEFT_SIDE>() == linkObj.getTargetPath<RIGHT_SIDE>()) //may both be empty if following link failed
            {
                if (linkObj.getShortName<LEFT_SIDE>() == linkObj.getShortName<RIGHT_SIDE>())
                    linkObj.setCategory<SYMLINK_EQUAL>();
                else
                    linkObj.setCategory<SYMLINK_DIFFERENT_METADATA>();
            }
            else
            {
                std::wstring conflictMsg = _("Conflict detected:") + L"\n" + _("Symlinks %x have the same date but a different target!");
                replace(conflictMsg, L"%x", std::wstring(L"\"") + linkObj.getRelativeName<LEFT_SIDE>() + L"\"");
                linkObj.setCategoryConflict(conflictMsg);
            }
            break;

        case CmpFileTime::TIME_LEFT_NEWER:
            linkObj.setCategory<SYMLINK_LEFT_NEWER>();
            break;

        case CmpFileTime::TIME_RIGHT_NEWER:
            linkObj.setCategory<SYMLINK_RIGHT_NEWER>();
            break;

        case CmpFileTime::TIME_LEFT_INVALID:
            linkObj.setCategoryConflict(getConflictInvalidDate(linkObj.getFullName<LEFT_SIDE>(), linkObj.getLastWriteTime<LEFT_SIDE>()));
            break;

        case CmpFileTime::TIME_RIGHT_INVALID:
            linkObj.setCategoryConflict(getConflictInvalidDate(linkObj.getFullName<RIGHT_SIDE>(), linkObj.getLastWriteTime<RIGHT_SIDE>()));
            break;
    }
}


void CompareProcess::compareByTimeSize(const FolderPairCfg& fpConfig, BaseDirMapping& output)
{
    //do basis scan and retrieve files existing on both sides as "compareCandidates"
    std::vector<FileMapping*> uncategorizedFiles;
    std::vector<SymLinkMapping*> uncategorizedLinks;
    performComparison(fpConfig, output, uncategorizedFiles, uncategorizedLinks);

    //finish symlink categorization
    std::for_each(uncategorizedLinks.begin(), uncategorizedLinks.end(),
    [&](SymLinkMapping* linkMap) { this->categorizeSymlinkByTime(*linkMap); });

    //categorize files that exist on both sides
    const CmpFileTime timeCmp(fileTimeTolerance);

    std::for_each(uncategorizedFiles.begin(), uncategorizedFiles.end(),
                  [&](FileMapping* fileObj)
    {
        switch (timeCmp.getResult(fileObj->getLastWriteTime<LEFT_SIDE>(),
                                  fileObj->getLastWriteTime<RIGHT_SIDE>()))
        {
            case CmpFileTime::TIME_EQUAL:
                if (fileObj->getFileSize<LEFT_SIDE>() == fileObj->getFileSize<RIGHT_SIDE>())
                {
                    if (fileObj->getShortName<LEFT_SIDE>() == fileObj->getShortName<RIGHT_SIDE>())
                        fileObj->setCategory<FILE_EQUAL>();
                    else
                        fileObj->setCategory<FILE_DIFFERENT_METADATA>();
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


UInt64 getBytesToCompare(const std::vector<FileMapping*>& rowsToCompare)
{
    UInt64 dataTotal;

    for (auto j = rowsToCompare.begin(); j != rowsToCompare.end(); ++j)
        dataTotal += (*j)->getFileSize<LEFT_SIDE>();  //left and right filesizes should be the same

    return dataTotal * 2U;
}


void CompareProcess::categorizeSymlinkByContent(SymLinkMapping& linkObj) const
{
    //categorize symlinks that exist on both sides
    const CmpFileTime timeCmp(fileTimeTolerance);

    if (
#ifdef FFS_WIN //type of symbolic link is relevant for Windows only
        linkObj.getLinkType<LEFT_SIDE>() == linkObj.getLinkType<RIGHT_SIDE>() &&
#endif
        linkObj.getTargetPath<LEFT_SIDE>() == linkObj.getTargetPath<RIGHT_SIDE>())
    {
        //symlinks have same "content"
        if (linkObj.getShortName<LEFT_SIDE>() == linkObj.getShortName<RIGHT_SIDE>() &&
            timeCmp.getResult(linkObj.getLastWriteTime<LEFT_SIDE>(),
                              linkObj.getLastWriteTime<RIGHT_SIDE>()) == CmpFileTime::TIME_EQUAL)
            linkObj.setCategory<SYMLINK_EQUAL>();
        else
            linkObj.setCategory<SYMLINK_DIFFERENT_METADATA>();
    }
    else
        linkObj.setCategory<SYMLINK_DIFFERENT>();
}


void CompareProcess::compareByContent(std::vector<std::pair<FolderPairCfg, BaseDirMapping*>>& workLoad)
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
        [&](SymLinkMapping* linkMap) { this->categorizeSymlinkByContent(*linkMap); });
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

    const size_t objectsTotal = filesToCompareBytewise.size() * 2;
    const UInt64 bytesTotal   = getBytesToCompare(filesToCompareBytewise);

    procCallback.initNewProcess(static_cast<int>(objectsTotal),
                                to<Int64>(bytesTotal),
                                ProcessCallback::PROCESS_COMPARING_CONTENT);

    const CmpFileTime timeCmp(fileTimeTolerance);

    const std::wstring txtComparingContentOfFiles = replaceCpy(_("Comparing content of files %x"), L"%x", L"\n\"%x\"", false);

    //compare files (that have same size) bytewise...
    std::for_each(filesToCompareBytewise.begin(), filesToCompareBytewise.end(),
                  [&](FileMapping* fileObj)
    {
        procCallback.reportStatus(replaceCpy(txtComparingContentOfFiles, L"%x", utf8CvrtTo<std::wstring>(fileObj->getRelativeName<LEFT_SIDE>()), false));

        //check files that exist in left and right model but have different content
        while (true)
        {
            try
            {
                if (filesHaveSameContentUpdating(fileObj->getFullName<LEFT_SIDE>(),
                                                 fileObj->getFullName<RIGHT_SIDE>(),
                                                 fileObj->getFileSize<LEFT_SIDE>() * 2U,
                                                 procCallback))
                {
                    if (fileObj->getShortName<LEFT_SIDE>() == fileObj->getShortName<RIGHT_SIDE>() &&
                        timeCmp.getResult(fileObj->getLastWriteTime<LEFT_SIDE>(),
                                          fileObj->getLastWriteTime<RIGHT_SIDE>()) == CmpFileTime::TIME_EQUAL)
                        fileObj->setCategory<FILE_EQUAL>();
                    else
                        fileObj->setCategory<FILE_DIFFERENT_METADATA>();
                }
                else
                    fileObj->setCategory<FILE_DIFFERENT>();

                procCallback.updateProcessedData(2, 0); //processed data is communicated in subfunctions!
                procCallback.requestUiRefresh(); //may throw
                break;
            }
            catch (FileError& error)
            {
                ProcessCallback::Response rv = procCallback.reportError(error.toString());
                if (rv == ProcessCallback::IGNORE_ERROR)
                {
                    fileObj->setCategoryConflict(_("Conflict detected:") + L"\n" + _("Comparing files by content failed."));
                    break;
                }

                else if (rv == ProcessCallback::RETRY)
                    ;   //continue with loop
                else
                    throw std::logic_error("Programming Error: Unknown return value!");
            }
        }
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

    auto finishLeft  = [&]() { std::for_each(iterLeft,  mapLeft .end(), lo); };
    auto finishRight = [&]() { std::for_each(iterRight, mapRight.end(), ro); };

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
    [&](const FileData& fileLeft)  { output.addSubFile<LEFT_SIDE> (fileLeft.first, fileLeft.second);   }, //left only
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
        DirMapping& newDirMap = output.addSubDir(dirLeft.first, dirRight.first);
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
        dirObj.setActive(filterProc.passDirFilter(dirObj.getObjRelativeName(), NULL)); //subObjMightMatch is always true in this context!
        processFilteredDirs(dirObj, filterProc);
    });

    //remove superfluous directories -> note: this does not invalidate "std::vector<FileMapping*>& undefinedFiles", since we delete folders only
    //and there is no side-effect for memory positions of FileMapping and SymlinkMapping thanks to std::list!
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
void CompareProcess::performComparison(const FolderPairCfg& fpCfg,
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

    procCallback.reportStatus(_("Generating file list..."));

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
