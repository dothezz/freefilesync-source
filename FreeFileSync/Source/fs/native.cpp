// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "native.h"
#include <zen/file_access.h>
#include <zen/symlink_target.h>
#include <zen/file_io.h>
#include <zen/file_id_def.h>
#include <zen/stl_tools.h>
#include <zen/recycler.h>
#include "../lib/resolve_path.h"
#include "../lib/icon_loader.h"
#include "native_traverser_impl.h"

    #include <fcntl.h> //fallocate, fcntl


namespace
{


void initComForThread() //throw FileError
{
}


class RecycleSessionNative : public AbstractFileSystem::RecycleSession
{
public:
    RecycleSessionNative(const Zstring folderPathPf) : baseFolderPathPf_(folderPathPf) {}

    bool recycleItem(const AbstractPath& itemPath, const Zstring& logicalRelPath) override; //throw FileError
    void tryCleanup(const std::function<void (const std::wstring& displayPath)>& notifyDeletionStatus) override; //throw FileError

private:

    const Zstring baseFolderPathPf_; //ends with path separator
};

//===========================================================================================================================

void preAllocateSpaceBestEffort(FileHandle fh, const std::uint64_t streamSize, const Zstring& displayPath) //throw FileError
{
    //don't use potentially inefficient ::posix_fallocate!
    const int rv = ::fallocate(fh,          //int fd,
                               0,           //int mode,
                               0,           //off_t offset
                               streamSize); //off_t len
    if (rv != 0)
        return; //may fail with EOPNOTSUPP, unlike posix_fallocate

}


    typedef struct ::stat FileAttribs; //GCC 5.2 fails when "::" is used in "using FileAttribs = struct ::stat"


inline
FileAttribs getFileAttributes(FileHandle fh, const Zstring& filePath) //throw FileError
{
    struct ::stat fileAttr = {};
    if (::fstat(fh, &fileAttr) != 0)
        THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtPath(filePath)), L"fstat");
    return fileAttr;
}


struct InputStreamNative : public AbstractFileSystem::InputStream
{
    InputStreamNative(const Zstring& filePath) : fi(filePath) {} //throw FileError, ErrorFileLocked

    size_t read(void* buffer, size_t bytesToRead) override { return fi.read(buffer, bytesToRead); } //throw FileError; returns "bytesToRead", unless end of file!
    AFS::FileId   getFileId          () override; //throw FileError
    std::int64_t  getModificationTime() override; //throw FileError
    std::uint64_t getFileSize        () override; //throw FileError
    size_t optimalBlockSize() const override { return fi.optimalBlockSize(); } //non-zero block size is AFS contract!

private:
    const FileAttribs& getBufferedAttributes() //throw FileError
    {
        if (!fileAttr)
            fileAttr = getFileAttributes(fi.getHandle(), fi.getFilePath()); //throw FileError
        return *fileAttr;
    }

    FileInput fi;
    Opt<FileAttribs> fileAttr;
};


inline
AFS::FileId InputStreamNative::getFileId()
{
    return convertToAbstractFileId(extractFileId(getBufferedAttributes())); //throw FileError
}


inline
std::int64_t InputStreamNative::getModificationTime()
{
    return getBufferedAttributes().st_mtime; //throw FileError
}


inline
std::uint64_t InputStreamNative::getFileSize()
{
    const FileAttribs& fa = getBufferedAttributes(); //throw FileError
    return makeUnsigned(fa.st_size); //throw FileError
}

//===========================================================================================================================

struct OutputStreamNative : public AbstractFileSystem::OutputStreamImpl
{
    OutputStreamNative(const Zstring& filePath, const std::uint64_t* streamSize, const std::int64_t* modTime) :
        fo(filePath, FileOutput::ACC_CREATE_NEW) //throw FileError, ErrorTargetExisting
    {
        if (modTime)
            modTime_ = *modTime;

        if (streamSize) //pre-allocate file space, because we can
            preAllocateSpaceBestEffort(fo.getHandle(), *streamSize, fo.getFilePath()); //throw FileError
    }

    size_t optimalBlockSize() const override { return fo.optimalBlockSize(); } //non-zero block size is AFS contract!

private:
    void write(const void* buffer, size_t bytesToWrite) override { fo.write(buffer, bytesToWrite); } //throw FileError

    AFS::FileId finalize(const std::function<void()>& onUpdateStatus) override //throw FileError
    {
        const AFS::FileId fileId = convertToAbstractFileId(extractFileId(getFileAttributes(fo.getHandle(), fo.getFilePath()))); //throw FileError
        if (onUpdateStatus) onUpdateStatus(); //throw X!

        fo.close(); //throw FileError
        if (onUpdateStatus) onUpdateStatus(); //throw X!

        try
        {
            if (modTime_)
                zen::setFileTime(fo.getFilePath(), *modTime_, ProcSymlink::FOLLOW); //throw FileError
        }
        catch (FileError&)
        {
            throw;
            //Failing to set modification time is not a serious problem from synchronization
            //perspective (treated like external update), except for the inconvenience.
            //Support additional scenarios like writing to FTP on Linux? Keep strict handling for now.
        }

        return fileId;
    }

    FileOutput fo;
    Opt<std::int64_t> modTime_;
};

//===========================================================================================================================

class NativeFileSystem : public AbstractFileSystem
{
public:
    //itemPathImpl := native full item path as used by OS APIs

    static Zstring getItemPathImplForRecycler(const AbstractPath& ap)
    {
        if (typeid(getAfs(ap)) != typeid(NativeFileSystem))
            throw std::logic_error("Programming Error: Contract violation! " + std::string(__FILE__) + ":" + numberTo<std::string>(__LINE__));
        return getItemPathImpl(ap);
    }

private:
    bool isNativeFileSystem() const override { return true; }

    Zstring getInitPathPhrase(const Zstring& itemPathImpl) const override { return itemPathImpl; }

    std::wstring getDisplayPath(const Zstring& itemPathImpl) const override { return utfCvrtTo<std::wstring>(itemPathImpl); }

    bool isNullPath(const Zstring& itemPathImpl) const override { return itemPathImpl.empty(); }

    Zstring appendRelPathToItemPathImpl(const Zstring& itemPathImpl, const Zstring& relPath) const override { return appendPaths(itemPathImpl, relPath, FILE_NAME_SEPARATOR); }

    //used during folder creation if parent folder is missing
    Opt<Zstring> getParentFolderPathImpl(const Zstring& itemPathImpl) const override
    {
        if (itemPathImpl == "/")
            return NoValue();

        const Zstring parentDir = beforeLast(itemPathImpl, FILE_NAME_SEPARATOR, IF_MISSING_RETURN_NONE);
        if (parentDir.empty())
            return Zstring("/");
        return parentDir;
    }

    Zstring getFileShortName(const Zstring& itemPathImpl) const override { return afterLast(itemPathImpl, FILE_NAME_SEPARATOR, IF_MISSING_RETURN_ALL); }

    bool lessItemPathSameAfsType(const Zstring& itemPathImplLhs, const AbstractPath& apRhs) const override { return LessFilePath()(itemPathImplLhs, getItemPathImpl(apRhs)); }

    bool havePathDependencySameAfsType(const Zstring& itemPathImplLhs, const AbstractPath& apRhs) const override
    {
        const Zstring& lhs = appendSeparator(itemPathImplLhs);
        const Zstring& rhs = appendSeparator(getItemPathImpl(apRhs));

        const size_t lenMin = std::min(lhs.length(), rhs.length());

        return cmpFilePath(lhs.c_str(), lenMin, rhs.c_str(), lenMin) == 0; //note: don't make this an equivalence relation!
    }

    //----------------------------------------------------------------------------------------------------------------
    bool fileExists     (const Zstring& itemPathImpl) const override { return zen::fileExists     (itemPathImpl); } //noexcept
    bool folderExists   (const Zstring& itemPathImpl) const override { return zen::dirExists      (itemPathImpl); } //noexcept
    bool symlinkExists  (const Zstring& itemPathImpl) const override { return zen::symlinkExists  (itemPathImpl); } //noexcept
    bool somethingExists(const Zstring& itemPathImpl) const override { return zen::somethingExists(itemPathImpl); } //noexcept
    //----------------------------------------------------------------------------------------------------------------

    //should provide for single ATOMIC folder creation!
    void createFolderSimple(const Zstring& itemPathImpl) const override //throw FileError, ErrorTargetExisting, ErrorTargetPathMissing
    {
        initComForThread(); //throw FileError
        copyNewDirectory(Zstring(), itemPathImpl, false /*copyFilePermissions*/);
    }

    void removeFolderSimple(const Zstring& itemPathImpl) const override //throw FileError
    {
        initComForThread(); //throw FileError
        zen::removeDirectorySimple(itemPathImpl); //throw FileError
    }

    bool removeFile(const Zstring& itemPathImpl) const override //throw FileError
    {
        initComForThread(); //throw FileError
        return zen::removeFile(itemPathImpl); //throw FileError
    }

    //----------------------------------------------------------------------------------------------------------------

    void setModTime(const Zstring& itemPathImpl, std::int64_t modificationTime) const override //throw FileError, follows symlinks
    {
        initComForThread(); //throw FileError
        zen::setFileTime(itemPathImpl, modificationTime, ProcSymlink::FOLLOW); //throw FileError
    }

    void setModTimeSymlink(const Zstring& itemPathImpl, std::int64_t modificationTime) const override //throw FileError
    {
        initComForThread(); //throw FileError
        zen::setFileTime(itemPathImpl, modificationTime, ProcSymlink::DIRECT); //throw FileError
    }

    Zstring getResolvedSymlinkPath(const Zstring& itemPathImpl) const override //throw FileError
    {
        initComForThread(); //throw FileError
        return zen::getResolvedSymlinkPath(itemPathImpl); //throw FileError
    }

    Zstring getSymlinkContentBuffer(const Zstring& itemPathImpl) const override //throw FileError
    {
        initComForThread(); //throw FileError
        return getSymlinkTargetRaw(itemPathImpl); //throw FileError
    }

    //----------------------------------------------------------------------------------------------------------------
    //return value always bound:
    std::unique_ptr<InputStream > getInputStream (const Zstring& itemPathImpl) const override //throw FileError, ErrorFileLocked
    {
        initComForThread(); //throw FileError
        return std::make_unique<InputStreamNative >(itemPathImpl); //throw FileError, ErrorFileLocked
    }

    std::unique_ptr<OutputStreamImpl> getOutputStream(const Zstring& itemPathImpl,  //throw FileError, ErrorTargetExisting
                                                      const std::uint64_t* streamSize,                     //optional
                                                      const std::int64_t* modificationTime) const override //
    {
        initComForThread(); //throw FileError
        return std::make_unique<OutputStreamNative>(itemPathImpl, streamSize, modificationTime); //throw FileError, ErrorTargetExisting
    }

    //----------------------------------------------------------------------------------------------------------------
    void traverseFolder(const Zstring& itemPathImpl, TraverserCallback& sink) const override //noexcept
    {
        DirTraverser::execute(itemPathImpl, sink); //noexcept
    }
    //----------------------------------------------------------------------------------------------------------------

    //symlink handling: follow link!
    FileAttribAfterCopy copyFileForSameAfsType(const Zstring& itemPathImplSource, const AbstractPath& apTarget, bool copyFilePermissions, //throw FileError, ErrorTargetExisting, ErrorFileLocked
                                               const std::function<void(std::int64_t bytesDelta)>& onNotifyCopyStatus) const override //may be nullptr; throw X!
    {
        initComForThread(); //throw FileError

        const InSyncAttributes attrNew = copyNewFile(itemPathImplSource, getItemPathImpl(apTarget), //throw FileError, ErrorTargetExisting, ErrorFileLocked
                                                     copyFilePermissions, onNotifyCopyStatus); //may be nullptr; throw X!
        FileAttribAfterCopy attrOut;
        attrOut.fileSize         = attrNew.fileSize;
        attrOut.modificationTime = attrNew.modificationTime;
        attrOut.sourceFileId     = convertToAbstractFileId(attrNew.sourceFileId);
        attrOut.targetFileId     = convertToAbstractFileId(attrNew.targetFileId);
        return attrOut;
    }

    //symlink handling: follow link!
    void copyNewFolderForSameAfsType(const Zstring& itemPathImplSource, const AbstractPath& apTarget, bool copyFilePermissions) const override //throw FileError
    {
        initComForThread(); //throw FileError
        zen::copyNewDirectory(itemPathImplSource, getItemPathImpl(apTarget), copyFilePermissions);
    }

    void copySymlinkForSameAfsType(const Zstring& itemPathImplSource, const AbstractPath& apTarget, bool copyFilePermissions) const override //throw FileError
    {
        initComForThread(); //throw FileError
        zen::copySymlink(itemPathImplSource, getItemPathImpl(apTarget), copyFilePermissions); //throw FileError
    }

    void renameItemForSameAfsType(const Zstring& itemPathImplSource, const AbstractPath& apTarget) const override //throw FileError, ErrorTargetExisting, ErrorDifferentVolume
    {
        initComForThread(); //throw FileError
        zen::renameFile(itemPathImplSource, getItemPathImpl(apTarget)); //throw FileError, ErrorTargetExisting, ErrorDifferentVolume
    }

    bool supportsPermissions(const Zstring& itemPathImpl) const override //throw FileError
    {
        initComForThread(); //throw FileError
        return zen::supportsPermissions(itemPathImpl);
    }

    //----------------------------------------------------------------------------------------------------------------
    ImageHolder getFileIcon(const Zstring& itemPathImpl, int pixelSize) const override //noexcept; optional return value
    {
        try
        {
            initComForThread(); //throw FileError
            return zen::getFileIcon(itemPathImpl, pixelSize);
        }
        catch (FileError&) { assert(false); return ImageHolder(); }
    }

    ImageHolder getThumbnailImage(const Zstring& itemPathImpl, int pixelSize) const override //noexcept; optional return value
    {
        try
        {
            initComForThread(); //throw FileError
            return zen::getThumbnailImage(itemPathImpl, pixelSize);
        }
        catch (FileError&) { assert(false); return ImageHolder(); }
    }

    bool folderExistsThrowing(const Zstring& itemPathImpl) const override //throw FileError
    {
        warn_static("finish file error detection")

        initComForThread(); //throw FileError
        return zen::dirExists(itemPathImpl);
    }

    void connectNetworkFolder(const Zstring& itemPathImpl, bool allowUserInteraction) const override //throw FileError
    {
        warn_static("clean-up/remove/re-think the getAsyncConnectFolder() function")

    }

    //----------------------------------------------------------------------------------------------------------------

    std::uint64_t getFreeDiskSpace(const Zstring& itemPathImpl) const override //throw FileError, returns 0 if not available
    {
        initComForThread(); //throw FileError
        return zen::getFreeDiskSpace(itemPathImpl); //throw FileError
    }

    bool supportsRecycleBin(const Zstring& itemPathImpl, const std::function<void ()>& onUpdateGui) const override //throw FileError
    {
        return true; //truth be told: no idea!!!
    }

    std::unique_ptr<RecycleSession> createRecyclerSession(const Zstring& itemPathImpl) const override //throw FileError, return value must be bound!
    {
        initComForThread(); //throw FileError
        assert(supportsRecycleBin(itemPathImpl, nullptr));
        return std::make_unique<RecycleSessionNative>(appendSeparator(itemPathImpl));
    }

    void recycleItemDirectly(const Zstring& itemPathImpl) const override //throw FileError
    {
        initComForThread(); //throw FileError
        zen::recycleOrDelete(itemPathImpl); //throw FileError
    }
};

//===========================================================================================================================



bool RecycleSessionNative::recycleItem(const AbstractPath& itemPath, const Zstring& logicalRelPath) //throw FileError
{
    const Zstring itemPathImpl = NativeFileSystem::getItemPathImplForRecycler(itemPath);
    assert(!startsWith(logicalRelPath, FILE_NAME_SEPARATOR));

    return recycleOrDelete(itemPathImpl); //throw FileError
}


void RecycleSessionNative::tryCleanup(const std::function<void (const std::wstring& displayPath)>& notifyDeletionStatus) //throw FileError
{
}
}


//coordinate changes with getResolvedFilePath()!
bool zen::acceptsItemPathPhraseNative(const Zstring& itemPathPhrase) //noexcept
{
    Zstring path = itemPathPhrase;
    path = expandMacros(path); //expand before trimming!
    trim(path);


    if (startsWith(path, Zstr("["))) //drive letter by volume name syntax
        return true;

    //don't accept relative paths!!! indistinguishable from Explorer MTP paths!

    return startsWith(path, Zstr("/"));
}


AbstractPath zen::createItemPathNative(const Zstring& itemPathPhrase) //noexcept
{
    warn_static("get volume by name hangs for idle HDD! => run createItemPathNative during getFolderStatusNonBlocking() but getResolvedFilePath currently not thread-safe!")
    const Zstring itemPathImpl = getResolvedFilePath(itemPathPhrase);
    return AbstractPath(std::make_shared<NativeFileSystem>(), itemPathImpl);
}


AbstractPath zen::createItemPathNativeNoFormatting(const Zstring& nativePath) //noexcept
{
    return AbstractPath(std::make_shared<NativeFileSystem>(), nativePath);
}
