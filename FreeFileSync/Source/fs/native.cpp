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
#include <zen/int64.h>
#include <zen/stl_tools.h>
#include <zen/recycler.h>
#include "../lib/resolve_path.h"
#include "../lib/icon_loader.h"
#include "native_traverser_impl.h"

#if defined ZEN_LINUX || defined ZEN_MAC
    #include <fcntl.h> //posix_fallocate, fcntl
#endif


namespace
{
class RecycleSessionNative : public AbstractBaseFolder::RecycleSession
{
public:
    RecycleSessionNative(const Zstring baseDirPathPf) : baseDirPathPf_(baseDirPathPf) {}

    bool recycleItem(const AbstractPathRef& ap, const Zstring& logicalRelPath) override; //throw FileError
    void tryCleanup(const std::function<void (const Zstring& displayPath)>& notifyDeletionStatus) override; //throw FileError

private:
#ifdef ZEN_WIN
    Zstring getOrCreateRecyclerTempDirPf(); //throw FileError

    std::vector<Zstring> toBeRecycled; //full path of files located in temporary folder, waiting for batch-recycling
    Zstring recyclerTmpDir; //temporary folder holding files/folders for *deferred* recycling
#endif

    const Zstring baseDirPathPf_; //ends with path separator
};

//===========================================================================================================================

template <class Function> inline
void evalAttributeByHandle(FileHandle fh, const Zstring& filePath, Function evalFileInfo) //throw FileError
{
#ifdef ZEN_WIN
    BY_HANDLE_FILE_INFORMATION fileInfo = {};
    if (!::GetFileInformationByHandle(fh, &fileInfo))
        throwFileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(filePath)), L"GetFileInformationByHandle", getLastError());

#elif defined ZEN_LINUX || defined ZEN_MAC
    struct ::stat fileInfo = {};
    if (::fstat(fh, &fileInfo) != 0)
        throwFileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(filePath)), L"fstat", getLastError());
#endif
    evalFileInfo(fileInfo);
}


inline
ABF::FileId getFileId(FileHandle fh, const Zstring& filePath) //throw FileError
{
    zen::FileId fid;
#ifdef ZEN_WIN
    evalAttributeByHandle(fh, filePath, [&](const BY_HANDLE_FILE_INFORMATION& fileInfo) { fid = extractFileId(fileInfo); }); //throw FileError
#elif defined ZEN_LINUX || defined ZEN_MAC
    evalAttributeByHandle(fh, filePath, [&](const              struct ::stat& fileInfo) { fid = extractFileId(fileInfo); }); //throw FileError
#endif
    return convertToAbstractFileId(fid);
}


inline
std::int64_t getModificationTime(FileHandle fh, const Zstring& filePath) //throw FileError
{
    std::int64_t modTime = 0;
#ifdef ZEN_WIN
    evalAttributeByHandle(fh, filePath, [&](const BY_HANDLE_FILE_INFORMATION& fileInfo) { modTime = filetimeToTimeT(fileInfo.ftLastWriteTime); }); //throw FileError
#elif defined ZEN_LINUX || defined ZEN_MAC
    evalAttributeByHandle(fh, filePath, [&](const              struct ::stat& fileInfo) { modTime = fileInfo.st_mtime; }); //throw FileError
#endif
    return modTime;
}


inline
std::uint64_t getFileSize(FileHandle fh, const Zstring& filePath) //throw FileError
{
    std::uint64_t fileSize = 0;
#ifdef ZEN_WIN
    evalAttributeByHandle(fh, filePath, [&](const BY_HANDLE_FILE_INFORMATION& fileInfo) { fileSize = get64BitUInt(fileInfo.nFileSizeLow, fileInfo.nFileSizeHigh); }); //throw FileError
#elif defined ZEN_LINUX || defined ZEN_MAC
    evalAttributeByHandle(fh, filePath, [&](const              struct ::stat& fileInfo) { fileSize = makeUnsigned(fileInfo.st_size); }); //throw FileError
#endif
    return fileSize;
}


void preAllocateSpace(FileHandle fh, const std::uint64_t streamSize, const Zstring& displayPath) //throw FileError
{
#ifdef ZEN_WIN
    LARGE_INTEGER fileSize = {};
    fileSize.QuadPart = streamSize;
    if (!::SetFilePointerEx(fh,          //__in       HANDLE hFile,
                            fileSize,    //__in       LARGE_INTEGER liDistanceToMove,
                            nullptr,     //__out_opt  PLARGE_INTEGER lpNewFilePointer,
                            FILE_BEGIN)) //__in       DWORD dwMoveMethod
        throwFileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(displayPath)), L"SetFilePointerEx", getLastError());

    if (!::SetEndOfFile(fh))
        throwFileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(displayPath)), L"SetEndOfFile", getLastError());

    if (!::SetFilePointerEx(fh, {}, nullptr, FILE_BEGIN))
        throwFileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(displayPath)), L"SetFilePointerEx", getLastError());

#elif defined ZEN_LINUX
    if (::posix_fallocate(fh, 0, streamSize) != 0)
        throwFileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(displayPath)), L"posix_fallocate", getLastError());

#elif defined ZEN_MAC
    struct ::fstore store = {};
    store.fst_flags = F_ALLOCATECONTIG;
    store.fst_posmode = F_PEOFPOSMODE; //allocate from physical end of file
    //store.fst_offset     -> start of the region
    store.fst_length = streamSize;
    //store.fst_bytesalloc -> out: number of bytes allocated

    if (::fcntl(fh, F_PREALLOCATE, &store) == -1) //fcntl needs not return 0 on success!
    {
        store.fst_flags = F_ALLOCATEALL; //retry, allowing non-contiguous storage
        if (::fcntl(fh, F_PREALLOCATE, &store) == -1)
            throwFileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(displayPath)), L"fcntl, F_PREALLOCATE", getLastError());
    }
#endif
}


struct InputStreamNative : public AbstractBaseFolder::InputStream
{
    InputStreamNative(const Zstring& filePath) : fi(filePath) {} //throw FileError, ErrorFileLocked

    size_t read(void* buffer, size_t bytesToRead) override { return fi.read(buffer, bytesToRead); } //throw FileError; returns "bytesToRead", unless end of file!
    ABF::FileId   getFileId          () override { return ::getFileId          (fi.getHandle(), fi.getFilePath()); } //throw FileError
    std::int64_t  getModificationTime() override { return ::getModificationTime(fi.getHandle(), fi.getFilePath()); } //throw FileError
    std::uint64_t getFileSize        () override { return ::getFileSize        (fi.getHandle(), fi.getFilePath()); } //throw FileError
    size_t optimalBlockSize() const override { return fi.optimalBlockSize(); } //non-zero block size is ABF contract!

private:
    FileInput fi;
};


struct OutputStreamNative : public AbstractBaseFolder::OutputStream
{
    OutputStreamNative(const Zstring& filePath, const AbstractPathRef& abstractFilePath, const std::uint64_t* streamSize, const std::int64_t* modTime) :
        OutputStream(abstractFilePath, streamSize),
        fo(filePath, FileOutput::ACC_CREATE_NEW) //throw FileError, ErrorTargetExisting
    {
        if (modTime) modTime_ = *modTime;

        if (streamSize) //pre-allocate file space, because we can
            preAllocateSpace(fo.getHandle(), *streamSize, fo.getFilePath()); //throw FileError
    }

    size_t optimalBlockSize() const override { return fo.optimalBlockSize(); } //non-zero block size is ABF contract!

private:
    void writeImpl(const void* buffer, size_t bytesToWrite) override { fo.write(buffer, bytesToWrite); } //throw FileError

    ABF::FileId finalizeImpl(const std::function<void()>& onUpdateStatus) override //throw FileError
    {
        const ABF::FileId fileId = getFileId(fo.getHandle(), fo.getFilePath()); //throw FileError
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

class NativeBaseFolder : public AbstractBaseFolder
{
public:
    //itemPathImpl := native full item path as used by OS APIs

    NativeBaseFolder(const Zstring& baseDirPathIn) : baseDirPath(baseDirPathIn), baseDirPathPf(appendSeparator(baseDirPathIn)) {}

    //- THREAD-SAFETY: must be thread-safe like an int! => no dangling references to this instance!
    std::function<void()> /*throw FileError*/ getAsyncConnectFolder(bool allowUserInteraction) const override //noexcept
    {
        const Zstring dirPath = baseDirPath; //help lambda capture syntax...

        return [dirPath, allowUserInteraction]()
        {
#ifdef ZEN_WIN
            //login to network share, if necessary
            loginNetworkShare(dirPath, allowUserInteraction);
#endif
        };
    };
    //----------------------------------------------------------------------------------------------------------------

    bool emptyBaseFolderPath() const override { return baseDirPath.empty(); };

    std::uint64_t getFreeDiskSpace() const override { return zen::getFreeDiskSpace(baseDirPath); } //throw FileError, returns 0 if not available

    //----------------------------------------------------------------------------------------------------------------
    bool supportsRecycleBin(const std::function<void ()>& onUpdateGui) const override //throw FileError
    {
#ifdef ZEN_WIN
        return recycleBinExists(baseDirPath, onUpdateGui); //throw FileError

#elif defined ZEN_LINUX || defined ZEN_MAC
        return true; //truth be told: no idea!!!
#endif
    }

    std::unique_ptr<RecycleSession> createRecyclerSession() const override //throw FileError, return value must be bound!
    {
        assert(supportsRecycleBin(nullptr));
        return make_unique<RecycleSessionNative>(baseDirPathPf);
    };

    static Zstring getItemPathImplForRecycler(const AbstractPathRef& ap)
    {
        if (typeid(getAbf(ap)) != typeid(NativeBaseFolder))
            throw std::logic_error("Programming Error: Contract violation! " + std::string(__FILE__) + ":" + numberTo<std::string>(__LINE__));
        return getItemPathImpl(ap);
    }

private:
    bool isNativeFileSystem() const override { return true; };

    //- THREAD-SAFETY: expect accesses from multiple threads to the same instance => synchronize internally!!!
    Zstring getDisplayPath(const Zstring& itemPathImpl) const override { return itemPathImpl; }

    //- THREAD-SAFETY: expect accesses from multiple threads to the same instance => synchronize internally!!!
    Zstring appendRelPathToItemPathImpl(const Zstring& itemPathImpl, const Zstring& relPath) const override { return appendPaths(itemPathImpl, relPath); }

    //- THREAD-SAFETY: expect accesses from multiple threads to the same instance => synchronize internally!!!
    Zstring getBasePathImpl() const override { return baseDirPath; }

    Zstring getFileShortName(const Zstring& itemPathImpl) const override { return afterLast(itemPathImpl, FILE_NAME_SEPARATOR); /*returns the whole string if term not found*/ }

    bool lessItemPathSameAbfType(const Zstring& itemPathImplLhs, const AbstractPathRef& apRhs) const override { return LessFilePath()(itemPathImplLhs, getItemPathImpl(apRhs)); }

    bool havePathDependencySameAbfType(const AbstractBaseFolder& other) const override
    {
        const Zstring& lhs = baseDirPathPf;
        const Zstring& rhs = static_cast<const NativeBaseFolder&>(other).baseDirPathPf;

        return EqualFilePath()(Zstring(lhs.c_str(), std::min(lhs.length(), rhs.length())), //note: this is NOT an equivalence relation!
                               Zstring(rhs.c_str(), std::min(lhs.length(), rhs.length())));
    }

    //----------------------------------------------------------------------------------------------------------------
    bool fileExists     (const Zstring& itemPathImpl) const override { return zen::fileExists     (itemPathImpl); } //noexcept
    bool dirExists      (const Zstring& itemPathImpl) const override { return zen::dirExists      (itemPathImpl); } //noexcept
    bool symlinkExists  (const Zstring& itemPathImpl) const override { return zen::symlinkExists  (itemPathImpl); } //noexcept
    bool somethingExists(const Zstring& itemPathImpl) const override { return zen::somethingExists(itemPathImpl); } //noexcept
    //----------------------------------------------------------------------------------------------------------------
    //should provide for single ATOMIC folder creation! creates recursively!
    void createNewFolder(const Zstring& itemPathImpl) const override { return zen::makeNewDirectory(itemPathImpl); } //throw FileError, ErrorTargetExisting

    bool removeFile(const Zstring& itemPathImpl) const override { return zen::removeFile(itemPathImpl); } //throw FileError

    void removeFolder(const Zstring& itemPathImpl, //throw FileError
                      const std::function<void (const Zstring& displayPath)>& onBeforeFileDeletion,  //optional;
                      const std::function<void (const Zstring& displayPath)>& onBeforeDirDeletion) const override { zen::removeDirectory(itemPathImpl, onBeforeFileDeletion, onBeforeDirDeletion); }
    //----------------------------------------------------------------------------------------------------------------
    void setFileTime(const Zstring& itemPathImpl, std::int64_t modificationTime, SymlinkHandling procSl) const override
    { zen::setFileTime(itemPathImpl, modificationTime, procSl == SymlinkHandling::DIRECT ? ProcSymlink::DIRECT : ProcSymlink::FOLLOW); } //throw FileError

    AbstractPathRef getResolvedSymlinkPath(const Zstring& itemPathImpl) const override { return makeAbstractItem(*this, getResolvedFilePath(itemPathImpl)); }; //throw FileError

    Zstring getSymlinkContentBuffer(const Zstring& itemPathImpl) const override { return getSymlinkTargetRaw(itemPathImpl); } //throw FileError

    void recycleItemDirectly(const Zstring& itemPathImpl) const override { zen::recycleOrDelete(itemPathImpl); } //throw FileError

    //----------------------------------------------------------------------------------------------------------------
    //- THREAD-SAFETY: must be thread-safe like an int! => no dangling references to this instance!
    IconLoader getAsyncIconLoader(const Zstring& itemPathImpl) const override //noexcept!
    {
        //COM should already have been initialized by icon buffer
        IconLoader wl = {};
        wl.getFileIcon       = [itemPathImpl](int pixelSize) { return getFileIcon      (itemPathImpl, pixelSize); }; //noexcept!
        wl.getThumbnailImage = [itemPathImpl](int pixelSize) { return getThumbnailImage(itemPathImpl, pixelSize); }; //
        return wl;
    }
    //- THREAD-SAFETY: must be thread-safe like an int! => no dangling references to this instance!
    std::function<bool()> getAsyncCheckDirExists(const Zstring& itemPathImpl) const override { return [itemPathImpl] { return zen::dirExists(itemPathImpl); }; } //noexcept
    //----------------------------------------------------------------------------------------------------------------
    //return value always bound:
    std::unique_ptr<InputStream > getInputStream (const Zstring& itemPathImpl) const override { return make_unique<InputStreamNative >(itemPathImpl); } //throw FileError, ErrorFileLocked
    std::unique_ptr<OutputStream> getOutputStream(const Zstring& itemPathImpl,  //throw FileError, ErrorTargetExisting
                                                  const std::uint64_t* streamSize,                     //optional
                                                  const std::int64_t* modificationTime) const override //
    { return make_unique<OutputStreamNative>(itemPathImpl, makeAbstractItem(*this, itemPathImpl), streamSize, modificationTime); } //throw FileError, ErrorTargetExisting
    //----------------------------------------------------------------------------------------------------------------
    //- THREAD-SAFETY: expect accesses from multiple threads to the same instance => synchronize internally!!!
    void traverseFolder(const Zstring& itemPathImpl, TraverserCallback& sink) const override { DirTraverser::execute(itemPathImpl, sink); }
    //----------------------------------------------------------------------------------------------------------------

    //symlink handling: follow link!
    FileAttribAfterCopy copyFileForSameAbfType(const Zstring& itemPathImplSource, const AbstractPathRef& apTarget, bool copyFilePermissions, //throw FileError, ErrorTargetExisting, ErrorFileLocked
                                               const std::function<void(std::int64_t bytesDelta)>& onNotifyCopyStatus) const override //may be nullptr; throw X!
    {
        const InSyncAttributes attrNew = copyNewFile(itemPathImplSource, getItemPathImpl(apTarget), //throw FileError, ErrorTargetExisting, ErrorFileLocked
                                                     copyFilePermissions, onNotifyCopyStatus); //may be nullptr; throw X!
        FileAttribAfterCopy attrOut = {};
        attrOut.fileSize         = attrNew.fileSize;
        attrOut.modificationTime = attrNew.modificationTime;
        attrOut.sourceFileId     = convertToAbstractFileId(attrNew.sourceFileId);
        attrOut.targetFileId     = convertToAbstractFileId(attrNew.targetFileId);
        return attrOut;
    }

    //symlink handling: follow link!
    void copyNewFolderForSameAbfType(const Zstring& itemPathImplSource, const AbstractPathRef& apTarget, bool copyFilePermissions) const override
    { zen::copyNewDirectory(itemPathImplSource, getItemPathImpl(apTarget), copyFilePermissions); } //throw FileError

    void copySymlinkForSameAbfType(const Zstring& itemPathImplSource, const AbstractPathRef& apTarget, bool copyFilePermissions) const override
    { zen::copySymlink(itemPathImplSource, getItemPathImpl(apTarget), copyFilePermissions); } //throw FileError

    void renameItemForSameAbfType(const Zstring& itemPathImplSource, const AbstractPathRef& apTarget) const override
    { zen::renameFile(itemPathImplSource, getItemPathImpl(apTarget));} //throw FileError, ErrorTargetExisting, ErrorDifferentVolume

    bool supportsPermissions() const override {  return zen::supportsPermissions(baseDirPath); } //throw FileError

    const Zstring baseDirPath;
    const Zstring baseDirPathPf;
};

//===========================================================================================================================

#ifdef ZEN_WIN
//create + returns temporary directory postfixed with file name separator
//to support later cleanup if automatic deletion fails for whatever reason
Zstring RecycleSessionNative::getOrCreateRecyclerTempDirPf() //throw FileError
{
    assert(!baseDirPathPf_.empty());
    if (baseDirPathPf_.empty())
        return Zstring();

    if (recyclerTmpDir.empty())
        recyclerTmpDir = [&]
    {
        assert(endsWith(baseDirPathPf_, FILE_NAME_SEPARATOR));
        /*
        -> this naming convention is too cute and confusing for end users:

        //1. generate random directory name
        static std::mt19937 rng(std::time(nullptr)); //don't use std::default_random_engine which leaves the choice to the STL implementer!
        //- the alternative std::random_device may not always be available and can even throw an exception!
        //- seed with second precision is sufficient: collisions are handled below

        const Zstring chars(Zstr("abcdefghijklmnopqrstuvwxyz")
                            Zstr("1234567890"));
        std::uniform_int_distribution<size_t> distrib(0, chars.size() - 1); //takes closed range

        auto generatePath = [&]() -> Zstring //e.g. C:\Source\3vkf74fq.ffs_tmp
        {
            Zstring path = baseDirPf;
            for (int i = 0; i < 8; ++i)
                path += chars[distrib(rng)];
            return path + TEMP_FILE_ENDING;
        };
        */

        //ensure unique ownership:
        Zstring dirpath = baseDirPathPf_ + Zstr("RecycleBin") + ABF::TEMP_FILE_ENDING;
        for (int i = 0;; ++i)
            try
            {
                makeNewDirectory(dirpath); //throw FileError, ErrorTargetExisting
                return dirpath;
            }
            catch (const ErrorTargetExisting&)
            {
                if (i == 10) throw; //avoid endless recursion in pathological cases
                dirpath = baseDirPathPf_ + Zstr("RecycleBin") + Zchar('_') + numberTo<Zstring>(i) + ABF::TEMP_FILE_ENDING;
            }
    }();

    //assemble temporary recycle bin directory with random name and .ffs_tmp ending
    return appendSeparator(recyclerTmpDir);
}
#endif


bool RecycleSessionNative::recycleItem(const AbstractPathRef& ap, const Zstring& logicalRelPath) //throw FileError
{
    const Zstring itemPath = NativeBaseFolder::getItemPathImplForRecycler(ap);
    assert(!startsWith(logicalRelPath, FILE_NAME_SEPARATOR));

#ifdef ZEN_WIN
    const Zstring tmpPath = getOrCreateRecyclerTempDirPf() + logicalRelPath; //throw FileError
    bool deleted = false;

    auto moveToTempDir = [&]
    {
        try
        {
            //performance optimization: Instead of moving each object into recycle bin separately,
            //we rename them one by one into a temporary directory and batch-recycle this directory after sync
            renameFile(itemPath, tmpPath); //throw FileError, ErrorDifferentVolume
            this->toBeRecycled.push_back(tmpPath);
            deleted = true;
        }
        catch (ErrorDifferentVolume&) //MoveFileEx() returns ERROR_PATH_NOT_FOUND *before* considering ERROR_NOT_SAME_DEVICE! => we have to create tmpParentDir anyway to find out!
        {
            deleted = recycleOrDelete(itemPath); //throw FileError
        }
    };

    try
    {
        moveToTempDir(); //throw FileError, ErrorDifferentVolume
    }
    catch (FileError&)
    {
        if (somethingExists(itemPath))
        {
            const Zstring tmpParentDir = beforeLast(tmpPath, FILE_NAME_SEPARATOR); //what if C:\ ?
            if (!somethingExists(tmpParentDir))
            {
                makeNewDirectory(tmpParentDir); //throw FileError -> may legitimately fail on Linux if permissions are missing
                moveToTempDir(); //throw FileError -> this should work now!
            }
            else
                throw;
        }
    }
    return deleted;

#elif defined ZEN_LINUX || defined ZEN_MAC
    return recycleOrDelete(itemPath); //throw FileError
#endif
}


void RecycleSessionNative::tryCleanup(const std::function<void (const Zstring& displayPath)>& notifyDeletionStatus) //throw FileError
{
#ifdef ZEN_WIN
    if (!toBeRecycled.empty())
    {
        //move content of temporary directory to recycle bin in a single call
        recycleOrDelete(toBeRecycled, notifyDeletionStatus); //throw FileError
        toBeRecycled.clear();
    }

    //clean up temp directory itself (should contain remnant empty directories only)
    if (!recyclerTmpDir.empty())
    {
        removeDirectory(recyclerTmpDir); //throw FileError
        recyclerTmpDir.clear();
    }
#endif
}
}


//coordinate changes with getResolvedDirectoryPath()!
bool zen::acceptsFolderPathPhraseNative(const Zstring& folderPathPhrase) //noexcept
{
    Zstring path = folderPathPhrase;
    path = expandMacros(path); //expand before trimming!
    trim(path);

#ifdef ZEN_WIN
    path = removeLongPathPrefix(path);
#endif

    if (startsWith(path, Zstr("["))) //drive letter by volume name syntax
        return true;

    //don't accept relative paths!!! indistinguishable from Explorer MTP paths!

#ifdef ZEN_WIN
    if (path.size() >= 3 &&
        std::iswalpha(path[0]) &&
        path[1] == L':' &&
        path[2] == L'\\')
        return true; //path starting with drive letter

    return path.size() >= 3 &&
           path[0] == L'\\' &&
           path[1] == L'\\' &&
           std::iswalpha(path[2]); //UNC path

#elif defined ZEN_LINUX || defined ZEN_MAC
    return startsWith(path, Zstr("/"));
#endif
}


Zstring zen::getResolvedDisplayPathNative(const Zstring& folderPathPhrase) //noexcept
{
    return getResolvedDirectoryPath(folderPathPhrase); //noexcept
    warn_static("get volume by name for idle HDD! => call async getFormattedDirectoryPath, but currently not thread-safe")
}


std::unique_ptr<AbstractBaseFolder> zen::createBaseFolderNative(const Zstring& folderPathPhrase) //noexcept
{
    return make_unique<NativeBaseFolder>(getResolvedDirectoryPath(folderPathPhrase));
}
