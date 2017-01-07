// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0           *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#ifndef ABSTRACT_H_873450978453042524534234
#define ABSTRACT_H_873450978453042524534234

#include <functional>
#include <zen/file_error.h>
#include <zen/zstring.h>
#include <zen/optional.h>
#include "../lib/icon_holder.h"


namespace zen
{
struct AbstractFileSystem;

//==============================================================================================================
class AbstractPath //THREAD-SAFETY: like an int!
{
public:
    template <class T1, class T2>
    AbstractPath(T1&& afsIn, T2&& itemPathImplIn) : afs(std::forward<T1>(afsIn)), itemPathImpl(std::forward<T2>(itemPathImplIn)) {}

private:
    friend struct AbstractFileSystem;

    std::shared_ptr<const AbstractFileSystem> afs; //always bound; "const AbstractFileSystem" => all accesses expected to be thread-safe!!!
    Zstring itemPathImpl; //valid only in context of a specific AbstractFileSystem instance*
};

//==============================================================================================================
struct AbstractFileSystem //THREAD-SAFETY: "const" member functions must model thread-safe access!
{
    struct LessAbstractPath;
    static bool equalAbstractPath(const AbstractPath& lhs, const AbstractPath& rhs);

    static Zstring getInitPathPhrase(const AbstractPath& ap) { return ap.afs->getInitPathPhrase(ap.itemPathImpl); }

    static std::wstring getDisplayPath(const AbstractPath& ap) { return ap.afs->getDisplayPath(ap.itemPathImpl); }

    static bool isNullPath(const AbstractPath& ap) { return ap.afs->isNullPath(ap.itemPathImpl); }

    static AbstractPath appendRelPath(const AbstractPath& ap, const Zstring& relPath);

    struct PathComplement
    {
        Zstring relPathL; //- relative path is filled only if the corresponding abstract path is a sub folder of the other (=> both relative paths are empty if abstract paths match)
        Zstring relPathR; //- without FILE_NAME_SEPARATOR prefix
    };
    static Opt<PathComplement> getPathComplement(const AbstractPath& lhs, const AbstractPath& rhs);

    static Zstring getItemName(const AbstractPath& ap) { assert(getParentFolderPath(ap)); return ap.afs->getItemName(ap.itemPathImpl); }

    static bool havePathDependency(const AbstractPath& lhs, const AbstractPath& rhs)
    {
        warn_static("remove after migration")
        return typeid(*lhs.afs) != typeid(*rhs.afs) ? false : lhs.afs->havePathDependencySameAfsType(lhs.itemPathImpl, rhs);
    }

    static Opt<Zstring> getNativeItemPath(const AbstractPath& ap) { return ap.afs->isNativeFileSystem() ? Opt<Zstring>(ap.itemPathImpl) : NoValue(); }

    static Opt<AbstractPath> getParentFolderPath(const AbstractPath& ap)
    {
        if (const Opt<Zstring> parentPathImpl = ap.afs->getParentFolderPathImpl(ap.itemPathImpl))
            return AbstractPath(ap.afs, *parentPathImpl);
        return NoValue();
    }

    //----------------------------------------------------------------------------------------------------------------
    enum class ItemType
    {
        FILE,
        FOLDER,
        SYMLINK,
    };
    struct PathDetails
    {
        ItemType existingType;
        AbstractPath existingPath;    //itemPath =: existingPath + relPath
        std::vector<Zstring> relPath; //
    };
    //(hopefully) fast: does not distinguish between error/not existing
    static ItemType getItemType(const AbstractPath& ap) { return ap.afs->getItemType(ap.itemPathImpl); } //throw FileError
    //execute potentially SLOW folder traversal but distinguish error/not existing
    static Opt<ItemType> getItemTypeIfExists(const AbstractPath& ap); //throw FileError
    static PathDetails getPathDetails(const AbstractPath& ap); //throw FileError
    //----------------------------------------------------------------------------------------------------------------

    //- error if already existing
    //- does NOT create parent directories recursively if not existing
    static void createFolderPlain(const AbstractPath& ap) { ap.afs->createFolderPlain(ap.itemPathImpl); } //throw FileError

    //- no error if already existing
    //- creates parent directories recursively if not existing
    static void createFolderIfMissingRecursion(const AbstractPath& ap); //throw FileError

    static bool removeFileIfExists   (const AbstractPath& ap); //throw FileError; return "false" if file is not existing
    static bool removeSymlinkIfExists(const AbstractPath& ap); //
    static void removeFolderIfExistsRecursion(const AbstractPath& ap, //throw FileError
                                              const std::function<void (const std::wstring& displayPath)>& onBeforeFileDeletion,    //optional
                                              const std::function<void (const std::wstring& displayPath)>& onBeforeFolderDeletion); //one call for each *existing* object!

    static void removeFilePlain   (const AbstractPath& ap) { ap.afs->removeFilePlain   (ap.itemPathImpl); } //throw FileError
    static void removeSymlinkPlain(const AbstractPath& ap) { ap.afs->removeSymlinkPlain(ap.itemPathImpl); } //throw FileError
    static void removeFolderPlain (const AbstractPath& ap) { ap.afs->removeFolderPlain (ap.itemPathImpl); } //throw FileError
    //----------------------------------------------------------------------------------------------------------------
    static void setModTime       (const AbstractPath& ap, std::int64_t modificationTime) { ap.afs->setModTime       (ap.itemPathImpl, modificationTime); } //throw FileError, follows symlinks
    static void setModTimeSymlink(const AbstractPath& ap, std::int64_t modificationTime) { ap.afs->setModTimeSymlink(ap.itemPathImpl, modificationTime); } //throw FileError

    static AbstractPath getResolvedSymlinkPath(const AbstractPath& ap) { return AbstractPath(ap.afs, ap.afs->getResolvedSymlinkPath(ap.itemPathImpl)); } //throw FileError
    static std::string getSymlinkBinaryContent(const AbstractPath& ap) { return ap.afs->getSymlinkBinaryContent(ap.itemPathImpl); } //throw FileError
    //----------------------------------------------------------------------------------------------------------------
    //noexcept; optional return value:
    static ImageHolder getFileIcon      (const AbstractPath& ap, int pixelSize) { return ap.afs->getFileIcon      (ap.itemPathImpl, pixelSize); }
    static ImageHolder getThumbnailImage(const AbstractPath& ap, int pixelSize) { return ap.afs->getThumbnailImage(ap.itemPathImpl, pixelSize); }

    static void connectNetworkFolder(const AbstractPath& ap, bool allowUserInteraction) { return ap.afs->connectNetworkFolder(ap.itemPathImpl, allowUserInteraction); } //throw FileError
    //----------------------------------------------------------------------------------------------------------------

    using FileId = Zbase<char>;

    //----------------------------------------------------------------------------------------------------------------
    struct InputStream
    {
        virtual ~InputStream() {}
        virtual size_t getBlockSize() const = 0; //non-zero block size is AFS contract! it's implementers job to always give a reasonable buffer size!
        virtual size_t tryRead(void* buffer, size_t bytesToRead) = 0; //throw FileError; may return short, only 0 means EOF! => CONTRACT: bytesToRead > 0
        virtual FileId        getFileId          () = 0; //throw FileError
        virtual std::int64_t  getModificationTime() = 0; //throw FileError
        virtual std::uint64_t getFileSize        () = 0; //throw FileError
    };

    struct OutputStreamImpl
    {
        virtual ~OutputStreamImpl() {}
        virtual size_t getBlockSize() const = 0; //non-zero block size is AFS contract!
        virtual size_t tryWrite(const void* buffer, size_t bytesToWrite) = 0; //throw FileError; may return short! CONTRACT: bytesToWrite > 0
        virtual FileId finalize(const std::function<void()>& onUpdateStatus) = 0; //throw FileError
    };

    //TRANSACTIONAL output stream! => call finalize when done!
    struct OutputStream
    {
        OutputStream(std::unique_ptr<OutputStreamImpl>&& outStream, const AbstractPath& filePath, const std::uint64_t* streamSize);
        ~OutputStream();
        size_t getBlockSize() const { return outStream_->getBlockSize(); } //non-zero block size is AFS contract!
        size_t tryWrite(const void* data, size_t len); //throw FileError; may return short! CONTRACT: bytesToWrite > 0
        FileId finalize(const std::function<void()>& onUpdateStatus); //throw FileError

    private:
        std::unique_ptr<OutputStreamImpl> outStream_; //bound!
        const AbstractPath filePath_;
        bool finalizeSucceeded_ = false;
        Opt<std::uint64_t> bytesExpected_;
        std::uint64_t bytesWrittenTotal_ = 0;
    };

    //return value always bound:
    static std::unique_ptr<InputStream > getInputStream (const AbstractPath& ap) { return ap.afs->getInputStream (ap.itemPathImpl); } //throw FileError, ErrorFileLocked
    static std::unique_ptr<OutputStream> getOutputStream(const AbstractPath& ap,
                                                         const std::uint64_t* streamSize,      //optional
                                                         const std::int64_t* modificationTime) //
    { return std::make_unique<OutputStream>(ap.afs->getOutputStream(ap.itemPathImpl, streamSize, modificationTime), ap, streamSize); } //throw FileError, ErrorTargetExisting
    //----------------------------------------------------------------------------------------------------------------

    struct TraverserCallback
    {
        virtual ~TraverserCallback() {}

        struct SymlinkInfo
        {
            Zstring itemName;
            std::int64_t lastWriteTime; //number of seconds since Jan. 1st 1970 UTC
        };

        struct FileInfo
        {
            Zstring itemName;
            std::uint64_t fileSize;      //unit: bytes!
            std::int64_t  lastWriteTime; //number of seconds since Jan. 1st 1970 UTC
            FileId id; //optional: empty if not supported!
            const SymlinkInfo* symlinkInfo; //only filled if file is a followed symlink
        };

        struct FolderInfo
        {
            Zstring itemName;
            const SymlinkInfo* symlinkInfo; //only filled if folder is a followed symlink
        };

        enum HandleLink
        {
            LINK_FOLLOW, //dereferences link, then calls "onFolder()" or "onFile()"
            LINK_SKIP
        };

        enum HandleError
        {
            ON_ERROR_RETRY,
            ON_ERROR_CONTINUE
        };

        virtual void                               onFile   (const FileInfo&    fi) = 0; //
        virtual HandleLink                         onSymlink(const SymlinkInfo& si) = 0; //throw X
        virtual std::unique_ptr<TraverserCallback> onFolder (const FolderInfo&  fi) = 0; //
        //nullptr: ignore directory, non-nullptr: traverse into, using the (new) callback

        virtual HandleError reportDirError (const std::wstring& msg, size_t retryNumber) = 0; //failed directory traversal -> consider directory data at current level as incomplete!
        virtual HandleError reportItemError(const std::wstring& msg, size_t retryNumber, const Zstring& itemName) = 0; //failed to get data for single file/dir/symlink only!
    };

    //- client needs to handle duplicate file reports! (FilePlusTraverser fallback, retrying to read directory contents, ...)
    static void traverseFolder(const AbstractPath& ap, TraverserCallback& sink) { ap.afs->traverseFolder(ap.itemPathImpl, sink); }
    //----------------------------------------------------------------------------------------------------------------

    static bool supportPermissionCopy(const AbstractPath& apSource, const AbstractPath& apTarget); //throw FileError

    struct FileAttribAfterCopy
    {
        std::uint64_t fileSize = 0;
        std::int64_t modificationTime = 0; //time_t UTC compatible
        FileId sourceFileId;
        FileId targetFileId;
    };
    //return current attributes at the time of copy
    //symlink handling: dereference source

    //Note: it MAY happen that copyFileTransactional() leaves temp files behind, e.g. temporary network drop.
    // => clean them up at an appropriate time (automatically set sync directions to delete them). They have the following ending:
    static const Zchar* TEMP_FILE_ENDING; //don't use Zstring as global constant: avoid static initialization order problem in global namespace!

    static FileAttribAfterCopy copyFileTransactional(const AbstractPath& apSource, const AbstractPath& apTarget, //throw FileError, ErrorFileLocked
                                                     bool copyFilePermissions,
                                                     bool transactionalCopy,
                                                     //if target is existing user needs to implement deletion: copyFile() NEVER overwrites target if already existing!
                                                     //if transactionalCopy == true, full read access on source had been proven at this point, so it's safe to delete it.
                                                     const std::function<void()>& onDeleteTargetFile,
                                                     //accummulated delta != file size! consider ADS, sparse, compressed files
                                                     const std::function<void(std::int64_t bytesDelta)>& notifyProgress);

    static void copyNewFolder(const AbstractPath& apSource, const AbstractPath& apTarget, bool copyFilePermissions); //throw FileError
    static void copySymlink  (const AbstractPath& apSource, const AbstractPath& apTarget, bool copyFilePermissions); //throw FileError
    static void renameItem   (const AbstractPath& apSource, const AbstractPath& apTarget); //throw FileError, ErrorTargetExisting, ErrorDifferentVolume

    //----------------------------------------------------------------------------------------------------------------

    static std::uint64_t getFreeDiskSpace(const AbstractPath& ap) { return ap.afs->getFreeDiskSpace(ap.itemPathImpl); } //throw FileError, returns 0 if not available

    static bool supportsRecycleBin(const AbstractPath& ap, const std::function<void ()>& onUpdateGui) { return ap.afs->supportsRecycleBin(ap.itemPathImpl, onUpdateGui); } //throw FileError

    struct RecycleSession
    {
        virtual ~RecycleSession() {}
        virtual bool recycleItem(const AbstractPath& itemPath, const Zstring& logicalRelPath) = 0; //throw FileError; return true if item existed
        virtual void tryCleanup(const std::function<void (const std::wstring& displayPath)>& notifyDeletionStatus /*optional; currentItem may be empty*/) = 0; //throw FileError
    };

    //precondition: supportsRecycleBin() must return true!
    static std::unique_ptr<RecycleSession> createRecyclerSession(const AbstractPath& ap) { return ap.afs->createRecyclerSession(ap.itemPathImpl); } //throw FileError, return value must be bound!

    static void recycleItemIfExists(const AbstractPath& ap) { ap.afs->recycleItemIfExists(ap.itemPathImpl); } //throw FileError

    //================================================================================================================

    //no need to protect access:
    static Zstring appendPaths(const Zstring& basePath, const Zstring& relPath, Zchar pathSep);

    virtual ~AbstractFileSystem() {}

protected: //grant derived classes access to AbstractPath:
    static const AbstractFileSystem& getAfs         (const AbstractPath& ap) { return *ap.afs; }
    static Zstring                   getItemPathImpl(const AbstractPath& ap) { return ap.itemPathImpl; }

    struct PathDetailsImpl
    {
        ItemType existingType;
        Zstring existingPathImpl;    //itemPathImpl =: existingPathImpl + relPath
        std::vector<Zstring> relPath; //
    };
    PathDetailsImpl getPathDetailsViaFolderTraversal(const Zstring& itemPathImpl) const; //throw FileError

    FileAttribAfterCopy copyFileAsStream(const Zstring& itemPathImplSource, const AbstractPath& apTarget, //throw FileError, ErrorTargetExisting, ErrorFileLocked
                                         const std::function<void(std::int64_t bytesDelta)>& notifyProgress) const; //may be nullptr; throw X!

private:
    virtual bool isNativeFileSystem() const { return false; }

    virtual Zstring getInitPathPhrase(const Zstring& itemPathImpl) const = 0;

    virtual std::wstring getDisplayPath(const Zstring& itemPathImpl) const = 0;

    virtual bool isNullPath(const Zstring& itemPathImpl) const = 0;

    virtual Zstring appendRelPathToItemPathImpl(const Zstring& itemPathImpl, const Zstring& relPath) const = 0;

    virtual Opt<PathComplement> getPathComplementSameAfsType(const Zstring& itemPathImplLhs, const AbstractPath& apRhs) const = 0;

    virtual bool lessItemPathSameAfsType(const Zstring& itemPathImplLhs, const AbstractPath& apRhs) const = 0;

    virtual Opt<Zstring> getParentFolderPathImpl(const Zstring& itemPathImpl) const = 0;

    virtual Zstring getItemName(const Zstring& itemPathImpl) const = 0;

    virtual bool havePathDependencySameAfsType(const Zstring& itemPathImplLhs, const AbstractPath& apRhs) const = 0;

    //----------------------------------------------------------------------------------------------------------------
    virtual ItemType getItemType(const Zstring& itemPathImpl) const = 0; //throw FileError
    virtual PathDetailsImpl getPathDetails(const Zstring& itemPathImpl) const = 0; //throw FileError
    //----------------------------------------------------------------------------------------------------------------

    virtual void createFolderPlain(const Zstring& itemPathImpl) const = 0; //throw FileError

    //non-recursive folder deletion:
    virtual void removeFilePlain   (const Zstring& itemPathImpl) const = 0; //throw FileError
    virtual void removeSymlinkPlain(const Zstring& itemPathImpl) const = 0; //throw FileError
    virtual void removeFolderPlain (const Zstring& itemPathImpl) const = 0; //throw FileError
    //----------------------------------------------------------------------------------------------------------------
    virtual void setModTime       (const Zstring& itemPathImpl, std::int64_t modificationTime) const = 0; //throw FileError, follows symlinks
    virtual void setModTimeSymlink(const Zstring& itemPathImpl, std::int64_t modificationTime) const = 0; //throw FileError

    virtual Zstring getResolvedSymlinkPath(const Zstring& itemPathImpl) const = 0; //throw FileError

    virtual std::string getSymlinkBinaryContent(const Zstring& itemPathImpl) const = 0; //throw FileError

    //----------------------------------------------------------------------------------------------------------------
    virtual std::unique_ptr<InputStream     > getInputStream (const Zstring& itemPathImpl) const = 0; //throw FileError, ErrorFileLocked
    virtual std::unique_ptr<OutputStreamImpl> getOutputStream(const Zstring& itemPathImpl,  //throw FileError, ErrorTargetExisting
                                                              const std::uint64_t* streamSize,                 //optional
                                                              const std::int64_t* modificationTime) const = 0; //
    //----------------------------------------------------------------------------------------------------------------
    virtual void traverseFolder(const Zstring& itemPathImpl, TraverserCallback& sink) const = 0; //noexcept
    //----------------------------------------------------------------------------------------------------------------

    //symlink handling: follow link!
    virtual FileAttribAfterCopy copyFileForSameAfsType(const Zstring& itemPathImplSource, const AbstractPath& apTarget, bool copyFilePermissions, //throw FileError, ErrorTargetExisting, ErrorFileLocked
                                                       //accummulated delta != file size! consider ADS, sparse, compressed files
                                                       const std::function<void(std::int64_t bytesDelta)>& notifyProgress) const = 0; //may be nullptr; throw X!

    //symlink handling: follow link!
    virtual void copyNewFolderForSameAfsType(const Zstring& itemPathImplSource, const AbstractPath& apTarget, bool copyFilePermissions) const = 0; //throw FileError
    virtual void copySymlinkForSameAfsType  (const Zstring& itemPathImplSource, const AbstractPath& apTarget, bool copyFilePermissions) const = 0; //throw FileError
    virtual void renameItemForSameAfsType   (const Zstring& itemPathImplSource, const AbstractPath& apTarget) const = 0; //throw FileError, ErrorTargetExisting, ErrorDifferentVolume
    virtual bool supportsPermissions(const Zstring& itemPathImpl) const = 0; //throw FileError

    //----------------------------------------------------------------------------------------------------------------
    virtual ImageHolder getFileIcon      (const Zstring& itemPathImpl, int pixelSize) const = 0; //noexcept; optional return value
    virtual ImageHolder getThumbnailImage(const Zstring& itemPathImpl, int pixelSize) const = 0; //

    virtual void connectNetworkFolder(const Zstring& itemPathImpl, bool allowUserInteraction) const = 0; //throw FileError
    //----------------------------------------------------------------------------------------------------------------

    virtual std::uint64_t getFreeDiskSpace(const Zstring& itemPathImpl) const = 0; //throw FileError, returns 0 if not available
    virtual bool supportsRecycleBin(const Zstring& itemPathImpl, const std::function<void ()>& onUpdateGui) const  = 0; //throw FileError
    virtual std::unique_ptr<RecycleSession> createRecyclerSession(const Zstring& itemPathImpl) const = 0; //throw FileError, return value must be bound!
    virtual void recycleItemIfExists(const Zstring& itemPathImpl) const = 0; //throw FileError
};


//implement "retry" in a generic way:
template <class Command> inline //function object expecting to throw FileError if operation fails
bool tryReportingDirError(Command cmd, AbstractFileSystem::TraverserCallback& callback) //return "true" on success, "false" if error was ignored
{
    for (size_t retryNumber = 0;; ++retryNumber)
        try
        {
            cmd(); //throw FileError
            return true;
        }
        catch (const FileError& e)
        {
            switch (callback.reportDirError(e.toString(), retryNumber)) //throw X
            {
                case AbstractFileSystem::TraverserCallback::ON_ERROR_RETRY:
                    break;
                case AbstractFileSystem::TraverserCallback::ON_ERROR_CONTINUE:
                    return false;
            }
        }
}


template <class Command> inline //function object expecting to throw FileError if operation fails
bool tryReportingItemError(Command cmd, AbstractFileSystem::TraverserCallback& callback, const Zstring& itemName) //return "true" on success, "false" if error was ignored
{
    for (size_t retryNumber = 0;; ++retryNumber)
        try
        {
            cmd(); //throw FileError
            return true;
        }
        catch (const FileError& e)
        {
            switch (callback.reportItemError(e.toString(), retryNumber, itemName)) //throw X
            {
                case AbstractFileSystem::TraverserCallback::ON_ERROR_RETRY:
                    break;
                case AbstractFileSystem::TraverserCallback::ON_ERROR_CONTINUE:
                    return false;
            }
        }
}








//------------------------------------ implementation -----------------------------------------
struct AbstractFileSystem::LessAbstractPath
{
    bool operator()(const AbstractPath& lhs, const AbstractPath& rhs) const
    {
        //note: in worst case, order is guaranteed to be stable only during each program run
        return typeid(*lhs.afs) != typeid(*rhs.afs) ? typeid(*lhs.afs).before(typeid(*rhs.afs)) : lhs.afs->lessItemPathSameAfsType(lhs.itemPathImpl, rhs);
        //caveat: typeid returns static type for pointers, dynamic type for references!!!
    }
};


inline
bool AbstractFileSystem::equalAbstractPath(const AbstractPath& lhs, const AbstractPath& rhs)
{
    return !LessAbstractPath()(lhs, rhs) && !LessAbstractPath()(rhs, lhs);
}

namespace impl
{
inline
bool isValidRelPath(const Zstring& relPath)
{
    const bool check1 = !contains(relPath, '\\');
    const bool check2 = relPath.empty() || (!startsWith(relPath, FILE_NAME_SEPARATOR) && !endsWith(relPath, FILE_NAME_SEPARATOR));
    return check1 && check2;
}
}

inline
AbstractPath AbstractFileSystem::appendRelPath(const AbstractPath& ap, const Zstring& relPath)
{
    assert(impl::isValidRelPath(relPath));
    return AbstractPath(ap.afs, ap.afs->appendRelPathToItemPathImpl(ap.itemPathImpl, relPath));
}


inline
auto AbstractFileSystem::getPathComplement(const AbstractPath& lhs, const AbstractPath& rhs) -> Opt<PathComplement>
{
    if (typeid(*lhs.afs) != typeid(*rhs.afs))
        return NoValue();
    Opt<PathComplement> result = lhs.afs->getPathComplementSameAfsType(lhs.itemPathImpl, rhs);
    assert(!result || (impl::isValidRelPath(result->relPathL)&& impl::isValidRelPath(result->relPathR)));
    assert(!result || result->relPathL.empty() || result->relPathR.empty());
    return result;
}


inline
Zstring AbstractFileSystem::appendPaths(const Zstring& basePath, const Zstring& relPath, Zchar pathSep)
{
    if (relPath.empty())
        return basePath;
    if (basePath.empty())
        return relPath;

    if (startsWith(relPath, pathSep))
    {
        assert(false);
        if (relPath.size() == 1)
            return basePath;

        if (endsWith(basePath, pathSep))
            return basePath + (relPath.c_str() + 1);
    }
    else if (!endsWith(basePath, pathSep))
        return basePath + pathSep + relPath;

    return basePath + relPath;
}

//--------------------------------------------------------------------------

inline
AbstractFileSystem::OutputStream::OutputStream(std::unique_ptr<OutputStreamImpl>&& outStream, const AbstractPath& filePath, const std::uint64_t* streamSize) :
    outStream_(std::move(outStream)), filePath_(filePath)
{
    if (streamSize)
        bytesExpected_ = *streamSize;
}


inline
AbstractFileSystem::OutputStream::~OutputStream()
{
    //we delete the file on errors: => fail if already existing BEFORE creating OutputStream instance!!

    outStream_.reset(); //close file handle *before* remove!

    if (!finalizeSucceeded_) //transactional output stream! => clean up!
        try { AbstractFileSystem::removeFilePlain(filePath_); /*throw FileError*/ }
        catch (FileError& e) { (void)e; assert(false); }
}


inline
size_t AbstractFileSystem::OutputStream::tryWrite(const void* data, size_t len) //throw FileError, CONTRACT: bytesToWrite > 0
{
    if (len == 0)
        throw std::logic_error("Contract violation! " + std::string(__FILE__) + ":" + numberTo<std::string>(__LINE__));

    const size_t bytesWritten = outStream_->tryWrite(data, len); //throw FileError; CONTRACT: bytesToWrite > 0
    if (bytesWritten > len)
        throw std::logic_error("Contract violation! " + std::string(__FILE__) + ":" + numberTo<std::string>(__LINE__));

    bytesWrittenTotal_ += bytesWritten;
    return bytesWritten;
}


inline
AbstractFileSystem::FileId AbstractFileSystem::OutputStream::finalize(const std::function<void()>& onUpdateStatus) //throw FileError
{
    //important check: catches corrupt sftp download with libssh2!
    if (bytesExpected_ && *bytesExpected_ != bytesWrittenTotal_)
        throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtPath(getDisplayPath(filePath_))),
                        replaceCpy(replaceCpy(_("Unexpected size of data stream.\nExpected: %x bytes\nActual: %y bytes"),
                                              L"%x", numberTo<std::wstring>(*bytesExpected_)),
                                   L"%y", numberTo<std::wstring>(bytesWrittenTotal_)));

    FileId fileId = outStream_->finalize(onUpdateStatus); //throw FileError
    finalizeSucceeded_ = true;
    return fileId;
}

//--------------------------------------------------------------------------

inline
void AbstractFileSystem::copyNewFolder(const AbstractPath& apSource, const AbstractPath& apTarget, bool copyFilePermissions) //throw FileError
{
    if (typeid(*apSource.afs) == typeid(*apTarget.afs))
        return apSource.afs->copyNewFolderForSameAfsType(apSource.itemPathImpl, apTarget, copyFilePermissions); //throw FileError

    //fall back:
    if (copyFilePermissions)
        throw FileError(replaceCpy(_("Cannot write permissions of %x."), L"%x", fmtPath(getDisplayPath(apTarget))),
                        _("Operation not supported for different base folder types."));

    createFolderPlain(apTarget); //throw FileError
}


inline
void AbstractFileSystem::copySymlink(const AbstractPath& apSource, const AbstractPath& apTarget, bool copyFilePermissions) //throw FileError
{
    if (typeid(*apSource.afs) == typeid(*apTarget.afs))
        return apSource.afs->copySymlinkForSameAfsType(apSource.itemPathImpl, apTarget, copyFilePermissions); //throw FileError

    throw FileError(replaceCpy(replaceCpy(_("Cannot copy symbolic link %x to %y."),
                                          L"%x", L"\n" + fmtPath(getDisplayPath(apSource))),
                               L"%y", L"\n" + fmtPath(getDisplayPath(apTarget))), _("Operation not supported for different base folder types."));
}


inline
void AbstractFileSystem::renameItem(const AbstractPath& apSource, const AbstractPath& apTarget) //throw FileError, ErrorTargetExisting, ErrorDifferentVolume
{
    if (typeid(*apSource.afs) == typeid(*apTarget.afs))
        return apSource.afs->renameItemForSameAfsType(apSource.itemPathImpl, apTarget); //throw FileError, ErrorTargetExisting, ErrorDifferentVolume

    throw ErrorDifferentVolume(replaceCpy(replaceCpy(_("Cannot move file %x to %y."),
                                                     L"%x", L"\n" + fmtPath(getDisplayPath(apSource))),
                                          L"%y", L"\n" + fmtPath(getDisplayPath(apTarget))), _("Operation not supported for different base folder types."));
}


inline
bool AbstractFileSystem::supportPermissionCopy(const AbstractPath& apSource, const AbstractPath& apTarget) //throw FileError
{
    if (typeid(*apSource.afs) != typeid(*apTarget.afs))
        return false;

    return apSource.afs->supportsPermissions(apSource.itemPathImpl) && //throw FileError
           apTarget.afs->supportsPermissions(apTarget.itemPathImpl);
}
}

#endif //ABSTRACT_H_873450978453042524534234
