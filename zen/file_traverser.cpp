// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "file_traverser.h"
#include "last_error.h"
#include "assert_static.h"
#include "symlink_target.h"

#ifdef FFS_WIN
#include <zen/win_ver.h>
#include "long_path_prefix.h"
#include "dst_hack.h"
#include "file_handling.h" //remove this huge dependency when getting rid of DST hack!! until then we need "setFileTime"
#include "dll.h"
#include "FindFilePlus/find_file_plus.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#include <dirent.h>
#endif

using namespace zen;


namespace
{
//implement "retry" in a generic way:

template <class Command> inline //function object expecting to throw FileError if operation fails
bool tryReportingError(Command cmd, zen::TraverseCallback& callback) //return "true" on success, "false" if error was ignored
{
    for (;;)
        try
        {
            cmd(); //throw FileError
            return true;
        }
        catch (const FileError& e)
        {
            switch (callback.onError(e.toString()))
            {
                case TraverseCallback::ON_ERROR_RETRY:
                    break;
                case TraverseCallback::ON_ERROR_IGNORE:
                    return false;
                    //default:
                    //  assert(false);
                    //break;
            }
        }
}


#ifdef FFS_WIN
inline
bool getTargetInfoFromSymlink(const Zstring& linkName, zen::TraverseCallback::FileInfo& output)
{
    //open handle to target of symbolic link
    HANDLE hFile = ::CreateFile(zen::applyLongPathPrefix(linkName).c_str(),
                                0,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                nullptr,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS, //needed to open a directory
                                nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hFile));

    BY_HANDLE_FILE_INFORMATION fileInfoByHandle = {};
    if (!::GetFileInformationByHandle(hFile, &fileInfoByHandle))
        return false;

    //write output
    output.fileSize         = UInt64(fileInfoByHandle.nFileSizeLow, fileInfoByHandle.nFileSizeHigh);
    output.lastWriteTimeRaw = toTimeT(fileInfoByHandle.ftLastWriteTime);
    output.id = FileId(); //= extractFileID(fileInfoByHandle); -> id from dereferenced symlink is problematic, since renaming will consider the link, not the target!
    return true;
}


DWORD retrieveVolumeSerial(const Zstring& pathName) //returns 0 on error or if serial is not supported!
{
    //this works for:
    //- root paths "C:\", "D:\"
    //- network shares: \\share\dirname
    //- indirection: subst S: %USERPROFILE%
    //                  -> GetVolumePathName() on the other hand resolves "S:\Desktop\somedir" to "S:\Desktop\" - nice try...

    const HANDLE hDir = ::CreateFile(zen::applyLongPathPrefix(pathName).c_str(),
                                     0,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     nullptr,
                                     OPEN_EXISTING,
                                     FILE_FLAG_BACKUP_SEMANTICS /*needed to open a directory*/ /*| FILE_FLAG_OPEN_REPARSE_POINT -> no, we follow symlinks!*/ ,
                                     nullptr);
    if (hDir == INVALID_HANDLE_VALUE)
        return 0;
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hDir));

    BY_HANDLE_FILE_INFORMATION fileInfo = {};
    if (!::GetFileInformationByHandle(hDir, &fileInfo))
        return 0;

    return fileInfo.dwVolumeSerialNumber;
}


/*
DWORD retrieveVolumeSerial(const Zstring& pathName) //returns 0 on error!
{
    //note: this works for network shares: \\share\dirname, but not "subst"!

    const DWORD BUFFER_SIZE = 10000;
    std::vector<wchar_t> buffer(BUFFER_SIZE);

    //full pathName need not yet exist!
    if (!::GetVolumePathName(pathName.c_str(), //__in   LPCTSTR lpszFileName,
                             &buffer[0],       //__out  LPTSTR lpszVolumePathName,
                             BUFFER_SIZE))     //__in   DWORD cchBufferLength
        return 0;

    Zstring volumePath = appendSeparator(&buffer[0]);

    DWORD volumeSerial = 0;
    if (!::GetVolumeInformation(volumePath.c_str(), //__in_opt   LPCTSTR lpRootPathName,
                                nullptr,            //__out      LPTSTR lpVolumeNameBuffer,
                                0,                  //__in       DWORD nVolumeNameSize,
                                &volumeSerial,      //__out_opt  LPDWORD lpVolumeSerialNumber,
                                nullptr,            //__out_opt  LPDWORD lpMaximumComponentLength,
                                nullptr,            //__out_opt  LPDWORD lpFileSystemFlags,
                                nullptr,            //__out      LPTSTR lpFileSystemNameBuffer,
                                0))                 //__in       DWORD nFileSystemNameSize
        return 0;

    return volumeSerial;
}
*/


const bool isXpOrLater = winXpOrLater(); //VS2010 compiled DLLs are not supported on Win 2000: Popup dialog "DecodePointer not found"

const auto openDir = isXpOrLater ? DllFun<findplus::FunType_openDir >(findplus::getDllName(), findplus::funName_openDir ) : DllFun<findplus::FunType_openDir >(); //
const auto readDir = isXpOrLater ? DllFun<findplus::FunType_readDir >(findplus::getDllName(), findplus::funName_readDir ) : DllFun<findplus::FunType_readDir >(); //load at startup: avoid pre C++11 static initialization MT issues
const auto closeDir= isXpOrLater ? DllFun<findplus::FunType_closeDir>(findplus::getDllName(), findplus::funName_closeDir) : DllFun<findplus::FunType_closeDir>(); //

/*
Common C-style interface for Win32 FindFirstFile(), FindNextFile() and FileFilePlus openDir(), closeDir():
struct TraverserPolicy //see "policy based design"
{
typedef ... DirHandle;
typedef ... FindData;

static void create(const Zstring& directory, DirHandle& hnd); //throw FileError - *no* concession to FindFirstFile(): open handle only, *no* return of data!
static void destroy(DirHandle hnd); //throw()

template <class FallbackFun>
static bool getEntry(DirHandle hnd, const Zstring& directory, FindData& fileInfo, FallbackFun fb) //throw FileError -> fb: fallback to FindFirstFile()/FindNextFile()

//FindData "member" functions
static void extractFileInfo            (const FindData& fileInfo, DWORD volumeSerial, TraverseCallback::FileInfo& output); //volumeSerial may be 0 if not available!
static Int64 getModTime                (const FindData& fileInfo);
static const FILETIME& getModTimeRaw   (const FindData& fileInfo); //yet another concession to DST hack
static const FILETIME& getCreateTimeRaw(const FindData& fileInfo); //
static const wchar_t* getShortName     (const FindData& fileInfo);
static bool isDirectory                (const FindData& fileInfo);
static bool isSymlink                  (const FindData& fileInfo);
}

Note: Win32 FindFirstFile(), FindNextFile() is a weaker abstraction than FileFilePlus openDir(), readDir(), closeDir() and Unix opendir(), closedir(), stat()
*/


struct Win32Traverser
{
    struct DirHandle
    {
        DirHandle() : searchHandle(nullptr), haveData(true), data() {}

        HANDLE searchHandle;

        bool haveData;
        WIN32_FIND_DATA data;
    };

    typedef WIN32_FIND_DATA FindData;

    static void create(const Zstring& directory, DirHandle& hnd) //throw FileError
    {
        const Zstring& directoryPf = appendSeparator(directory);

        hnd.searchHandle = ::FindFirstFile(applyLongPathPrefix(directoryPf + L'*').c_str(), &hnd.data);
        //no noticable performance difference compared to FindFirstFileEx with FindExInfoBasic, FIND_FIRST_EX_CASE_SENSITIVE and/or FIND_FIRST_EX_LARGE_FETCH
        if (hnd.searchHandle == INVALID_HANDLE_VALUE)
            throw FileError(replaceCpy(_("Cannot read directory %x."), L"%x", fmtFileName(directory)) + L"\n\n" + getLastErrorFormatted());

        //::GetLastError() == ERROR_FILE_NOT_FOUND -> *usually* NOT okay:
        //directory may not exist *or* it is completely empty: not all directories contain "., .." entries, e.g. a drive's root directory
        //usually a directory is never completely empty due to "sync.ffs_lock", so we assume it's not existing and let the error propagate
        // -> FindFirstFile() is a nice example of violation of API design principle of single responsibility
    }

    static void destroy(const DirHandle& hnd) { ::FindClose(hnd.searchHandle); } //throw()

    template <class FallbackFun>
    static bool getEntry(DirHandle& hnd, const Zstring& directory, FindData& fileInfo, FallbackFun) //throw FileError
    {
        if (hnd.haveData)
        {
            hnd.haveData = false;
            ::memcpy(&fileInfo, &hnd.data, sizeof(fileInfo));
            return true;
        }

        if (!::FindNextFile(hnd.searchHandle, &fileInfo))
        {
            if (::GetLastError() == ERROR_NO_MORE_FILES) //not an error situation
                return false;
            //else we have a problem... report it:
            throw FileError(replaceCpy(_("Cannot read directory %x."), L"%x", fmtFileName(directory)) + L"\n\n" + getLastErrorFormatted());
        }
        return true;
    }

    template <class FindData>
    static void extractFileInfo(const FindData& fileInfo, DWORD volumeSerial, TraverseCallback::FileInfo& output)
    {
        output.fileSize         = UInt64(fileInfo.nFileSizeLow, fileInfo.nFileSizeHigh);
        output.lastWriteTimeRaw = getModTime(fileInfo);
        output.id = FileId();
    }

    template <class FindData>
    static Int64 getModTime(const FindData& fileInfo) { return toTimeT(fileInfo.ftLastWriteTime); }

    template <class FindData>
    static const FILETIME& getModTimeRaw(const FindData& fileInfo) { return fileInfo.ftLastWriteTime; }

    template <class FindData>
    static const FILETIME& getCreateTimeRaw(const FindData& fileInfo) { return fileInfo.ftCreationTime; }

    template <class FindData>
    static const wchar_t* getShortName(const FindData& fileInfo) { return fileInfo.cFileName; }

    template <class FindData>
    static bool isDirectory(const FindData& fileInfo) { return (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }

    template <class FindData>
    static bool isSymlink(const FindData& fileInfo) { return (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0; }
};


struct FilePlusTraverser
{
    struct DirHandle
    {
        DirHandle() : searchHandle(nullptr) {}

        findplus::FindHandle searchHandle;
    };

    typedef findplus::FileInformation FindData;

    static void create(const Zstring& directory, DirHandle& hnd) //throw FileError
    {
        hnd.searchHandle = ::openDir(applyLongPathPrefix(directory).c_str());
        if (hnd.searchHandle == nullptr)
            throw FileError(replaceCpy(_("Cannot read directory %x."), L"%x", fmtFileName(directory)) + L"\n\n" + getLastErrorFormatted() + L" (+)");
    }

    static void destroy(DirHandle hnd) { ::closeDir(hnd.searchHandle); } //throw()

    template <class FallbackFun>
    static bool getEntry(DirHandle hnd, const Zstring& directory, FindData& fileInfo, FallbackFun fb) //throw FileError
    {
        if (!::readDir(hnd.searchHandle, fileInfo))
        {
            const DWORD lastError = ::GetLastError();
            if (lastError == ERROR_NO_MORE_FILES) //not an error situation
                return false;

            /*
            fallback to default directory query method, if FileIdBothDirectoryInformation is not properly implemented
            this is required for NetDrive mounted Webdav, e.g. www.box.net and NT4, 2000 remote drives, et al.
            */
            if (lastError == ERROR_NOT_SUPPORTED)
            {
                fb(); //fallback should apply to whole directory sub-tree!
                return false;
            }

            //else we have a problem... report it:
            throw FileError(replaceCpy(_("Cannot read directory %x."), L"%x", fmtFileName(directory)) + L"\n\n" + getLastErrorFormatted(lastError) + L" (+)");
        }
        return true;
    }

    template <class FindData>
    static void extractFileInfo(const FindData& fileInfo, DWORD volumeSerial, TraverseCallback::FileInfo& output)
    {
        output.fileSize         = fileInfo.fileSize.QuadPart;
        output.lastWriteTimeRaw = getModTime(fileInfo);
        output.id               = extractFileID(volumeSerial, fileInfo.fileId);
    }

    template <class FindData>
    static Int64 getModTime(const FindData& fileInfo) { return toTimeT(fileInfo.lastWriteTime); }

    template <class FindData>
    static const FILETIME& getModTimeRaw(const FindData& fileInfo) { return fileInfo.lastWriteTime; }

    template <class FindData>
    static const FILETIME& getCreateTimeRaw(const FindData& fileInfo) { return fileInfo.creationTime; }

    template <class FindData>
    static const wchar_t* getShortName(const FindData& fileInfo) { return fileInfo.shortName; }

    template <class FindData>
    static bool isDirectory(const FindData& fileInfo) { return (fileInfo.fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }

    template <class FindData>
    static bool isSymlink(const FindData& fileInfo) { return (fileInfo.fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0; }
};


class DirTraverser
{
public:
    DirTraverser(const Zstring& baseDirectory, TraverseCallback& sink, DstHackCallback* dstCallback) :
        isFatFileSystem(dst::isFatDrive(baseDirectory)),
        volumeSerial(retrieveVolumeSerial(baseDirectory)) //return 0 on error
    {
        try //traversing certain folders with restricted permissions requires this privilege! (but copying these files may still fail)
        {
            activatePrivilege(SE_BACKUP_NAME); //throw FileError
        }
        catch (FileError&) {} //don't cause issues in user mode

        if (::openDir && ::readDir && ::closeDir)
            traverse<FilePlusTraverser>(baseDirectory, sink, 0);
        else //fallback
            traverse<Win32Traverser>(baseDirectory, sink, 0);

        //apply daylight saving time hack AFTER file traversing, to give separate feedback to user
        if (dstCallback && isFatFileSystem)
            applyDstHack(*dstCallback);
    }

private:
    DirTraverser(const DirTraverser&);
    DirTraverser& operator=(const DirTraverser&);

    template <class Trav>
    void traverse(const Zstring& directory, zen::TraverseCallback& sink, int level)
    {
        tryReportingError([&]
        {
            if (level == 100) //notify endless recursion
                throw FileError(replaceCpy(_("Cannot read directory %x."), L"%x", fmtFileName(directory)) + L"\n\n" + _("Detected endless directory recursion."));
        }, sink);

        typename Trav::DirHandle searchHandle;

        if (!tryReportingError([&]
    {
        typedef Trav Trav; //f u VS!
        Trav::create(directory, searchHandle); //throw FileError
        }, sink))
        return; //ignored error
        ZEN_ON_SCOPE_EXIT(typedef Trav Trav; Trav::destroy(searchHandle));

        typename Trav::FindData findData = {};

        auto fallback = [&] { this->traverse<Win32Traverser>(directory, sink, level); }; //help VS2010 a little by avoiding too deeply nested lambdas

        for (;;)
        {
            bool gotEntry = false;
            tryReportingError([&] { typedef Trav Trav; /*VS 2010 bug*/ gotEntry = Trav::getEntry(searchHandle, directory, findData, fallback); }, sink); //throw FileError
            if (!gotEntry) //no more items or ignored error
                return;

            //skip "." and ".."
            const Zchar* const shortName = Trav::getShortName(findData);
            if (shortName[0] == L'.' &&
                (shortName[1] == 0 || (shortName[1] == L'.' && shortName[2] == 0)))
                continue;

            const Zstring& fullName = appendSeparator(directory) + shortName;

            if (Trav::isSymlink(findData)) //check first!
            {
                TraverseCallback::SymlinkInfo linkInfo;
                try
                {
                    linkInfo.targetPath = getSymlinkRawTargetString(fullName); //throw FileError
                }
                catch (FileError&) { assert(false); }
                linkInfo.lastWriteTimeRaw = Trav::getModTime (findData);
                linkInfo.dirLink          = Trav::isDirectory(findData); //directory symlinks have this flag on Windows

                switch (sink.onSymlink(shortName, fullName, linkInfo))
                {
                    case TraverseCallback::LINK_FOLLOW:
                    {
                        //try to resolve symlink (and report error on failure!!!)
                        TraverseCallback::FileInfo targetInfo;
                        const bool validLink = tryReportingError([&]
                        {
                            if (!getTargetInfoFromSymlink(fullName, targetInfo))
                                throw FileError(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(fullName)) + L"\n\n" + getLastErrorFormatted());
                        }, sink);

                        if (validLink)
                        {
                            if (Trav::isDirectory(findData))
                            {
                                if (const std::shared_ptr<TraverseCallback>& rv = sink.onDir(shortName, fullName))
                                    traverse<Trav>(fullName, *rv, level + 1);
                            }
                            else //a file
                                sink.onFile(shortName, fullName, targetInfo);
                        }
                        else //report broken symlink as file!
                            sink.onFile(shortName, fullName, TraverseCallback::FileInfo());
                    }
                    break;

                    case TraverseCallback::LINK_SKIP:
                        break;
                }
            }
            else if (Trav::isDirectory(findData))
            {
                if (const std::shared_ptr<TraverseCallback>& rv = sink.onDir(shortName, fullName))
                    traverse<Trav>(fullName, *rv, level + 1);
            }
            else //a file
            {
                TraverseCallback::FileInfo fileInfo;
                Trav::extractFileInfo(findData, volumeSerial, fileInfo);

                //####################################### DST hack ###########################################
                if (isFatFileSystem)
                {
                    const dst::RawTime rawTime(Trav::getCreateTimeRaw(findData), Trav::getModTimeRaw(findData));

                    if (dst::fatHasUtcEncoded(rawTime)) //throw std::runtime_error
                        fileInfo.lastWriteTimeRaw = toTimeT(dst::fatDecodeUtcTime(rawTime)); //return real UTC time; throw (std::runtime_error)
                    else
                        markForDstHack.push_back(std::make_pair(fullName, toTimeT(rawTime.writeTimeRaw)));
                }
                //####################################### DST hack ###########################################

                sink.onFile(shortName, fullName, fileInfo);
            }
        }
    }


    //####################################### DST hack ###########################################
    void applyDstHack(zen::DstHackCallback& dstCallback)
    {
        int failedAttempts  = 0;
        int filesToValidate = 50; //don't let data verification become a performance issue

        for (auto iter = markForDstHack.begin(); iter != markForDstHack.end(); ++iter)
        {
            if (failedAttempts >= 10) //some cloud storages don't support changing creation/modification times => don't waste (a lot of) time trying to
                return;

            dstCallback.requestUiRefresh(iter->first);

            try
            {
                //set modification time including DST hack: this function is too clever to not introduce this dependency
                setFileTime(iter->first, iter->second, SYMLINK_FOLLOW); //throw FileError
            }
            catch (FileError&)
            {
                ++failedAttempts;
                assert(false); //don't throw exceptions due to dst hack here
                continue;
            }

            //even at this point it's not sure whether data was written correctly, again cloud storages tend to lie about success status
            if (filesToValidate-- > 0)
            {
                const dst::RawTime encodedTime = dst::fatEncodeUtcTime(tofiletime(iter->second)); //throw std::runtime_error

                //dst hack: verify data written; attention: this check may fail for "sync.ffs_lock"
                WIN32_FILE_ATTRIBUTE_DATA debugeAttr = {};
                ::GetFileAttributesEx(zen::applyLongPathPrefix(iter->first).c_str(), //__in   LPCTSTR lpFileName,
                                      GetFileExInfoStandard,                //__in   GET_FILEEX_INFO_LEVELS fInfoLevelId,
                                      &debugeAttr);                         //__out  LPVOID lpFileInformation

                if (::CompareFileTime(&debugeAttr.ftCreationTime,  &encodedTime.createTimeRaw) != 0 ||
                    ::CompareFileTime(&debugeAttr.ftLastWriteTime, &encodedTime.writeTimeRaw)  != 0)
                {
                    ++failedAttempts;
                    assert(false); //don't throw exceptions due to dst hack here
                    continue;
                }
            }
        }
    }

    const bool isFatFileSystem;
    typedef std::vector<std::pair<Zstring, Int64> > FilenameTimeList;
    FilenameTimeList markForDstHack;
    //####################################### DST hack ###########################################

    const DWORD volumeSerial; //may be 0!
};


#elif defined FFS_LINUX
class DirTraverser
{
public:
    DirTraverser(const Zstring& baseDirectory, zen::TraverseCallback& sink, zen::DstHackCallback* dstCallback)
    {
        const Zstring directoryFormatted = //remove trailing slash
            baseDirectory.size() > 1 && endsWith(baseDirectory, FILE_NAME_SEPARATOR) ?  //exception: allow '/'
            beforeLast(baseDirectory, FILE_NAME_SEPARATOR) :
            baseDirectory;

        /* quote: "Since POSIX.1 does not specify the size of the d_name field, and other nonstandard fields may precede
                   that field within the dirent structure, portable applications that use readdir_r() should allocate
                   the buffer whose address is passed in entry as follows:
                       len = offsetof(struct dirent, d_name) + pathconf(dirpath, _PC_NAME_MAX) + 1
                       entryp = malloc(len); */
        const long maxPath = std::max<long>(::pathconf(directoryFormatted.c_str(), _PC_NAME_MAX), 10000); //::pathconf may return -1
        buffer.resize(offsetof(struct ::dirent, d_name) + maxPath + 1);

        traverse(directoryFormatted, sink, 0);
    }

private:
    DirTraverser(const DirTraverser&);
    DirTraverser& operator=(const DirTraverser&);

    void traverse(const Zstring& directory, zen::TraverseCallback& sink, int level)
    {
        tryReportingError([&]
        {
            if (level == 100) //notify endless recursion
                throw FileError(replaceCpy(_("Cannot read directory %x."), L"%x", fmtFileName(directory)) + L"\n\n" + _("Detected endless directory recursion."));
        }, sink);


        DIR* dirObj = nullptr;
        if (!tryReportingError([&]
    {
        dirObj = ::opendir(directory.c_str()); //directory must NOT end with path separator, except "/"
            if (!dirObj)
                throw FileError(replaceCpy(_("Cannot read directory %x."), L"%x", fmtFileName(directory)) + L"\n\n" + getLastErrorFormatted());
        }, sink))
        return; //ignored error
        ZEN_ON_SCOPE_EXIT(::closedir(dirObj)); //never close nullptr handles! -> crash

        for (;;)
        {
            struct ::dirent* dirEntry = nullptr;
            tryReportingError([&]
            {
                if (::readdir_r(dirObj, reinterpret_cast< ::dirent*>(&buffer[0]), &dirEntry) != 0)
                    throw FileError(replaceCpy(_("Cannot read directory %x."), L"%x", fmtFileName(directory)) + L"\n\n" + getLastErrorFormatted());
            }, sink);
            if (!dirEntry) //no more items or ignored error
                return;


            //don't return "." and ".."
            const char* const shortName = dirEntry->d_name; //evaluate dirEntry *before* going into recursion => we use a single "buffer"!
            if (shortName[0] == '.' &&
                (shortName[1] == 0 || (shortName[1] == '.' && shortName[2] == 0)))
                continue;

            const Zstring& fullName = appendSeparator(directory) + shortName;

            struct ::stat statData = {};
            if (!tryReportingError([&]
        {
            if (::lstat(fullName.c_str(), &statData) != 0) //lstat() does not resolve symlinks
                    throw FileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(fullName)) + L"\n\n" + getLastErrorFormatted());
            }, sink))
            continue; //ignore error: skip file

            if (S_ISLNK(statData.st_mode)) //on Linux there is no distinction between file and directory symlinks!
            {
                struct ::stat statDataTrg = {};
                bool validLink = ::stat(fullName.c_str(), &statDataTrg) == 0; //if "LINK_SKIP", a broken link is no error!

                TraverseCallback::SymlinkInfo linkInfo;
                try
                {
                    linkInfo.targetPath = getSymlinkRawTargetString(fullName); //throw FileError
                }
                catch (FileError&) { assert(false); }
                linkInfo.lastWriteTimeRaw = statData.st_mtime; //UTC time (ANSI C format); unit: 1 second
                linkInfo.dirLink          = validLink && S_ISDIR(statDataTrg.st_mode);
                //S_ISDIR and S_ISLNK are mutually exclusive on Linux => explicitly need to follow link

                switch (sink.onSymlink(shortName, fullName, linkInfo))
                {
                    case TraverseCallback::LINK_FOLLOW:
                        //try to resolve symlink (and report error on failure!!!)
                        validLink = tryReportingError([&]
                        {
                            if (validLink) return; //no need to check twice
                            if (::stat(fullName.c_str(), &statDataTrg) != 0)
                                throw FileError(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(fullName)) + L"\n\n" + getLastErrorFormatted());
                        }, sink);

                        if (validLink)
                        {
                            if (S_ISDIR(statDataTrg.st_mode)) //a directory
                            {
                                if (const std::shared_ptr<TraverseCallback>& rv = sink.onDir(shortName, fullName))
                                    traverse(fullName, *rv, level + 1);
                            }
                            else //a file
                            {
                                TraverseCallback::FileInfo fileInfo;
                                fileInfo.fileSize         = zen::UInt64(statDataTrg.st_size);
                                fileInfo.lastWriteTimeRaw = statDataTrg.st_mtime; //UTC time (time_t format); unit: 1 second
                                //fileInfo.id               = extractFileID(statDataTrg); -> id from dereferenced symlink is problematic, since renaming will consider the link, not the target!
                                sink.onFile(shortName, fullName, fileInfo);
                            }
                        }
                        else //report broken symlink as file!
                            sink.onFile(shortName, fullName, TraverseCallback::FileInfo());
                        break;

                    case TraverseCallback::LINK_SKIP:
                        break;
                }
            }
            else if (S_ISDIR(statData.st_mode)) //a directory
            {
                if (const std::shared_ptr<TraverseCallback>& rv = sink.onDir(shortName, fullName))
                    traverse(fullName, *rv, level + 1);
            }
            else //a file
            {
                TraverseCallback::FileInfo fileInfo;
                fileInfo.fileSize         = zen::UInt64(statData.st_size);
                fileInfo.lastWriteTimeRaw = statData.st_mtime; //UTC time (time_t format); unit: 1 second
                fileInfo.id               = extractFileID(statData);

                sink.onFile(shortName, fullName, fileInfo);
            }
        }
    }

    std::vector<char> buffer;
};
#endif
}


void zen::traverseFolder(const Zstring& directory, TraverseCallback& sink, DstHackCallback* dstCallback)
{
    DirTraverser(directory, sink, dstCallback);
}
