// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0           *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#include "abstract.h"
#include <zen/serialize.h>

using namespace zen;
using AFS = AbstractFileSystem;

const Zchar* AFS::TEMP_FILE_ENDING = Zstr(".ffs_tmp");


bool zen::isValidRelPath(const Zstring& relPath)
{
    const bool check1 = !contains(relPath, '\\');
    const bool check2 = !startsWith(relPath, FILE_NAME_SEPARATOR) && !endsWith(relPath, FILE_NAME_SEPARATOR);
    const bool check3 = !contains(relPath, Zstring() + FILE_NAME_SEPARATOR + FILE_NAME_SEPARATOR);
    return check1 && check2 && check3;
}


int AFS::compareAbstractPath(const AbstractPath& lhs, const AbstractPath& rhs)
{
    //note: in worst case, order is guaranteed to be stable only during each program run
    if (typeid(*lhs.afs).before(typeid(*rhs.afs)))
        return -1;
    if (typeid(*rhs.afs).before(typeid(*lhs.afs)))
        return 1;
    assert(typeid(*lhs.afs) == typeid(*rhs.afs));
    //caveat: typeid returns static type for pointers, dynamic type for references!!!

    const int rv = lhs.afs->compareDeviceRootSameAfsType(*rhs.afs);
    if (rv != 0)
        return rv;

    return CmpFilePath()(lhs.afsPath.value.c_str(), lhs.afsPath.value.size(),
                         rhs.afsPath.value.c_str(), rhs.afsPath.value.size());
}


AFS::PathComponents AFS::getPathComponents(const AbstractPath& ap)
{
    return { AbstractPath(ap.afs, AfsPath(Zstring())), split(ap.afsPath.value, FILE_NAME_SEPARATOR, SplitType::SKIP_EMPTY) };
}


Opt<AbstractPath> AFS::getParentFolderPath(const AbstractPath& ap)
{
    if (const Opt<AfsPath> parentAfsPath = getParentAfsPath(ap.afsPath))
        return AbstractPath(ap.afs, *parentAfsPath);

    return NoValue();
}


Opt<AfsPath> AFS::getParentAfsPath(const AfsPath& afsPath)
{
    if (afsPath.value.empty())
        return NoValue();

    return AfsPath(beforeLast(afsPath.value, FILE_NAME_SEPARATOR, IF_MISSING_RETURN_NONE));
}


AFS::FileAttribAfterCopy AFS::copyFileAsStream(const AfsPath& afsPathSource, const AbstractPath& apTarget, //throw FileError, ErrorTargetExisting, ErrorFileLocked
                                               const IOCallback& notifyUnbufferedIO) const
{
    int64_t totalUnbufferedIO = 0;

    auto streamIn = getInputStream(afsPathSource, IOCallbackDivider(notifyUnbufferedIO, totalUnbufferedIO)); //throw FileError, ErrorFileLocked
	warn_static("save following file access to improve SFTP perf?")
    const uint64_t fileSizeExpected = streamIn->getFileSize        (); //throw FileError
    const int64_t  modificationTime = streamIn->getModificationTime(); //throw FileError
    const FileId   sourceFileId     = streamIn->getFileId          (); //throw FileError

    auto streamOut = getOutputStream(apTarget, &fileSizeExpected, &modificationTime, IOCallbackDivider(notifyUnbufferedIO, totalUnbufferedIO)); //throw FileError, ErrorTargetExisting

    bufferedStreamCopy(*streamIn, *streamOut); //throw FileError, X

    const FileId targetFileId = streamOut->finalize(); //throw FileError, X
    //- modification time should be set here!
    //- checks if "expected == actual number of bytes written"

    //extra check: bytes reported via notifyUnbufferedIO() should match actual number of bytes written
    if (totalUnbufferedIO != 2 * makeSigned(fileSizeExpected))
        throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtPath(AFS::getDisplayPath(apTarget))),
                        replaceCpy(replaceCpy(_("Unexpected size of data stream.\nExpected: %x bytes\nActual: %y bytes"),
                                              L"%x", numberTo<std::wstring>(2 * fileSizeExpected)),
                                   L"%y", numberTo<std::wstring>(totalUnbufferedIO)) + L" [notifyUnbufferedIO]");

    AFS::FileAttribAfterCopy attr;
    attr.fileSize         = fileSizeExpected;
    attr.modificationTime = modificationTime;
    attr.sourceFileId     = sourceFileId;
    attr.targetFileId     = targetFileId;
    return attr;
}


AFS::FileAttribAfterCopy AFS::copyFileTransactional(const AbstractPath& apSource, const AbstractPath& apTarget, //throw FileError, ErrorFileLocked
                                                    bool copyFilePermissions,
                                                    bool transactionalCopy,
                                                    const std::function<void()>& onDeleteTargetFile,
                                                    const IOCallback& notifyUnbufferedIO)
{
    auto copyFileBestEffort = [&](const AbstractPath& apTargetTmp)
    {
        //caveat: typeid returns static type for pointers, dynamic type for references!!!
        if (typeid(*apSource.afs) == typeid(*apTarget.afs))
            return apSource.afs->copyFileForSameAfsType(apSource.afsPath, apTargetTmp, copyFilePermissions, notifyUnbufferedIO); //throw FileError, ErrorTargetExisting, ErrorFileLocked

        //fall back to stream-based file copy:
        if (copyFilePermissions)
            throw FileError(replaceCpy(_("Cannot write permissions of %x."), L"%x", fmtPath(AFS::getDisplayPath(apTargetTmp))),
                            _("Operation not supported for different base folder types."));

        return apSource.afs->copyFileAsStream(apSource.afsPath, apTargetTmp, notifyUnbufferedIO); //throw FileError, ErrorTargetExisting, ErrorFileLocked
    };

    if (transactionalCopy)
    {
        AbstractPath apTargetTmp(apTarget.afs, AfsPath(apTarget.afsPath.value + TEMP_FILE_ENDING));
        AFS::FileAttribAfterCopy attr;

        for (int i = 0;; ++i)
            try
            {
                attr = copyFileBestEffort(apTargetTmp); //throw FileError, ErrorTargetExisting, ErrorFileLocked
                break;
            }
            catch (const ErrorTargetExisting&) //optimistic strategy: assume everything goes well, but recover on error -> minimize file accesses
            {
                if (i == 10) throw; //avoid endless recursion in pathological cases, e.g. http://www.freefilesync.org/forum/viewtopic.php?t=1592
                apTargetTmp.afsPath.value = apTarget.afsPath.value + Zchar('_') + numberTo<Zstring>(i) + TEMP_FILE_ENDING;
            }

        //transactional behavior: ensure cleanup; not needed before copyFileBestEffort() which is already transactional
        ZEN_ON_SCOPE_FAIL( try { AFS::removeFilePlain(apTargetTmp); }
        catch (FileError&) {} );

        //have target file deleted (after read access on source and target has been confirmed) => allow for almost transactional overwrite
        if (onDeleteTargetFile)
            onDeleteTargetFile(); //throw X

        //perf: this call is REALLY expensive on unbuffered volumes! ~40% performance decrease on FAT USB stick!
        renameItem(apTargetTmp, apTarget); //throw FileError, (ErrorDifferentVolume)

        /*
        CAVEAT on FAT/FAT32: the sequence of deleting the target file and renaming "file.txt.ffs_tmp" to "file.txt" does
        NOT PRESERVE the creation time of the .ffs_tmp file, but SILENTLY "reuses" whatever creation time the old "file.txt" had!
        This "feature" is called "File System Tunneling":
        https://blogs.msdn.microsoft.com/oldnewthing/20050715-14/?p=34923
        http://support.microsoft.com/kb/172190/en-us
        */
        return attr;
    }
    else
    {
        /*
           Note: non-transactional file copy solves at least four problems:
                -> skydrive - doesn't allow for .ffs_tmp extension and returns ERROR_INVALID_PARAMETER
                -> network renaming issues
                -> allow for true delete before copy to handle low disk space problems
                -> higher performance on non-buffered drives (e.g. usb sticks)
        */
        if (onDeleteTargetFile)
            onDeleteTargetFile();

        return copyFileBestEffort(apTarget); //throw FileError, ErrorTargetExisting, ErrorFileLocked
    }
}


void AFS::createFolderIfMissingRecursion(const AbstractPath& ap) //throw FileError
{
    if (!getParentFolderPath(ap)) //device root
        return static_cast<void>(/*ItemType =*/ getItemType(ap)); //throw FileError

    try
    {
        createFolderPlain(ap); //throw FileError
    }
    catch (FileError&)
    {
        Opt<PathStatus> pd;
        try { pd = getPathStatus(ap); /*throw FileError*/ }
        catch (FileError&) {} //previous exception is more relevant

        if (pd && pd->existingType != ItemType::FILE)
        {
            AbstractPath intermediatePath = pd->existingPath;
            for (const Zstring& itemName : pd->relPath)
                createFolderPlain(intermediatePath = appendRelPath(intermediatePath, itemName)); //throw FileError
            return;
        }
        throw;
    }
}


namespace
{
struct ItemSearchCallback: public AFS::TraverserCallback
{
    ItemSearchCallback(const Zstring& itemName) : itemName_(itemName) {}

    void                               onFile   (const FileInfo&    fi) override { if (equalFilePath(fi.itemName, itemName_)) throw AFS::ItemType::FILE; }
    std::unique_ptr<TraverserCallback> onFolder (const FolderInfo&  fi) override { if (equalFilePath(fi.itemName, itemName_)) throw AFS::ItemType::FOLDER; return nullptr; }
    HandleLink                         onSymlink(const SymlinkInfo& si) override { if (equalFilePath(si.itemName, itemName_)) throw AFS::ItemType::SYMLINK; return TraverserCallback::LINK_SKIP; }
    HandleError reportDirError (const std::wstring& msg, size_t retryNumber)                          override { throw FileError(msg); }
    HandleError reportItemError(const std::wstring& msg, size_t retryNumber, const Zstring& itemName) override { throw FileError(msg); }

private:
    const Zstring itemName_;
};
}


AFS::PathStatusImpl AFS::getPathStatusViaFolderTraversal(const AfsPath& afsPath) const //throw FileError
{
    const Opt<AfsPath> parentAfsPath = getParentAfsPath(afsPath);
    try
    {
        return { getItemType(afsPath), afsPath, {} }; //throw FileError
    }
    catch (FileError&)
    {
        if (!parentAfsPath) //device root
            throw;
        //else: let's dig deeper... don't bother checking Win32 codes; e.g. not existing item may have the codes:
        //  ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_INVALID_NAME, ERROR_INVALID_DRIVE,
        //  ERROR_NOT_READY, ERROR_INVALID_PARAMETER, ERROR_BAD_PATHNAME, ERROR_BAD_NETPATH => not reliable
    }
    const Zstring itemName = getItemName(afsPath);
    assert(!itemName.empty());

    PathStatusImpl ps = getPathStatusViaFolderTraversal(*parentAfsPath); //throw FileError
    if (!ps.relPath.empty())
    {
        ps.relPath.push_back(itemName);
        return { ps.existingType, ps.existingAfsPath, ps.relPath };
    }

    try
    {
        ItemSearchCallback iscb(itemName);
        traverseFolder(*parentAfsPath, iscb); //throw FileError, ItemType

        return { ps.existingType, *parentAfsPath, { itemName } }; //throw FileError
    }
    catch (const ItemType& type) { return { type, afsPath, {} }; } //yes, exceptions for control-flow are bad design... but, but...
    //we're not CPU-bound here and finding the item after getItemType() previously failed is exceptional (even C:\pagefile.sys should be found)
}


Opt<AFS::ItemType> AFS::getItemTypeIfExists(const AbstractPath& ap) //throw FileError
{
    const PathStatus pd = getPathStatus(ap); //throw FileError
    if (pd.relPath.empty())
        return pd.existingType;
    return NoValue();
}


AFS::PathStatus AFS::getPathStatus(const AbstractPath& ap) //throw FileError
{
    const PathStatusImpl pdi = ap.afs->getPathStatus(ap.afsPath); //throw FileError
    return { pdi.existingType, AbstractPath(ap.afs, pdi.existingAfsPath), pdi.relPath };
}


namespace
{
struct FlatTraverserCallback: public AFS::TraverserCallback
{
    FlatTraverserCallback(const AbstractPath& folderPath) : folderPath_(folderPath) {}

    void                               onFile   (const FileInfo&    fi) override { fileNames_   .push_back(fi.itemName); }
    std::unique_ptr<TraverserCallback> onFolder (const FolderInfo&  fi) override { folderNames_ .push_back(fi.itemName); return nullptr; }
    HandleLink                         onSymlink(const SymlinkInfo& si) override { symlinkNames_.push_back(si.itemName); return TraverserCallback::LINK_SKIP; }
    HandleError reportDirError (const std::wstring& msg, size_t retryNumber)                          override { throw FileError(msg); }
    HandleError reportItemError(const std::wstring& msg, size_t retryNumber, const Zstring& itemName) override { throw FileError(msg); }

    const std::vector<Zstring>& refFileNames   () const { return fileNames_; }
    const std::vector<Zstring>& refFolderNames () const { return folderNames_; }
    const std::vector<Zstring>& refSymlinkNames() const { return symlinkNames_; }

private:
    const AbstractPath folderPath_;
    std::vector<Zstring> fileNames_;
    std::vector<Zstring> folderNames_;
    std::vector<Zstring> symlinkNames_;
};


void removeFolderIfExistsRecursionImpl(const AbstractPath& folderPath, //throw FileError
                                       const std::function<void (const std::wstring& displayPath)>& onBeforeFileDeletion, //optional
                                       const std::function<void (const std::wstring& displayPath)>& onBeforeFolderDeletion) //one call for each *existing* object!
{

    FlatTraverserCallback ft(folderPath); //deferred recursion => save stack space and allow deletion of extremely deep hierarchies!
    AFS::traverseFolder(folderPath, ft); //throw FileError

    for (const Zstring& fileName : ft.refFileNames())
    {
        const AbstractPath filePath = AFS::appendRelPath(folderPath, fileName);
        if (onBeforeFileDeletion)
            onBeforeFileDeletion(AFS::getDisplayPath(filePath));

        AFS::removeFilePlain(filePath); //throw FileError
    }

    for (const Zstring& symlinkName : ft.refSymlinkNames())
    {
        const AbstractPath linkPath = AFS::appendRelPath(folderPath, symlinkName);
        if (onBeforeFileDeletion)
            onBeforeFileDeletion(AFS::getDisplayPath(linkPath));

        AFS::removeSymlinkPlain(linkPath); //throw FileError
    }

    for (const Zstring& folderName : ft.refFolderNames())
        removeFolderIfExistsRecursionImpl(AFS::appendRelPath(folderPath, folderName), //throw FileError
                                          onBeforeFileDeletion, onBeforeFolderDeletion);

    if (onBeforeFolderDeletion)
        onBeforeFolderDeletion(AFS::getDisplayPath(folderPath));

    AFS::removeFolderPlain(folderPath); //throw FileError
}
}


void AFS::removeFolderIfExistsRecursion(const AbstractPath& ap, //throw FileError
                                        const std::function<void (const std::wstring& displayPath)>& onBeforeFileDeletion, //optional
                                        const std::function<void (const std::wstring& displayPath)>& onBeforeFolderDeletion) //one call for each *existing* object!
{
    if (Opt<ItemType> type = AFS::getItemTypeIfExists(ap)) //throw FileError
    {
        if (*type == AFS::ItemType::SYMLINK)
        {
            if (onBeforeFileDeletion)
                onBeforeFileDeletion(AFS::getDisplayPath(ap));

            AFS::removeSymlinkPlain(ap); //throw FileError
        }
        else
            removeFolderIfExistsRecursionImpl(ap, onBeforeFileDeletion, onBeforeFolderDeletion); //throw FileError
    }
    //no error situation if directory is not existing! manual deletion relies on it!
}


bool AFS::removeFileIfExists(const AbstractPath& ap) //throw FileError
{
    try
    {
        removeFilePlain(ap); //throw FileError
        return true;
    }
    catch (FileError&)
    {
        try
        {
            if (!getItemTypeIfExists(ap)) //throw FileError
                return false;
        }
        catch (FileError&) {} //previous exception is more relevant

        throw;
    }
}


bool AFS::removeSymlinkIfExists(const AbstractPath& ap) //throw FileError
{
    try
    {
        AFS::removeSymlinkPlain(ap); //throw FileError
        return true;
    }
    catch (FileError&)
    {
        try
        {
            if (!getItemTypeIfExists(ap)) //throw FileError
                return false;
        }
        catch (FileError&) {} //previous exception is more relevant

        throw;
    }
}
