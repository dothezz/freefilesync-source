#include "versioning.h"
#include <map>
#include <zen/file_handling.h>
#include <zen/file_traverser.h>
#include <zen/string_tools.h>

using namespace zen;


namespace
{
Zstring getExtension(const Zstring& relativeName) //including "." if extension is existing, returns empty string otherwise
{
    auto iterSep = find_last(relativeName.begin(), relativeName.end(), FILE_NAME_SEPARATOR);
    auto iterName = iterSep != relativeName.end() ? iterSep + 1 : relativeName.begin(); //find beginning of short name
    auto iterDot = find_last(iterName, relativeName.end(), Zstr('.')); //equal to relativeName.end() if file has no extension!!
    return Zstring(&*iterDot, relativeName.end() - iterDot);
};
}

bool impl::isMatchingVersion(const Zstring& shortname, const Zstring& shortnameVersion) //e.g. ("Sample.txt", "Sample.txt 2012-05-15 131513.txt")
{
    auto iter = shortnameVersion.begin();
    auto last = shortnameVersion.end();

    auto nextDigit = [&]() -> bool
    {
        if (iter == last || !isDigit(*iter))
            return false;
        ++iter;
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
        if (iter == last || *iter != c)
            return false;
        ++iter;
        return true;
    };
    auto nextStringI = [&](const Zstring& str) -> bool //windows: ignore case!
    {
        if (last - iter < static_cast<ptrdiff_t>(str.size()) || !EqualFilename()(str, Zstring(&*iter, str.size())))
            return false;
        iter += str.size();
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
           nextStringI(getExtension(shortname)) &&
           iter == last;
}


namespace
{
template <class Function>
void moveItemToVersioning(const Zstring& sourceObj, //throw FileError
                          const Zstring& relativeName,
                          const Zstring& versioningDirectory,
                          const Zstring& timestamp,
                          Function moveObj) //move source -> target; allowed to throw FileError
{
    assert(!startsWith(relativeName, FILE_NAME_SEPARATOR));
    assert(endsWith(sourceObj, relativeName)); //usually, yes, but we might relax this in the future

    //assemble time-stamped version name
    const Zstring targetObj = appendSeparator(versioningDirectory) + relativeName + Zstr(' ') + timestamp + getExtension(relativeName);
    assert(impl::isMatchingVersion(afterLast(relativeName, FILE_NAME_SEPARATOR), afterLast(targetObj, FILE_NAME_SEPARATOR))); //paranoid? no!

    try
    {
        moveObj(sourceObj, targetObj); //throw FileError
    }
    catch (FileError&) //expected to fail if target directory is not yet existing!
    {
        if (!somethingExists(sourceObj)) //no source at all is not an error (however a directory as source when a file is expected, *is* an error!)
            return; //object *not* processed

        //create intermediate directories if missing
        const Zstring targetDir = beforeLast(targetObj, FILE_NAME_SEPARATOR);
        if (!dirExists(targetDir)) //->(minor) file system race condition!
        {
            makeDirectory(targetDir); //throw FileError
            moveObj(sourceObj, targetObj); //throw FileError -> this should work now!
        }
        else
            throw;
    }
}


//move source to target across volumes; prerequisite: all super-directories of target exist
//if target already contains some files/dirs they are seen as remnants of a previous incomplete move
void moveFile(const Zstring& sourceFile, const Zstring& targetFile, CallbackCopyFile& callback) //throw FileError
{
    //first try to move directly without copying
    try
    {
        renameFile(sourceFile, targetFile); //throw FileError, ErrorDifferentVolume, ErrorTargetExisting
        return; //great, we get away cheaply!
    }
    //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the file)
    catch (const ErrorDifferentVolume&) {}
    catch (const ErrorTargetExisting&) {}

    //create target
    if (!fileExists(targetFile)) //check even if ErrorTargetExisting: me may have clashed with another item type of the same name!!!
    {
        //file is on a different volume: let's copy it
        if (symlinkExists(sourceFile))
            copySymlink(sourceFile, targetFile, false); //throw FileError; don't copy filesystem permissions
        else
            copyFile(sourceFile, targetFile, false, true, &callback); //throw FileError - permissions "false", transactional copy "true"
    }

    //delete source
    removeFile(sourceFile); //throw FileError; newly copied file is NOT deleted if exception is thrown here!
}


void moveDirSymlink(const Zstring& sourceLink, const Zstring& targetLink) //throw FileError
{
    //first try to move directly without copying
    try
    {
        renameFile(sourceLink, targetLink); //throw FileError, ErrorDifferentVolume, ErrorTargetExisting
        return; //great, we get away cheaply!
    }
    //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the file)
    catch (const ErrorDifferentVolume&) {}
    catch (const ErrorTargetExisting&) {}

    //create target
    if (!symlinkExists(targetLink)) //check even if ErrorTargetExisting: me may have clashed with another item type of the same name!!!
    {
        //link is on a different volume: let's copy it
        copySymlink(sourceLink, targetLink, false); //throw FileError; don't copy filesystem permissions
    }

    //delete source
    removeDirectory(sourceLink); //throw FileError; newly copied link is NOT deleted if exception is thrown here!
}


struct CopyCallbackImpl : public CallbackCopyFile
{
    CopyCallbackImpl(CallbackMoveFile& callback) : callback_(callback) {}

private:
    virtual void deleteTargetFile(const Zstring& targetFile) { assert(!somethingExists(targetFile)); }
    virtual void updateCopyStatus(Int64 bytesDelta) { callback_.updateStatus(bytesDelta); }

    CallbackMoveFile& callback_;
};


class TraverseFilesOneLevel : public TraverseCallback
{
public:
    TraverseFilesOneLevel(std::vector<Zstring>& files, std::vector<Zstring>& dirs) : files_(files), dirs_(dirs) {}

private:
    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details)
    {
        files_.push_back(shortName);
    }

    virtual HandleLink onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details)
    {
        if (details.dirLink)
            dirs_.push_back(shortName);
        else
            files_.push_back(shortName);
        return LINK_SKIP;
    }

    virtual std::shared_ptr<TraverseCallback> onDir(const Zchar* shortName, const Zstring& fullName)
    {
        dirs_.push_back(shortName);
        return nullptr; //DON'T traverse into subdirs; moveDirectory works recursively!
    }

    virtual HandleError onError(const std::wstring& msg) { throw FileError(msg); }

    std::vector<Zstring>& files_;
    std::vector<Zstring>& dirs_;
};


struct RemoveCallbackImpl : public CallbackRemoveDir
{
    RemoveCallbackImpl(CallbackMoveFile& callback) : callback_(callback) {}

private:
    virtual void notifyFileDeletion(const Zstring& filename) { callback_.updateStatus(0); }
    virtual void notifyDirDeletion (const Zstring& dirname ) { callback_.updateStatus(0); }

    CallbackMoveFile& callback_;
};
}


void FileVersioner::revisionFile(const Zstring& sourceFile, const Zstring& relativeName, CallbackMoveFile& callback) //throw FileError
{
    moveItemToVersioning(sourceFile, //throw FileError
                         relativeName,
                         versioningDirectory_,
                         timeStamp_,
                         [&](const Zstring& source, const Zstring& target)
    {
        callback.onBeforeFileMove(source, target);

        CopyCallbackImpl copyCallback(callback);
        moveFile(source, target, copyCallback); //throw FileError
        callback.objectProcessed();
    });

    fileRelnames.push_back(relativeName);
}


void FileVersioner::revisionDir(const Zstring& sourceDir, const Zstring& relativeName, CallbackMoveFile& callback) //throw FileError
{
    //note: we cannot support "throw exception if target already exists": If we did, we would have to do a full cleanup
    //removing all newly created directories in case of an exception so that subsequent tries would not fail with "target already existing".
    //However an exception may also happen during final deletion of source folder, in which case cleanup effectively leads to data loss!

    //create target
    if (symlinkExists(sourceDir)) //on Linux there is just one type of symlinks, and since we do revision file symlinks, we should revision dir symlinks as well!
    {
        moveItemToVersioning(sourceDir, //throw FileError
                             relativeName,
                             versioningDirectory_,
                             timeStamp_,
                             [&](const Zstring& source, const Zstring& target)
        {
            callback.onBeforeDirMove(source, target);
            moveDirSymlink(source, target); //throw FileError
            callback.objectProcessed();
        });

        fileRelnames.push_back(relativeName);
    }
    else
    {
        assert(!startsWith(relativeName, FILE_NAME_SEPARATOR));
        assert(endsWith(sourceDir, relativeName));
        const Zstring targetDir = appendSeparator(versioningDirectory_) + relativeName;

        callback.onBeforeDirMove(sourceDir, targetDir);

        //makeDirectory(targetDir); //FileError -> create only when needed in moveFileToVersioning(); avoids empty directories

        //traverse source directory one level
        std::vector<Zstring> fileList; //list of *short* names
        std::vector<Zstring> dirList;  //
        try
        {
            TraverseFilesOneLevel tol(fileList, dirList); //throw FileError
            traverseFolder(sourceDir, tol);                   //
        }
        catch (FileError&)
        {
            if (!somethingExists(sourceDir)) //no source at all is not an error (however a file as source when a directory is expected, *is* an error!)
                return; //object *not* processed
            throw;
        }

        const Zstring sourceDirPf = appendSeparator(sourceDir);
        const Zstring relnamePf   = appendSeparator(relativeName);

        //move files
        std::for_each(fileList.begin(), fileList.end(),
                      [&](const Zstring& shortname)
        {
            revisionFile(sourceDirPf + shortname, //throw FileError
                         relnamePf + shortname,
                         callback);
        });

        //move directories
        std::for_each(dirList.begin(), dirList.end(),
                      [&](const Zstring& shortname)
        {
            revisionDir(sourceDirPf + shortname, //throw FileError
                        relnamePf + shortname,
                        callback);
        });

        //delete source
        RemoveCallbackImpl removeCallback(callback);
        removeDirectory(sourceDir, &removeCallback); //throw FileError

        callback.objectProcessed();
    }
}


namespace
{
class TraverseVersionsOneLevel : public TraverseCallback
{
public:
    TraverseVersionsOneLevel(std::vector<Zstring>& files, std::function<void()> updateUI) : files_(files), updateUI_(updateUI) {}

private:
    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details) { files_.push_back(shortName); updateUI_(); }
    virtual HandleLink onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details) { files_.push_back(shortName); updateUI_(); return LINK_SKIP; }
    virtual std::shared_ptr<TraverseCallback> onDir(const Zchar* shortName, const Zstring& fullName) { updateUI_(); return nullptr; } //DON'T traverse into subdirs
    virtual HandleError onError(const std::wstring& msg) { throw FileError(msg); }

    std::vector<Zstring>& files_;
    std::function<void()> updateUI_;
};
}


void FileVersioner::limitVersions(std::function<void()> updateUI) //throw FileError
{
    if (versionCountLimit_ < 0) //no limit!
        return;

    //buffer map "directory |-> list of immediate child file and symlink short names"
    std::map<Zstring, std::vector<Zstring>, LessFilename> dirBuffer;

    auto getVersionsBuffered = [&](const Zstring& dirname) -> const std::vector<Zstring>&
    {
        auto iter = dirBuffer.find(dirname);
        if (iter != dirBuffer.end())
            return iter->second;

        std::vector<Zstring> fileShortNames;
        TraverseVersionsOneLevel tol(fileShortNames, updateUI); //throw FileError
        traverseFolder(dirname, tol);

        auto& newEntry = dirBuffer[dirname]; //transactional behavior!!!
        newEntry.swap(fileShortNames);       //-> until C++11 emplace is available

        return newEntry;
    };

    std::for_each(fileRelnames.begin(), fileRelnames.end(),
                  [&](const Zstring& relativeName) //e.g. "subdir\Sample.txt"
    {
        const Zstring fullname = appendSeparator(versioningDirectory_) + relativeName; //e.g. "D:\Revisions\subdir\Sample.txt"
        const Zstring parentDir = beforeLast(fullname, FILE_NAME_SEPARATOR);    //e.g. "D:\Revisions\subdir"
        const Zstring shortname = afterLast(relativeName, FILE_NAME_SEPARATOR); //e.g. "Sample.txt"; returns the whole string if seperator not found

        const std::vector<Zstring>& allVersions = getVersionsBuffered(parentDir);

        //filter out only those versions that match the given relative name
        std::vector<Zstring> matches; //e.g. "Sample.txt 2012-05-15 131513.txt"

        std::copy_if(allVersions.begin(), allVersions.end(), std::back_inserter(matches),
        [&](const Zstring& shortnameVer) { return impl::isMatchingVersion(shortname, shortnameVer); });

        //take advantage of version naming convention to find oldest versions
        if (matches.size() <= static_cast<size_t>(versionCountLimit_))
            return;
        std::nth_element(matches.begin(), matches.end() - versionCountLimit_, matches.end(), LessFilename()); //windows: ignore case!

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
#ifdef FFS_WIN //if it's a directory symlink:
                if (symlinkExists(fullnameVer) && dirExists(fullnameVer))
                    removeDirectory(fullnameVer); //throw FileError
                else
#endif
                    throw;
            }
        });
    });
}
