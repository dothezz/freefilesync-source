// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "synchronization.h"
#include <memory>
#include <deque>
#include <stdexcept>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include <wx/file.h>
#include <wx+/string_conv.h>
#include <wx+/format_unit.h>
#include <zen/scope_guard.h>
#include "lib/status_handler.h"
#include <zen/file_handling.h>
#include "lib/resolve_path.h"
#include "lib/recycler.h"
#include "lib/db_file.h"
#include "lib/dir_exist_async.h"
#include "lib/cmp_filetime.h"
#include <zen/file_io.h>
#include <zen/time.h>

#ifdef FFS_WIN
#include <zen/long_path_prefix.h>
#include <zen/perf.h>
#include "lib/shadow.h"
#endif

using namespace zen;


void SyncStatistics::init()
{
    createLeft     = 0;
    createRight    = 0;
    overwriteLeft  = 0;
    overwriteRight = 0;
    deleteLeft     = 0;
    deleteRight    = 0;
    conflict       = 0;
    rowsTotal      = 0;
}


SyncStatistics::SyncStatistics(const FolderComparison& folderCmp)
{
    init();
    std::for_each(begin(folderCmp), end(folderCmp), [&](const BaseDirMapping& baseMap) { getNumbersRecursively(baseMap); });
}


SyncStatistics::SyncStatistics(const HierarchyObject&  hierObj)
{
    init();
    getNumbersRecursively(hierObj);
}


inline
void SyncStatistics::getNumbersRecursively(const HierarchyObject& hierObj)
{
    std::for_each(hierObj.refSubDirs().begin(), hierObj.refSubDirs().end(),
    [&](const DirMapping& dirObj) { getDirNumbers(dirObj); });

    std::for_each(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(),
    [&](const FileMapping& fileObj) { getFileNumbers(fileObj); });

    std::for_each(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(),
    [&](const SymLinkMapping& linkObj) { getLinkNumbers(linkObj); });

    rowsTotal += hierObj.refSubDirs(). size();
    rowsTotal += hierObj.refSubFiles().size();
    rowsTotal += hierObj.refSubLinks().size();
}


inline
void SyncStatistics::getFileNumbers(const FileMapping& fileObj)
{
    switch (fileObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_CREATE_NEW_LEFT:
            ++createLeft;
            dataToProcess += fileObj.getFileSize<RIGHT_SIDE>();
            break;

        case SO_CREATE_NEW_RIGHT:
            ++createRight;
            dataToProcess += fileObj.getFileSize<LEFT_SIDE>();
            break;

        case SO_DELETE_LEFT:
            ++deleteLeft;
            break;

        case SO_DELETE_RIGHT:
            ++deleteRight;
            break;

        case SO_OVERWRITE_LEFT:
            ++overwriteLeft;
            dataToProcess += fileObj.getFileSize<RIGHT_SIDE>();
            break;

        case SO_OVERWRITE_RIGHT:
            ++overwriteRight;
            dataToProcess += fileObj.getFileSize<LEFT_SIDE>();
            break;

        case SO_UNRESOLVED_CONFLICT:
            ++conflict;
            if (firstConflicts.size() < 3) //save the first 3 conflict texts
                firstConflicts.push_back(std::make_pair(fileObj.getObjRelativeName(), fileObj.getSyncOpConflict()));
            break;

        case SO_COPY_METADATA_TO_LEFT:
            ++overwriteLeft;
            break;

        case SO_COPY_METADATA_TO_RIGHT:
            ++overwriteRight;
            break;

        case SO_DO_NOTHING:
        case SO_EQUAL:
            break;
    }
}


inline
void SyncStatistics::getLinkNumbers(const SymLinkMapping& linkObj)
{
    switch (linkObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_CREATE_NEW_LEFT:
            ++createLeft;
            break;

        case SO_CREATE_NEW_RIGHT:
            ++createRight;
            break;

        case SO_DELETE_LEFT:
            ++deleteLeft;
            break;

        case SO_DELETE_RIGHT:
            ++deleteRight;
            break;

        case SO_OVERWRITE_LEFT:
        case SO_COPY_METADATA_TO_LEFT:
            ++overwriteLeft;
            break;

        case SO_OVERWRITE_RIGHT:
        case SO_COPY_METADATA_TO_RIGHT:
            ++overwriteRight;
            break;

        case SO_UNRESOLVED_CONFLICT:
            ++conflict;
            if (firstConflicts.size() < 3) //save the first 3 conflict texts
                firstConflicts.push_back(std::make_pair(linkObj.getObjRelativeName(), linkObj.getSyncOpConflict()));
            break;

        case SO_DO_NOTHING:
        case SO_EQUAL:
            break;
    }
}


inline
void SyncStatistics::getDirNumbers(const DirMapping& dirObj)
{
    switch (dirObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_CREATE_NEW_LEFT:
            ++createLeft;
            break;

        case SO_CREATE_NEW_RIGHT:
            ++createRight;
            break;

        case SO_DELETE_LEFT:
            ++deleteLeft;
            break;

        case SO_DELETE_RIGHT:
            ++deleteRight;
            break;

        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
            assert(false);
            break;

        case SO_UNRESOLVED_CONFLICT:
            ++conflict;
            if (firstConflicts.size() < 3) //save the first 3 conflict texts
                firstConflicts.push_back(std::make_pair(dirObj.getObjRelativeName(), dirObj.getSyncOpConflict()));
            break;

        case SO_COPY_METADATA_TO_LEFT:
            ++overwriteLeft;
            break;

        case SO_COPY_METADATA_TO_RIGHT:
            ++overwriteRight;
            break;

        case SO_DO_NOTHING:
        case SO_EQUAL:
            break;
    }

    //recurse into sub-dirs
    getNumbersRecursively(dirObj);
}


std::vector<zen::FolderPairSyncCfg> zen::extractSyncCfg(const MainConfiguration& mainCfg)
{
    //merge first and additional pairs
    std::vector<FolderPairEnh> allPairs;
    allPairs.push_back(mainCfg.firstPair);
    allPairs.insert(allPairs.end(),
                    mainCfg.additionalPairs.begin(), //add additional pairs
                    mainCfg.additionalPairs.end());

    std::vector<FolderPairSyncCfg> output;

    //process all pairs
    for (auto i = allPairs.begin(); i != allPairs.end(); ++i)
    {
        SyncConfig syncCfg = i->altSyncConfig.get() ? *i->altSyncConfig : mainCfg.syncCfg;

        output.push_back(
            FolderPairSyncCfg(syncCfg.directionCfg.var == DirectionConfig::AUTOMATIC,
                              syncCfg.handleDeletion,
                              toZ(syncCfg.customDeletionDirectory)));
    }

    return output;
}
//------------------------------------------------------------------------------------------------------------

namespace
{
bool synchronizationNeeded(const SyncStatistics& statisticsTotal)
{
    return statisticsTotal.getCreate() +
           statisticsTotal.getOverwrite() +
           statisticsTotal.getDelete() +
           //conflicts (unless excluded) justify a sync! Also note: if this method returns false, no warning about unresolved conflicts were shown!
           statisticsTotal.getConflict() != 0;
}
}


//test if user accidentally tries to sync the wrong folders
bool significantDifferenceDetected(const SyncStatistics& folderPairStat)
{
    //initial file copying shall not be detected as major difference
    if (folderPairStat.getCreate<LEFT_SIDE>() == 0 &&
        folderPairStat.getOverwrite()         == 0 &&
        folderPairStat.getDelete()            == 0 &&
        folderPairStat.getConflict()          == 0) return false;
    if (folderPairStat.getCreate<RIGHT_SIDE>() == 0 &&
        folderPairStat.getOverwrite()         == 0 &&
        folderPairStat.getDelete()            == 0 &&
        folderPairStat.getConflict()          == 0) return false;

    const int changedRows = folderPairStat.getCreate()    +
                            folderPairStat.getOverwrite() +
                            folderPairStat.getDelete()    +
                            folderPairStat.getConflict();

    return changedRows >= 10 && changedRows > 0.5 * folderPairStat.getRowCount();
}
//#################################################################################################################


FolderPairSyncCfg::FolderPairSyncCfg(bool automaticMode,
                                     const DeletionPolicy handleDel,
                                     const Zstring& custDelDir) :
    inAutomaticMode(automaticMode),
    handleDeletion(handleDel),
    custDelFolder(zen::getFormattedDirectoryName(custDelDir)) {}
//-----------------------------------------------------------------------------------------------------------


template <typename Function> inline
bool tryReportingError(ProcessCallback& handler, Function cmd) //return "true" on success, "false" if error was ignored
{
    while (true)
    {
        try
        {
            cmd(); //throw FileError
            return true;
        }
        catch (FileError& error)
        {
            ProcessCallback::Response rv = handler.reportError(error.toString()); //may throw!
            if (rv == ProcessCallback::IGNORE_ERROR)
                return false;
            else if (rv == ProcessCallback::RETRY)
                ;   //continue with loop
            else
                throw std::logic_error("Programming Error: Unknown return value!");
        }
    }
}


/*
add some postfix to alternate deletion directory: deletionDirectory\<prefix>2010-06-30 12-59-12\
*/
Zstring getSessionDeletionDir(const Zstring& deletionDirectory, const Zstring& prefix = Zstring())
{
    Zstring formattedDir = deletionDirectory;
    if (formattedDir.empty())
        return Zstring(); //no valid directory for deletion specified (checked later)

    if (!endsWith(formattedDir, FILE_NAME_SEPARATOR))
        formattedDir += FILE_NAME_SEPARATOR;

    formattedDir += prefix;
    formattedDir += formatTime<Zstring>(Zstr("%Y-%m-%d %H%M%S"));

    //ensure that session directory does not yet exist (must be unique)
    Zstring output = formattedDir;

    //ensure uniqueness
    for (int i = 1; zen::somethingExists(output); ++i)
        output = formattedDir + Zchar('_') + toString<Zstring>(i);

    output += FILE_NAME_SEPARATOR;
    return output;
}


SyncProcess::SyncProcess(xmlAccess::OptionalDialogs& warnings,
                         bool verifyCopiedFiles,
                         bool copyLockedFiles,
                         bool copyFilePermissions,
                         bool transactionalFileCopy,
                         bool runWithBackgroundPriority,
                         ProcessCallback& handler) :
    verifyCopiedFiles_(verifyCopiedFiles),
    copyLockedFiles_(copyLockedFiles),
    copyFilePermissions_(copyFilePermissions),
    transactionalFileCopy_(transactionalFileCopy),
    m_warnings(warnings),
    procCallback(handler)
    {
if (runWithBackgroundPriority)
procBackground.reset(new ScheduleForBackgroundProcessing);
    }
//--------------------------------------------------------------------------------------------------------------

namespace
{
struct CallbackRemoveDirImpl;
}

class DeletionHandling //e.g. generate name of alternate deletion directory (unique for session AND folder pair)
{
public:
    DeletionHandling(DeletionPolicy handleDel,
                     const Zstring& custDelFolder,
                     const Zstring& baseDir, //with separator postfix
                     ProcessCallback& procCallback);
    ~DeletionHandling(); //always (try to) clean up, even if synchronization is aborted!

    //clean-up temporary directory (recycler bin optimization)
    void tryCleanup(); //throw FileError -> call this in non-exceptional coding, i.e. after Sync somewhere!

    void removeFile  (const Zstring& relativeName) const; //throw FileError
    void removeFolder(const Zstring& relativeName) const; //throw FileError

    const wxString& getTxtRemovingFile   () const { return txtRemovingFile;      } //
    const wxString& getTxtRemovingSymLink() const { return txtRemovingSymlink;   } //status text templates
    const wxString& getTxtRemovingDir    () const { return txtRemovingDirectory; } //

    //evaluate whether a deletion will actually free space within a volume
    bool deletionFreesSpace() const;

private:
    friend struct ::CallbackRemoveDirImpl;
    DeletionPolicy deletionType;
    ProcessCallback* procCallback_; //always bound! need assignment operator => not a reference

    Zstring sessionDelDir;  //target deletion folder for current folder pair (with timestamp, ends with path separator)

    Zstring baseDir_;  //with separator postfix

    //preloaded status texts:
    wxString txtRemovingFile;
    wxString txtRemovingSymlink;
    wxString txtRemovingDirectory;

    bool cleanedUp;
};


DeletionHandling::DeletionHandling(DeletionPolicy handleDel,
                                   const Zstring& custDelFolder,
                                   const Zstring& baseDir, //with separator postfix
                                   ProcessCallback& procCallback) :
    deletionType(handleDel),
    procCallback_(&procCallback),
    baseDir_(baseDir),
    cleanedUp(false)
{
#ifdef FFS_WIN
    if (baseDir.empty() ||
        (deletionType == MOVE_TO_RECYCLE_BIN && recycleBinStatus(baseDir) != STATUS_REC_EXISTS))
        deletionType = DELETE_PERMANENTLY; //Windows' ::SHFileOperation() will do this anyway, but we have a better and faster deletion routine (e.g. on networks)
#endif

    switch (deletionType)
    {
        case DELETE_PERMANENTLY:
            txtRemovingFile      = replaceCpy(_("Deleting file %x"         ), L"%x", L"\n\"%x\"", false);
            txtRemovingDirectory = replaceCpy(_("Deleting folder %x"       ), L"%x", L"\n\"%x\"", false);
            txtRemovingSymlink   = replaceCpy(_("Deleting symbolic link %x"), L"%x", L"\n\"%x\"", false);
            break;

        case MOVE_TO_RECYCLE_BIN:
            sessionDelDir = getSessionDeletionDir(baseDir_, Zstr("FFS "));

            txtRemovingFile      = replaceCpy(_("Moving file %x to recycle bin"         ), L"%x", L"\n\"%x\"", false);
            txtRemovingDirectory = replaceCpy(_("Moving folder %x to recycle bin"       ), L"%x", L"\n\"%x\"", false);
            txtRemovingSymlink   = replaceCpy(_("Moving symbolic link %x to recycle bin"), L"%x", L"\n\"%x\"", false);
            break;

        case MOVE_TO_CUSTOM_DIRECTORY:
            sessionDelDir = getSessionDeletionDir(custDelFolder);

            txtRemovingFile      = replaceCpy(replaceCpy(_("Moving file %x to %y"         ), L"%x", L"\n\"%x\"", false), L"%y", L"\"" + utf8CvrtTo<wxString>(custDelFolder) + L"\"", false);
            txtRemovingDirectory = replaceCpy(replaceCpy(_("Moving folder %x to %y"       ), L"%x", L"\n\"%x\"", false), L"%y", L"\"" + utf8CvrtTo<wxString>(custDelFolder) + L"\"", false);
            txtRemovingSymlink   = replaceCpy(replaceCpy(_("Moving symbolic link %x to %y"), L"%x", L"\n\"%x\"", false), L"%y", L"\"" + utf8CvrtTo<wxString>(custDelFolder) + L"\"", false);
            break;
    }
}


DeletionHandling::~DeletionHandling()
{
    //always (try to) clean up, even if synchronization is aborted!
    try { tryCleanup(); }
    catch (...) {}     //make sure this stays non-blocking!
}


void DeletionHandling::tryCleanup() //throw FileError
{
    if (!cleanedUp)
    {
        if (deletionType == MOVE_TO_RECYCLE_BIN) //clean-up temporary directory (recycle bin)
            zen::moveToRecycleBin(beforeLast(sessionDelDir, FILE_NAME_SEPARATOR)); //throw FileError

        cleanedUp = true;
    }
}


namespace
{
class CallbackMoveFileImpl : public CallbackMoveFile //callback functionality
{
public:
    CallbackMoveFileImpl(ProcessCallback& handler) : statusHandler_(handler) {}

    virtual void requestUiRefresh(const Zstring& currentObject)  //DON'T throw exceptions here, at least in Windows build!
    {
        statusHandler_.requestUiRefresh(); //exceptions may be thrown here!
    }

private:
    ProcessCallback& statusHandler_;
};


struct CallbackRemoveDirImpl : public CallbackRemoveDir
{
    CallbackRemoveDirImpl(const DeletionHandling& delHandling) : delHandling_(delHandling) {}

    virtual void notifyFileDeletion(const Zstring& filename)
    {
        delHandling_.procCallback_->reportStatus(replaceCpy(delHandling_.getTxtRemovingFile(), L"%x", utf8CvrtTo<wxString>(filename)));
    }

    virtual void notifyDirDeletion(const Zstring& dirname)
    {
        delHandling_.procCallback_->reportStatus(replaceCpy(delHandling_.getTxtRemovingDir(), L"%x", utf8CvrtTo<wxString>(dirname)));
    }

private:
    const DeletionHandling& delHandling_;
};
}


void DeletionHandling::removeFile(const Zstring& relativeName) const
{
    const Zstring fullName = baseDir_ + relativeName;

    switch (deletionType)
    {
        case DELETE_PERMANENTLY:
            zen::removeFile(fullName);
            break;

        case MOVE_TO_RECYCLE_BIN:
            if (fileExists(fullName))
            {
                const Zstring targetFile = sessionDelDir + relativeName; //altDeletionDir ends with path separator
                const Zstring targetDir  = beforeLast(targetFile, FILE_NAME_SEPARATOR);

                try //rename file: no copying!!!
                {
                    if (!dirExists(targetDir)) //no reason to update gui or overwrite status text!
                        createDirectory(targetDir); //throw FileError -> may legitimately fail on Linux if permissions are missing

                    //performance optimization!! Instead of moving each object into recycle bin separately, we rename them ony by one into a
                    //temporary directory and delete this directory only ONCE!
                    renameFile(fullName, targetFile); //throw FileError
                }
                catch (FileError&) //if anything went wrong, move to recycle bin the standard way (single file processing: slow)
                {
                    moveToRecycleBin(fullName);  //throw FileError
                }
            }
            break;

        case MOVE_TO_CUSTOM_DIRECTORY:
            if (fileExists(fullName))
            {
                const Zstring targetFile = sessionDelDir + relativeName; //altDeletionDir ends with path separator
                const Zstring targetDir  = beforeLast(targetFile, FILE_NAME_SEPARATOR);

                if (!dirExists(targetDir))
                    createDirectory(targetDir); //throw FileError

                CallbackMoveFileImpl callBack(*procCallback_); //if file needs to be copied we need callback functionality to update screen and offer abort
                moveFile(fullName, targetFile, true, &callBack);
            }
            break;
    }
}


void DeletionHandling::removeFolder(const Zstring& relativeName) const
{
    const Zstring fullName = baseDir_ + relativeName;

    switch (deletionType)
    {
        case DELETE_PERMANENTLY:
        {
            CallbackRemoveDirImpl remDirCallback(*this);
            removeDirectory(fullName, &remDirCallback);
        }
        break;

        case MOVE_TO_RECYCLE_BIN:
            if (dirExists(fullName))
            {
                const Zstring targetDir      = sessionDelDir + relativeName;
                const Zstring targetSuperDir = beforeLast(targetDir, FILE_NAME_SEPARATOR);

                try //rename directory: no copying!!!
                {
                    if (!dirExists(targetSuperDir))
                        createDirectory(targetSuperDir); //throw FileError -> may legitimately fail on Linux if permissions are missing

                    //performance optimization!! Instead of moving each object into recycle bin separately, we rename them one by one into a
                    //temporary directory and delete this directory only ONCE!
                    renameFile(fullName, targetDir); //throw FileError
                }
                catch (FileError&) //if anything went wrong, move to recycle bin the standard way (single file processing: slow)
                {
                    moveToRecycleBin(fullName); //throw FileError
                }
            }
            break;

        case MOVE_TO_CUSTOM_DIRECTORY:
            if (dirExists(fullName))
            {
                const Zstring targetDir      = sessionDelDir + relativeName;
                const Zstring targetSuperDir = beforeLast(targetDir, FILE_NAME_SEPARATOR);

                if (!dirExists(targetSuperDir))
                    createDirectory(targetSuperDir); //throw FileError

                CallbackMoveFileImpl callBack(*procCallback_); //if files need to be copied, we need callback functionality to update screen and offer abort
                moveDirectory(fullName, targetDir, true, &callBack);
            }
            break;
    }
}

//evaluate whether a deletion will actually free space within a volume
bool DeletionHandling::deletionFreesSpace() const
{
    switch (deletionType)
    {
        case DELETE_PERMANENTLY:
            return true;
        case MOVE_TO_RECYCLE_BIN:
            return false; //in general... (unless Recycle Bin is full)
        case MOVE_TO_CUSTOM_DIRECTORY:
            switch (zen::onSameVolume(baseDir_, sessionDelDir))
            {
                case IS_SAME_YES:
                    return false;
                case IS_SAME_NO:
                    return true; //but other volume (sessionDelDir) may become full...
                case IS_SAME_CANT_SAY:
                    return true; //a rough guess!
            }
    }
    assert(false);
    return true;
}
//------------------------------------------------------------------------------------------------------------

namespace
{

class DiskSpaceNeeded
{
public:
    DiskSpaceNeeded(const BaseDirMapping& baseObj, bool freeSpaceDelLeft, bool freeSpaceDelRight) :
        freeSpaceDelLeft_(freeSpaceDelLeft),
        freeSpaceDelRight_(freeSpaceDelRight)
    {
        processRecursively(baseObj);
    }

    std::pair<zen::Int64, zen::Int64> getSpaceTotal() const
    {
        return std::make_pair(spaceNeededLeft, spaceNeededRight);
    }

private:
    void processRecursively(const HierarchyObject& hierObj)
    {
        //don't process directories

        //process files
        for (auto i = hierObj.refSubFiles().begin(); i != hierObj.refSubFiles().end(); ++i)
            switch (i->getSyncOperation()) //evaluate comparison result and sync direction
            {
                case SO_CREATE_NEW_LEFT:
                    spaceNeededLeft += to<zen::Int64>(i->getFileSize<RIGHT_SIDE>());
                    break;

                case SO_CREATE_NEW_RIGHT:
                    spaceNeededRight += to<zen::Int64>(i->getFileSize<LEFT_SIDE>());
                    break;

                case SO_DELETE_LEFT:
                    if (freeSpaceDelLeft_)
                        spaceNeededLeft -= to<zen::Int64>(i->getFileSize<LEFT_SIDE>());
                    break;

                case SO_DELETE_RIGHT:
                    if (freeSpaceDelRight_)
                        spaceNeededRight -= to<zen::Int64>(i->getFileSize<RIGHT_SIDE>());
                    break;

                case SO_OVERWRITE_LEFT:
                    if (freeSpaceDelLeft_)
                        spaceNeededLeft -= to<zen::Int64>(i->getFileSize<LEFT_SIDE>());
                    spaceNeededLeft += to<zen::Int64>(i->getFileSize<RIGHT_SIDE>());
                    break;

                case SO_OVERWRITE_RIGHT:
                    if (freeSpaceDelRight_)
                        spaceNeededRight -= to<zen::Int64>(i->getFileSize<RIGHT_SIDE>());
                    spaceNeededRight += to<zen::Int64>(i->getFileSize<LEFT_SIDE>());
                    break;

                case SO_DO_NOTHING:
                case SO_EQUAL:
                case SO_UNRESOLVED_CONFLICT:
                case SO_COPY_METADATA_TO_LEFT:
                case SO_COPY_METADATA_TO_RIGHT:
                    break;
            }

        //symbolic links
        //[...]

        //recurse into sub-dirs
        std::for_each(hierObj.refSubDirs().begin(), hierObj.refSubDirs().end(), [&](const HierarchyObject& subDir) { this->processRecursively(subDir); });
        //for (auto& subDir : hierObj.refSubDirs())
        //      processRecursively(subDir);
    }

    const bool freeSpaceDelLeft_;
    const bool freeSpaceDelRight_;

    zen::Int64 spaceNeededLeft;
    zen::Int64 spaceNeededRight;
};
}
//----------------------------------------------------------------------------------------

//test if current sync-line will result in deletion of files or
//big file is overwritten by smaller one -> used to avoid disc space bottlenecks (at least if permanent deletion is active)
inline
bool diskSpaceIsReduced(const FileMapping& fileObj)
{
    switch (fileObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_DELETE_LEFT:
        case SO_DELETE_RIGHT:
            return true;

        case SO_OVERWRITE_LEFT:
            return fileObj.getFileSize<LEFT_SIDE>() > fileObj.getFileSize<RIGHT_SIDE>();

        case SO_OVERWRITE_RIGHT:
            return fileObj.getFileSize<LEFT_SIDE>() < fileObj.getFileSize<RIGHT_SIDE>();

        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
            return false;
    }
    assert(false);
    return false; //dummy
}


inline
bool diskSpaceIsReduced(const DirMapping& dirObj)
{
    switch (dirObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_DELETE_LEFT:
        case SO_DELETE_RIGHT:
            return true;

        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
            assert(false);
        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
            return false;
    }
    assert(false);
    return false; //dummy
}
//---------------------------------------------------------------------------------------------------------------


class zen::SynchronizeFolderPair
{
public:
    SynchronizeFolderPair(const SyncProcess& syncProc,
#ifdef FFS_WIN
                          shadow::ShadowCopy* shadowCopyHandler,
#endif
                          const DeletionHandling& delHandlingLeft,
                          const DeletionHandling& delHandlingRight) :
        procCallback_(syncProc.procCallback),
#ifdef FFS_WIN
        shadowCopyHandler_(shadowCopyHandler),
#endif
        delHandlingLeft_(delHandlingLeft),
        delHandlingRight_(delHandlingRight),
        verifyCopiedFiles(syncProc.verifyCopiedFiles_),
        copyFilePermissions(syncProc.copyFilePermissions_),
        transactionalFileCopy(syncProc.transactionalFileCopy_),
        txtCreatingFile     (replaceCpy(_("Creating file %x"            ), L"%x", L"\"%x\"", false)),
        txtCreatingLink     (replaceCpy(_("Creating symbolic link %x"   ), L"%x", L"\"%x\"", false)),
        txtCreatingFolder   (replaceCpy(_("Creating folder %x"          ), L"%x", L"\"%x\"", false)),
        txtOverwritingFile  (replaceCpy(_("Overwriting file %x"         ), L"%x", L"\"%x\"", false)),
        txtOverwritingLink  (replaceCpy(_("Overwriting symbolic link %x"), L"%x", L"\"%x\"", false)),
        txtVerifying        (replaceCpy(_("Verifying file %x"           ), L"%x", L"\"%x\"", false)),
        txtWritingAttributes(replaceCpy(_("Updating attributes of %x"   ), L"%x", L"\"%x\"", false)) {}

    void startSync(BaseDirMapping& baseMap)
    {
        //loop through all files twice; reason: first delete files (or overwrite big ones with smaller ones), then copy rest
        execute<FIRST_PASS >(baseMap);
        execute<SECOND_PASS>(baseMap);
    }

private:
    enum PassId
    {
        FIRST_PASS, //delete files
        SECOND_PASS //create, modify
    };

    template <PassId pass>
    void execute(HierarchyObject& hierObj);

    void synchronizeFile  (FileMapping&    fileObj) const;
    void synchronizeLink  (SymLinkMapping& linkObj) const;
    void synchronizeFolder(DirMapping&     dirObj)  const;

    //more low level helper
    template <zen::SelectedSide side>
    void deleteSymlink(const SymLinkMapping& linkObj) const;
    template <SelectedSide side, class DelTargetCommand>
    void copyFileUpdatingTo(const FileMapping& fileObj, const DelTargetCommand& cmd, FileDescriptor& sourceAttr) const;
    void verifyFileCopy(const Zstring& source, const Zstring& target) const;

    ProcessCallback& procCallback_;
#ifdef FFS_WIN
    shadow::ShadowCopy* shadowCopyHandler_; //optional!
#endif
    const DeletionHandling& delHandlingLeft_;
    const DeletionHandling& delHandlingRight_;

    const bool verifyCopiedFiles;
    const bool copyFilePermissions;
    const bool transactionalFileCopy;

    //preload status texts
    const wxString txtCreatingFile;
    const wxString txtCreatingLink;
    const wxString txtCreatingFolder;
    const wxString txtOverwritingFile;
    const wxString txtOverwritingLink;
    const wxString txtVerifying;
    const wxString txtWritingAttributes;
};


template <SynchronizeFolderPair::PassId pass>
void SynchronizeFolderPair::execute(HierarchyObject& hierObj)
{
    //synchronize files:
    std::for_each(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(),
                  [&](FileMapping& fileObj)
    {
        const bool letsDoThis = (pass == FIRST_PASS) == diskSpaceIsReduced(fileObj); //to be deleted files on first pass, rest on second!
        if (letsDoThis)
            tryReportingError(procCallback_, [&]() { synchronizeFile(fileObj); });
    });

    //synchronize symbolic links: (process in second step only)
    if (pass == SECOND_PASS)
        std::for_each(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(),
        [&](SymLinkMapping& linkObj) { tryReportingError(procCallback_, [&]() { synchronizeLink(linkObj); }); });

    //synchronize folders:
    std::for_each(hierObj.refSubDirs().begin(), hierObj.refSubDirs().end(),
                  [&](DirMapping& dirObj)
    {
        const bool letsDoThis = (pass == FIRST_PASS) == diskSpaceIsReduced(dirObj); //to be deleted dirs on first pass, rest on second!
        if (letsDoThis) //if we created all folders on first pass it would look strange for initial mirror sync when all folders are created at once
            tryReportingError(procCallback_, [&]() { synchronizeFolder(dirObj); });

        //recursion!
        this->execute<pass>(dirObj);
    });
}


void SynchronizeFolderPair::synchronizeFile(FileMapping& fileObj) const
{
    wxString logText;
    Zstring target;

    switch (fileObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_CREATE_NEW_LEFT:
            target = fileObj.getBaseDirPf<LEFT_SIDE>() + fileObj.getRelativeName<RIGHT_SIDE>(); //can't use "getFullName" as target is not yet existing

            logText = txtCreatingFile;
            replace(logText, L"%x", utf8CvrtTo<wxString>(target));
            procCallback_.reportInfo(logText);

            try
            {
                FileDescriptor sourceAttr;
                copyFileUpdatingTo<LEFT_SIDE>(fileObj,
                []() {},  //no target to delete
                sourceAttr);

                fileObj.copyTo<LEFT_SIDE>(&sourceAttr); //update FileMapping
            }
            catch (FileError&)
            {
                if (fileExists(fileObj.getFullName<RIGHT_SIDE>()))
                    throw;
                //source deleted meanwhile...
                procCallback_.updateProcessedData(0, to<zen::Int64>(fileObj.getFileSize<RIGHT_SIDE>()));
                fileObj.removeObject<RIGHT_SIDE>();
            }
            break;

        case SO_CREATE_NEW_RIGHT:
            target = fileObj.getBaseDirPf<RIGHT_SIDE>() + fileObj.getRelativeName<LEFT_SIDE>();

            logText = txtCreatingFile;
            replace(logText, L"%x", utf8CvrtTo<wxString>(target));
            procCallback_.reportInfo(logText);

            try
            {
                FileDescriptor sourceAttr;
                copyFileUpdatingTo<RIGHT_SIDE>(fileObj,
                []() {},  //no target to delete
                sourceAttr);

                fileObj.copyTo<RIGHT_SIDE>(&sourceAttr); //update FileMapping
            }
            catch (FileError&)
            {
                if (fileExists(fileObj.getFullName<LEFT_SIDE>()))
                    throw;
                //source deleted meanwhile...
                procCallback_.updateProcessedData(0, to<zen::Int64>(fileObj.getFileSize<LEFT_SIDE>()));
                fileObj.removeObject<LEFT_SIDE>();
            }
            break;

        case SO_DELETE_LEFT:
            logText = replaceCpy(delHandlingLeft_.getTxtRemovingFile(), L"%x", utf8CvrtTo<wxString>(fileObj.getFullName<LEFT_SIDE>()));
            procCallback_.reportInfo(logText);

            delHandlingLeft_.removeFile(fileObj.getObjRelativeName()); //throw FileError
            fileObj.removeObject<LEFT_SIDE>(); //update FileMapping
            break;

        case SO_DELETE_RIGHT:
            logText = replaceCpy(delHandlingRight_.getTxtRemovingFile(), L"%x", utf8CvrtTo<wxString>(fileObj.getFullName<RIGHT_SIDE>()));
            procCallback_.reportInfo(logText);

            delHandlingRight_.removeFile(fileObj.getObjRelativeName()); //throw FileError
            fileObj.removeObject<RIGHT_SIDE>(); //update FileMapping
            break;

        case SO_OVERWRITE_LEFT:
        {
            target = fileObj.getBaseDirPf<LEFT_SIDE>() + fileObj.getRelativeName<RIGHT_SIDE>(); //respect differences in case of source object

            logText = txtOverwritingFile;
            replace(logText, L"%x", utf8CvrtTo<wxString>(target));
            procCallback_.reportInfo(logText);

            FileDescriptor sourceAttr;
            copyFileUpdatingTo<LEFT_SIDE>(fileObj,
                                          [&]() //delete target at appropriate time
            {
                procCallback_.reportStatus(replaceCpy(delHandlingLeft_.getTxtRemovingFile(), L"%x", utf8CvrtTo<wxString>(fileObj.getFullName<LEFT_SIDE>())));

                delHandlingLeft_.removeFile(fileObj.getObjRelativeName()); //throw FileError
                fileObj.removeObject<LEFT_SIDE>(); //remove file from FileMapping, to keep in sync (if subsequent copying fails!!)

                procCallback_.reportStatus(logText); //restore status text copy file
            }, sourceAttr);

            fileObj.copyTo<LEFT_SIDE>(&sourceAttr); //update FileMapping
        }
        break;

        case SO_OVERWRITE_RIGHT:
        {
            target = fileObj.getBaseDirPf<RIGHT_SIDE>() + fileObj.getRelativeName<LEFT_SIDE>(); //respect differences in case of source object

            logText = txtOverwritingFile;
            replace(logText, L"%x", utf8CvrtTo<wxString>(target));
            procCallback_.reportInfo(logText);

            FileDescriptor sourceAttr;
            copyFileUpdatingTo<RIGHT_SIDE>(fileObj,
                                           [&]() //delete target at appropriate time
            {
                procCallback_.reportStatus(replaceCpy(delHandlingRight_.getTxtRemovingFile(), L"%x", utf8CvrtTo<wxString>(fileObj.getFullName<RIGHT_SIDE>())));

                delHandlingRight_.removeFile(fileObj.getObjRelativeName()); //throw FileError
                fileObj.removeObject<RIGHT_SIDE>(); //remove file from FileMapping, to keep in sync (if subsequent copying fails!!)

                procCallback_.reportStatus(logText); //restore status text copy file
            }, sourceAttr);

            fileObj.copyTo<RIGHT_SIDE>(&sourceAttr); //update FileMapping
        }
        break;

        case SO_COPY_METADATA_TO_LEFT:
            logText = replaceCpy(txtWritingAttributes, L"%x", utf8CvrtTo<wxString>(fileObj.getFullName<LEFT_SIDE>()));
            procCallback_.reportInfo(logText);

            if (fileObj.getShortName<LEFT_SIDE>() != fileObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(fileObj.getFullName<LEFT_SIDE>(),
                           beforeLast(fileObj.getFullName<LEFT_SIDE>(), FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + fileObj.getShortName<RIGHT_SIDE>()); //throw FileError;

            if (!sameFileTime(fileObj.getLastWriteTime<LEFT_SIDE>(), fileObj.getLastWriteTime<RIGHT_SIDE>(), 2)) //respect 2 second FAT/FAT32 precision
                setFileTime(fileObj.getFullName<LEFT_SIDE>(), fileObj.getLastWriteTime<RIGHT_SIDE>(), SYMLINK_FOLLOW); //throw FileError
            //do NOT read *current* source file time, but use buffered value which corresponds to time of comparison!

            fileObj.copyTo<LEFT_SIDE>(NULL); //-> both sides *should* be completely equal now...
            break;

        case SO_COPY_METADATA_TO_RIGHT:
            logText = replaceCpy(txtWritingAttributes, L"%x", utf8CvrtTo<wxString>(fileObj.getFullName<RIGHT_SIDE>()));
            procCallback_.reportInfo(logText);

            if (fileObj.getShortName<LEFT_SIDE>() != fileObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(fileObj.getFullName<RIGHT_SIDE>(),
                           beforeLast(fileObj.getFullName<RIGHT_SIDE>(), FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + fileObj.getShortName<LEFT_SIDE>()); //throw FileError;

            if (!sameFileTime(fileObj.getLastWriteTime<LEFT_SIDE>(), fileObj.getLastWriteTime<RIGHT_SIDE>(), 2)) //respect 2 second FAT/FAT32 precision
                setFileTime(fileObj.getFullName<RIGHT_SIDE>(), fileObj.getLastWriteTime<LEFT_SIDE>(), SYMLINK_FOLLOW); //throw FileError

            fileObj.copyTo<RIGHT_SIDE>(NULL); //-> both sides *should* be completely equal now...
            break;

        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
            return; //no update on processed data!
    }

    //progress indicator update
    //indicator is updated only if file is sync'ed correctly (and if some sync was done)!
    procCallback_.updateProcessedData(1, 0); //processed data is communicated in subfunctions!
    procCallback_.requestUiRefresh(); //may throw
}


void SynchronizeFolderPair::synchronizeLink(SymLinkMapping& linkObj) const
{
    wxString logText;
    Zstring target;

    switch (linkObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_CREATE_NEW_LEFT:
            target = linkObj.getBaseDirPf<LEFT_SIDE>() + linkObj.getRelativeName<RIGHT_SIDE>();

            logText = txtCreatingLink;
            replace(logText, L"%x", utf8CvrtTo<wxString>(target));
            procCallback_.reportInfo(logText);

            try
            {
                zen::copySymlink(linkObj.getFullName<RIGHT_SIDE>(), target, copyFilePermissions); //throw FileError

                linkObj.copyTo<LEFT_SIDE>(); //update SymLinkMapping
            }
            catch (FileError&)
            {
                if (fileExists(linkObj.getFullName<RIGHT_SIDE>()))
                    throw;
                //source deleted meanwhile...
                linkObj.removeObject<RIGHT_SIDE>();
            }
            break;

        case SO_CREATE_NEW_RIGHT:
            target = linkObj.getBaseDirPf<RIGHT_SIDE>() + linkObj.getRelativeName<LEFT_SIDE>();

            logText = txtCreatingLink;
            replace(logText, L"%x", utf8CvrtTo<wxString>(target));
            procCallback_.reportInfo(logText);

            try
            {
                zen::copySymlink(linkObj.getFullName<LEFT_SIDE>(), target, copyFilePermissions); //throw FileError

                linkObj.copyTo<RIGHT_SIDE>(); //update SymLinkMapping
            }
            catch (FileError&)
            {
                if (fileExists(linkObj.getFullName<LEFT_SIDE>()))
                    throw;
                //source deleted meanwhile...
                linkObj.removeObject<LEFT_SIDE>();
            }
            break;

        case SO_DELETE_LEFT:
            logText = replaceCpy(delHandlingLeft_.getTxtRemovingSymLink(), L"%x", utf8CvrtTo<wxString>(linkObj.getFullName<LEFT_SIDE>()));
            procCallback_.reportInfo(logText);

            deleteSymlink<LEFT_SIDE>(linkObj); //throw FileError

            linkObj.removeObject<LEFT_SIDE>(); //update SymLinkMapping
            break;

        case SO_DELETE_RIGHT:
            logText = replaceCpy(delHandlingRight_.getTxtRemovingSymLink(), L"%x", utf8CvrtTo<wxString>(linkObj.getFullName<RIGHT_SIDE>()));
            procCallback_.reportInfo(logText);

            deleteSymlink<RIGHT_SIDE>(linkObj); //throw FileError

            linkObj.removeObject<RIGHT_SIDE>(); //update SymLinkMapping
            break;

        case SO_OVERWRITE_LEFT:
            target = linkObj.getBaseDirPf<LEFT_SIDE>() + linkObj.getRelativeName<RIGHT_SIDE>(); //respect differences in case of source object

            logText = txtOverwritingLink;
            replace(logText, L"%x", utf8CvrtTo<wxString>(target));
            procCallback_.reportInfo(logText);

            procCallback_.reportStatus(replaceCpy(delHandlingLeft_.getTxtRemovingSymLink(), L"%x", utf8CvrtTo<wxString>(linkObj.getFullName<LEFT_SIDE>())));
            deleteSymlink<LEFT_SIDE>(linkObj); //throw FileError
            linkObj.removeObject<LEFT_SIDE>(); //remove file from FileMapping, to keep in sync (if subsequent copying fails!!)

            procCallback_.reportStatus(logText); //restore status text
            zen::copySymlink(linkObj.getFullName<RIGHT_SIDE>(), target, copyFilePermissions); //throw FileError

            linkObj.copyTo<LEFT_SIDE>(); //update SymLinkMapping
            break;

        case SO_OVERWRITE_RIGHT:
            target = linkObj.getBaseDirPf<RIGHT_SIDE>() + linkObj.getRelativeName<LEFT_SIDE>(); //respect differences in case of source object

            logText = txtOverwritingLink;
            replace(logText, L"%x", utf8CvrtTo<wxString>(target));
            procCallback_.reportInfo(logText);

            procCallback_.reportStatus(replaceCpy(delHandlingRight_.getTxtRemovingSymLink(), L"%x", utf8CvrtTo<wxString>(linkObj.getFullName<RIGHT_SIDE>())));
            deleteSymlink<RIGHT_SIDE>(linkObj); //throw FileError
            linkObj.removeObject<RIGHT_SIDE>(); //remove file from FileMapping, to keep in sync (if subsequent copying fails!!)

            procCallback_.reportStatus(logText); //restore status text
            zen::copySymlink(linkObj.getFullName<LEFT_SIDE>(), target, copyFilePermissions); //throw FileError

            linkObj.copyTo<RIGHT_SIDE>(); //update SymLinkMapping
            break;

        case SO_COPY_METADATA_TO_LEFT:
            logText = replaceCpy(txtWritingAttributes, L"%x", utf8CvrtTo<wxString>(linkObj.getFullName<LEFT_SIDE>()));
            procCallback_.reportInfo(logText);

            if (linkObj.getShortName<LEFT_SIDE>() != linkObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(linkObj.getFullName<LEFT_SIDE>(),
                           beforeLast(linkObj.getFullName<LEFT_SIDE>(), FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + linkObj.getShortName<RIGHT_SIDE>()); //throw FileError;

            if (!sameFileTime(linkObj.getLastWriteTime<LEFT_SIDE>(), linkObj.getLastWriteTime<RIGHT_SIDE>(), 2)) //respect 2 second FAT/FAT32 precision
                setFileTime(linkObj.getFullName<LEFT_SIDE>(), linkObj.getLastWriteTime<RIGHT_SIDE>(), SYMLINK_DIRECT); //throw FileError

            linkObj.copyTo<LEFT_SIDE>(); //-> both sides *should* be completely equal now...
            break;

        case SO_COPY_METADATA_TO_RIGHT:
            logText = replaceCpy(txtWritingAttributes, L"%x", utf8CvrtTo<wxString>(linkObj.getFullName<RIGHT_SIDE>()));
            procCallback_.reportInfo(logText);

            if (linkObj.getShortName<LEFT_SIDE>() != linkObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(linkObj.getFullName<RIGHT_SIDE>(),
                           beforeLast(linkObj.getFullName<RIGHT_SIDE>(), FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + linkObj.getShortName<LEFT_SIDE>()); //throw FileError;

            if (!sameFileTime(linkObj.getLastWriteTime<LEFT_SIDE>(), linkObj.getLastWriteTime<RIGHT_SIDE>(), 2)) //respect 2 second FAT/FAT32 precision
                setFileTime(linkObj.getFullName<RIGHT_SIDE>(), linkObj.getLastWriteTime<LEFT_SIDE>(), SYMLINK_DIRECT); //throw FileError

            linkObj.copyTo<RIGHT_SIDE>(); //-> both sides *should* be completely equal now...
            break;

        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
            return; //no update on processed data!
    }

    //progress indicator update
    //indicator is updated only if file is sync'ed correctly (and if some sync was done)!
    procCallback_.updateProcessedData(1, 0); //processed data is communicated in subfunctions!
    procCallback_.requestUiRefresh(); //may throw
}


void SynchronizeFolderPair::synchronizeFolder(DirMapping& dirObj) const
{
    wxString logText;
    Zstring target;

    //synchronize folders:
    switch (dirObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_CREATE_NEW_LEFT:
            //some check to catch the error that directory on source has been deleted externally after "compare"...
            if (!dirExists(dirObj.getFullName<RIGHT_SIDE>()))
            {
                // throw FileError(_ ("Source directory does not exist anymore:") + "\n\"" + dirObj.getFullName<RIGHT_SIDE>() + "\"");
                const SyncStatistics subObjects(dirObj); //DON'T forget to notify about implicitly deleted objects!
                procCallback_.updateProcessedData(subObjects.getCreate() + subObjects.getOverwrite() + subObjects.getDelete(), to<zen::Int64>(subObjects.getDataToProcess()));

                dirObj.refSubFiles().clear(); //...then remove sub-objects
                dirObj.refSubLinks().clear(); //
                dirObj.refSubDirs ().clear(); //
                dirObj.removeObject<RIGHT_SIDE>();
            }
            else
            {
                target = dirObj.getBaseDirPf<LEFT_SIDE>() + dirObj.getRelativeName<RIGHT_SIDE>();

                logText = replaceCpy(txtCreatingFolder, L"%x", utf8CvrtTo<wxString>(target));
                procCallback_.reportInfo(logText);

                createDirectory(target, dirObj.getFullName<RIGHT_SIDE>(), copyFilePermissions); //no symlink copying!
                dirObj.copyTo<LEFT_SIDE>(); //update DirMapping
            }
            break;

        case SO_CREATE_NEW_RIGHT:
            //some check to catch the error that directory on source has been deleted externally after "compare"...
            if (!dirExists(dirObj.getFullName<LEFT_SIDE>()))
            {
                //throw FileError(_ ("Source directory does not exist anymore:") + "\n\"" + dirObj.getFullName<LEFT_SIDE>() + "\"");
                const SyncStatistics subObjects(dirObj); //DON'T forget to notify about implicitly deleted objects!
                procCallback_.updateProcessedData(subObjects.getCreate() + subObjects.getOverwrite() + subObjects.getDelete(), to<zen::Int64>(subObjects.getDataToProcess()));

                dirObj.refSubFiles().clear(); //...then remove sub-objects
                dirObj.refSubLinks().clear(); //
                dirObj.refSubDirs ().clear(); //
                dirObj.removeObject<LEFT_SIDE>();
            }
            else
            {
                target = dirObj.getBaseDirPf<RIGHT_SIDE>() + dirObj.getRelativeName<LEFT_SIDE>();

                logText = replaceCpy(txtCreatingFolder, L"%x", utf8CvrtTo<wxString>(target));
                procCallback_.reportInfo(logText);

                createDirectory(target, dirObj.getFullName<LEFT_SIDE>(), copyFilePermissions); //no symlink copying!
                dirObj.copyTo<RIGHT_SIDE>(); //update DirMapping
            }
            break;

        case SO_COPY_METADATA_TO_LEFT:
            logText = replaceCpy(txtWritingAttributes, L"%x", utf8CvrtTo<wxString>(dirObj.getFullName<LEFT_SIDE>()));
            procCallback_.reportInfo(logText);

            if (dirObj.getShortName<LEFT_SIDE>() != dirObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(dirObj.getFullName<LEFT_SIDE>(),
                           beforeLast(dirObj.getFullName<LEFT_SIDE>(), FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + dirObj.getShortName<RIGHT_SIDE>()); //throw FileError;
            //copyFileTimes(dirObj.getFullName<RIGHT_SIDE>(), dirObj.getFullName<LEFT_SIDE>(), true); //throw FileError -> is executed after sub-objects have finished synchronization

            dirObj.copyTo<LEFT_SIDE>(); //-> both sides *should* be completely equal now...
            break;

        case SO_COPY_METADATA_TO_RIGHT:
            logText = replaceCpy(txtWritingAttributes, L"%x", utf8CvrtTo<wxString>(dirObj.getFullName<RIGHT_SIDE>()));
            procCallback_.reportInfo(logText);

            if (dirObj.getShortName<LEFT_SIDE>() != dirObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(dirObj.getFullName<RIGHT_SIDE>(),
                           beforeLast(dirObj.getFullName<RIGHT_SIDE>(), FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + dirObj.getShortName<LEFT_SIDE>()); //throw FileError;
            //copyFileTimes(dirObj.getFullName<LEFT_SIDE>(), dirObj.getFullName<RIGHT_SIDE>(), true); //throw FileError -> is executed after sub-objects have finished synchronization

            dirObj.copyTo<RIGHT_SIDE>(); //-> both sides *should* be completely equal now...
            break;

        case SO_DELETE_LEFT:
            //status information
            logText = replaceCpy(delHandlingLeft_.getTxtRemovingDir(), L"%x", utf8CvrtTo<wxString>(dirObj.getFullName<LEFT_SIDE>()));
            procCallback_.reportInfo(logText);

            delHandlingLeft_.removeFolder(dirObj.getObjRelativeName()); //throw FileError
            {
                //progress indicator update: DON'T forget to notify about implicitly deleted objects!
                const SyncStatistics subObjects(dirObj);
                //...then remove everything
                dirObj.refSubFiles().clear();
                dirObj.refSubLinks().clear();
                dirObj.refSubDirs ().clear();
                procCallback_.updateProcessedData(subObjects.getCreate() + subObjects.getOverwrite() + subObjects.getDelete(), to<zen::Int64>(subObjects.getDataToProcess()));
            }
            dirObj.removeObject<LEFT_SIDE>(); //update DirMapping
            break;

        case SO_DELETE_RIGHT:
            //status information
            logText = replaceCpy(delHandlingRight_.getTxtRemovingDir(), L"%x", utf8CvrtTo<wxString>(dirObj.getFullName<RIGHT_SIDE>()));
            procCallback_.reportInfo(logText);

            delHandlingRight_.removeFolder(dirObj.getObjRelativeName()); //throw FileError
            {
                //progress indicator update: DON'T forget to notify about implicitly deleted objects!
                const SyncStatistics subObjects(dirObj);
                //...then remove everything
                dirObj.refSubFiles().clear();
                dirObj.refSubLinks().clear();
                dirObj.refSubDirs ().clear();
                procCallback_.updateProcessedData(subObjects.getCreate() + subObjects.getOverwrite() + subObjects.getDelete(), to<zen::Int64>(subObjects.getDataToProcess()));
            }
            dirObj.removeObject<RIGHT_SIDE>(); //update DirMapping
            break;

        case SO_OVERWRITE_RIGHT:
        case SO_OVERWRITE_LEFT:
            assert(false);
        case SO_UNRESOLVED_CONFLICT:
        case SO_DO_NOTHING:
        case SO_EQUAL:
            return; //no update on processed data!
    }

    //progress indicator update
    //indicator is updated only if directory is sync'ed correctly (and if some work was done)!
    procCallback_.updateProcessedData(1, 0);  //each call represents one processed file
    procCallback_.requestUiRefresh(); //may throw
}


namespace
{
void makeSameLength(std::wstring& first, std::wstring& second)
{
    const size_t maxPref = std::max(first.length(), second.length());
    first .resize(maxPref, L' ');
    second.resize(maxPref, L' ');
}

struct LessDependentDirectory : public std::binary_function<Zstring, Zstring, bool>
{
    bool operator()(const Zstring& lhs, const Zstring& rhs) const
    {
        return LessFilename()(Zstring(lhs.c_str(), std::min(lhs.length(), rhs.length())),
                              Zstring(rhs.c_str(), std::min(lhs.length(), rhs.length())));
    }
};

struct EqualDependentDirectory : public std::binary_function<Zstring, Zstring, bool>
{
    bool operator()(const Zstring& lhs, const Zstring& rhs) const
    {
        return EqualFilename()(Zstring(lhs.c_str(), std::min(lhs.length(), rhs.length())),
                               Zstring(rhs.c_str(), std::min(lhs.length(), rhs.length())));
    }
};
}


void SyncProcess::startSynchronizationProcess(const std::vector<FolderPairSyncCfg>& syncConfig, FolderComparison& folderCmp)
{
        //prevent shutdown while synchronization is in progress
        DisableStandby dummy;
        (void)dummy;

#ifdef NDEBUG
    wxLogNull noWxLogs; //prevent wxWidgets logging
#endif

    //PERF_START;

    if (syncConfig.size() != folderCmp.size())
        throw std::logic_error("Programming Error: Contract violation!");


    //inform about the total amount of data that will be processed from now on
    const SyncStatistics statisticsTotal(folderCmp);

    //keep at beginning so that all gui elements are initialized properly
    procCallback.initNewProcess(statisticsTotal.getCreate() + statisticsTotal.getOverwrite() + statisticsTotal.getDelete(),
                                to<zen::Int64>(statisticsTotal.getDataToProcess()),
                                ProcessCallback::PROCESS_SYNCHRONIZING);


    std::deque<bool> skipFolderPair(folderCmp.size()); //folder pairs may be skipped after fatal errors were found


    //initialize deletion handling: already required when checking for warnings
    std::vector<std::pair<DeletionHandling, DeletionHandling>> delHandler;
    for (auto j = begin(folderCmp); j != end(folderCmp); ++j)
    {
        const size_t folderIndex = j - folderCmp.begin();
        const FolderPairSyncCfg& folderPairCfg = syncConfig[folderIndex];

        delHandler.push_back(std::make_pair(DeletionHandling(folderPairCfg.handleDeletion,
                                                             folderPairCfg.custDelFolder,
                                                             j->getBaseDirPf<LEFT_SIDE>(),
                                                             procCallback),

                                            DeletionHandling(folderPairCfg.handleDeletion,
                                                             folderPairCfg.custDelFolder,
                                                             j->getBaseDirPf<RIGHT_SIDE>(),
                                                             procCallback)));
    }

    //-------------------some basic checks:------------------------------------------


    //aggregate information
    typedef std::set<Zstring,         LessDependentDirectory> DirReadSet;  //count (at least one) read access
    typedef std::map<Zstring, size_t, LessDependentDirectory> DirWriteMap; //count (read+)write accesses
    DirReadSet  dirReadCount;
    DirWriteMap dirWriteCount;

    typedef std::vector<std::pair<Zstring, Zstring> > DirPairList;
    DirPairList significantDiff;

    typedef std::vector<std::pair<Zstring, std::pair<zen::Int64, zen::Int64> > > DirSpaceRequAvailList; //dirname / space required / space available
    DirSpaceRequAvailList diskSpaceMissing;

    typedef std::set<Zstring, LessDependentDirectory> DirRecyclerMissing;
    DirRecyclerMissing recyclMissing;

    //start checking folder pairs
    for (auto j = begin(folderCmp); j != end(folderCmp); ++j)
    {
        const size_t folderIndex = j - begin(folderCmp);

        //exclude some pathological case (leftdir, rightdir are empty)
        if (EqualFilename()(j->getBaseDirPf<LEFT_SIDE>(), j->getBaseDirPf<RIGHT_SIDE>()))
            continue;

        const FolderPairSyncCfg& folderPairCfg = syncConfig[folderIndex];
        const std::pair<DeletionHandling, DeletionHandling>& delHandlerFp = delHandler[folderIndex];

        const SyncStatistics folderPairStat(*j);

        //aggregate basic information
        const bool writeLeft = folderPairStat.getCreate   <LEFT_SIDE>() +
                               folderPairStat.getOverwrite<LEFT_SIDE>() +
                               folderPairStat.getDelete   <LEFT_SIDE>() > 0;

        const bool writeRight = folderPairStat.getCreate   <RIGHT_SIDE>() +
                                folderPairStat.getOverwrite<RIGHT_SIDE>() +
                                folderPairStat.getDelete   <RIGHT_SIDE>() > 0;

        //skip folder pair if there is nothing to do (except for automatic mode, where data base needs to be written even in this case)
        if (!writeLeft && !writeRight &&
            !folderPairCfg.inAutomaticMode)
        {
            skipFolderPair[folderIndex] = true; //skip creating (not yet existing) base directories in particular if there's no need
            continue;
        }


        //check empty input fields: basically this only makes sense if empty field is not target (and not automatic mode: because of db file creation)
        if ((j->getBaseDirPf<LEFT_SIDE>(). empty() && (writeLeft  || folderPairCfg.inAutomaticMode)) ||
            (j->getBaseDirPf<RIGHT_SIDE>().empty() && (writeRight || folderPairCfg.inAutomaticMode)))
        {
            procCallback.reportFatalError(_("Target directory name must not be empty!"));
            skipFolderPair[folderIndex] = true;
            continue;
        }

        //aggregate information of folders used by multiple pairs in read/write access
        if (!EqualDependentDirectory()(j->getBaseDirPf<LEFT_SIDE>(), j->getBaseDirPf<RIGHT_SIDE>())) //true in general
        {
            if (writeLeft)
            {
                ++dirWriteCount[j->getBaseDirPf<LEFT_SIDE>()];
                if (writeRight)
                    ++dirWriteCount[j->getBaseDirPf<RIGHT_SIDE>()];
                else
                    dirReadCount.insert(j->getBaseDirPf<RIGHT_SIDE>());
            }
            else if (writeRight)
            {
                dirReadCount.insert(j->getBaseDirPf<LEFT_SIDE>());
                ++dirWriteCount[j->getBaseDirPf<RIGHT_SIDE>()];
            }
        }
        else //if folder pair contains two dependent folders, a warning was already issued after comparison; in this context treat as one write access at most
        {
            if (writeLeft || writeRight)
                ++dirWriteCount[j->getBaseDirPf<LEFT_SIDE>()];
        }


        if (folderPairStat.getOverwrite() + folderPairStat.getDelete() > 0)
        {
            if (folderPairCfg.handleDeletion == zen::MOVE_TO_CUSTOM_DIRECTORY)
            {
                //check if user-defined directory for deletion was specified
                if (folderPairCfg.custDelFolder.empty())
                {
                    procCallback.reportFatalError(_("User-defined directory for deletion was not specified!"));
                    skipFolderPair[folderIndex] = true;
                    continue;
                }
            }
        }

        //avoid data loss when source directory doesn't (temporarily?) exist anymore AND user chose to ignore errors (else we wouldn't arrive here)
        if (folderPairStat.getCreate() +  folderPairStat.getOverwrite() + folderPairStat.getConflict() == 0 &&
            folderPairStat.getDelete() > 0) //deletions only... (respect filtered items!)
        {
            Zstring missingSrcDir;
            if (!j->getBaseDirPf<LEFT_SIDE>().empty() && !j->wasExisting<LEFT_SIDE>()) //important: we need to evaluate existence status from time of comparison!
                missingSrcDir = j->getBaseDirPf<LEFT_SIDE>();
            if (!j->getBaseDirPf<RIGHT_SIDE>().empty() && !j->wasExisting<RIGHT_SIDE>())
                missingSrcDir = j->getBaseDirPf<RIGHT_SIDE>();

            if (!missingSrcDir.empty())
            {
                procCallback.reportFatalError(_("Source directory does not exist anymore:") + L"\n\"" + missingSrcDir + L"\"");
                skipFolderPair[folderIndex] = true;
                continue;
            }
        }

        //check if more than 50% of total number of files/dirs are to be created/overwritten/deleted
        if (significantDifferenceDetected(folderPairStat))
            significantDiff.push_back(std::make_pair(j->getBaseDirPf<LEFT_SIDE>(), j->getBaseDirPf<RIGHT_SIDE>()));

        //check for sufficient free diskspace in left directory
        const std::pair<zen::Int64, zen::Int64> spaceNeeded = DiskSpaceNeeded(*j,
                                                                              delHandlerFp.first.deletionFreesSpace(),
                                                                              delHandlerFp.second.deletionFreesSpace()).getSpaceTotal();
        wxLongLong freeDiskSpaceLeft;
        if (wxGetDiskSpace(toWx(j->getBaseDirPf<LEFT_SIDE>()), NULL, &freeDiskSpaceLeft))
        {
            if (0 < freeDiskSpaceLeft && //zero disk space is either an error or not: in both cases this warning message is obsolete (WebDav seems to report 0)
                freeDiskSpaceLeft.ToDouble() < to<double>(spaceNeeded.first))
                diskSpaceMissing.push_back(std::make_pair(j->getBaseDirPf<LEFT_SIDE>(), std::make_pair(spaceNeeded.first, freeDiskSpaceLeft.ToDouble())));
        }

        //check for sufficient free diskspace in right directory
        wxLongLong freeDiskSpaceRight;
        if (wxGetDiskSpace(toWx(j->getBaseDirPf<RIGHT_SIDE>()), NULL, &freeDiskSpaceRight))
        {
            if (0 < freeDiskSpaceRight && //zero disk space is either an error or not: in both cases this warning message is obsolete (WebDav seems to report 0)
                freeDiskSpaceRight.ToDouble() < to<double>(spaceNeeded.second))
                diskSpaceMissing.push_back(std::make_pair(j->getBaseDirPf<RIGHT_SIDE>(), std::make_pair(spaceNeeded.second, freeDiskSpaceRight.ToDouble())));
        }

        //windows: check if recycle bin really exists; if not, Windows will silently delete, which is wrong
#ifdef FFS_WIN
        if (folderPairCfg.handleDeletion == MOVE_TO_RECYCLE_BIN)
        {
            if (folderPairStat.getOverwrite<LEFT_SIDE>() +
                folderPairStat.getDelete   <LEFT_SIDE>() > 0 &&

                recycleBinStatus(j->getBaseDirPf<LEFT_SIDE>()) != STATUS_REC_EXISTS)
                recyclMissing.insert(j->getBaseDirPf<LEFT_SIDE>());

            if (folderPairStat.getOverwrite<RIGHT_SIDE>() +
                folderPairStat.getDelete   <RIGHT_SIDE>() > 0 &&

                recycleBinStatus(j->getBaseDirPf<RIGHT_SIDE>()) != STATUS_REC_EXISTS)
                recyclMissing.insert(j->getBaseDirPf<RIGHT_SIDE>());
        }
#endif
    }

    //check if unresolved conflicts exist
    if (statisticsTotal.getConflict() > 0)
    {
        //show the first few conflicts in warning message also:
        wxString warningMessage = wxString(_("Unresolved conflicts existing!")) +
                                  wxT(" (") + toStringSep(statisticsTotal.getConflict()) + wxT(")\n\n");

        const SyncStatistics::ConflictTexts& firstConflicts = statisticsTotal.getFirstConflicts(); //get first few sync conflicts
        for (SyncStatistics::ConflictTexts::const_iterator i = firstConflicts.begin(); i != firstConflicts.end(); ++i)
        {
            wxString conflictDescription = i->second;
            //conflictDescription.Replace(wxT("\n"), wxT(" ")); //remove line-breaks

            warningMessage += std::wstring(L"\"") + i->first + L"\": " + conflictDescription + L"\n\n";
        }

        if (statisticsTotal.getConflict() > static_cast<int>(firstConflicts.size()))
            warningMessage += wxT("[...]\n\n");

        warningMessage += _("You can ignore conflicts and continue synchronization.");

        procCallback.reportWarning(warningMessage, m_warnings.warningUnresolvedConflicts);
    }


    //check if more than 50% of total number of files/dirs are to be created/overwritten/deleted
    if (!significantDiff.empty())
    {
        wxString warningMessage = _("Significant difference detected:");

        for (DirPairList::const_iterator i = significantDiff.begin(); i != significantDiff.end(); ++i)
            warningMessage += std::wstring(L"\n\n") +
                              i->first + L" <-> " + L"\n" +
                              i->second;
        warningMessage += wxString(L"\n\n") + _("More than 50% of the total number of files will be copied or deleted!");

        procCallback.reportWarning(warningMessage, m_warnings.warningSignificantDifference);
    }


    //check for sufficient free diskspace
    if (!diskSpaceMissing.empty())
    {
        wxString warningMessage = _("Not enough free disk space available in:");

        for (auto i = diskSpaceMissing.begin(); i != diskSpaceMissing.end(); ++i)
            warningMessage += std::wstring(L"\n\n") +
                              L"\"" + i->first + L"\"\n" +
                              _("Free disk space required:")  + wxT(" ") + filesizeToShortString(to<UInt64>(i->second.first))  + L"\n" +
                              _("Free disk space available:") + wxT(" ") + filesizeToShortString(to<UInt64>(i->second.second));

        procCallback.reportWarning(warningMessage, m_warnings.warningNotEnoughDiskSpace);
    }


    //windows: check if recycle bin really exists; if not, Windows will silently delete, which is wrong
#ifdef FFS_WIN
    if (!recyclMissing.empty())
    {
        wxString warningMessage = _("Recycle Bin is not available for the following paths! Files will be deleted permanently instead:");
        warningMessage += L"\n";

        std::for_each(recyclMissing.begin(), recyclMissing.end(),
        [&](const Zstring& path) { warningMessage += L"\n" + toWx(path); });

        procCallback.reportWarning(warningMessage, m_warnings.warningRecyclerMissing);
    }
#endif

    //check if folders are used by multiple pairs in read/write access
    std::vector<Zstring> conflictDirs;
    for (DirWriteMap::const_iterator i = dirWriteCount.begin(); i != dirWriteCount.end(); ++i)
        if (i->second >= 2 ||                                  //multiple write accesses
            (i->second == 1 && dirReadCount.find(i->first) != dirReadCount.end())) //read/write access
            conflictDirs.push_back(i->first);

    if (!conflictDirs.empty())
    {
        wxString warningMessage = wxString(_("A directory will be modified which is part of multiple folder pairs! Please review synchronization settings!")) + wxT("\n");
        for (std::vector<Zstring>::const_iterator i = conflictDirs.begin(); i != conflictDirs.end(); ++i)
            warningMessage += std::wstring(L"\n") +
                              L"\"" + *i + L"\"";
        procCallback.reportWarning(warningMessage, m_warnings.warningMultiFolderWriteAccess);
    }

    //-------------------end of basic checks------------------------------------------

#ifdef FFS_WIN
    //shadow copy buffer: per sync-instance, not folder pair
    boost::scoped_ptr<shadow::ShadowCopy> shadowCopyHandler(copyLockedFiles_ ? new shadow::ShadowCopy : NULL);
#endif

    try
    {
        //loop through all directory pairs
        for (auto j = begin(folderCmp); j != end(folderCmp); ++j)
        {
            const size_t folderIndex = j - begin(folderCmp);

            const FolderPairSyncCfg& folderPairCfg = syncConfig[folderIndex];
            std::pair<DeletionHandling, DeletionHandling>& delHandlerFp = delHandler[folderIndex];

            if (skipFolderPair[folderIndex]) //folder pairs may be skipped after fatal errors were found
                continue;

            //exclude some pathological case (leftdir, rightdir are empty)
            if (EqualFilename()(j->getBaseDirPf<LEFT_SIDE>(), j->getBaseDirPf<RIGHT_SIDE>()))
                continue;

            //------------------------------------------------------------------------------------------
            //info about folder pair to be processed (useful for logfile)
            std::wstring left  = _("Left")  + L": ";
            std::wstring right = _("Right") + L": ";
            makeSameLength(left, right);

            const std::wstring statusTxt = _("Processing folder pair:") + L" \n" +
                                           L"\t" + left  + L"\"" + j->getBaseDirPf<LEFT_SIDE>()  + L"\"" + L" \n" +
                                           L"\t" + right + L"\"" + j->getBaseDirPf<RIGHT_SIDE>() + L"\"";
            procCallback.reportInfo(statusTxt);

            //------------------------------------------------------------------------------------------

            //create base directories first (if not yet existing) -> no symlink or attribute copying! -> single error message instead of one per file (e.g. unplugged network drive)
            const Zstring dirnameLeft = beforeLast(j->getBaseDirPf<LEFT_SIDE>(), FILE_NAME_SEPARATOR);
            if (!dirnameLeft.empty() && !dirExistsUpdating(dirnameLeft, false, procCallback))
            {
                if (!tryReportingError(procCallback, [&]() { createDirectory(dirnameLeft); })) //may throw in error-callback!
                continue; //skip this folder pair
            }
            const Zstring dirnameRight = beforeLast(j->getBaseDirPf<RIGHT_SIDE>(), FILE_NAME_SEPARATOR);
            if (!dirnameRight.empty() && !dirExistsUpdating(dirnameRight, false, procCallback))
            {
                if (!tryReportingError(procCallback, [&]() { createDirectory(dirnameRight); })) //may throw in error-callback!
                continue; //skip this folder pair
            }
            //------------------------------------------------------------------------------------------
            //execute synchronization recursively

            //update synchronization database (automatic sync only)
            zen::ScopeGuard guardUpdateDb = zen::makeGuard([&]()
            {
                if (folderPairCfg.inAutomaticMode)
                    zen::saveToDisk(*j); //throw FileError
            });

            //guarantee removal of invalid entries (where element on both sides is empty)
            ZEN_ON_BLOCK_EXIT(BaseDirMapping::removeEmpty(*j););

            SynchronizeFolderPair syncFP(*this,
#ifdef FFS_WIN
                                         shadowCopyHandler.get(),
#endif
                                         delHandlerFp.first, delHandlerFp.second);

            syncFP.startSync(*j);

            //(try to gracefully) cleanup temporary folders (Recycle bin optimization) -> will be done in ~DeletionHandling anyway...
            tryReportingError(procCallback, [&]() { delHandlerFp.first .tryCleanup(); }); //show error dialog if necessary
            tryReportingError(procCallback, [&]() { delHandlerFp.second.tryCleanup(); }); //

            //(try to gracefully) write database file (will be done in ~EnforceUpdateDatabase anyway...)
            if (folderPairCfg.inAutomaticMode)
            {
                procCallback.reportStatus(_("Generating database..."));
                procCallback.forceUiRefresh();

                tryReportingError(procCallback, [&]() { zen::saveToDisk(*j); }); //throw FileError
                guardUpdateDb.dismiss();
            }
        }

        if (!synchronizationNeeded(statisticsTotal))
            procCallback.reportInfo(_("Nothing to synchronize according to configuration!")); //inform about this special case
    }
    catch (const std::exception& e)
    {
        procCallback.reportFatalError(wxString::FromUTF8(e.what()));
    }
}


//###########################################################################################
//callback functionality for smooth progress indicators

template <class DelTargetCommand>
class WhileCopying : public zen::CallbackCopyFile //callback functionality
{
public:
    WhileCopying(UInt64& bytesReported,
                 ProcessCallback& statusHandler,
                 const DelTargetCommand& cmd) :
        bytesReported_(bytesReported),
        statusHandler_(statusHandler),
        cmd_(cmd) {}

    virtual void deleteTargetFile(const Zstring& targetFile) { cmd_(); }

    virtual void updateCopyStatus(UInt64 totalBytesTransferred)
    {
        //inform about the (differential) processed amount of data
        statusHandler_.updateProcessedData(0, to<Int64>(totalBytesTransferred) - to<Int64>(bytesReported_)); //throw()! -> this ensures client and service provider are in sync!
        bytesReported_ = totalBytesTransferred;                                                              //

        statusHandler_.requestUiRefresh(); //may throw
    }

private:
    UInt64&     bytesReported_;
    ProcessCallback& statusHandler_;
    DelTargetCommand cmd_;
};


//copy file while refreshing UI
template <SelectedSide side, class DelTargetCommand>
void SynchronizeFolderPair::copyFileUpdatingTo(const FileMapping& fileObj, const DelTargetCommand& cmd, FileDescriptor& sourceAttr) const
{
    const UInt64 totalBytesToCpy = fileObj.getFileSize<OtherSide<side>::result>();
    Zstring source = fileObj.getFullName<OtherSide<side>::result>();
    const Zstring& target = fileObj.getBaseDirPf<side>() + fileObj.getRelativeName<OtherSide<side>::result>();

    auto copyOperation = [&]()
    {
        //start of (possibly) long-running copy process: ensure status updates are performed regularly
        UInt64 bytesReported;
        //in error situation: undo communication of processed amount of data
        zen::ScopeGuard guardStatistics = zen::makeGuard([&]() { procCallback_.updateProcessedData(0, -1 * to<Int64>(bytesReported)); });

        WhileCopying<DelTargetCommand> callback(bytesReported, procCallback_, cmd);
        FileAttrib fileAttr;

        zen::copyFile(source, //type File implicitly means symlinks need to be dereferenced!
                      target,
                      copyFilePermissions,
                      transactionalFileCopy,
                      &callback,
                      &fileAttr); //throw FileError, ErrorFileLocked

        sourceAttr = FileDescriptor(fileAttr.modificationTime, fileAttr.fileSize);

        //inform about the (remaining) processed amount of data
        procCallback_.updateProcessedData(0, to<Int64>(totalBytesToCpy) - to<Int64>(bytesReported));
        bytesReported = totalBytesToCpy;

        guardStatistics.dismiss();
    };

#ifdef FFS_WIN
    try
    {
        copyOperation();
    }
    catch (ErrorFileLocked&)
    {
        //if file is locked (try to) use Windows Volume Shadow Copy Service
        if (shadowCopyHandler_ == NULL)
            throw;
        try
        {
            //contains prefix: E.g. "\\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy1\Program Files\FFS\sample.dat"
            source = shadowCopyHandler_->makeShadowCopy(source); //throw FileError
        }
        catch (const FileError& e)
        {
            const std::wstring errorMsg = replaceCpy(_("Error copying locked file %x!"), L"%x", std::wstring(L"\"") + source + L"\"");
            throw FileError(errorMsg + L"\n\n" + e.toString());
        }

        //now try again
        copyOperation();
    }
#else
    copyOperation();
#endif


    //#################### Verification #############################
    if (verifyCopiedFiles)
    {
        zen::ScopeGuard guardTarget     = zen::makeGuard([&]() { removeFile(target); }); //delete target if verification fails
        zen::ScopeGuard guardStatistics = zen::makeGuard([&]() { procCallback_.updateProcessedData(0, -1 * to<Int64>(totalBytesToCpy)); });

        verifyFileCopy(source, target); //throw FileError

        guardTarget.dismiss();
        guardStatistics.dismiss();
    }
}


template <zen::SelectedSide side>
void SynchronizeFolderPair::deleteSymlink(const SymLinkMapping& linkObj) const
{
    const DeletionHandling& delHandling = side == LEFT_SIDE ? delHandlingLeft_ : delHandlingRight_;

    switch (linkObj.getLinkType<side>())
    {
        case LinkDescriptor::TYPE_DIR:
            delHandling.removeFolder(linkObj.getObjRelativeName()); //throw FileError
            break;

        case LinkDescriptor::TYPE_FILE: //Windows: true file symlink; Linux: file-link or broken link
            delHandling.removeFile(linkObj.getObjRelativeName()); //throw FileError
            break;
    }
}


//--------------------- data verification -------------------------

//callback functionality for status updates while verifying
struct VerifyCallback
{
    virtual ~VerifyCallback() {}
    virtual void updateStatus() = 0;
};


void verifyFiles(const Zstring& source, const Zstring& target, VerifyCallback& callback) // throw (FileError)
{
    static std::vector<char> memory1(1024 * 1024); //1024 kb seems to be a reasonable buffer size
    static std::vector<char> memory2(1024 * 1024);

#ifdef FFS_WIN
    wxFile file1(applyLongPathPrefix(source).c_str(), wxFile::read); //don't use buffered file input for verification!
#elif defined FFS_LINUX
    wxFile file1(::open(source.c_str(), O_RDONLY)); //utilize UTF-8 filename
#endif
    if (!file1.IsOpened())
        throw FileError(_("Error opening file:") + L" \"" + source + L"\"");

#ifdef FFS_WIN
    wxFile file2(applyLongPathPrefix(target).c_str(), wxFile::read); //don't use buffered file input for verification!
#elif defined FFS_LINUX
    wxFile file2(::open(target.c_str(), O_RDONLY)); //utilize UTF-8 filename
#endif
    if (!file2.IsOpened()) //NO cleanup necessary for (wxFile) file1
        throw FileError(_("Error opening file:") + L" \"" + target + L"\"");

    do
    {
        const size_t length1 = file1.Read(&memory1[0], memory1.size());
        if (file1.Error())
            throw FileError(_("Error reading file:") + L" \"" + source + L"\"");
        callback.updateStatus(); //send progress updates

        const size_t length2 = file2.Read(&memory2[0], memory2.size());
        if (file2.Error())
            throw FileError(_("Error reading file:") + L" \"" + target + L"\"");
        callback.updateStatus(); //send progress updates

        if (length1 != length2 || ::memcmp(&memory1[0], &memory2[0], length1) != 0)
            throw FileError(_("Data verification error: Source and target file have different content!") + L"\n" + L"\"" + source + L"\" -> \n\"" + target + L"\"");
    }
    while (!file1.Eof());

    if (!file2.Eof())
        throw FileError(_("Data verification error: Source and target file have different content!") + L"\n" + L"\"" + source + L"\" -> \n\"" + target + L"\"");
}


class VerifyStatusUpdater : public VerifyCallback
{
public:
    VerifyStatusUpdater(ProcessCallback& statusHandler) : statusHandler_(statusHandler) {}

    virtual void updateStatus() { statusHandler_.requestUiRefresh(); } //trigger display refresh

private:
    ProcessCallback& statusHandler_;
};


void SynchronizeFolderPair::verifyFileCopy(const Zstring& source, const Zstring& target) const
{
    wxString logText = replaceCpy(txtVerifying, L"%x", utf8CvrtTo<wxString>(target));
    procCallback_.reportInfo(logText);

    VerifyStatusUpdater callback(procCallback_);

    tryReportingError(procCallback_, [&]() { ::verifyFiles(source, target, callback);});
}
