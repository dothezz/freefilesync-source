#include "versioning.h"
#include <cstddef> //required by GCC 4.8.1 to find ptrdiff_t

using namespace zen;
using ABF = AbstractBaseFolder;

namespace
{
inline
Zstring getDotExtension(const Zstring& relativePath) //including "." if extension is existing, returns empty string otherwise
{
    const Zstring& extension = getFileExtension(relativePath);
    return extension.empty() ? extension : Zstr('.') + extension;
};
}

bool impl::isMatchingVersion(const Zstring& shortname, const Zstring& shortnameVersioned) //e.g. ("Sample.txt", "Sample.txt 2012-05-15 131513.txt")
{
    auto it = shortnameVersioned.begin();
    auto itLast = shortnameVersioned.end();

    auto nextDigit = [&]() -> bool
    {
        if (it == itLast || !isDigit(*it))
            return false;
        ++it;
        return true;
    };
    auto nextDigits = [&](size_t count) -> bool
    {
        while (count-- > 0)
            if (!nextDigit())
                return false;
        return true;
    };
    auto nextChar = [&](Zchar c) -> bool
    {
        if (it == itLast || *it != c)
            return false;
        ++it;
        return true;
    };
    auto nextStringI = [&](const Zstring& str) -> bool //windows: ignore case!
    {
        if (itLast - it < static_cast<ptrdiff_t>(str.size()) || !EqualFilePath()(str, Zstring(&*it, str.size())))
            return false;
        it += str.size();
        return true;
    };

    return nextStringI(shortname) && //versioned file starts with original name
           nextChar(Zstr(' ')) && //validate timestamp: e.g. "2012-05-15 131513"; Regex: \d{4}-\d{2}-\d{2} \d{6}
           nextDigits(4)       && //YYYY
           nextChar(Zstr('-')) && //
           nextDigits(2)       && //MM
           nextChar(Zstr('-')) && //
           nextDigits(2)       && //DD
           nextChar(Zstr(' ')) && //
           nextDigits(6)       && //HHMMSS
           nextStringI(getDotExtension(shortname)) &&
           it == itLast;
}


namespace
{
/*
- handle not existing source
- create target super directories if missing
*/
template <class Function>
void moveItemToVersioning(const AbstractPathRef& itemPath, //throw FileError
                          const Zstring& relativePath,
                          AbstractBaseFolder& versioningFolder,
                          const Zstring& timestamp,
                          VersioningStyle versioningStyle,
                          Function moveItem) //move source -> target; may throw FileError
{
    assert(!startsWith(relativePath, FILE_NAME_SEPARATOR));
    assert(!endsWith  (relativePath, FILE_NAME_SEPARATOR));
    assert(!relativePath.empty());

    Zstring versionedRelPath;
    switch (versioningStyle)
    {
        default:
            assert(false);
        case VER_STYLE_REPLACE:
            versionedRelPath = relativePath;

        case VER_STYLE_ADD_TIMESTAMP: //assemble time-stamped version name
            versionedRelPath = relativePath + Zstr(' ') + timestamp + getDotExtension(relativePath);
            assert(impl::isMatchingVersion(afterLast(relativePath, FILE_NAME_SEPARATOR), afterLast(versionedRelPath, FILE_NAME_SEPARATOR))); //paranoid? no!
    }

    const AbstractPathRef versionedItemPath = versioningFolder.getAbstractPath(versionedRelPath);

    try
    {
        moveItem(itemPath, versionedItemPath); //throw FileError
    }
    catch (FileError&) //expected to fail if target directory is not yet existing!
    {
        if (!ABF::somethingExists(itemPath)) //no source at all is not an error (however a directory as source when a file is expected, *is* an error!)
            return; //object *not* processed

        //create intermediate directories if missing
        const AbstractPathRef versionedParentPath = versioningFolder.getAbstractPath(beforeLast(versionedRelPath, FILE_NAME_SEPARATOR)); //returns empty string if term not found
        if (!ABF::somethingExists(versionedParentPath)) //->(minor) file system race condition!
        {
            ABF::createNewFolder(versionedParentPath); //throw FileError, (ErrorTargetExisting)
            //retry: this should work now!
            moveItem(itemPath, versionedItemPath); //throw FileError
        }
        else
            throw;
    }
}


//move source to target across volumes
//no need to check if: - super-directories of target exist - source exists: done by moveItemToVersioning()
//if target already exists, it is overwritten, even if it is a different type, e.g. a directory!
template <class Function>
void moveItem(const AbstractPathRef& sourcePath, //throw FileError
              const AbstractPathRef& targetPath,
              Function copyDelete) //throw FileError; fallback if move failed
{
    assert(ABF::fileExists(sourcePath) || ABF::symlinkExists(sourcePath) || !ABF::somethingExists(sourcePath)); //we process files and symlinks only

    auto removeTarget = [&]
    {
        //remove target object
        if (ABF::dirExists(targetPath)) //directory or dir-symlink
        {
            assert(false); //we do not expect targetPath to be a directory in general (but possible!) => no removeFolder() callback required!
            ABF::removeFolder(targetPath); //throw FileError
        }
        else //file or (broken) file-symlink
            ABF::removeFile(targetPath); //throw FileError
    };

    //first try to move directly without copying
    try
    {
        ABF::renameItem(sourcePath, targetPath); //throw FileError, ErrorTargetExisting, ErrorDifferentVolume
        return; //great, we get away cheaply!
    }
    //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the file)
    catch (const ErrorDifferentVolume&)
    {
        removeTarget(); //throw FileError
        copyDelete();   //
    }
    catch (const ErrorTargetExisting&)
    {
        removeTarget(); //throw FileError
        try
        {
            ABF::renameItem(sourcePath, targetPath); //throw FileError, (ErrorTargetExisting), ErrorDifferentVolume
        }
        catch (const ErrorDifferentVolume&)
        {
            copyDelete(); //throw FileError
        }
    }
}


void moveFile(const AbstractPathRef& sourcePath, //throw FileError
              const AbstractPathRef& targetPath,
              const std::function<void(std::int64_t bytesDelta)>& onNotifyCopyStatus) //may be nullptr
{
    auto copyDelete = [&]
    {
        assert(!ABF::somethingExists(targetPath));

        //create target
        if (ABF::symlinkExists(sourcePath))
            ABF::copySymlink(sourcePath, targetPath, false /*copy filesystem permissions*/); //throw FileError
        else
            ABF::copyFileTransactional(sourcePath, targetPath, //throw FileError, (ErrorFileLocked)
            false /*copyFilePermissions*/, true /*transactionalCopy*/, nullptr, onNotifyCopyStatus);

        //delete source
        ABF::removeFile(sourcePath); //throw FileError; newly copied file is NOT deleted if exception is thrown here!
    };

    moveItem(sourcePath, targetPath, copyDelete); //throw FileError
}


void moveDirSymlink(const AbstractPathRef& sourcePath, const AbstractPathRef& targetPath) //throw FileError
{
    auto copyDelete = [&] //throw FileError
    {
        ABF::copySymlink(sourcePath, targetPath, false /*copy filesystem permissions*/); //throw FileError
        ABF::removeFolder(sourcePath); //throw FileError; newly copied link is NOT deleted if exception is thrown here!
    };

    moveItem(sourcePath, targetPath, copyDelete);
}


struct FlatTraverserCallback: public ABF::TraverserCallback
{
    FlatTraverserCallback(const AbstractPathRef& folderPath) : folderPath_(folderPath) {}

    void               onFile   (const FileInfo&    fi) override { fileNames_  .push_back(fi.shortName); }
    TraverserCallback* onDir    (const DirInfo&     di) override { folderNames_.push_back(di.shortName); return nullptr; }
    HandleLink         onSymlink(const SymlinkInfo& si) override
    {
        if (ABF::dirExists(ABF::appendRelPath(folderPath_, si.shortName))) //dir symlink
            folderNames_.push_back(si.shortName);
        else //file symlink, broken symlink
            fileNames_.push_back(si.shortName);
        return TraverserCallback::LINK_SKIP;
    }
    HandleError reportDirError (const std::wstring& msg, size_t retryNumber)                         override { throw FileError(msg); }
    HandleError reportItemError(const std::wstring& msg, size_t retryNumber, const Zchar* shortName) override { throw FileError(msg); }

    const std::vector<Zstring>& refFileNames  () const { return fileNames_; }
    const std::vector<Zstring>& refFolderNames() const { return folderNames_; }

private:
    const AbstractPathRef& folderPath_;
    std::vector<Zstring> fileNames_;
    std::vector<Zstring> folderNames_;
};
}


bool FileVersioner::revisionFile(const AbstractPathRef& filePath, const Zstring& relativePath, const std::function<void(std::int64_t bytesDelta)>& onNotifyCopyStatus) //throw FileError
{
    return revisionFileImpl(filePath, relativePath, nullptr, onNotifyCopyStatus); //throw FileError
}


bool FileVersioner::revisionFileImpl(const AbstractPathRef& filePath, //throw FileError
                                     const Zstring& relativePath,
                                     const std::function<void(const Zstring& displayPathFrom, const Zstring& displayPathTo)>& onBeforeFileMove,
                                     const std::function<void(std::int64_t bytesDelta)>& onNotifyCopyStatus)
{
    bool moveSuccessful = false;

    moveItemToVersioning(filePath, //throw FileError
                         relativePath,
                         *versioningFolder_,
                         timeStamp_,
                         versioningStyle_,
                         [&](const AbstractPathRef& sourcePath, const AbstractPathRef& targetPath)
    {
        if (onBeforeFileMove)
            onBeforeFileMove(ABF::getDisplayPath(sourcePath), ABF::getDisplayPath(targetPath)); //if we're called by revisionFolderImpl() we know that "source" exists!
        //when called by revisionFile(), "source" might not exist, but onBeforeFileMove() == nullptr in this case anyway

        moveFile(sourcePath, targetPath, onNotifyCopyStatus); //throw FileError
        moveSuccessful = true;
    });
    return moveSuccessful;
}


void FileVersioner::revisionFolder(const AbstractPathRef& folderPath, const Zstring& relativePath, //throw FileError
                                   const std::function<void(const Zstring& displayPathFrom, const Zstring& displayPathTo)>& onBeforeFileMove,
                                   const std::function<void(const Zstring& displayPathFrom, const Zstring& displayPathTo)>& onBeforeDirMove,
                                   const std::function<void(std::int64_t bytesDelta)>& onNotifyCopyStatus)
{
    //no error situation if directory is not existing! manual deletion relies on it!
    if (!ABF::somethingExists(folderPath))
        return; //neither directory nor any other object (e.g. broken symlink) with that name existing
    revisionFolderImpl(folderPath, relativePath, onBeforeFileMove, onBeforeDirMove, onNotifyCopyStatus); //throw FileError
}


void FileVersioner::revisionFolderImpl(const AbstractPathRef& folderPath, const Zstring& relativePath, //throw FileError
                                       const std::function<void(const Zstring& displayPathFrom, const Zstring& displayPathTo)>& onBeforeFileMove,
                                       const std::function<void(const Zstring& displayPathFrom, const Zstring& displayPathTo)>& onBeforeDirMove,
                                       const std::function<void(std::int64_t bytesDelta)>& onNotifyCopyStatus)
{
    assert(ABF::somethingExists(folderPath)); //[!], see call from revisionFolder()

    //create target
    if (ABF::symlinkExists(folderPath)) //on Linux there is just one type of symlink, and since we do revision file symlinks, we should revision dir symlinks as well!
    {
        moveItemToVersioning(folderPath, //throw FileError
                             relativePath,
                             *versioningFolder_,
                             timeStamp_,
                             versioningStyle_,
                             [&](const AbstractPathRef& sourcePath, const AbstractPathRef& targetPath)
        {
            if (onBeforeDirMove)
                onBeforeDirMove(ABF::getDisplayPath(sourcePath), ABF::getDisplayPath(targetPath));
            moveDirSymlink(sourcePath, targetPath); //throw FileError
        });
    }
    else
    {
        //create target directories only when needed in moveFileToVersioning(): avoid empty directories!

        FlatTraverserCallback ft(folderPath); //traverse source directory one level deep
        ABF::traverseFolder(folderPath, ft);

        const Zstring relPathPf = appendSeparator(relativePath);

        //move files
        for (const Zstring& fileName : ft.refFileNames())
            revisionFileImpl(ABF::appendRelPath(folderPath, fileName), //throw FileError
                             relPathPf + fileName,
                             onBeforeFileMove, onNotifyCopyStatus);

        //move folders
        for (const Zstring& folderName : ft.refFolderNames())
            revisionFolderImpl(ABF::appendRelPath(folderPath, folderName), //throw FileError
                               relPathPf + folderName,
                               onBeforeFileMove, onBeforeDirMove, onNotifyCopyStatus);
        //delete source
        if (onBeforeDirMove)
            onBeforeDirMove(ABF::getDisplayPath(folderPath), ABF::getDisplayPath(versioningFolder_->getAbstractPath(relativePath)));
        ABF::removeFolder(folderPath); //throw FileError
    }
}


/*
void FileVersioner::limitVersions(std::function<void()> updateUI) //throw FileError
{
    if (versionCountLimit_ < 0) //no limit!
        return;

    //buffer map "directory |-> list of immediate child file and symlink short names"
    std::map<Zstring, std::vector<Zstring>, LessFilePath> dirBuffer;

    auto getVersionsBuffered = [&](const Zstring& dirpath) -> const std::vector<Zstring>&
    {
        auto it = dirBuffer.find(dirpath);
        if (it != dirBuffer.end())
            return it->second;

        std::vector<Zstring> fileShortNames;
        TraverseVersionsOneLevel tol(fileShortNames, updateUI); //throw FileError
        traverseFolder(dirpath, tol);

        auto& newEntry = dirBuffer[dirpath]; //transactional behavior!!!
        newEntry.swap(fileShortNames);       //-> until C++11 emplace is available

        return newEntry;
    };

    std::for_each(fileRelNames.begin(), fileRelNames.end(),
                  [&](const Zstring& relativePath) //e.g. "subdir\Sample.txt"
    {
        const Zstring filepath = appendSeparator(versioningDirectory_) + relativePath; //e.g. "D:\Revisions\subdir\Sample.txt"
        const Zstring parentDir = beforeLast(filepath, FILE_NAME_SEPARATOR);    //e.g. "D:\Revisions\subdir"
        const Zstring shortname = afterLast(relativePath, FILE_NAME_SEPARATOR); //e.g. "Sample.txt"; returns the whole string if seperator not found

        const std::vector<Zstring>& allVersions = getVersionsBuffered(parentDir);

        //filter out only those versions that match the given relative name
        std::vector<Zstring> matches; //e.g. "Sample.txt 2012-05-15 131513.txt"

        std::copy_if(allVersions.begin(), allVersions.end(), std::back_inserter(matches),
        [&](const Zstring& shortnameVer) { return impl::isMatchingVersion(shortname, shortnameVer); });

        //take advantage of version naming convention to find oldest versions
        if (matches.size() <= static_cast<size_t>(versionCountLimit_))
            return;
        std::nth_element(matches.begin(), matches.end() - versionCountLimit_, matches.end(), LessFilePath()); //windows: ignore case!

        //delete obsolete versions
        std::for_each(matches.begin(), matches.end() - versionCountLimit_,
                      [&](const Zstring& shortnameVer)
        {
            updateUI();
            const Zstring fullnameVer = parentDir + FILE_NAME_SEPARATOR + shortnameVer;
            try
            {
                removeFile(fullnameVer); //throw FileError
            }
            catch (FileError&)
            {
#ifdef ZEN_WIN //if it's a directory symlink:
                if (symlinkExists(fullnameVer) && dirExists(fullnameVer))
                    removeDirectory(fullnameVer); //throw FileError
                else
#endif
                    throw;
            }
        });
    });
}
*/
