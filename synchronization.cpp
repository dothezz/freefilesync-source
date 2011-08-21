// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "synchronization.h"
#include <stdexcept>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include "shared/string_conv.h"
#include "shared/util.h"
#include "shared/loki/ScopeGuard.h"
#include "library/status_handler.h"
#include "shared/file_handling.h"
#include "shared/resolve_path.h"
#include "shared/recycler.h"
#include "shared/i18n.h"
#include <wx/file.h>
#include <boost/bind.hpp>
#include "shared/global_func.h"
#include "shared/disable_standby.h"
#include <memory>
#include "library/db_file.h"
#include "library/dir_exist_async.h"
#include "library/cmp_filetime.h"
#include "shared/file_io.h"
#include <deque>

#ifdef FFS_WIN
#include "shared/long_path_prefix.h"
#include <boost/scoped_ptr.hpp>
#include "shared/perf.h"
#include "shared/shadow.h"
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
    std::for_each(folderCmp.begin(), folderCmp.end(), boost::bind(&SyncStatistics::getNumbersRecursively, this, _1));
}


SyncStatistics::SyncStatistics(const HierarchyObject&  hierObj)
{
    init();
    getNumbersRecursively(hierObj);
}


inline
void SyncStatistics::getNumbersRecursively(const HierarchyObject& hierObj)
{
    //process directories
    std::for_each(hierObj.useSubDirs().begin(), hierObj.useSubDirs().end(),
                  boost::bind(&SyncStatistics::getDirNumbers, this, _1));

    //process files
    std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(),
                  boost::bind(&SyncStatistics::getFileNumbers, this, _1));

    //process symlinks
    std::for_each(hierObj.useSubLinks().begin(), hierObj.useSubLinks().end(),
                  boost::bind(&SyncStatistics::getLinkNumbers, this, _1));

    rowsTotal += hierObj.useSubDirs(). size();
    rowsTotal += hierObj.useSubFiles().size();
    rowsTotal += hierObj.useSubLinks().size();
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
    for (std::vector<FolderPairEnh>::const_iterator i = allPairs.begin(); i != allPairs.end(); ++i)
        output.push_back(
            i->altSyncConfig.get() ?

            FolderPairSyncCfg(i->altSyncConfig->syncConfiguration.var == SyncConfig::AUTOMATIC,
                              i->altSyncConfig->handleDeletion,
                              toZ(i->altSyncConfig->customDeletionDirectory)) :

            FolderPairSyncCfg(mainCfg.syncConfiguration.var == SyncConfig::AUTOMATIC,
                              mainCfg.handleDeletion,
                              toZ(mainCfg.customDeletionDirectory)));
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
            ProcessCallback::Response rv = handler.reportError(error.msg()); //may throw!
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

    if (!formattedDir.EndsWith(FILE_NAME_SEPARATOR))
        formattedDir += FILE_NAME_SEPARATOR;

    wxString timeNow = wxDateTime::Now().FormatISOTime();
    replace(timeNow, L":", L"");

    const wxString sessionName = wxDateTime::Now().FormatISODate() + wxChar(' ') + timeNow;
    formattedDir += prefix + toZ(sessionName);


    //ensure that session directory does not yet exist (must be unique)
    Zstring output = formattedDir;

    //ensure uniqueness
    for (int i = 1; zen::somethingExists(output); ++i)
        output = formattedDir + Zchar('_') + Zstring::fromNumber(i);

    output += FILE_NAME_SEPARATOR;
    return output;
}


SyncProcess::SyncProcess(xmlAccess::OptionalDialogs& warnings,
                         bool verifyCopiedFiles,
                         bool copyLockedFiles,
                         bool copyFilePermissions,
                         ProcessCallback& handler) :
    verifyCopiedFiles_(verifyCopiedFiles),
    copyLockedFiles_(copyLockedFiles),
    copyFilePermissions_(copyFilePermissions),
    m_warnings(warnings),
    procCallback(handler) {}
//--------------------------------------------------------------------------------------------------------------


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

    void removeFile  (const Zstring& relativeName) const; //throw (FileError)
    void removeFolder(const Zstring& relativeName) const; //throw (FileError)

    const Zstring& getTxtRemovingFile   () const { return txtRemovingFile;      } //
    const Zstring& getTxtRemovingSymLink() const { return txtRemovingSymlink;   } //status text templates
    const Zstring& getTxtRemovingDir    () const { return txtRemovingDirectory; }; //

    //evaluate whether a deletion will actually free space within a volume
    bool deletionFreesSpace() const;

private:
    DeletionPolicy deletionType;
    ProcessCallback* procCallback_; //always bound! need assignment operator => not a reference

    Zstring sessionDelDir;  //target deletion folder for current folder pair (with timestamp, ends with path separator)

    Zstring baseDir_;  //with separator postfix

    //preloaded status texts:
    Zstring txtRemovingFile;
    Zstring txtRemovingSymlink;
    Zstring txtRemovingDirectory;

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
    if (deletionType == MOVE_TO_RECYCLE_BIN && recycleBinStatus(baseDir) != STATUS_REC_EXISTS)
        deletionType = DELETE_PERMANENTLY; //Windows' ::SHFileOperation() will do this anyway, but we have a better and faster deletion routine (e.g. on networks)
#endif

    switch (deletionType)
    {
        case DELETE_PERMANENTLY:
            txtRemovingFile      = toZ(_("Deleting file %x")).Replace(Zstr("%x"), Zstr("\n\"%x\""), false);
            txtRemovingSymlink   = toZ(_("Deleting Symbolic Link %x")).Replace(Zstr("%x"), Zstr("\n\"%x\""), false);
            txtRemovingDirectory = toZ(_("Deleting folder %x")).Replace( Zstr("%x"), Zstr("\n\"%x\""), false);
            break;

        case MOVE_TO_RECYCLE_BIN:
            sessionDelDir = getSessionDeletionDir(baseDir_, Zstr("FFS "));

            txtRemovingFile =
                txtRemovingSymlink =
                    txtRemovingDirectory = toZ(_("Moving %x to Recycle Bin")).Replace(Zstr("%x"), Zstr("\"%x\""), false);
            break;

        case MOVE_TO_CUSTOM_DIRECTORY:
            sessionDelDir = getSessionDeletionDir(custDelFolder);

            txtRemovingFile      = toZ(_("Moving file %x to user-defined directory %y")).         Replace(Zstr("%x"), Zstr("\"%x\"\n"), false).Replace(Zstr("%y"), Zstring(Zstr("\"")) + custDelFolder + Zstr("\""), false);
            txtRemovingDirectory = toZ(_("Moving folder %x to user-defined directory %y")).       Replace(Zstr("%x"), Zstr("\"%x\"\n"), false).Replace(Zstr("%y"), Zstring(Zstr("\"")) + custDelFolder + Zstr("\""), false);
            txtRemovingSymlink   = toZ(_("Moving Symbolic Link %x to user-defined directory %y")).Replace(Zstr("%x"), Zstr("\"%x\"\n"), false).Replace(Zstr("%y"), Zstring(Zstr("\"")) + custDelFolder + Zstr("\""), false);
            break;
    }
}


DeletionHandling::~DeletionHandling()
{
    //always (try to) clean up, even if synchronization is aborted!
    try { tryCleanup(); }
    catch (...) {}   //make sure this stays non-blocking!
}


void DeletionHandling::tryCleanup() //throw FileError
{
    if (!cleanedUp)
    {
        if (deletionType == MOVE_TO_RECYCLE_BIN) //clean-up temporary directory (recycle bin)
            zen::moveToRecycleBin(sessionDelDir.BeforeLast(FILE_NAME_SEPARATOR)); //throw FileError

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
    CallbackRemoveDirImpl(ProcessCallback& handler) : statusHandler_(handler) {}

    virtual void notifyDeletion(const Zstring& currentObject)
    {
        statusHandler_.requestUiRefresh(); //exceptions may be thrown here!
    }

private:
    ProcessCallback& statusHandler_;
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
                const Zstring targetDir  = targetFile.BeforeLast(FILE_NAME_SEPARATOR);

                try //rename file: no copying!!!
                {
                    if (!dirExistsUpdating(targetDir, *procCallback_))
                        createDirectory(targetDir); //throw FileError -> may legitimately fail on Linux if permissions are missing

                    //performance optimization!! Instead of moving each object into recycle bin separately, we rename them ony by one into a
                    //temporary directory and delete this directory only ONCE!
                    renameFile(fullName, targetFile); //throw (FileError);
                }
                catch (...)
                {
                    //if anything went wrong, move to recycle bin the standard way (single file processing: slow)
                    moveToRecycleBin(fullName);  //throw (FileError)
                }
            }
            break;

        case MOVE_TO_CUSTOM_DIRECTORY:
            if (fileExists(fullName))
            {
                const Zstring targetFile = sessionDelDir + relativeName; //altDeletionDir ends with path separator
                const Zstring targetDir  = targetFile.BeforeLast(FILE_NAME_SEPARATOR);

                if (!dirExistsUpdating(targetDir, *procCallback_))
                    createDirectory(targetDir); //throw (FileError)

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
            CallbackRemoveDirImpl remDirCallback(*procCallback_);
            removeDirectory(fullName, &remDirCallback);
        }
        break;

        case MOVE_TO_RECYCLE_BIN:
            if (!dirExistsUpdating(fullName, *procCallback_))
            {
                const Zstring targetDir      = sessionDelDir + relativeName;
                const Zstring targetSuperDir = targetDir.BeforeLast(FILE_NAME_SEPARATOR);

                try //rename directory: no copying!!!
                {
                    if (!dirExistsUpdating(targetSuperDir, *procCallback_))
                        createDirectory(targetSuperDir); //throw FileError -> may legitimately fail on Linux if permissions are missing

                    //performance optimization!! Instead of moving each object into recycle bin separately, we rename them ony by one into a
                    //temporary directory and delete this directory only ONCE!
                    renameFile(fullName, targetDir); //throw (FileError);
                }
                catch (...)
                {
                    //if anything went wrong, move to recycle bin the standard way (single file processing: slow)
                    moveToRecycleBin(fullName); //throw (FileError)

                }
            }
            break;

        case MOVE_TO_CUSTOM_DIRECTORY:
            if (!dirExistsUpdating(fullName, *procCallback_))
            {
                const Zstring targetDir      = sessionDelDir + relativeName;
                const Zstring targetSuperDir = targetDir.BeforeLast(FILE_NAME_SEPARATOR);

                if (!dirExistsUpdating(targetSuperDir, *procCallback_))
                    createDirectory(targetSuperDir); //throw (FileError)

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
        for (HierarchyObject::SubFileMapping::const_iterator i = hierObj.useSubFiles().begin(); i != hierObj.useSubFiles().end(); ++i)
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
        std::for_each(hierObj.useSubDirs().begin(), hierObj.useSubDirs().end(), boost::bind(&DiskSpaceNeeded::processRecursively, this, _1));
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
        verifyCopiedFiles_(syncProc.verifyCopiedFiles_),
        copyFilePermissions_(syncProc.copyFilePermissions_),
        txtCopyingFile    (toZ(_("Copying new file %x to %y")).         Replace(Zstr("%x"), Zstr("\"%x\""), false).Replace(Zstr("%y"), Zstr("\n\"%y\""), false)),
        txtCopyingLink    (toZ(_("Copying new Symbolic Link %x to %y")).Replace(Zstr("%x"), Zstr("\"%x\""), false).Replace(Zstr("%y"), Zstr("\n\"%y\""), false)),
        txtOverwritingFile(toZ(_("Overwriting file %x in %y")).         Replace(Zstr("%x"), Zstr("\"%x\""), false).Replace(Zstr("%y"), Zstr("\n\"%y\""), false)),
        txtOverwritingLink(toZ(_("Overwriting Symbolic Link %x in %y")).Replace(Zstr("%x"), Zstr("\"%x\""), false).Replace(Zstr("%y"), Zstr("\n\"%y\""), false)),
        txtCreatingFolder (toZ(_("Creating folder %x")).Replace(Zstr("%x"), Zstr("\n\"%x\""), false)),
        txtVerifying      (toZ(_("Verifying file %x")). Replace(Zstr("%x"), Zstr("\n\"%x\""), false)),
        txtWritingAttributes(toZ(_("Updating attributes of %x")).Replace(Zstr("%x"), Zstr("\n\"%x\""), false)) {}


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
    void copySymlink(const Zstring& source, const Zstring& target, LinkDescriptor::LinkType type, bool inRecursion = false) const;
    template <class DelTargetCommand>
    void copyFileUpdating(const Zstring& source, const Zstring& target, const DelTargetCommand& cmd, zen::UInt64 sourceFileSize, int recursionLvl = 0) const;
    void verifyFileCopy(const Zstring& source, const Zstring& target) const;

    ProcessCallback& procCallback_;
#ifdef FFS_WIN
    shadow::ShadowCopy* shadowCopyHandler_; //optional!
#endif
    const DeletionHandling& delHandlingLeft_;
    const DeletionHandling& delHandlingRight_;

    const bool verifyCopiedFiles_;
    const bool copyFilePermissions_;

    //preload status texts
    const Zstring txtCopyingFile;
    const Zstring txtCopyingLink;
    const Zstring txtOverwritingFile;
    const Zstring txtOverwritingLink;
    const Zstring txtCreatingFolder;
    const Zstring txtVerifying;
    const Zstring txtWritingAttributes;
};


template <SynchronizeFolderPair::PassId pass>
void SynchronizeFolderPair::execute(HierarchyObject& hierObj)
{
    //synchronize files:
    std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(),
                  [&](FileMapping& fileObj)
    {
        const bool letsDoThis = (pass == FIRST_PASS) == diskSpaceIsReduced(fileObj); //to be deleted files on first pass, rest on second!
        if (letsDoThis)
            tryReportingError(procCallback_, [&]() { synchronizeFile(fileObj); });
    });

    //synchronize symbolic links: (process in second step only)
    if (pass == SECOND_PASS)
        std::for_each(hierObj.useSubLinks().begin(), hierObj.useSubLinks().end(),
        [&](SymLinkMapping& linkObj) { tryReportingError(procCallback_, [&]() { synchronizeLink(linkObj); }); });

    //synchronize folders:
    std::for_each(hierObj.useSubDirs().begin(), hierObj.useSubDirs().end(),
                  [&](DirMapping& dirObj)
    {
        const bool letsDoThis = (pass == FIRST_PASS) == diskSpaceIsReduced(dirObj); //to be deleted files on first pass, rest on second!
        if (letsDoThis) //running folder creation on first pass only, works, but looks strange for initial mirror sync when all folders are created at once
            tryReportingError(procCallback_, [&]() { synchronizeFolder(dirObj); });

        //recursion!
        this->execute<pass>(dirObj);
    });
}


namespace
{
struct NullCommand
{
    void operator()() const {}
};


template <SelectedSide side>
class DelTargetCommand
{
public:
    DelTargetCommand(FileMapping& fileObj, const DeletionHandling& delHandling) : fileObj_(fileObj), delHandling_(delHandling) {}

    void operator()() const
    {
        //delete target and copy source
        delHandling_.removeFile(fileObj_.getObjRelativeName()); //throw (FileError)
        fileObj_.removeObject<side>(); //remove file from FileMapping, to keep in sync (if subsequent copying fails!!)
    }

private:
    FileMapping& fileObj_;
    const DeletionHandling& delHandling_;
};
}


void SynchronizeFolderPair::synchronizeFile(FileMapping& fileObj) const
{
    Zstring statusText;
    Zstring target;

    switch (fileObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_CREATE_NEW_LEFT:
            target = fileObj.getBaseDirPf<LEFT_SIDE>() + fileObj.getRelativeName<RIGHT_SIDE>(); //can't use "getFullName" as target is not yet existing

            statusText = txtCopyingFile;
            statusText.Replace(Zstr("%x"), fileObj.getShortName<RIGHT_SIDE>(), false);
            statusText.Replace(Zstr("%y"), target.BeforeLast(FILE_NAME_SEPARATOR), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            copyFileUpdating(fileObj.getFullName<RIGHT_SIDE>(), target,
                             NullCommand(), //no target to delete
                             fileObj.getFileSize<RIGHT_SIDE>());
            break;

        case SO_CREATE_NEW_RIGHT:
            target = fileObj.getBaseDirPf<RIGHT_SIDE>() + fileObj.getRelativeName<LEFT_SIDE>();

            statusText = txtCopyingFile;
            statusText.Replace(Zstr("%x"), fileObj.getShortName<LEFT_SIDE>(), false);
            statusText.Replace(Zstr("%y"), target.BeforeLast(FILE_NAME_SEPARATOR), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            copyFileUpdating(fileObj.getFullName<LEFT_SIDE>(), target,
                             NullCommand(), //no target to delete
                             fileObj.getFileSize<LEFT_SIDE>());
            break;

        case SO_DELETE_LEFT:
            statusText = delHandlingLeft_.getTxtRemovingFile();
            statusText.Replace(Zstr("%x"), fileObj.getFullName<LEFT_SIDE>(), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            delHandlingLeft_.removeFile(fileObj.getObjRelativeName()); //throw (FileError)
            break;

        case SO_DELETE_RIGHT:
            statusText = delHandlingRight_.getTxtRemovingFile();
            statusText.Replace(Zstr("%x"), fileObj.getFullName<RIGHT_SIDE>(), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            delHandlingRight_.removeFile(fileObj.getObjRelativeName()); //throw (FileError)
            break;

        case SO_OVERWRITE_LEFT:
            target = fileObj.getBaseDirPf<LEFT_SIDE>() + fileObj.getRelativeName<RIGHT_SIDE>(); //respect differences in case of source object

            statusText = txtOverwritingFile;
            statusText.Replace(Zstr("%x"), fileObj.getShortName<RIGHT_SIDE>(), false);
            statusText.Replace(Zstr("%y"), fileObj.getFullName<LEFT_SIDE>().BeforeLast(FILE_NAME_SEPARATOR), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            copyFileUpdating(fileObj.getFullName<RIGHT_SIDE>(), target,
                             DelTargetCommand<LEFT_SIDE>(fileObj, delHandlingLeft_), //delete target at appropriate time
                             fileObj.getFileSize<RIGHT_SIDE>());
            break;

        case SO_OVERWRITE_RIGHT:
            target = fileObj.getBaseDirPf<RIGHT_SIDE>() + fileObj.getRelativeName<LEFT_SIDE>(); //respect differences in case of source object

            statusText = txtOverwritingFile;
            statusText.Replace(Zstr("%x"), fileObj.getShortName<LEFT_SIDE>(), false);
            statusText.Replace(Zstr("%y"), fileObj.getFullName<RIGHT_SIDE>().BeforeLast(FILE_NAME_SEPARATOR), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            copyFileUpdating(fileObj.getFullName<LEFT_SIDE>(), target,
                             DelTargetCommand<RIGHT_SIDE>(fileObj, delHandlingRight_), //delete target at appropriate time
                             fileObj.getFileSize<LEFT_SIDE>());
            break;

        case SO_COPY_METADATA_TO_LEFT:
            statusText = txtWritingAttributes;
            statusText.Replace(Zstr("%x"), fileObj.getFullName<LEFT_SIDE>(), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            if (fileObj.getShortName<LEFT_SIDE>() != fileObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(fileObj.getFullName<LEFT_SIDE>(),
                           fileObj.getFullName<LEFT_SIDE>().BeforeLast(FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + fileObj.getShortName<RIGHT_SIDE>()); //throw (FileError);

            if (!sameFileTime(fileObj.getLastWriteTime<LEFT_SIDE>(), fileObj.getLastWriteTime<RIGHT_SIDE>(), 2)) ////respect 2 second FAT/FAT32 precision
                copyFileTimes(fileObj.getFullName<RIGHT_SIDE>(), fileObj.getFullName<LEFT_SIDE>(), true); //deref symlinks; throw (FileError)
            break;

        case SO_COPY_METADATA_TO_RIGHT:
            statusText = txtWritingAttributes;
            statusText.Replace(Zstr("%x"), fileObj.getFullName<RIGHT_SIDE>(), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            if (fileObj.getShortName<LEFT_SIDE>() != fileObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(fileObj.getFullName<RIGHT_SIDE>(),
                           fileObj.getFullName<RIGHT_SIDE>().BeforeLast(FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + fileObj.getShortName<LEFT_SIDE>()); //throw (FileError);

            if (!sameFileTime(fileObj.getLastWriteTime<LEFT_SIDE>(), fileObj.getLastWriteTime<RIGHT_SIDE>(), 2)) ////respect 2 second FAT/FAT32 precision
                copyFileTimes(fileObj.getFullName<LEFT_SIDE>(), fileObj.getFullName<RIGHT_SIDE>(), true); //deref symlinks; throw (FileError)
            break;

        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
            return; //no update on processed data!
    }

    //update FileMapping
    fileObj.synchronizeSides();

    //progress indicator update
    //indicator is updated only if file is sync'ed correctly (and if some sync was done)!
    procCallback_.updateProcessedData(1, 0); //processed data is communicated in subfunctions!
    procCallback_.requestUiRefresh(); //may throw
}


void SynchronizeFolderPair::synchronizeLink(SymLinkMapping& linkObj) const
{
    Zstring statusText;
    Zstring target;

    switch (linkObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_CREATE_NEW_LEFT:
            target = linkObj.getBaseDirPf<LEFT_SIDE>() + linkObj.getRelativeName<RIGHT_SIDE>();

            statusText = txtCopyingLink;
            statusText.Replace(Zstr("%x"), linkObj.getShortName<RIGHT_SIDE>(), false);
            statusText.Replace(Zstr("%y"), target.BeforeLast(FILE_NAME_SEPARATOR), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            copySymlink(linkObj.getFullName<RIGHT_SIDE>(), target, linkObj.getLinkType<RIGHT_SIDE>());
            break;

        case SO_CREATE_NEW_RIGHT:
            target = linkObj.getBaseDirPf<RIGHT_SIDE>() + linkObj.getRelativeName<LEFT_SIDE>();

            statusText = txtCopyingLink;
            statusText.Replace(Zstr("%x"), linkObj.getShortName<LEFT_SIDE>(), false);
            statusText.Replace(Zstr("%y"), target.BeforeLast(FILE_NAME_SEPARATOR), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            copySymlink(linkObj.getFullName<LEFT_SIDE>(), target, linkObj.getLinkType<LEFT_SIDE>());
            break;

        case SO_DELETE_LEFT:
            statusText = delHandlingLeft_.getTxtRemovingSymLink();
            statusText.Replace(Zstr("%x"), linkObj.getFullName<LEFT_SIDE>(), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            deleteSymlink<LEFT_SIDE>(linkObj); //throw (FileError)
            break;

        case SO_DELETE_RIGHT:
            statusText = delHandlingRight_.getTxtRemovingSymLink();
            statusText.Replace(Zstr("%x"), linkObj.getFullName<RIGHT_SIDE>(), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            deleteSymlink<RIGHT_SIDE>(linkObj); //throw (FileError)
            break;

        case SO_OVERWRITE_LEFT:
            target = linkObj.getBaseDirPf<LEFT_SIDE>() + linkObj.getRelativeName<RIGHT_SIDE>(); //respect differences in case of source object

            statusText = txtOverwritingLink;
            statusText.Replace(Zstr("%x"), linkObj.getShortName<RIGHT_SIDE>(), false);
            statusText.Replace(Zstr("%y"), linkObj.getFullName<LEFT_SIDE>().BeforeLast(FILE_NAME_SEPARATOR), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            deleteSymlink<LEFT_SIDE>(linkObj); //throw (FileError)
            linkObj.removeObject<LEFT_SIDE>(); //remove file from FileMapping, to keep in sync (if subsequent copying fails!!)

            copySymlink(linkObj.getFullName<RIGHT_SIDE>(), target, linkObj.getLinkType<RIGHT_SIDE>());
            break;

        case SO_OVERWRITE_RIGHT:
            target = linkObj.getBaseDirPf<RIGHT_SIDE>() + linkObj.getRelativeName<LEFT_SIDE>(); //respect differences in case of source object

            statusText = txtOverwritingLink;
            statusText.Replace(Zstr("%x"), linkObj.getShortName<LEFT_SIDE>(), false);
            statusText.Replace(Zstr("%y"), linkObj.getFullName<RIGHT_SIDE>().BeforeLast(FILE_NAME_SEPARATOR), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            deleteSymlink<RIGHT_SIDE>(linkObj); //throw (FileError)
            linkObj.removeObject<RIGHT_SIDE>(); //remove file from FileMapping, to keep in sync (if subsequent copying fails!!)

            copySymlink(linkObj.getFullName<LEFT_SIDE>(), target, linkObj.getLinkType<LEFT_SIDE>());
            break;

        case SO_COPY_METADATA_TO_LEFT:
            statusText = txtWritingAttributes;
            statusText.Replace(Zstr("%x"), linkObj.getFullName<LEFT_SIDE>(), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            if (linkObj.getShortName<LEFT_SIDE>() != linkObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(linkObj.getFullName<LEFT_SIDE>(),
                           linkObj.getFullName<LEFT_SIDE>().BeforeLast(FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + linkObj.getShortName<RIGHT_SIDE>()); //throw (FileError);

            if (!sameFileTime(linkObj.getLastWriteTime<LEFT_SIDE>(), linkObj.getLastWriteTime<RIGHT_SIDE>(), 2)) ////respect 2 second FAT/FAT32 precision
                copyFileTimes(linkObj.getFullName<RIGHT_SIDE>(), linkObj.getFullName<LEFT_SIDE>(), false); //don't deref symlinks; throw (FileError)
            break;

        case SO_COPY_METADATA_TO_RIGHT:
            statusText = txtWritingAttributes;
            statusText.Replace(Zstr("%x"), linkObj.getFullName<RIGHT_SIDE>(), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            if (linkObj.getShortName<LEFT_SIDE>() != linkObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(linkObj.getFullName<RIGHT_SIDE>(),
                           linkObj.getFullName<RIGHT_SIDE>().BeforeLast(FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + linkObj.getShortName<LEFT_SIDE>()); //throw (FileError);

            if (!sameFileTime(linkObj.getLastWriteTime<LEFT_SIDE>(), linkObj.getLastWriteTime<RIGHT_SIDE>(), 2)) ////respect 2 second FAT/FAT32 precision
                copyFileTimes(linkObj.getFullName<LEFT_SIDE>(), linkObj.getFullName<RIGHT_SIDE>(), false); //don't deref symlinks; throw (FileError)
            break;

        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
            return; //no update on processed data!
    }

    //update FileMapping
    linkObj.synchronizeSides();

    //progress indicator update
    //indicator is updated only if file is sync'ed correctly (and if some sync was done)!
    procCallback_.updateProcessedData(1, 0); //processed data is communicated in subfunctions!
    procCallback_.requestUiRefresh(); //may throw
}


void SynchronizeFolderPair::synchronizeFolder(DirMapping& dirObj) const
{
    Zstring statusText;
    Zstring target;

    //synchronize folders:
    switch (dirObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_CREATE_NEW_LEFT:
            target = dirObj.getBaseDirPf<LEFT_SIDE>() + dirObj.getRelativeName<RIGHT_SIDE>();

            statusText = txtCreatingFolder;
            statusText.Replace(Zstr("%x"), target, false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            //some check to catch the error that directory on source has been deleted externally after "compare"...
            if (!dirExistsUpdating(dirObj.getFullName<RIGHT_SIDE>(), procCallback_))
                throw FileError(_("Source directory does not exist anymore:") + "\n\"" + dirObj.getFullName<RIGHT_SIDE>() + "\"");
            createDirectory(target, dirObj.getFullName<RIGHT_SIDE>(), copyFilePermissions_); //no symlink copying!
            break;

        case SO_CREATE_NEW_RIGHT:
            target = dirObj.getBaseDirPf<RIGHT_SIDE>() + dirObj.getRelativeName<LEFT_SIDE>();

            statusText = txtCreatingFolder;
            statusText.Replace(Zstr("%x"), target, false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            //some check to catch the error that directory on source has been deleted externally after "compare"...
            if (!dirExistsUpdating(dirObj.getFullName<LEFT_SIDE>(), procCallback_))
                throw FileError(_("Source directory does not exist anymore:") + "\n\"" + dirObj.getFullName<LEFT_SIDE>() + "\"");
            createDirectory(target, dirObj.getFullName<LEFT_SIDE>(), copyFilePermissions_); //no symlink copying!
            break;

        case SO_COPY_METADATA_TO_LEFT:
            statusText = txtWritingAttributes;
            statusText.Replace(Zstr("%x"), dirObj.getFullName<LEFT_SIDE>(), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            if (dirObj.getShortName<LEFT_SIDE>() != dirObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(dirObj.getFullName<LEFT_SIDE>(),
                           dirObj.getFullName<LEFT_SIDE>().BeforeLast(FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + dirObj.getShortName<RIGHT_SIDE>()); //throw (FileError);
            //copyFileTimes(dirObj.getFullName<RIGHT_SIDE>(), dirObj.getFullName<LEFT_SIDE>(), true); //throw (FileError) -> is executed after sub-objects have finished synchronization
            break;

        case SO_COPY_METADATA_TO_RIGHT:
            statusText = txtWritingAttributes;
            statusText.Replace(Zstr("%x"), dirObj.getFullName<RIGHT_SIDE>(), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            if (dirObj.getShortName<LEFT_SIDE>() != dirObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(dirObj.getFullName<RIGHT_SIDE>(),
                           dirObj.getFullName<RIGHT_SIDE>().BeforeLast(FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + dirObj.getShortName<LEFT_SIDE>()); //throw (FileError);
            //copyFileTimes(dirObj.getFullName<LEFT_SIDE>(), dirObj.getFullName<RIGHT_SIDE>(), true); //throw (FileError) -> is executed after sub-objects have finished synchronization
            break;

        case SO_DELETE_LEFT:
            //status information
            statusText = delHandlingLeft_.getTxtRemovingDir();
            statusText.Replace(Zstr("%x"), dirObj.getFullName<LEFT_SIDE>(), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            delHandlingLeft_.removeFolder(dirObj.getObjRelativeName()); //throw (FileError)
            {
                //progress indicator update: DON'T forget to notify about implicitly deleted objects!
                const SyncStatistics subObjects(dirObj);
                //...then remove everything
                dirObj.useSubFiles().clear();
                dirObj.useSubLinks().clear();
                dirObj.useSubDirs ().clear();
                procCallback_.updateProcessedData(subObjects.getCreate() + subObjects.getOverwrite() + subObjects.getDelete(), to<zen::Int64>(subObjects.getDataToProcess()));
            }
            break;

        case SO_DELETE_RIGHT:
            //status information
            statusText = delHandlingRight_.getTxtRemovingDir();
            statusText.Replace(Zstr("%x"), dirObj.getFullName<RIGHT_SIDE>(), false);
            procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

            delHandlingRight_.removeFolder(dirObj.getObjRelativeName()); //throw (FileError)
            {
                //progress indicator update: DON'T forget to notify about implicitly deleted objects!
                const SyncStatistics subObjects(dirObj);
                //...then remove everything
                dirObj.useSubFiles().clear();
                dirObj.useSubLinks().clear();
                dirObj.useSubDirs ().clear();
                procCallback_.updateProcessedData(subObjects.getCreate() + subObjects.getOverwrite() + subObjects.getDelete(), to<zen::Int64>(subObjects.getDataToProcess()));
            }
            break;

        case SO_OVERWRITE_RIGHT:
        case SO_OVERWRITE_LEFT:
            assert(false);
        case SO_UNRESOLVED_CONFLICT:
        case SO_DO_NOTHING:
        case SO_EQUAL:
            return; //no update on processed data!
    }

    //update DirMapping
    dirObj.synchronizeSides();

    //progress indicator update
    //indicator is updated only if directory is sync'ed correctly (and if some work was done)!
    procCallback_.updateProcessedData(1, 0);  //each call represents one processed file
    procCallback_.requestUiRefresh(); //may throw
}


//avoid data loss when source directory doesn't (temporarily?) exist anymore AND user chose to ignore errors (else we wouldn't arrive here)
bool dataLossPossible(const Zstring& dirName, const SyncStatistics& folderPairStat, ProcessCallback& procCallback)
{
    return folderPairStat.getCreate() +  folderPairStat.getOverwrite() + folderPairStat.getConflict() == 0 &&
           folderPairStat.getDelete() > 0 && //deletions only... (respect filtered items!)
           !dirName.empty() && !dirExistsUpdating(dirName, procCallback);
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


class EnforceUpdateDatabase
{
public:
    EnforceUpdateDatabase(const BaseDirMapping& baseMap) :
        dbWasWritten(false),
        baseMap_(baseMap) {}

    ~EnforceUpdateDatabase()
    {
        try
        {
            tryWriteDB(); //throw FileError, keep non-blocking!!!
        }
        catch (...) {}
    }

    void tryWriteDB() //(try to gracefully) write db file; throw FileError
    {
        if (!dbWasWritten)
        {
            zen::saveToDisk(baseMap_); //throw FileError
            dbWasWritten = true;
        }
    }

private:
    bool dbWasWritten;
    const BaseDirMapping& baseMap_;
};
}


void SyncProcess::startSynchronizationProcess(const std::vector<FolderPairSyncCfg>& syncConfig, FolderComparison& folderCmp)
{
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

    if (!synchronizationNeeded(statisticsTotal))
        procCallback.reportInfo(_("Nothing to synchronize according to configuration!")); //inform about this special case


    std::deque<bool> skipFolderPair(folderCmp.size()); //folder pairs may be skipped after fatal errors were found


    //initialize deletion handling: already required when checking for warnings
    std::vector<std::pair<DeletionHandling, DeletionHandling>> delHandler;
    for (FolderComparison::iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        const size_t folderIndex = j - folderCmp.begin();
        const FolderPairSyncCfg& folderPairCfg = syncConfig[folderIndex];

        delHandler.push_back(std::make_pair(DeletionHandling(folderPairCfg.handleDeletion,
                                                             folderPairCfg.custDelFolder,
                                                             j->getBaseDir<LEFT_SIDE>(),
                                                             procCallback),

                                            DeletionHandling(folderPairCfg.handleDeletion,
                                                             folderPairCfg.custDelFolder,
                                                             j->getBaseDir<RIGHT_SIDE>(),
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
    for (FolderComparison::const_iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        const size_t folderIndex = j - folderCmp.begin();

        //exclude some pathological case (leftdir, rightdir are empty)
        if (EqualFilename()(j->getBaseDir<LEFT_SIDE>(), j->getBaseDir<RIGHT_SIDE>()))
            continue;

        const FolderPairSyncCfg& folderPairCfg = syncConfig[folderIndex];
        const std::pair<DeletionHandling, DeletionHandling>& delHandlerFp = delHandler[folderIndex];

        const SyncStatistics statisticsFolderPair(*j);

        //aggregate basic information
        const bool writeLeft = statisticsFolderPair.getCreate   <LEFT_SIDE>() +
                               statisticsFolderPair.getOverwrite<LEFT_SIDE>() +
                               statisticsFolderPair.getDelete   <LEFT_SIDE>() > 0;

        const bool writeRight = statisticsFolderPair.getCreate   <RIGHT_SIDE>() +
                                statisticsFolderPair.getOverwrite<RIGHT_SIDE>() +
                                statisticsFolderPair.getDelete   <RIGHT_SIDE>() > 0;

        //skip folder pair if there is nothing to do (except for automatic mode, where data base needs to be written even in this case)
        if (!writeLeft && !writeRight &&
            !folderPairCfg.inAutomaticMode)
        {
            skipFolderPair[folderIndex] = true; //skip creating (not yet existing) base directories in particular if there's no need
            continue;
        }


        //check empty input fields: basically this only makes sense if empty field is not target (and not automatic mode: because of db file creation)
        if ((j->getBaseDir<LEFT_SIDE>(). empty() && (writeLeft  || folderPairCfg.inAutomaticMode)) ||
            (j->getBaseDir<RIGHT_SIDE>().empty() && (writeRight || folderPairCfg.inAutomaticMode)))
        {
            procCallback.reportFatalError(_("Target directory name must not be empty!"));
            skipFolderPair[folderIndex] = true;
            continue;
        }

        //aggregate information of folders used by multiple pairs in read/write access
        if (!EqualDependentDirectory()(j->getBaseDir<LEFT_SIDE>(), j->getBaseDir<RIGHT_SIDE>())) //true in general
        {
            if (writeLeft)
            {
                ++dirWriteCount[j->getBaseDir<LEFT_SIDE>()];
                if (writeRight)
                    ++dirWriteCount[j->getBaseDir<RIGHT_SIDE>()];
                else
                    dirReadCount.insert(j->getBaseDir<RIGHT_SIDE>());
            }
            else if (writeRight)
            {
                dirReadCount.insert(j->getBaseDir<LEFT_SIDE>());
                ++dirWriteCount[j->getBaseDir<RIGHT_SIDE>()];
            }
        }
        else //if folder pair contains two dependent folders, a warning was already issued after comparison; in this context treat as one write access at most
        {
            if (writeLeft || writeRight)
                ++dirWriteCount[j->getBaseDir<LEFT_SIDE>()];
        }


        if (statisticsFolderPair.getOverwrite() + statisticsFolderPair.getDelete() > 0)
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

        //avoid data loss when source directory doesn't (temporarily?) exist anymore AND user chose to ignore errors(else we wouldn't arrive here)
        if (dataLossPossible(j->getBaseDir<LEFT_SIDE>(), statisticsFolderPair, procCallback))
        {
            procCallback.reportFatalError(_("Source directory does not exist anymore:") + "\n\"" + j->getBaseDir<LEFT_SIDE>() + "\"");
            skipFolderPair[folderIndex] = true;
            continue;
        }
        if (dataLossPossible(j->getBaseDir<RIGHT_SIDE>(), statisticsFolderPair, procCallback))
        {
            procCallback.reportFatalError(_("Source directory does not exist anymore:") + "\n\"" + j->getBaseDir<RIGHT_SIDE>() + "\"");
            skipFolderPair[folderIndex] = true;
            continue;
        }

        //check if more than 50% of total number of files/dirs are to be created/overwritten/deleted
        if (significantDifferenceDetected(statisticsFolderPair))
            significantDiff.push_back(std::make_pair(j->getBaseDir<LEFT_SIDE>(), j->getBaseDir<RIGHT_SIDE>()));

        //check for sufficient free diskspace in left directory
        const std::pair<zen::Int64, zen::Int64> spaceNeeded = DiskSpaceNeeded(*j,
                                                                              delHandlerFp.first.deletionFreesSpace(),
                                                                              delHandlerFp.second.deletionFreesSpace()).getSpaceTotal();

        wxLongLong freeDiskSpaceLeft;
        if (wxGetDiskSpace(toWx(j->getBaseDir<LEFT_SIDE>()), NULL, &freeDiskSpaceLeft))
        {
            if (0 < freeDiskSpaceLeft && //zero disk space is either an error or not: in both cases this warning message is obsolete (WebDav seems to report 0)
                freeDiskSpaceLeft.ToDouble() < to<double>(spaceNeeded.first))
                diskSpaceMissing.push_back(std::make_pair(j->getBaseDir<LEFT_SIDE>(), std::make_pair(spaceNeeded.first, freeDiskSpaceLeft.ToDouble())));
        }

        //check for sufficient free diskspace in right directory
        wxLongLong freeDiskSpaceRight;
        if (wxGetDiskSpace(toWx(j->getBaseDir<RIGHT_SIDE>()), NULL, &freeDiskSpaceRight))
        {
            if (0 < freeDiskSpaceRight && //zero disk space is either an error or not: in both cases this warning message is obsolete (WebDav seems to report 0)
                freeDiskSpaceRight.ToDouble() < to<double>(spaceNeeded.second))
                diskSpaceMissing.push_back(std::make_pair(j->getBaseDir<RIGHT_SIDE>(), std::make_pair(spaceNeeded.second, freeDiskSpaceRight.ToDouble())));
        }

        //windows: check if recycle bin really exists; if not, Windows will silently delete, which is wrong
#ifdef FFS_WIN
        if (folderPairCfg.handleDeletion == MOVE_TO_RECYCLE_BIN)
        {
            if (statisticsFolderPair.getOverwrite<LEFT_SIDE>() +
                statisticsFolderPair.getDelete   <LEFT_SIDE>() > 0 &&

                recycleBinStatus(j->getBaseDir<LEFT_SIDE>()) != STATUS_REC_EXISTS)
                recyclMissing.insert(j->getBaseDir<LEFT_SIDE>());

            if (statisticsFolderPair.getOverwrite<RIGHT_SIDE>() +
                statisticsFolderPair.getDelete   <RIGHT_SIDE>() > 0 &&

                recycleBinStatus(j->getBaseDir<RIGHT_SIDE>()) != STATUS_REC_EXISTS)
                recyclMissing.insert(j->getBaseDir<RIGHT_SIDE>());
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

            warningMessage += wxString(wxT("\"")) + i->first + "\": " + conflictDescription + "\n\n";
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
            warningMessage += wxString(wxT("\n\n")) +
                              i->first + " <-> " + "\n" +
                              i->second;
        warningMessage += wxString(wxT("\n\n")) +_("More than 50% of the total number of files will be copied or deleted!");

        procCallback.reportWarning(warningMessage, m_warnings.warningSignificantDifference);
    }


    //check for sufficient free diskspace
    if (!diskSpaceMissing.empty())
    {
        wxString warningMessage = _("Not enough free disk space available in:");

        for (auto i = diskSpaceMissing.begin(); i != diskSpaceMissing.end(); ++i)
            warningMessage += wxString(wxT("\n\n")) +
                              "\"" + i->first + "\"\n" +
                              _("Free disk space required:")  + wxT(" ") + formatFilesizeToShortString(to<UInt64>(i->second.first))  + wxT("\n") +
                              _("Free disk space available:") + wxT(" ") + formatFilesizeToShortString(to<UInt64>(i->second.second));

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
            warningMessage += wxString(wxT("\n")) +
                              "\"" + *i + "\"";
        procCallback.reportWarning(warningMessage, m_warnings.warningMultiFolderWriteAccess);
    }

    //-------------------end of basic checks------------------------------------------

#ifdef FFS_WIN
    //shadow copy buffer: per sync-instance, not folder pair
    boost::scoped_ptr<shadow::ShadowCopy> shadowCopyHandler(copyLockedFiles_ ? new shadow::ShadowCopy : NULL);
#endif

    try
    {
        //prevent shutdown while synchronization is in progress
        util::DisableStandby dummy;
        (void)dummy;

        //loop through all directory pairs
        for (FolderComparison::iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
        {
            const size_t folderIndex = j - folderCmp.begin();

            const FolderPairSyncCfg& folderPairCfg = syncConfig[folderIndex];
            std::pair<DeletionHandling, DeletionHandling>& delHandlerFp = delHandler[folderIndex];

            if (skipFolderPair[folderIndex]) //folder pairs may be skipped after fatal errors were found
                continue;

            //exclude some pathological case (leftdir, rightdir are empty)
            if (EqualFilename()(j->getBaseDir<LEFT_SIDE>(), j->getBaseDir<RIGHT_SIDE>()))
                continue;

            //------------------------------------------------------------------------------------------
            //info about folder pair to be processed (useful for logfile)
            std::wstring left  = _("Left")  + ": ";
            std::wstring right = _("Right") + ": ";
            makeSameLength(left, right);

            const std::wstring statusTxt = _("Processing folder pair:") + " \n" +
                                           "\t" + left  + "\"" + j->getBaseDir<LEFT_SIDE>()  + "\"" + " \n" +
                                           "\t" + right + "\"" + j->getBaseDir<RIGHT_SIDE>() + "\"";
            procCallback.reportInfo(statusTxt);

            //------------------------------------------------------------------------------------------

            //create base directories first (if not yet existing) -> no symlink or attribute copying! -> single error message instead of one per file (e.g. unplugged network drive)
            const Zstring dirnameLeft = j->getBaseDir<LEFT_SIDE>().BeforeLast(FILE_NAME_SEPARATOR);
            if (!dirnameLeft.empty() && !dirExistsUpdating(dirnameLeft, procCallback))
            {
                if (!tryReportingError(procCallback, boost::bind(createDirectory, boost::cref(dirnameLeft)))) //may throw in error-callback!
                    continue; //skip this folder pair
            }
            const Zstring dirnameRight = j->getBaseDir<RIGHT_SIDE>().BeforeLast(FILE_NAME_SEPARATOR);
            if (!dirnameRight.empty() && !dirExistsUpdating(dirnameRight, procCallback))
            {
                if (!tryReportingError(procCallback, boost::bind(createDirectory, boost::cref(dirnameRight)))) //may throw in error-callback!
                    continue; //skip this folder pair
            }
            //------------------------------------------------------------------------------------------
            //execute synchronization recursively

            //update synchronization database (automatic sync only)
            std::auto_ptr<EnforceUpdateDatabase> guardUpdateDb;
            if (folderPairCfg.inAutomaticMode)
                guardUpdateDb.reset(new EnforceUpdateDatabase(*j));

            //enforce removal of invalid entries (where element on both sides is empty)
            struct RemoveInvalid
            {
                RemoveInvalid(BaseDirMapping& baseDir) : baseDir_(baseDir) {}
                ~RemoveInvalid()
                {
                    FileSystemObject::removeEmpty(baseDir_);
                }
                BaseDirMapping& baseDir_;
            } dummy1(*j);


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
            if (guardUpdateDb.get())
            {
                procCallback.reportInfo(_("Generating database..."));
                procCallback.forceUiRefresh();
                tryReportingError(procCallback, [&]() { guardUpdateDb->tryWriteDB(); });
            }
        }
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
template <class DelTargetCommand>
void SynchronizeFolderPair::copyFileUpdating(const Zstring& source, const Zstring& target, const DelTargetCommand& cmd, UInt64 totalBytesToCpy, int recursionLvl) const
{
    const int exceptionPaths = 2;

    //start of (possibly) long-running copy process: ensure status updates are performed regularly
    UInt64 bytesReported;

    //in error situation: undo communication of processed amount of data
    Loki::ScopeGuard guardStatistics = Loki::MakeGuard([&]() { procCallback_.updateProcessedData(0, -1 * to<Int64>(bytesReported)); });

    try
    {
        WhileCopying<DelTargetCommand> callback(bytesReported, procCallback_, cmd);

        copyFile(source, //type File implicitly means symlinks need to be dereferenced!
                 target,
                 copyFilePermissions_,
                 &callback); //throw (FileError, ErrorFileLocked);

        //inform about the (remaining) processed amount of data
        procCallback_.updateProcessedData(0, to<Int64>(totalBytesToCpy) - to<Int64>(bytesReported));
        bytesReported = totalBytesToCpy;
    }
#ifdef FFS_WIN
    catch (ErrorFileLocked&)
    {
        if (recursionLvl >= exceptionPaths) throw;

        //if file is locked (try to) use Windows Volume Shadow Copy Service
        if (shadowCopyHandler_ == NULL) throw;

        Zstring shadowFilename;
        try
        {
            //contains prefix: E.g. "\\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy1\Program Files\FFS\sample.dat"
            shadowFilename = shadowCopyHandler_->makeShadowCopy(source); //throw (FileError)
        }
        catch (const FileError& e)
        {
            std::wstring errorMsg = _("Error copying locked file %x!");
            replace(errorMsg, L"%x", std::wstring(L"\"") + source + "\"");
            throw FileError(errorMsg + "\n\n" + e.msg());
        }

        //now try again
        return copyFileUpdating(shadowFilename, target, cmd, totalBytesToCpy, recursionLvl + 1);
    }
#endif
    catch (ErrorTargetPathMissing&)
    {
        if (recursionLvl >= exceptionPaths) throw;

        //create folders "first" (see http://sourceforge.net/tracker/index.php?func=detail&aid=2628943&group_id=234430&atid=1093080)
        //using optimistic strategy: assume everything goes well, but cover up on error -> minimize file accesses
        const Zstring targetDir   = target.BeforeLast(FILE_NAME_SEPARATOR);
        const Zstring templateDir = source.BeforeLast(FILE_NAME_SEPARATOR);

        if (!targetDir.empty())
        {
            createDirectory(targetDir, templateDir, copyFilePermissions_); //throw (FileError)
            /*symbolic link handling:
            if "not traversing symlinks": fullName == c:\syncdir<symlinks>\some\dirs\leaf<symlink>
            => setting irrelevant
            if "traversing symlinks":     fullName == c:\syncdir<symlinks>\some\dirs<symlinks>\leaf<symlink>
            => setting NEEDS to be false: We want to move leaf, therefore symlinks in "some\dirs" must not interfere */

            //now try again
            return copyFileUpdating(source, target, cmd, totalBytesToCpy, recursionLvl + 1);
        }
        throw;
    }

    //todo: transactional behavior: delete target if verification fails?

    if (verifyCopiedFiles_) //verify if data was copied correctly
        verifyFileCopy(source, target); //throw (FileError)

    guardStatistics.Dismiss();
}


void SynchronizeFolderPair::copySymlink(const Zstring& source, const Zstring& target, LinkDescriptor::LinkType type, bool inRecursion) const
{
    try
    {
        switch (type)
        {
            case LinkDescriptor::TYPE_DIR:
                zen::copySymlink(source, target, SYMLINK_TYPE_DIR, copyFilePermissions_); //throw (FileError)
                break;

            case LinkDescriptor::TYPE_FILE: //Windows: true file symlink; Linux: file-link or broken link
                zen::copySymlink(source, target, SYMLINK_TYPE_FILE, copyFilePermissions_); //throw (FileError)
                break;
        }
    }
    catch (FileError&)
    {
        if (inRecursion) throw;

        //create folders "first" (see http://sourceforge.net/tracker/index.php?func=detail&aid=2628943&group_id=234430&atid=1093080)
        //using optimistic strategy: assume everything goes well, but cover up on error -> minimize file accesses
        const Zstring targetDir   = target.BeforeLast(FILE_NAME_SEPARATOR);
        const Zstring templateDir = source.BeforeLast(FILE_NAME_SEPARATOR);

        if (!targetDir.empty() && !dirExistsUpdating(targetDir, procCallback_))
        {
            createDirectory(targetDir, templateDir, copyFilePermissions_); //throw (FileError)
            /*symbolic link handling:
            if "not traversing symlinks": fullName == c:\syncdir<symlinks>\some\dirs\leaf<symlink>
            => setting irrelevant
            if "traversing symlinks":     fullName == c:\syncdir<symlinks>\some\dirs<symlinks>\leaf<symlink>
            => setting NEEDS to be false: We want to move leaf, therefore symlinks in "some\dirs" must not interfere */

            //now try again
            return copySymlink(source, target, type, true);
        }

        throw;
    }
}


template <zen::SelectedSide side>
void SynchronizeFolderPair::deleteSymlink(const SymLinkMapping& linkObj) const
{
    const DeletionHandling& delHandling = side == LEFT_SIDE ? delHandlingLeft_ : delHandlingRight_;

    switch (linkObj.getLinkType<side>())
    {
        case LinkDescriptor::TYPE_DIR:
            delHandling.removeFolder(linkObj.getObjRelativeName()); //throw (FileError)
            break;

        case LinkDescriptor::TYPE_FILE: //Windows: true file symlink; Linux: file-link or broken link
            delHandling.removeFile(linkObj.getObjRelativeName()); //throw (FileError)
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
        throw FileError(_("Error opening file:") + " \"" + source + "\"");

#ifdef FFS_WIN
    wxFile file2(applyLongPathPrefix(target).c_str(), wxFile::read); //don't use buffered file input for verification!
#elif defined FFS_LINUX
    wxFile file2(::open(target.c_str(), O_RDONLY)); //utilize UTF-8 filename
#endif
    if (!file2.IsOpened()) //NO cleanup necessary for (wxFile) file1
        throw FileError(_("Error opening file:") + " \"" + target + "\"");

    do
    {
        const size_t length1 = file1.Read(&memory1[0], memory1.size());
        if (file1.Error())
            throw FileError(_("Error reading file:") + " \"" + source + "\"");
        callback.updateStatus(); //send progress updates

        const size_t length2 = file2.Read(&memory2[0], memory2.size());
        if (file2.Error())
            throw FileError(_("Error reading file:") + " \"" + target + "\"");
        callback.updateStatus(); //send progress updates

        if (length1 != length2 || ::memcmp(&memory1[0], &memory2[0], length1) != 0)
            throw FileError(_("Data verification error: Source and target file have different content!") + "\n" + "\"" + source + "\" -> \n\"" + target + "\"");
    }
    while (!file1.Eof());

    if (!file2.Eof())
        throw FileError(_("Data verification error: Source and target file have different content!") + "\n" + "\"" + source + "\" -> \n\"" + target + "\"");
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
    Zstring statusText = txtVerifying;
    statusText.Replace(Zstr("%x"), target, false);
    procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));

    VerifyStatusUpdater callback(procCallback_);

    tryReportingError(procCallback_, [&]() { ::verifyFiles(source, target, callback);});
}
