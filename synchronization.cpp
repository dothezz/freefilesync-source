// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "synchronization.h"
#include <stdexcept>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include "shared/string_conv.h"
#include "shared/util.h"
#include "shared/system_constants.h"
#include "library/status_handler.h"
#include "shared/file_handling.h"
#include "shared/resolve_path.h"
#include "shared/recycler.h"
#include "shared/i18n.h"
#include <wx/file.h>
#include <boost/bind.hpp>
#include "shared/global_func.h"
#include <boost/scoped_array.hpp>
#include <memory>
#include "library/db_file.h"
#include "shared/disable_standby.h"
#include "library/cmp_filetime.h"
#include "shared/file_io.h"
#include <deque>

#ifdef FFS_WIN
#include "shared/long_path_prefix.h"
#include <boost/scoped_ptr.hpp>
#include "shared/perf.h"
#include "shared/shadow.h"
#endif

using namespace ffs3;


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


SyncStatistics::SyncStatistics(const HierarchyObject&  hierObj)
{
    init();
    getNumbersRecursively(hierObj);
}


SyncStatistics::SyncStatistics(const FolderComparison& folderCmp)
{
    init();
    std::for_each(folderCmp.begin(), folderCmp.end(), boost::bind(&SyncStatistics::getNumbersRecursively, this, _1));
}


int SyncStatistics::getConflict() const
{
    return conflict;
}

const SyncStatistics::ConflictTexts& SyncStatistics::getFirstConflicts() const //get first few sync conflicts
{
    return firstConflicts;
}

wxULongLong SyncStatistics::getDataToProcess() const
{
    return dataToProcess;
}


size_t SyncStatistics::getRowCount() const
{
    return rowsTotal;
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


std::vector<ffs3::FolderPairSyncCfg> ffs3::extractSyncCfg(const MainConfiguration& mainCfg)
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

            FolderPairSyncCfg(i->altSyncConfig->syncConfiguration.automatic,
                              i->altSyncConfig->handleDeletion,
                              wxToZ(i->altSyncConfig->customDeletionDirectory)) :

            FolderPairSyncCfg(mainCfg.syncConfiguration.automatic,
                              mainCfg.handleDeletion,
                              wxToZ(mainCfg.customDeletionDirectory)));
    return output;
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

    std::pair<wxLongLong, wxLongLong> getSpaceTotal() const
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
                    spaceNeededLeft += common::convertToSigned(i->getFileSize<RIGHT_SIDE>());
                    break;

                case SO_CREATE_NEW_RIGHT:
                    spaceNeededRight += common::convertToSigned(i->getFileSize<LEFT_SIDE>());
                    break;

                case SO_DELETE_LEFT:
                    if (freeSpaceDelLeft_)
                        spaceNeededLeft -= common::convertToSigned(i->getFileSize<LEFT_SIDE>());
                    break;

                case SO_DELETE_RIGHT:
                    if (freeSpaceDelRight_)
                        spaceNeededRight -= common::convertToSigned(i->getFileSize<RIGHT_SIDE>());
                    break;

                case SO_OVERWRITE_LEFT:
                    if (freeSpaceDelLeft_)
                        spaceNeededLeft -= common::convertToSigned(i->getFileSize<LEFT_SIDE>());
                    spaceNeededLeft += common::convertToSigned(i->getFileSize<RIGHT_SIDE>());
                    break;

                case SO_OVERWRITE_RIGHT:
                    if (freeSpaceDelRight_)
                        spaceNeededRight -= common::convertToSigned(i->getFileSize<RIGHT_SIDE>());
                    spaceNeededRight += common::convertToSigned(i->getFileSize<LEFT_SIDE>());
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

    wxLongLong spaceNeededLeft;
    wxLongLong spaceNeededRight;
};


//evaluate whether a deletion will actually free space within a volume
bool deletionFreesSpace(const Zstring& baseDir,
                        const DeletionPolicy handleDeletion,
                        const Zstring& custDelFolderFmt)
{
    switch (handleDeletion)
    {
        case DELETE_PERMANENTLY:
            return true;
        case MOVE_TO_RECYCLE_BIN:
            return false; //in general... (unless Recycle Bin is full)
        case MOVE_TO_CUSTOM_DIRECTORY:
            switch (ffs3::onSameVolume(baseDir, custDelFolderFmt))
            {
                case VOLUME_SAME:
                    return false;
                case VOLUME_DIFFERENT:
                    return true; //but other volume (custDelFolderFmt) may become full...
                case VOLUME_CANT_SAY:
                    return true; //a rough guess!
            }
    }
    assert(false);
    return true;
}


std::pair<wxLongLong, wxLongLong> freeDiskSpaceNeeded(
    const BaseDirMapping& baseDirObj,
    const DeletionPolicy handleDeletion,
    const Zstring& custDelFolderFmt)
{
    const bool freeSpaceDelLeft  = deletionFreesSpace(baseDirObj.getBaseDir<LEFT_SIDE>(),  handleDeletion, custDelFolderFmt);
    const bool freeSpaceDelRight = deletionFreesSpace(baseDirObj.getBaseDir<RIGHT_SIDE>(), handleDeletion, custDelFolderFmt);

    return DiskSpaceNeeded(baseDirObj, freeSpaceDelLeft, freeSpaceDelRight).getSpaceTotal();
}
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
    custDelFolder(ffs3::getFormattedDirectoryName(custDelDir)) {}
//-----------------------------------------------------------------------------------------------------------


template <typename Function>
inline
bool tryReportingError(StatusHandler& handler, Function cmd) //return "true" on success, "false" if error was ignored
{
    while (true)
    {
        try
        {
            cmd();
            return true;
        }
        catch (FileError& error)
        {
            //User abort when copying files or moving files/directories into custom deletion directory:
            //windows build: abort if requested, don't show error message if cancelled by user!
            //linux build: this refresh is not necessary, because user abort triggers an AbortThisProcess() exception without a FileError()
            handler.requestUiRefresh(true); //may throw!

            ErrorHandler::Response rv = handler.reportError(error.msg()); //may throw!
            if (rv == ErrorHandler::IGNORE_ERROR)
                return false;
            else if (rv == ErrorHandler::RETRY)
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

    if (!formattedDir.EndsWith(common::FILE_NAME_SEPARATOR))
        formattedDir += common::FILE_NAME_SEPARATOR;

    wxString timeNow = wxDateTime::Now().FormatISOTime();
    timeNow.Replace(wxT(":"), wxT(""));

    const wxString sessionName = wxDateTime::Now().FormatISODate() + wxChar(' ') + timeNow;
    formattedDir += prefix + wxToZ(sessionName);


    //ensure that session directory does not yet exist (must be unique)
    Zstring output = formattedDir;

    //ensure uniqueness
    for (int i = 1; ffs3::somethingExists(output); ++i)
        output = formattedDir + Zchar('_') + Zstring::fromNumber(i);

    output += common::FILE_NAME_SEPARATOR;
    return output;
}


SyncProcess::SyncProcess(xmlAccess::OptionalDialogs& warnings,
                         bool verifyCopiedFiles,
                         bool copyLockedFiles,
                         bool copyFilePermissions,
                         StatusHandler& handler) :
    verifyCopiedFiles_(verifyCopiedFiles),
    copyLockedFiles_(copyLockedFiles),
    copyFilePermissions_(copyFilePermissions),
    m_warnings(warnings),
    statusUpdater(handler) {}
//--------------------------------------------------------------------------------------------------------------


class DeletionHandling
{
public:
    DeletionHandling(const DeletionPolicy handleDel,
                     const Zstring& custDelFolder,
                     const Zstring& baseDirLeft,
                     const Zstring& baseDirRight,
                     StatusHandler& statusUpdater);
    ~DeletionHandling(); //always (try to) clean up, even if synchronization is aborted!

    //clean-up temporary directory (recycler bin optimization)
    void tryCleanup() const; //throw (FileError) -> call this in non-exceptional coding, i.e. after Sync somewhere!

    template <ffs3::SelectedSide side>
    void removeFile(const FileSystemObject& fileObj) const; //throw (FileError)

    template <ffs3::SelectedSide side>
    void removeFolder(const FileSystemObject& dirObj) const; //throw (FileError)

    const Zstring& getTxtRemovingFile() const; //status text templates
    const Zstring& getTxtRemovingSymLink() const;
    const Zstring& getTxtRemovingDir() const;  //status text templates

private:
    template <SelectedSide side>
    const Zstring& getSessionDir() const;

    void tryCleanupLeft() const;  //throw (FileError)
    void tryCleanupRight() const; //throw (FileError)

    const DeletionPolicy deletionType;
    StatusHandler& statusUpdater_;

    Zstring sessionDelDirLeft;  //target deletion folder for current folder pair (with timestamp, ends with path separator)
    Zstring sessionDelDirRight; //

    //preloaded status texts:
    Zstring txtRemovingFile;
    Zstring txtRemovingSymlink;
    Zstring txtRemovingDirectory;
};


DeletionHandling::DeletionHandling(const DeletionPolicy handleDel,
                                   const Zstring& custDelFolder,
                                   const Zstring& baseDirLeft,
                                   const Zstring& baseDirRight,
                                   StatusHandler& statusUpdater) :
    deletionType(handleDel),
    statusUpdater_(statusUpdater)
{
    switch (handleDel)
    {
        case DELETE_PERMANENTLY:
            txtRemovingFile      = wxToZ(_("Deleting file %x")).Replace(Zstr("%x"), Zstr("\n\"%x\""), false);
            txtRemovingSymlink   = wxToZ(_("Deleting Symbolic Link %x")).Replace(Zstr("%x"), Zstr("\n\"%x\""), false);
            txtRemovingDirectory = wxToZ(_("Deleting folder %x")).Replace( Zstr("%x"), Zstr("\n\"%x\""), false);
            break;

        case MOVE_TO_RECYCLE_BIN:
            sessionDelDirLeft  = getSessionDeletionDir(baseDirLeft,  Zstr("FFS "));
            sessionDelDirRight = getSessionDeletionDir(baseDirRight, Zstr("FFS "));

            txtRemovingFile = txtRemovingSymlink = txtRemovingDirectory = wxToZ(_("Moving %x to Recycle Bin")).Replace(Zstr("%x"), Zstr("\"%x\""), false);
            break;

        case MOVE_TO_CUSTOM_DIRECTORY:
            sessionDelDirLeft = sessionDelDirRight = getSessionDeletionDir(custDelFolder);

            txtRemovingFile      = wxToZ(_("Moving file %x to user-defined directory %y")).         Replace(Zstr("%x"), Zstr("\"%x\"\n"), false).Replace(Zstr("%y"), Zstring(Zstr("\"")) + custDelFolder + Zstr("\""), false);
            txtRemovingDirectory = wxToZ(_("Moving folder %x to user-defined directory %y")).       Replace(Zstr("%x"), Zstr("\"%x\"\n"), false).Replace(Zstr("%y"), Zstring(Zstr("\"")) + custDelFolder + Zstr("\""), false);
            txtRemovingSymlink   = wxToZ(_("Moving Symbolic Link %x to user-defined directory %y")).Replace(Zstr("%x"), Zstr("\"%x\"\n"), false).Replace(Zstr("%y"), Zstring(Zstr("\"")) + custDelFolder + Zstr("\""), false);
            break;
    }
}


DeletionHandling::~DeletionHandling()
{
    try //always (try to) clean up, even if synchronization is aborted!
    {
        tryCleanupLeft();
    }
    catch (...) {}

    try //always clean up BOTH sides separately!
    {
        tryCleanupRight();
    }
    catch (...) {}
}


void DeletionHandling::tryCleanup() const //throw(AbortThisProcess)
{
    tryReportingError(statusUpdater_, boost::bind(&DeletionHandling::tryCleanupLeft,  this));
    tryReportingError(statusUpdater_, boost::bind(&DeletionHandling::tryCleanupRight, this));
}


void DeletionHandling::tryCleanupLeft() const //throw (FileError)
{
    if (deletionType == MOVE_TO_RECYCLE_BIN) //clean-up temporary directory (recycle bin)
        ffs3::moveToRecycleBin(sessionDelDirLeft.BeforeLast(common::FILE_NAME_SEPARATOR));  //throw (FileError)
}


void DeletionHandling::tryCleanupRight() const //throw (FileError)
{
    if (deletionType == MOVE_TO_RECYCLE_BIN) //clean-up temporary directory (recycle bin)
        ffs3::moveToRecycleBin(sessionDelDirRight.BeforeLast(common::FILE_NAME_SEPARATOR));  //throw (FileError)
}


inline
const Zstring& DeletionHandling::getTxtRemovingFile() const
{
    return txtRemovingFile;
}


inline
const Zstring& DeletionHandling::getTxtRemovingDir() const
{
    return txtRemovingDirectory;
}


inline
const Zstring& DeletionHandling::getTxtRemovingSymLink() const
{
    return txtRemovingSymlink;
}


template <>
inline
const Zstring& DeletionHandling::getSessionDir<LEFT_SIDE>() const
{
    return sessionDelDirLeft;
}

template <>
inline
const Zstring& DeletionHandling::getSessionDir<RIGHT_SIDE>() const
{
    return sessionDelDirRight;
}


namespace
{
class CallbackMoveFileImpl : public CallbackMoveFile //callback functionality
{
public:
    CallbackMoveFileImpl(StatusHandler& handler) : statusHandler_(handler) {}

    virtual Response requestUiRefresh(const Zstring& currentObject)  //DON'T throw exceptions here, at least in Windows build!
    {
#ifdef FFS_WIN
        statusHandler_.requestUiRefresh(false); //don't allow throwing exception within this call: windows copying callback can't handle this
        if (statusHandler_.abortIsRequested())
            return CallbackMoveFile::CANCEL;
#elif defined FFS_LINUX
        statusHandler_.requestUiRefresh(); //exceptions may be thrown here!
#endif
        return CallbackMoveFile::CONTINUE;
    }

private:
    StatusHandler& statusHandler_;
};


struct CallbackRemoveDirImpl : public CallbackRemoveDir
{
    CallbackRemoveDirImpl(StatusHandler& handler) : statusHandler_(handler) {}

    virtual void requestUiRefresh(const Zstring& currentObject)
    {
        statusHandler_.requestUiRefresh(); //exceptions may be thrown here!
    }

private:
    StatusHandler& statusHandler_;
};
}


template <ffs3::SelectedSide side>
void DeletionHandling::removeFile(const FileSystemObject& fileObj) const
{
    using namespace ffs3;

    switch (deletionType)
    {
        case DELETE_PERMANENTLY:
            ffs3::removeFile(fileObj.getFullName<side>());
            break;

        case MOVE_TO_RECYCLE_BIN:
            if (fileExists(fileObj.getFullName<side>()))
            {
                const Zstring targetFile = getSessionDir<side>() + fileObj.getRelativeName<side>(); //altDeletionDir ends with path separator
                const Zstring targetDir  = targetFile.BeforeLast(common::FILE_NAME_SEPARATOR);

                if (!dirExists(targetDir))
                    createDirectory(targetDir); //throw (FileError)

                try //rename file: no copying!!!
                {
                    //performance optimization!! Instead of moving each object into recycle bin separately, we rename them ony by one into a
                    //temporary directory and delete this directory only ONCE!
                    renameFile(fileObj.getFullName<side>(), targetFile); //throw (FileError);
                }
                catch (...)
                {
                    //if anything went wrong, move to recycle bin the standard way (single file processing: slow)
                    moveToRecycleBin(fileObj.getFullName<side>());  //throw (FileError)
                }
            }
            break;

        case MOVE_TO_CUSTOM_DIRECTORY:
            if (fileExists(fileObj.getFullName<side>()))
            {
                const Zstring targetFile = getSessionDir<side>() + fileObj.getRelativeName<side>(); //altDeletionDir ends with path separator
                const Zstring targetDir  = targetFile.BeforeLast(common::FILE_NAME_SEPARATOR);

                if (!dirExists(targetDir))
                    createDirectory(targetDir); //throw (FileError)

                CallbackMoveFileImpl callBack(statusUpdater_); //if file needs to be copied we need callback functionality to update screen and offer abort
                moveFile(fileObj.getFullName<side>(), targetFile, true, &callBack);
            }
            break;
    }
}


template <ffs3::SelectedSide side>
void DeletionHandling::removeFolder(const FileSystemObject& dirObj) const
{
    using namespace ffs3;

    switch (deletionType)
    {
        case DELETE_PERMANENTLY:
        {
            CallbackRemoveDirImpl remDirCallback(statusUpdater_);
            removeDirectory(dirObj.getFullName<side>(), &remDirCallback);
        }
        break;

        case MOVE_TO_RECYCLE_BIN:
            if (dirExists(dirObj.getFullName<side>()))
            {
                const Zstring targetDir      = getSessionDir<side>() + dirObj.getRelativeName<side>();
                const Zstring targetSuperDir = targetDir.BeforeLast(common::FILE_NAME_SEPARATOR);

                if (!dirExists(targetSuperDir))
                    createDirectory(targetSuperDir); //throw (FileError)

                try //rename directory: no copying!!!
                {
                    //performance optimization!! Instead of moving each object into recycle bin separately, we rename them ony by one into a
                    //temporary directory and delete this directory only ONCE!
                    renameFile(dirObj.getFullName<side>(), targetDir); //throw (FileError);
                }
                catch (...)
                {
                    //if anything went wrong, move to recycle bin the standard way (single file processing: slow)
                    moveToRecycleBin(dirObj.getFullName<side>());  //throw (FileError)

                }
            }
            break;

        case MOVE_TO_CUSTOM_DIRECTORY:
            if (dirExists(dirObj.getFullName<side>()))
            {
                const Zstring targetDir      = getSessionDir<side>() + dirObj.getRelativeName<side>();
                const Zstring targetSuperDir = targetDir.BeforeLast(common::FILE_NAME_SEPARATOR);

                if (!dirExists(targetSuperDir))
                    createDirectory(targetSuperDir); //throw (FileError)

                CallbackMoveFileImpl callBack(statusUpdater_); //if files need to be copied, we need callback functionality to update screen and offer abort
                moveDirectory(dirObj.getFullName<side>(), targetDir, true, &callBack);
            }
            break;
    }
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
        case SO_UNRESOLVED_CONFLICT:
        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
            return false;
    }
    return false; //dummy
}
//---------------------------------------------------------------------------------------------------------------


class ffs3::SynchronizeFolderPair
{
public:
    SynchronizeFolderPair(const SyncProcess& syncProc,
#ifdef FFS_WIN
                          shadow::ShadowCopy* shadowCopyHandler,
#endif
                          const DeletionHandling& delHandling) :
        statusUpdater_(syncProc.statusUpdater),
#ifdef FFS_WIN
        shadowCopyHandler_(shadowCopyHandler),
#endif
        delHandling_(delHandling),
        verifyCopiedFiles_(syncProc.verifyCopiedFiles_),
        copyFilePermissions_(syncProc.copyFilePermissions_),
        txtCopyingFile    (wxToZ(_("Copying new file %x to %y")).         Replace(Zstr("%x"), Zstr("\"%x\""), false).Replace(Zstr("%y"), Zstr("\n\"%y\""), false)),
        txtCopyingLink    (wxToZ(_("Copying new Symbolic Link %x to %y")).Replace(Zstr("%x"), Zstr("\"%x\""), false).Replace(Zstr("%y"), Zstr("\n\"%y\""), false)),
        txtOverwritingFile(wxToZ(_("Overwriting file %x in %y")).         Replace(Zstr("%x"), Zstr("\"%x\""), false).Replace(Zstr("%y"), Zstr("\n\"%y\""), false)),
        txtOverwritingLink(wxToZ(_("Overwriting Symbolic Link %x in %y")).Replace(Zstr("%x"), Zstr("\"%x\""), false).Replace(Zstr("%y"), Zstr("\n\"%y\""), false)),
        txtCreatingFolder (wxToZ(_("Creating folder %x")).Replace(Zstr("%x"), Zstr("\n\"%x\""), false)),
        txtVerifying      (wxToZ(_("Verifying file %x")). Replace(Zstr("%x"), Zstr("\n\"%x\""), false)),
        txtWritingAttributes(wxToZ(_("Updating attributes of %x")).Replace(Zstr("%x"), Zstr("\n\"%x\""), false)) {}


    template <bool reduceDiskSpace> //"true" if files deletion shall happen only
    void startSync(BaseDirMapping& baseMap)
    {
        execute<reduceDiskSpace>(baseMap);
    }

private:
    template <bool reduceDiskSpace> //"true" if files deletion shall happen only
    void execute(HierarchyObject& hierObj);

    void synchronizeFile(FileMapping& fileObj) const;
    void synchronizeLink(SymLinkMapping& linkObj) const;
    void synchronizeFolder(DirMapping& dirObj) const;

    //more low level helper
    template <ffs3::SelectedSide side>
    void deleteSymlink(const SymLinkMapping& linkObj) const;
    void copySymlink(const Zstring& source, const Zstring& target, LinkDescriptor::LinkType type, bool inRecursion = false) const;
    template <class DelTargetCommand>
    void copyFileUpdating(const Zstring& source, const Zstring& target, const DelTargetCommand& cmd, const wxULongLong& sourceFileSize, int recursionLvl = 0) const;
    void verifyFileCopy(const Zstring& source, const Zstring& target) const;

    StatusHandler& statusUpdater_;
#ifdef FFS_WIN
    shadow::ShadowCopy* shadowCopyHandler_; //optional!
#endif
    const DeletionHandling& delHandling_;

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


template <bool reduceDiskSpace> //"true" if file deletion happens only
void SynchronizeFolderPair::execute(HierarchyObject& hierObj)
{
    //synchronize files:
    for (HierarchyObject::SubFileMapping::iterator i = hierObj.useSubFiles().begin(); i != hierObj.useSubFiles().end(); ++i)
    {
        if (( reduceDiskSpace &&  diskSpaceIsReduced(*i)) ||
            (!reduceDiskSpace && !diskSpaceIsReduced(*i)))
            tryReportingError(statusUpdater_, boost::bind(&SynchronizeFolderPair::synchronizeFile, this, boost::ref(*i)));
    }

    //synchronize symbolic links: (process in second step only)
    if (!reduceDiskSpace)
        for (HierarchyObject::SubLinkMapping::iterator i = hierObj.useSubLinks().begin(); i != hierObj.useSubLinks().end(); ++i)
            tryReportingError(statusUpdater_, boost::bind(&SynchronizeFolderPair::synchronizeLink, this, boost::ref(*i)));

    //synchronize folders:
    for (HierarchyObject::SubDirMapping::iterator i = hierObj.useSubDirs().begin(); i != hierObj.useSubDirs().end(); ++i)
    {
        const SyncOperation syncOp = i->getSyncOperation();

        if (( reduceDiskSpace &&  diskSpaceIsReduced(*i)) || //ensure folder creation happens in second pass, to enable time adaption below
            (!reduceDiskSpace && !diskSpaceIsReduced(*i)))   //
            tryReportingError(statusUpdater_, boost::bind(&SynchronizeFolderPair::synchronizeFolder, this, boost::ref(*i)));

        //recursive synchronization:
        execute<reduceDiskSpace>(*i);

        //adapt folder modification dates: apply AFTER all subobjects have been synced to preserve folder modification date!
        try
        {
            switch (syncOp)
            {
                case SO_CREATE_NEW_LEFT:
                case SO_COPY_METADATA_TO_LEFT:
                    copyFileTimes(i->getFullName<RIGHT_SIDE>(), i->getFullName<LEFT_SIDE>(), true); //deref symlinks; throw (FileError)
                    break;
                case SO_CREATE_NEW_RIGHT:
                case SO_COPY_METADATA_TO_RIGHT:
                    copyFileTimes(i->getFullName<LEFT_SIDE>(), i->getFullName<RIGHT_SIDE>(), true); //deref symlinks; throw (FileError)
                    break;
                case SO_OVERWRITE_RIGHT:
                case SO_OVERWRITE_LEFT:
                    assert(false);
                case SO_UNRESOLVED_CONFLICT:
                case SO_DELETE_LEFT:
                case SO_DELETE_RIGHT:
                case SO_DO_NOTHING:
                case SO_EQUAL:
                    break;
            }
        }
        catch (...) {}
    }
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
        delHandling_.removeFile<side>(fileObj_); //throw (FileError)
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
            statusText.Replace(Zstr("%y"), target.BeforeLast(common::FILE_NAME_SEPARATOR), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            copyFileUpdating(fileObj.getFullName<RIGHT_SIDE>(), target,
                             NullCommand(), //no target to delete
                             fileObj.getFileSize<RIGHT_SIDE>());
            break;

        case SO_CREATE_NEW_RIGHT:
            target = fileObj.getBaseDirPf<RIGHT_SIDE>() + fileObj.getRelativeName<LEFT_SIDE>();

            statusText = txtCopyingFile;
            statusText.Replace(Zstr("%x"), fileObj.getShortName<LEFT_SIDE>(), false);
            statusText.Replace(Zstr("%y"), target.BeforeLast(common::FILE_NAME_SEPARATOR), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            copyFileUpdating(fileObj.getFullName<LEFT_SIDE>(), target,
                             NullCommand(), //no target to delete
                             fileObj.getFileSize<LEFT_SIDE>());
            break;

        case SO_DELETE_LEFT:
            statusText = delHandling_.getTxtRemovingFile();
            statusText.Replace(Zstr("%x"), fileObj.getFullName<LEFT_SIDE>(), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            delHandling_.removeFile<LEFT_SIDE>(fileObj); //throw (FileError)
            break;

        case SO_DELETE_RIGHT:
            statusText = delHandling_.getTxtRemovingFile();
            statusText.Replace(Zstr("%x"), fileObj.getFullName<RIGHT_SIDE>(), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            delHandling_.removeFile<RIGHT_SIDE>(fileObj); //throw (FileError)
            break;

        case SO_OVERWRITE_LEFT:
            target = fileObj.getBaseDirPf<LEFT_SIDE>() + fileObj.getRelativeName<RIGHT_SIDE>(); //respect differences in case of source object

            statusText = txtOverwritingFile;
            statusText.Replace(Zstr("%x"), fileObj.getShortName<RIGHT_SIDE>(), false);
            statusText.Replace(Zstr("%y"), fileObj.getFullName<LEFT_SIDE>().BeforeLast(common::FILE_NAME_SEPARATOR), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            copyFileUpdating(fileObj.getFullName<RIGHT_SIDE>(), target,
                             DelTargetCommand<LEFT_SIDE>(fileObj, delHandling_), //delete target at appropriate time
                             fileObj.getFileSize<RIGHT_SIDE>());
            break;

        case SO_OVERWRITE_RIGHT:
            target = fileObj.getBaseDirPf<RIGHT_SIDE>() + fileObj.getRelativeName<LEFT_SIDE>(); //respect differences in case of source object

            statusText = txtOverwritingFile;
            statusText.Replace(Zstr("%x"), fileObj.getShortName<LEFT_SIDE>(), false);
            statusText.Replace(Zstr("%y"), fileObj.getFullName<RIGHT_SIDE>().BeforeLast(common::FILE_NAME_SEPARATOR), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            copyFileUpdating(fileObj.getFullName<LEFT_SIDE>(), target,
                             DelTargetCommand<RIGHT_SIDE>(fileObj, delHandling_), //delete target at appropriate time
                             fileObj.getFileSize<LEFT_SIDE>());
            break;

        case SO_COPY_METADATA_TO_LEFT:
            statusText = txtWritingAttributes;
            statusText.Replace(Zstr("%x"), fileObj.getFullName<LEFT_SIDE>(), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            if (fileObj.getShortName<LEFT_SIDE>() != fileObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(fileObj.getFullName<LEFT_SIDE>(),
                           fileObj.getFullName<LEFT_SIDE>().BeforeLast(common::FILE_NAME_SEPARATOR) + common::FILE_NAME_SEPARATOR + fileObj.getShortName<RIGHT_SIDE>()); //throw (FileError);

            if (!sameFileTime(fileObj.getLastWriteTime<LEFT_SIDE>(), fileObj.getLastWriteTime<RIGHT_SIDE>(), 2)) ////respect 2 second FAT/FAT32 precision
                copyFileTimes(fileObj.getFullName<RIGHT_SIDE>(), fileObj.getFullName<LEFT_SIDE>(), true); //deref symlinks; throw (FileError)
            break;

        case SO_COPY_METADATA_TO_RIGHT:
            statusText = txtWritingAttributes;
            statusText.Replace(Zstr("%x"), fileObj.getFullName<RIGHT_SIDE>(), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            if (fileObj.getShortName<LEFT_SIDE>() != fileObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(fileObj.getFullName<RIGHT_SIDE>(),
                           fileObj.getFullName<RIGHT_SIDE>().BeforeLast(common::FILE_NAME_SEPARATOR) + common::FILE_NAME_SEPARATOR + fileObj.getShortName<LEFT_SIDE>()); //throw (FileError);

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
    statusUpdater_.updateProcessedData(1, 0); //processed data is communicated in subfunctions!
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
            statusText.Replace(Zstr("%y"), target.BeforeLast(common::FILE_NAME_SEPARATOR), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            copySymlink(linkObj.getFullName<RIGHT_SIDE>(), target, linkObj.getLinkType<RIGHT_SIDE>());
            break;

        case SO_CREATE_NEW_RIGHT:
            target = linkObj.getBaseDirPf<RIGHT_SIDE>() + linkObj.getRelativeName<LEFT_SIDE>();

            statusText = txtCopyingLink;
            statusText.Replace(Zstr("%x"), linkObj.getShortName<LEFT_SIDE>(), false);
            statusText.Replace(Zstr("%y"), target.BeforeLast(common::FILE_NAME_SEPARATOR), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            copySymlink(linkObj.getFullName<LEFT_SIDE>(), target, linkObj.getLinkType<LEFT_SIDE>());
            break;

        case SO_DELETE_LEFT:
            statusText = delHandling_.getTxtRemovingSymLink();
            statusText.Replace(Zstr("%x"), linkObj.getFullName<LEFT_SIDE>(), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            deleteSymlink<LEFT_SIDE>(linkObj); //throw (FileError)
            break;

        case SO_DELETE_RIGHT:
            statusText = delHandling_.getTxtRemovingSymLink();
            statusText.Replace(Zstr("%x"), linkObj.getFullName<RIGHT_SIDE>(), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            deleteSymlink<RIGHT_SIDE>(linkObj); //throw (FileError)
            break;

        case SO_OVERWRITE_LEFT:
            target = linkObj.getBaseDirPf<LEFT_SIDE>() + linkObj.getRelativeName<RIGHT_SIDE>(); //respect differences in case of source object

            statusText = txtOverwritingLink;
            statusText.Replace(Zstr("%x"), linkObj.getShortName<RIGHT_SIDE>(), false);
            statusText.Replace(Zstr("%y"), linkObj.getFullName<LEFT_SIDE>().BeforeLast(common::FILE_NAME_SEPARATOR), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            deleteSymlink<LEFT_SIDE>(linkObj); //throw (FileError)
            linkObj.removeObject<LEFT_SIDE>(); //remove file from FileMapping, to keep in sync (if subsequent copying fails!!)

            copySymlink(linkObj.getFullName<RIGHT_SIDE>(), target, linkObj.getLinkType<RIGHT_SIDE>());
            break;

        case SO_OVERWRITE_RIGHT:
            target = linkObj.getBaseDirPf<RIGHT_SIDE>() + linkObj.getRelativeName<LEFT_SIDE>(); //respect differences in case of source object

            statusText = txtOverwritingLink;
            statusText.Replace(Zstr("%x"), linkObj.getShortName<LEFT_SIDE>(), false);
            statusText.Replace(Zstr("%y"), linkObj.getFullName<RIGHT_SIDE>().BeforeLast(common::FILE_NAME_SEPARATOR), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            deleteSymlink<RIGHT_SIDE>(linkObj); //throw (FileError)
            linkObj.removeObject<RIGHT_SIDE>(); //remove file from FileMapping, to keep in sync (if subsequent copying fails!!)

            copySymlink(linkObj.getFullName<LEFT_SIDE>(), target, linkObj.getLinkType<LEFT_SIDE>());
            break;

        case SO_COPY_METADATA_TO_LEFT:
            statusText = txtWritingAttributes;
            statusText.Replace(Zstr("%x"), linkObj.getFullName<LEFT_SIDE>(), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            if (linkObj.getShortName<LEFT_SIDE>() != linkObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(linkObj.getFullName<LEFT_SIDE>(),
                           linkObj.getFullName<LEFT_SIDE>().BeforeLast(common::FILE_NAME_SEPARATOR) + common::FILE_NAME_SEPARATOR + linkObj.getShortName<RIGHT_SIDE>()); //throw (FileError);

            if (!sameFileTime(linkObj.getLastWriteTime<LEFT_SIDE>(), linkObj.getLastWriteTime<RIGHT_SIDE>(), 2)) ////respect 2 second FAT/FAT32 precision
                copyFileTimes(linkObj.getFullName<RIGHT_SIDE>(), linkObj.getFullName<LEFT_SIDE>(), false); //don't deref symlinks; throw (FileError)
            break;

        case SO_COPY_METADATA_TO_RIGHT:
            statusText = txtWritingAttributes;
            statusText.Replace(Zstr("%x"), linkObj.getFullName<RIGHT_SIDE>(), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            if (linkObj.getShortName<LEFT_SIDE>() != linkObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(linkObj.getFullName<RIGHT_SIDE>(),
                           linkObj.getFullName<RIGHT_SIDE>().BeforeLast(common::FILE_NAME_SEPARATOR) + common::FILE_NAME_SEPARATOR + linkObj.getShortName<LEFT_SIDE>()); //throw (FileError);

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
    statusUpdater_.updateProcessedData(1, 0); //processed data is communicated in subfunctions!
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
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            //some check to catch the error that directory on source has been deleted externally after "compare"...
            if (!ffs3::dirExists(dirObj.getFullName<RIGHT_SIDE>()))
                throw FileError(wxString(_("Source directory does not exist anymore:")) + wxT("\n\"") + zToWx(dirObj.getFullName<RIGHT_SIDE>()) + wxT("\""));
            createDirectory(target, dirObj.getFullName<RIGHT_SIDE>(), copyFilePermissions_); //no symlink copying!
            break;

        case SO_CREATE_NEW_RIGHT:
            target = dirObj.getBaseDirPf<RIGHT_SIDE>() + dirObj.getRelativeName<LEFT_SIDE>();

            statusText = txtCreatingFolder;
            statusText.Replace(Zstr("%x"), target, false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            //some check to catch the error that directory on source has been deleted externally after "compare"...
            if (!ffs3::dirExists(dirObj.getFullName<LEFT_SIDE>()))
                throw FileError(wxString(_("Source directory does not exist anymore:")) + wxT("\n\"") + zToWx(dirObj.getFullName<LEFT_SIDE>()) + wxT("\""));
            createDirectory(target, dirObj.getFullName<LEFT_SIDE>(), copyFilePermissions_); //no symlink copying!
            break;

        case SO_DELETE_LEFT:
            //status information
            statusText = delHandling_.getTxtRemovingDir();
            statusText.Replace(Zstr("%x"), dirObj.getFullName<LEFT_SIDE>(), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            delHandling_.removeFolder<LEFT_SIDE>(dirObj); //throw (FileError)
            {
                //progress indicator update: DON'T forget to notify about implicitly deleted objects!
                const SyncStatistics subObjects(dirObj);
                //...then remove everything
                dirObj.useSubFiles().clear();
                dirObj.useSubLinks().clear();
                dirObj.useSubDirs().clear();
                statusUpdater_.updateProcessedData(subObjects.getCreate() + subObjects.getOverwrite() + subObjects.getDelete(), subObjects.getDataToProcess().ToDouble());
            }
            break;

        case SO_DELETE_RIGHT:
            //status information
            statusText = delHandling_.getTxtRemovingDir();
            statusText.Replace(Zstr("%x"), dirObj.getFullName<RIGHT_SIDE>(), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            delHandling_.removeFolder<RIGHT_SIDE>(dirObj); //throw (FileError)
            {
                //progress indicator update: DON'T forget to notify about implicitly deleted objects!
                const SyncStatistics subObjects(dirObj);
                //...then remove everything
                dirObj.useSubFiles().clear();
                dirObj.useSubLinks().clear();
                dirObj.useSubDirs().clear();
                statusUpdater_.updateProcessedData(subObjects.getCreate() + subObjects.getOverwrite() + subObjects.getDelete(), subObjects.getDataToProcess().ToDouble());
            }
            break;

        case SO_COPY_METADATA_TO_LEFT:
            statusText = txtWritingAttributes;
            statusText.Replace(Zstr("%x"), dirObj.getFullName<LEFT_SIDE>(), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            if (dirObj.getShortName<LEFT_SIDE>() != dirObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(dirObj.getFullName<LEFT_SIDE>(),
                           dirObj.getFullName<LEFT_SIDE>().BeforeLast(common::FILE_NAME_SEPARATOR) + common::FILE_NAME_SEPARATOR + dirObj.getShortName<RIGHT_SIDE>()); //throw (FileError);
            //copyFileTimes(dirObj.getFullName<RIGHT_SIDE>(), dirObj.getFullName<LEFT_SIDE>(), true); //throw (FileError) -> is executed after sub-objects have finished synchronization
            break;

        case SO_COPY_METADATA_TO_RIGHT:
            statusText = txtWritingAttributes;
            statusText.Replace(Zstr("%x"), dirObj.getFullName<RIGHT_SIDE>(), false);
            statusUpdater_.reportInfo(statusText);
            statusUpdater_.requestUiRefresh(); //trigger display refresh

            if (dirObj.getShortName<LEFT_SIDE>() != dirObj.getShortName<RIGHT_SIDE>()) //adapt difference in case (windows only)
                renameFile(dirObj.getFullName<RIGHT_SIDE>(),
                           dirObj.getFullName<RIGHT_SIDE>().BeforeLast(common::FILE_NAME_SEPARATOR) + common::FILE_NAME_SEPARATOR + dirObj.getShortName<LEFT_SIDE>()); //throw (FileError);
            //copyFileTimes(dirObj.getFullName<LEFT_SIDE>(), dirObj.getFullName<RIGHT_SIDE>(), true); //throw (FileError) -> is executed after sub-objects have finished synchronization
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
    statusUpdater_.updateProcessedData(1, 0);  //each call represents one processed file
}


//avoid data loss when source directory doesn't (temporarily?) exist anymore AND user chose to ignore errors (else we wouldn't arrive here)
bool dataLossPossible(const Zstring& dirName, const SyncStatistics& folderPairStat)
{
    return folderPairStat.getCreate() +  folderPairStat.getOverwrite() + folderPairStat.getConflict() == 0 &&
           folderPairStat.getDelete() > 0 && //deletions only... (respect filtered items!)
           !dirName.empty() && !ffs3::dirExists(dirName);
}


namespace
{
void makeSameLength(wxString& first, wxString& second)
{
    const size_t maxPref = std::max(first.length(), second.length());
    first.Pad(maxPref - first.length(), wxT(' '), true);
    second.Pad(maxPref - second.length(), wxT(' '), true);
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
#ifdef NDEBUG
    wxLogNull noWxLogs; //prevent wxWidgets logging
#endif

    //PERF_START;

    if (syncConfig.size() != folderCmp.size())
        throw std::logic_error("Programming Error: Contract violation!");


    //inform about the total amount of data that will be processed from now on
    const SyncStatistics statisticsTotal(folderCmp);

    //keep at beginning so that all gui elements are initialized properly
    statusUpdater.initNewProcess(statisticsTotal.getCreate() + statisticsTotal.getOverwrite() + statisticsTotal.getDelete(),
                                 common::convertToSigned(statisticsTotal.getDataToProcess()),
                                 StatusHandler::PROCESS_SYNCHRONIZING);

    if (!synchronizationNeeded(statisticsTotal))
        statusUpdater.reportInfo(wxToZ(_("Nothing to synchronize according to configuration!"))); //inform about this special case


    std::deque<bool> skipFolderPair(folderCmp.size()); //folder pairs may be skipped after fatal errors were found

    //-------------------some basic checks:------------------------------------------


    //aggregate information
    typedef std::set<Zstring,         LessDependentDirectory> DirReadSet;  //count (at least one) read access
    typedef std::map<Zstring, size_t, LessDependentDirectory> DirWriteMap; //count (read+)write accesses
    DirReadSet  dirReadCount;
    DirWriteMap dirWriteCount;

    typedef std::vector<std::pair<Zstring, Zstring> > DirPairList;
    DirPairList significantDiff;

    typedef std::vector<std::pair<Zstring, std::pair<wxLongLong, wxLongLong> > > DirSpaceRequAvailList; //dirname / space required / space available
    DirSpaceRequAvailList diskSpaceMissing;


    //start checking folder pairs
    for (FolderComparison::const_iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        const size_t folderIndex = j - folderCmp.begin();

        //exclude some pathological case (leftdir, rightdir are empty)
        if (EqualFilename()(j->getBaseDir<LEFT_SIDE>(), j->getBaseDir<RIGHT_SIDE>()))
            continue;

        const FolderPairSyncCfg& folderPairCfg = syncConfig[folderIndex];

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
            statusUpdater.reportFatalError(_("Cannot write to empty directory path!"));
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
            //test existence of Recycle Bin
            if (folderPairCfg.handleDeletion == ffs3::MOVE_TO_RECYCLE_BIN && !ffs3::recycleBinExists())
            {
                statusUpdater.reportFatalError(_("Recycle Bin not yet supported for this system!"));
                skipFolderPair[folderIndex] = true;
                continue;
            }

            if (folderPairCfg.handleDeletion == ffs3::MOVE_TO_CUSTOM_DIRECTORY)
            {
                //check if user-defined directory for deletion was specified
                if (folderPairCfg.custDelFolder.empty())
                {
                    statusUpdater.reportFatalError(_("User-defined directory for deletion was not specified!"));
                    skipFolderPair[folderIndex] = true;
                    continue;
                }
            }
        }

        //avoid data loss when source directory doesn't (temporarily?) exist anymore AND user chose to ignore errors(else we wouldn't arrive here)
        if (dataLossPossible(j->getBaseDir<LEFT_SIDE>(), statisticsFolderPair))
        {
            statusUpdater.reportFatalError(wxString(_("Source directory does not exist anymore:")) + wxT("\n\"") + zToWx(j->getBaseDir<LEFT_SIDE>()) + wxT("\""));
            skipFolderPair[folderIndex] = true;
            continue;
        }
        if (dataLossPossible(j->getBaseDir<RIGHT_SIDE>(), statisticsFolderPair))
        {
            statusUpdater.reportFatalError(wxString(_("Source directory does not exist anymore:")) + wxT("\n\"") + zToWx(j->getBaseDir<RIGHT_SIDE>()) + wxT("\"") );
            skipFolderPair[folderIndex] = true;
            continue;
        }

        //check if more than 50% of total number of files/dirs are to be created/overwritten/deleted
        if (significantDifferenceDetected(statisticsFolderPair))
            significantDiff.push_back(std::make_pair(j->getBaseDir<LEFT_SIDE>(), j->getBaseDir<RIGHT_SIDE>()));

        //check for sufficient free diskspace in left directory
        const std::pair<wxLongLong, wxLongLong> spaceNeeded = freeDiskSpaceNeeded(*j, folderPairCfg.handleDeletion, folderPairCfg.custDelFolder);

        wxLongLong freeDiskSpaceLeft;
        if (wxGetDiskSpace(zToWx(j->getBaseDir<LEFT_SIDE>()), NULL, &freeDiskSpaceLeft))
        {
            if (0 < freeDiskSpaceLeft && //zero disk space is either an error or not: in both cases this warning message is obsolete (WebDav seems to report 0)
                freeDiskSpaceLeft < spaceNeeded.first)
                diskSpaceMissing.push_back(std::make_pair(j->getBaseDir<LEFT_SIDE>(), std::make_pair(spaceNeeded.first, freeDiskSpaceLeft)));
        }

        //check for sufficient free diskspace in right directory
        wxLongLong freeDiskSpaceRight;
        if (wxGetDiskSpace(zToWx(j->getBaseDir<RIGHT_SIDE>()), NULL, &freeDiskSpaceRight))
        {
            if (0 < freeDiskSpaceRight && //zero disk space is either an error or not: in both cases this warning message is obsolete (WebDav seems to report 0)
                freeDiskSpaceRight < spaceNeeded.second)
                diskSpaceMissing.push_back(std::make_pair(j->getBaseDir<RIGHT_SIDE>(), std::make_pair(spaceNeeded.second, freeDiskSpaceRight)));
        }
    }

    //check if unresolved conflicts exist
    if (statisticsTotal.getConflict() > 0)
    {
        //show the first few conflicts in warning message also:
        wxString warningMessage = wxString(_("Unresolved conflicts existing!")) +
                                  wxT(" (") + numberToStringSep(statisticsTotal.getConflict()) + wxT(")\n\n");

        const SyncStatistics::ConflictTexts& firstConflicts = statisticsTotal.getFirstConflicts(); //get first few sync conflicts
        for (SyncStatistics::ConflictTexts::const_iterator i = firstConflicts.begin(); i != firstConflicts.end(); ++i)
        {
            wxString conflictDescription = i->second;
            //conflictDescription.Replace(wxT("\n"), wxT(" ")); //remove line-breaks

            warningMessage += wxString(wxT("\"")) + zToWx(i->first) + wxT("\": ") + conflictDescription + wxT("\n\n");
        }

        if (statisticsTotal.getConflict() > static_cast<int>(firstConflicts.size()))
            warningMessage += wxT("[...]\n\n");

        warningMessage += _("You can ignore conflicts and continue synchronization.");

        statusUpdater.reportWarning(warningMessage, m_warnings.warningUnresolvedConflicts);
    }


    //check if more than 50% of total number of files/dirs are to be created/overwritten/deleted
    if (!significantDiff.empty())
    {
        wxString warningMessage = _("Significant difference detected:");

        for (DirPairList::const_iterator i = significantDiff.begin(); i != significantDiff.end(); ++i)
            warningMessage += wxString(wxT("\n\n")) +
                              zToWx(i->first) + wxT(" <-> ") + wxT("\n") +
                              zToWx(i->second);
        warningMessage += wxString(wxT("\n\n")) +_("More than 50% of the total number of files will be copied or deleted!");

        statusUpdater.reportWarning(warningMessage, m_warnings.warningSignificantDifference);
    }


    //check for sufficient free diskspace
    if (!diskSpaceMissing.empty())
    {
        wxString warningMessage = _("Not enough free disk space available in:");

        for (DirSpaceRequAvailList::const_iterator i = diskSpaceMissing.begin(); i != diskSpaceMissing.end(); ++i)
            warningMessage += wxString(wxT("\n\n")) +
                              wxT("\"") + zToWx(i->first) + wxT("\"\n") +
                              _("Free disk space required:")  + wxT(" ") + formatFilesizeToShortString(i->second.first)  + wxT("\n") +
                              _("Free disk space available:") + wxT(" ") + formatFilesizeToShortString(i->second.second);

        statusUpdater.reportWarning(warningMessage, m_warnings.warningNotEnoughDiskSpace);
    }


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
                              wxT("\"") + zToWx(*i) + wxT("\"");
        statusUpdater.reportWarning(warningMessage, m_warnings.warningMultiFolderWriteAccess);
    }

    //-------------------end of basic checks------------------------------------------

#ifdef FFS_WIN
    struct ShadowCallback : public shadow::WaitingForShadow
    {
        ShadowCallback(StatusHandler& updater) : statusUpdater_(updater) {}
        virtual void requestUiRefresh() //allowed to throw exceptions
        {
            statusUpdater_.requestUiRefresh();
        }
        virtual void reportInfo(const Zstring& text)
        {
            statusUpdater_.reportInfo(text);
        }
    private:
        StatusHandler& statusUpdater_;
    } shadowCb(statusUpdater);

    //shadow copy buffer: per sync-instance, not folder pair
    boost::scoped_ptr<shadow::ShadowCopy> shadowCopyHandler(copyLockedFiles_ ? new shadow::ShadowCopy(&shadowCb) : NULL);
#endif

    try
    {
        //prevent shutdown while synchronization is in progress
        util::DisableStandby dummy;

        //loop through all directory pairs
        assert(syncConfig.size() == folderCmp.size());
        for (FolderComparison::iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
        {
            const size_t folderIndex = j - folderCmp.begin();

            const FolderPairSyncCfg& folderPairCfg = syncConfig[folderIndex];

            if (skipFolderPair[folderIndex]) //folder pairs may be skipped after fatal errors were found
                continue;

            //exclude some pathological case (leftdir, rightdir are empty)
            if (EqualFilename()(j->getBaseDir<LEFT_SIDE>(), j->getBaseDir<RIGHT_SIDE>()))
                continue;

            //------------------------------------------------------------------------------------------
            //info about folder pair to be processed (useful for logfile)
            wxString left  = wxString(_("Left"))  + wxT(": ");
            wxString right = wxString(_("Right")) + wxT(": ");
            makeSameLength(left, right);

            const wxString statusTxt = wxString(_("Processing folder pair:")) + wxT(" \n") +
                                       wxT("\t") + left  + wxT("\"") + zToWx(j->getBaseDir<LEFT_SIDE>())  + wxT("\"")+ wxT(" \n") +
                                       wxT("\t") + right + wxT("\"") + zToWx(j->getBaseDir<RIGHT_SIDE>()) + wxT("\"");
            statusUpdater.reportInfo(wxToZ(statusTxt));

            //------------------------------------------------------------------------------------------

            //create base directories first (if not yet existing) -> no symlink or attribute copying! -> single error message instead of one per file (e.g. unplugged network drive)
            const Zstring dirnameLeft = j->getBaseDir<LEFT_SIDE>().BeforeLast(common::FILE_NAME_SEPARATOR);
            if (!dirnameLeft.empty() && !ffs3::dirExists(dirnameLeft))
            {
                if (!tryReportingError(statusUpdater, boost::bind(ffs3::createDirectory, boost::cref(dirnameLeft)))) //may throw in error-callback!
                    continue; //skip this folder pair
            }
            const Zstring dirnameRight = j->getBaseDir<RIGHT_SIDE>().BeforeLast(common::FILE_NAME_SEPARATOR);
            if (!dirnameRight.empty() && !ffs3::dirExists(dirnameRight))
            {
                if (!tryReportingError(statusUpdater, boost::bind(ffs3::createDirectory, boost::cref(dirnameRight)))) //may throw in error-callback!
                    continue; //skip this folder pair
            }
            //------------------------------------------------------------------------------------------

            //generate name of alternate deletion directory (unique for session AND folder pair)
            const DeletionHandling currentDelHandling(
                folderPairCfg.handleDeletion,
                folderPairCfg.custDelFolder,
                j->getBaseDir<LEFT_SIDE>(), j->getBaseDir<RIGHT_SIDE>(),
                statusUpdater);

            //------------------------------------------------------------------------------------------
            //execute synchronization recursively

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
                                         currentDelHandling);

            //loop through all files twice; reason: first delete files (or overwrite big ones with smaller ones), then copy rest
            syncFP.startSync<true>(*j);
            syncFP.startSync<false>(*j);


            //(try to gracefully) cleanup temporary folders (Recycle bin optimization) -> will be done in ~DeletionHandling anyway...
            currentDelHandling.tryCleanup(); //show error dialog if necessary

            //------------------------------------------------------------------------------------------

            //update synchronization database (automatic sync only)
            if (folderPairCfg.inAutomaticMode)
            {
                statusUpdater.reportInfo(wxToZ(_("Generating database...")));
                statusUpdater.forceUiRefresh();
                tryReportingError(statusUpdater, boost::bind(ffs3::saveToDisk, boost::cref(*j))); //may throw in error-callback!
            }
        }
    }
    catch (const std::exception& e)
    {
        statusUpdater.reportFatalError(wxString::FromUTF8(e.what()));
    }
}


//###########################################################################################
//callback functionality for smooth progress indicators

template <class DelTargetCommand>
class WhileCopying : public ffs3::CallbackCopyFile //callback functionality
{
public:
    WhileCopying(wxLongLong& bytesTransferredLast,
                 StatusHandler& statusHandler,
                 const DelTargetCommand& cmd) :
        bytesTransferredLast_(bytesTransferredLast),
        statusHandler_(statusHandler),
        cmd_(cmd) {}

    virtual void deleteTargetFile(const Zstring& targetFile) { cmd_(); }

    virtual Response updateCopyStatus(const wxULongLong& totalBytesTransferred)
    {
        //convert to signed
        const wxLongLong totalBytes = common::convertToSigned(totalBytesTransferred);

        //inform about the (differential) processed amount of data
        statusHandler_.updateProcessedData(0, totalBytes - bytesTransferredLast_);
        bytesTransferredLast_ = totalBytes;

#ifdef FFS_WIN
        statusHandler_.requestUiRefresh(false); //don't allow throwing exception within this call: windows copying callback can't handle this
        if (statusHandler_.abortIsRequested())
            return CallbackCopyFile::CANCEL;
#elif defined FFS_LINUX
        statusHandler_.requestUiRefresh(); //exceptions may be thrown here!
#endif
        return CallbackCopyFile::CONTINUE;
    }

private:
    wxLongLong&      bytesTransferredLast_;
    StatusHandler&   statusHandler_;
    DelTargetCommand cmd_;
};


class CleanUpStats //lambdas coming soon... unfortunately Loki::ScopeGuard is no option because of lazy function argument evaluation (-1 * ...)
{
    bool dismissed;
    StatusHandler& statusUpdater_;
    const wxLongLong& bytesTransferred_;

public:
    CleanUpStats(StatusHandler& statusUpdater, const wxLongLong& bytesTransferred) : dismissed(false), statusUpdater_(statusUpdater), bytesTransferred_(bytesTransferred) {}
    ~CleanUpStats()
    {
        if (!dismissed)
            try
            {
                statusUpdater_.updateProcessedData(0, bytesTransferred_ * -1); //throw ?
            }
            catch (...) {}
    }
    void dismiss() { dismissed = true; }
};


//copy file while executing statusUpdater->requestUiRefresh() calls
template <class DelTargetCommand>
void SynchronizeFolderPair::copyFileUpdating(const Zstring& source, const Zstring& target, const DelTargetCommand& cmd, const wxULongLong& totalBytesToCpy, int recursionLvl) const
{
    const int exceptionPaths = 2;

    try
    {
        //start of (possibly) long-running copy process: ensure status updates are performed regularly
        wxLongLong totalBytesTransferred;
        CleanUpStats dummy(statusUpdater_, totalBytesTransferred); //error situation: undo communication of processed amount of data

        WhileCopying<DelTargetCommand> callback(totalBytesTransferred, statusUpdater_, cmd);

        ffs3::copyFile(source, //type File implicitly means symlinks need to be dereferenced!
                       target,
                       copyFilePermissions_,
                       &callback); //throw (FileError, ErrorFileLocked);

        //inform about the (remaining) processed amount of data
        dummy.dismiss();
        statusUpdater_.updateProcessedData(0, common::convertToSigned(totalBytesToCpy) - totalBytesTransferred);
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
            wxString errorMsg = _("Error copying locked file %x!");
            errorMsg.Replace(wxT("%x"), wxString(wxT("\"")) + zToWx(source) + wxT("\""));
            errorMsg += wxT("\n\n") + e.msg();
            throw FileError(errorMsg);
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
        const Zstring targetDir   = target.BeforeLast(common::FILE_NAME_SEPARATOR);
        const Zstring templateDir = source.BeforeLast(common::FILE_NAME_SEPARATOR);

        if (!targetDir.empty() && !ffs3::dirExists(targetDir))
        {
            ffs3::createDirectory(targetDir, templateDir, copyFilePermissions_); //throw (FileError)
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
}


void SynchronizeFolderPair::copySymlink(const Zstring& source, const Zstring& target, LinkDescriptor::LinkType type, bool inRecursion) const
{
    try
    {
        switch (type)
        {
            case LinkDescriptor::TYPE_DIR:
                ffs3::copySymlink(source, target, SYMLINK_TYPE_DIR, copyFilePermissions_); //throw (FileError)
                break;

            case LinkDescriptor::TYPE_FILE: //Windows: true file symlink; Linux: file-link or broken link
                ffs3::copySymlink(source, target, SYMLINK_TYPE_FILE, copyFilePermissions_); //throw (FileError)
                break;
        }
    }
    catch (FileError&)
    {
        if (inRecursion) throw;

        //create folders "first" (see http://sourceforge.net/tracker/index.php?func=detail&aid=2628943&group_id=234430&atid=1093080)
        //using optimistic strategy: assume everything goes well, but cover up on error -> minimize file accesses
        const Zstring targetDir   = target.BeforeLast(common::FILE_NAME_SEPARATOR);
        const Zstring templateDir = source.BeforeLast(common::FILE_NAME_SEPARATOR);

        if (!targetDir.empty() && !ffs3::dirExists(targetDir))
        {
            ffs3::createDirectory(targetDir, templateDir, copyFilePermissions_); //throw (FileError)
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


template <ffs3::SelectedSide side>
void SynchronizeFolderPair::deleteSymlink(const SymLinkMapping& linkObj) const
{
    switch (linkObj.getLinkType<side>())
    {
        case LinkDescriptor::TYPE_DIR:
            delHandling_.removeFolder<side>(linkObj); //throw (FileError)
            break;

        case LinkDescriptor::TYPE_FILE: //Windows: true file symlink; Linux: file-link or broken link
            delHandling_.removeFile<side>(linkObj); //throw (FileError)
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


void verifyFiles(const Zstring& source, const Zstring& target, VerifyCallback* callback) // throw (FileError)
{
    const size_t BUFFER_SIZE = 1024 * 1024; //1024 kb seems to be a reasonable buffer size
    static const boost::scoped_array<char> memory1(new char[BUFFER_SIZE]);
    static const boost::scoped_array<char> memory2(new char[BUFFER_SIZE]);

#ifdef FFS_WIN
    wxFile file1(ffs3::applyLongPathPrefix(source).c_str(), wxFile::read); //don't use buffered file input for verification!
#elif defined FFS_LINUX
    wxFile file1(::open(source.c_str(), O_RDONLY)); //utilize UTF-8 filename
#endif
    if (!file1.IsOpened())
        throw FileError(wxString(_("Error opening file:")) + wxT(" \"") + zToWx(source) + wxT("\""));

#ifdef FFS_WIN
    wxFile file2(ffs3::applyLongPathPrefix(target).c_str(), wxFile::read); //don't use buffered file input for verification!
#elif defined FFS_LINUX
    wxFile file2(::open(target.c_str(), O_RDONLY)); //utilize UTF-8 filename
#endif
    if (!file2.IsOpened()) //NO cleanup necessary for (wxFile) file1
        throw FileError(wxString(_("Error opening file:")) + wxT(" \"") + zToWx(target) + wxT("\""));

    do
    {
        const size_t length1 = file1.Read(memory1.get(), BUFFER_SIZE);
        if (file1.Error()) throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + zToWx(source) + wxT("\""));
        callback->updateStatus(); //send progress updates

        const size_t length2 = file2.Read(memory2.get(), BUFFER_SIZE);
        if (file2.Error()) throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + zToWx(target) + wxT("\""));
        callback->updateStatus(); //send progress updates

        if (length1 != length2 || ::memcmp(memory1.get(), memory2.get(), length1) != 0)
        {
            const wxString errorMsg = wxString(_("Data verification error: Source and target file have different content!")) + wxT("\n");
            throw FileError(errorMsg + wxT("\"") + zToWx(source) + wxT("\" -> \n\"") + zToWx(target) + wxT("\""));
        }
    }
    while (!file1.Eof());

    if (!file2.Eof())
    {
        const wxString errorMsg = wxString(_("Data verification error: Source and target file have different content!")) + wxT("\n");
        throw FileError(errorMsg + wxT("\"") + zToWx(source) + wxT("\" -> \n\"") + zToWx(target) + wxT("\""));
    }
}


class VerifyStatusUpdater : public VerifyCallback
{
public:
    VerifyStatusUpdater(StatusHandler& statusHandler) : statusHandler_(statusHandler) {}

    virtual void updateStatus()
    {
        statusHandler_.requestUiRefresh(); //trigger display refresh
    }

private:
    StatusHandler& statusHandler_;
};


void SynchronizeFolderPair::verifyFileCopy(const Zstring& source, const Zstring& target) const
{
    Zstring statusText = txtVerifying;
    statusText.Replace(Zstr("%x"), target, false);
    statusUpdater_.reportInfo(statusText);
    statusUpdater_.requestUiRefresh(); //trigger display refresh

    VerifyStatusUpdater callback(statusUpdater_);

    tryReportingError(statusUpdater_, boost::bind(&::verifyFiles, boost::ref(source), boost::ref(target), &callback));
}
