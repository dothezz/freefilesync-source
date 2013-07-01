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
    auto it = shortnameVersion.begin();
    auto last = shortnameVersion.end();

    auto nextDigit = [&]() -> bool
    {
        if (it == last || !isDigit(*it))
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
        if (it == last || *it != c)
            return false;
        ++it;
        return true;
    };
    auto nextStringI = [&](const Zstring& str) -> bool //windows: ignore case!
    {
        if (last - it < static_cast<ptrdiff_t>(str.size()) || !EqualFilename()(str, Zstring(&*it, str.size())))
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
           nextStringI(getExtension(shortname)) &&
           it == last;
}


namespace
{
/*
- handle not existing source
- create target super directories if missing
*/
template <class Function>
void moveItemToVersioning(const Zstring& fullName, //throw FileError
                          const Zstring& relativeName,
                          const Zstring& versioningDirectory,
                          const Zstring& timestamp,
                          VersioningStyle versioningStyle,
                          Function moveObj) //move source -> target; may throw FileError
{
    assert(!startsWith(relativeName, FILE_NAME_SEPARATOR));
    assert(!endsWith  (relativeName, FILE_NAME_SEPARATOR));

    Zstring targetName;
    switch (versioningStyle)
    {
        case VER_STYLE_REPLACE:
            targetName = appendSeparator(versioningDirectory) + relativeName;
            break;

        case VER_STYLE_ADD_TIMESTAMP:
            //assemble time-stamped version name
            targetName = appendSeparator(versioningDirectory) + relativeName + Zstr(' ') + timestamp + getExtension(relativeName);
            assert(impl::isMatchingVersion(afterLast(relativeName, FILE_NAME_SEPARATOR), afterLast(targetName, FILE_NAME_SEPARATOR))); //paranoid? no!
            break;
    }

    try
    {
        moveObj(fullName, targetName); //throw FileError
    }
    catch (FileError&) //expected to fail if target directory is not yet existing!
    {
        if (!somethingExists(fullName)) //no source at all is not an error (however a directory as source when a file is expected, *is* an error!)
            return; //object *not* processed

        //create intermediate directories if missing
        const Zstring targetDir = beforeLast(targetName, FILE_NAME_SEPARATOR);
        if (!dirExists(targetDir)) //->(minor) file system race condition!
        {
            makeDirectory(targetDir); //throw FileError
            moveObj(fullName, targetName); //throw FileError -> this should work now!
        }
        else
            throw;
    }
}


//move source to target across volumes
//no need to check if: - super-directories of target exist - source exists
//if target already exists, it is overwritten, even if it is a different type, e.g. a directory!
template <class Function>
void moveObject(const Zstring& sourceFile, //throw FileError
                const Zstring& targetFile,
                Function copyDelete) //throw FileError; fallback if move failed
{
    assert(!dirExists(sourceFile) || symlinkExists(sourceFile)); //we process files and symlinks only

    auto removeTarget = [&]()
    {
        //remove target object
        if (fileExists(targetFile)) //file or symlink
            removeFile(targetFile); //throw FileError
        else if (dirExists(targetFile)) //directory or symlink
            removeDirectory(targetFile); //throw FileError
        //we do not expect targetFile to be a directory in general => no callback required
        else assert(false);
    };

    //first try to move directly without copying
    try
    {
        renameFile(sourceFile, targetFile); //throw FileError, ErrorDifferentVolume, ErrorTargetExisting
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
            renameFile(sourceFile, targetFile); //throw FileError, ErrorDifferentVolume, ErrorTargetExisting
        }
        catch (const ErrorDifferentVolume&)
        {
            copyDelete(); //throw FileError
        }
    }
}


void moveFile(const Zstring& sourceFile, const Zstring& targetFile, CallbackCopyFile& callback) //throw FileError
{
    moveObject(sourceFile, //throw FileError
               targetFile,
               [&]
    {
        //create target
        if (symlinkExists(sourceFile))
            copySymlink(sourceFile, targetFile, false); //throw FileError; don't copy filesystem permissions
        else
            copyFile(sourceFile, targetFile, false, true, &callback); //throw FileError - permissions "false", transactional copy "true"

        //delete source
        removeFile(sourceFile); //throw FileError; newly copied file is NOT deleted if exception is thrown here!
    });
}


void moveDirSymlink(const Zstring& sourceLink, const Zstring& targetLink) //throw FileError
{
    moveObject(sourceLink, //throw FileError
               targetLink,
               [&]
    {
        //create target
        copySymlink(sourceLink, targetLink, false); //throw FileError; don't copy filesystem permissions

        //delete source
        removeDirectory(sourceLink); //throw FileError; newly copied link is NOT deleted if exception is thrown here!
    });
}


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
        switch (getSymlinkType(fullName))
        {
            case SYMLINK_TYPE_DIR:
                dirs_.push_back(shortName);
                break;

            case SYMLINK_TYPE_FILE:
            case SYMLINK_TYPE_UNKNOWN:
                files_.push_back(shortName);
                break;
        }
        return LINK_SKIP;
    }

    virtual TraverseCallback* onDir(const Zchar* shortName, const Zstring& fullName)
    {
        dirs_.push_back(shortName);
        return nullptr; //DON'T traverse into subdirs; moveDirectory works recursively!
    }

    virtual HandleError reportDirError (const std::wstring& msg)                         { throw FileError(msg); }
    virtual HandleError reportItemError(const std::wstring& msg, const Zchar* shortName) { throw FileError(msg); }

    std::vector<Zstring>& files_;
    std::vector<Zstring>& dirs_;
};
}


bool FileVersioner::revisionFile(const Zstring& fullName, const Zstring& relativeName, CallbackMoveFile& callback) //throw FileError
{
    struct CallbackMoveFileImpl : public CallbackMoveDir
    {
        CallbackMoveFileImpl(CallbackMoveFile& callback) : callback_(callback) {}
    private:
        virtual void onBeforeFileMove(const Zstring& fileFrom, const Zstring& fileTo) {}
        virtual void onBeforeDirMove (const Zstring& dirFrom,  const Zstring& dirTo ) {}
        virtual void updateStatus(Int64 bytesDelta) { callback_.updateStatus(bytesDelta); }
        CallbackMoveFile& callback_;
    } cb(callback);

    return revisionFileImpl(fullName, relativeName, cb); //throw FileError
}


bool FileVersioner::revisionFileImpl(const Zstring& fullName, const Zstring& relativeName, CallbackMoveDir& callback) //throw FileError
{
    bool moveSuccessful = false;

    moveItemToVersioning(fullName, //throw FileError
                         relativeName,
                         versioningDirectory_,
                         timeStamp_,
                         versioningStyle_,
                         [&](const Zstring& source, const Zstring& target)
    {
        callback.onBeforeFileMove(source, target); //if we're called by revisionDirImpl() we know that "source" exists!
        //when called by revisionFile(), "source" might not exist, however onBeforeFileMove() is not propagated in this case!

        struct CopyCallbackImpl : public CallbackCopyFile
        {
            CopyCallbackImpl(CallbackMoveDir& callback) : callback_(callback) {}
        private:
            virtual void deleteTargetFile(const Zstring& targetFile) { assert(!somethingExists(targetFile)); }
            virtual void updateCopyStatus(Int64 bytesDelta) { callback_.updateStatus(bytesDelta); }
            CallbackMoveDir& callback_;
        } copyCallback(callback);

        moveFile(source, target, copyCallback); //throw FileError
        moveSuccessful = true;
    });
    return moveSuccessful;
}


void FileVersioner::revisionDir(const Zstring& fullName, const Zstring& relativeName, CallbackMoveDir& callback) //throw FileError
{
    //no error situation if directory is not existing! manual deletion relies on it!
    if (!somethingExists(fullName))
        return; //neither directory nor any other object (e.g. broken symlink) with that name existing
    revisionDirImpl(fullName, relativeName, callback); //throw FileError
}


void FileVersioner::revisionDirImpl(const Zstring& fullName, const Zstring& relativeName, CallbackMoveDir& callback) //throw FileError
{
    assert(somethingExists(fullName)); //[!]

    //create target
    if (symlinkExists(fullName)) //on Linux there is just one type of symlink, and since we do revision file symlinks, we should revision dir symlinks as well!
    {
        moveItemToVersioning(fullName, //throw FileError
                             relativeName,
                             versioningDirectory_,
                             timeStamp_,
                             versioningStyle_,
                             [&](const Zstring& source, const Zstring& target)
        {
            callback.onBeforeDirMove(source, target);
            moveDirSymlink(source, target); //throw FileError
        });
    }
    else
    {
        assert(!startsWith(relativeName, FILE_NAME_SEPARATOR));
        assert(endsWith(fullName, relativeName)); //usually, yes, but we might relax this in the future
        const Zstring targetDir = appendSeparator(versioningDirectory_) + relativeName;

        //makeDirectory(targetDir); //FileError -> create only when needed in moveFileToVersioning(); avoids empty directories

        //traverse source directory one level
        std::vector<Zstring> fileList; //list of *short* names
        std::vector<Zstring> dirList;  //
        {
            TraverseFilesOneLevel tol(fileList, dirList); //throw FileError
            traverseFolder(fullName, tol);               //
        }

        const Zstring fullNamePf = appendSeparator(fullName);
        const Zstring relnamePf   = appendSeparator(relativeName);

        //move files
        std::for_each(fileList.begin(), fileList.end(),
                      [&](const Zstring& shortname)
        {
            revisionFileImpl(fullNamePf + shortname, //throw FileError
                             relnamePf + shortname,
                             callback);
        });

        //move items in subdirectories
        std::for_each(dirList.begin(), dirList.end(),
                      [&](const Zstring& shortname)
        {
            revisionDirImpl(fullNamePf + shortname, //throw FileError
                            relnamePf + shortname,
                            callback);
        });

        //delete source
        callback.onBeforeDirMove(fullName, targetDir);
        removeDirectory(fullName); //throw FileError
    }
}


/*
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
    virtual HandleError reportDirError (const std::wstring& msg)                         { throw FileError(msg); }
    virtual HandleError reportItemError(const std::wstring& msg, const Zchar* shortName) { throw FileError(msg); }

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
        auto it = dirBuffer.find(dirname);
        if (it != dirBuffer.end())
            return it->second;

        std::vector<Zstring> fileShortNames;
        TraverseVersionsOneLevel tol(fileShortNames, updateUI); //throw FileError
        traverseFolder(dirname, tol);

        auto& newEntry = dirBuffer[dirname]; //transactional behavior!!!
        newEntry.swap(fileShortNames);       //-> until C++11 emplace is available

        return newEntry;
    };

    std::for_each(fileRelNames.begin(), fileRelNames.end(),
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
