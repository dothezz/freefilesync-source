// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "file_traverser.h"
#include "sys_error.h"
#include "assert_static.h"
#include "symlink_target.h"

#ifdef ZEN_WIN
#include <zen/win_ver.h>
#include "long_path_prefix.h"
#include "dst_hack.h"
#include "file_handling.h" //remove this huge dependency when getting rid of DST hack!! until then we need "setFileTime"
#include "dll.h"
#include "FindFilePlus/find_file_plus.h"

#elif defined ZEN_MAC
#include <zen/osx_string.h>
#endif

#if defined ZEN_LINUX || defined ZEN_MAC
#include <sys/stat.h>
#include <dirent.h>
#endif

using namespace zen;


namespace
{
//implement "retry" in a generic way:

template <class Command> inline //function object expecting to throw FileError if operation fails
bool tryReportingDirError(Command cmd, zen::TraverseCallback& callback) //return "true" on success, "false" if error was ignored
{
    for (;;)
        try
        {
            cmd(); //throw FileError
            return true;
        }
        catch (const FileError& e)
        {
            switch (callback.reportDirError(e.toString()))
            {
                case TraverseCallback::ON_ERROR_RETRY:
                    break;
                case TraverseCallback::ON_ERROR_IGNORE:
                    return false;
            }
        }
}

template <class Command> inline //function object expecting to throw FileError if operation fails
bool tryReportingItemError(Command cmd, zen::TraverseCallback& callback, const Zchar* shortName) //return "true" on success, "false" if error was ignored
{
    for (;;)
        try
        {
            cmd(); //throw FileError
            return true;
        }
        catch (const FileError& e)
        {
            switch (callback.reportItemError(e.toString(), shortName))
            {
                case TraverseCallback::ON_ERROR_RETRY:
                    break;
                case TraverseCallback::ON_ERROR_IGNORE:
                    return false;
            }
        }
}


#ifdef ZEN_WIN
void getInfoFromFileSymlink(const Zstring& linkName, zen::TraverseCallback::FileInfo& output) //throw FileError
{
    //open handle to target of symbolic link
    HANDLE hFile = ::CreateFile(zen::applyLongPathPrefix(linkName).c_str(),
                                0,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                nullptr,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS, //needed to open a directory -> keep it even if we expect to open a file! See comment below
                                nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        throw FileError(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(linkName)), formatSystemError(L"CreateFile", getLastError()));
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hFile));

    BY_HANDLE_FILE_INFORMATION fileInfo = {};
    if (!::GetFileInformationByHandle(hFile, &fileInfo))
        throw FileError(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(linkName)), formatSystemError(L"GetFileInformationByHandle", getLastError()));

    //a file symlink may incorrectly point to a directory, but both CreateFile() and GetFileInformationByHandle() will succeed and return garbage!
    //- if we did not use FILE_FLAG_BACKUP_SEMANTICS above, CreateFile() would error out with an even less helpful ERROR_ACCESS_DENIED!
    //- reinterpreting the link as a directory symlink would still fail during traversal, so just show an error here
    //- OTOH a directory symlink that points to a file fails immediately in ::FindFirstFile() with ERROR_DIRECTORY! -> nothing to do in this case
    if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        throw FileError(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(linkName)), formatSystemError(L"GetFileInformationByHandle", static_cast<DWORD>(ERROR_FILE_INVALID)));

    //write output
    output.fileSize      = UInt64(fileInfo.nFileSizeLow, fileInfo.nFileSizeHigh);
    output.lastWriteTime = toTimeT(fileInfo.ftLastWriteTime);
    output.id            = extractFileID(fileInfo); //consider detection of moved files: allow for duplicate file ids, renaming affects symlink, not target, ...
    //output.symlinkInfo -> not filled here
}


DWORD retrieveVolumeSerial(const Zstring& pathName) //returns 0 on error or if serial is not supported!
{
    //this works for:
    //- root paths "C:\", "D:\"
    //- network shares: \\share\dirname
    //- indirection: subst S: %USERPROFILE%
    //   -> GetVolumePathName()+GetVolumeInformation() OTOH incorrectly resolves "S:\Desktop\somedir" to "S:\Desktop\" - nice try...
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


const bool isXpOrLater = winXpOrLater(); //VS2010 compiled DLLs are not supported on Win 2000: Popup dialog "DecodePointer not found"

#define DEF_DLL_FUN(name) const auto name = isXpOrLater ? DllFun<findplus::FunType_##name>(findplus::getDllName(), findplus::funName_##name) : DllFun<findplus::FunType_##name>();
DEF_DLL_FUN(openDir);   //
DEF_DLL_FUN(readDir);   //load at startup: avoid pre C++11 static initialization MT issues
DEF_DLL_FUN(closeDir);  //

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

    static void create(const Zstring& dirname, DirHandle& hnd) //throw FileError
    {
        const Zstring& dirnamePf = appendSeparator(dirname);

        hnd.searchHandle = ::FindFirstFile(applyLongPathPrefix(dirnamePf + L'*').c_str(), &hnd.data);
        //no noticable performance difference compared to FindFirstFileEx with FindExInfoBasic, FIND_FIRST_EX_CASE_SENSITIVE and/or FIND_FIRST_EX_LARGE_FETCH
        if (hnd.searchHandle == INVALID_HANDLE_VALUE)
        {
            const ErrorCode lastError = getLastError(); //copy before making other system calls!
            hnd.haveData = false;
            if (lastError == ERROR_FILE_NOT_FOUND)
            {
                //1. directory may not exist *or* 2. it is completely empty: not all directories contain "., .." entries, e.g. a drive's root directory; NetDrive
                // -> FindFirstFile() is a nice example of violation of API design principle of single responsibility
                if (dirExists(dirname)) //yes, a race-condition, still the best we can do
                    return;
            }
            throw FileError(replaceCpy(_("Cannot open directory %x."), L"%x", fmtFileName(dirname)), formatSystemError(L"FindFirstFile", lastError));
        }
    }

    static void destroy(const DirHandle& hnd) { ::FindClose(hnd.searchHandle); } //throw()

    template <class FallbackFun>
    static bool getEntry(DirHandle& hnd, const Zstring& dirname, FindData& fileInfo, FallbackFun) //throw FileError
    {
        if (hnd.searchHandle == INVALID_HANDLE_VALUE) //handle special case of "truly empty directories"
            return false;

        if (hnd.haveData)
        {
            hnd.haveData = false;
            ::memcpy(&fileInfo, &hnd.data, sizeof(fileInfo));
            return true;
        }

        if (!::FindNextFile(hnd.searchHandle, &fileInfo))
        {
            const ErrorCode lastError = getLastError(); //copy before making other system calls!
            if (lastError == ERROR_NO_MORE_FILES) //not an error situation
                return false;
            //else we have a problem... report it:
            throw FileError(replaceCpy(_("Cannot enumerate directory %x."), L"%x", fmtFileName(dirname)), formatSystemError(L"FindNextFile", lastError));
        }
        return true;
    }

    static void extractFileInfo(const FindData& fileInfo, DWORD volumeSerial, TraverseCallback::FileInfo& output)
    {
        output.fileSize      = UInt64(fileInfo.nFileSizeLow, fileInfo.nFileSizeHigh);
        output.lastWriteTime = getModTime(fileInfo);
        output.id = FileId();
    }

    static Int64           getModTime      (const FindData& fileInfo) { return toTimeT(fileInfo.ftLastWriteTime); }
    static const FILETIME& getModTimeRaw   (const FindData& fileInfo) { return fileInfo.ftLastWriteTime; }
    static const FILETIME& getCreateTimeRaw(const FindData& fileInfo) { return fileInfo.ftCreationTime; }
    static const wchar_t*  getShortName    (const FindData& fileInfo) { return fileInfo.cFileName; }
    static bool            isDirectory     (const FindData& fileInfo) { return (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }
    static bool            isSymlink       (const FindData& fileInfo) { return zen::isSymlink(fileInfo); } //[!] keep namespace
};


struct FilePlusTraverser
{
    struct DirHandle
    {
        DirHandle() : searchHandle(nullptr) {}

        findplus::FindHandle searchHandle;
    };

    typedef findplus::FileInformation FindData;

    static void create(const Zstring& dirname, DirHandle& hnd) //throw FileError
    {
        hnd.searchHandle = ::openDir(applyLongPathPrefix(dirname).c_str());
        if (hnd.searchHandle == nullptr)
            throw FileError(replaceCpy(_("Cannot open directory %x."), L"%x", fmtFileName(dirname)), formatSystemError(L"openDir", getLastError()));
    }

    static void destroy(DirHandle hnd) { ::closeDir(hnd.searchHandle); } //throw()

    template <class FallbackFun>
    static bool getEntry(DirHandle hnd, const Zstring& dirname, FindData& fileInfo, FallbackFun fb) //throw FileError
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
                fb(); //fallback should apply to whole directory sub-tree! => client needs to handle duplicate file notifications!
                return false;
            }

            //else we have a problem... report it:
            throw FileError(replaceCpy(_("Cannot enumerate directory %x."), L"%x", fmtFileName(dirname)), formatSystemError(L"readDir", lastError));
        }

        return true;
    }

    static void extractFileInfo(const FindData& fileInfo, DWORD volumeSerial, TraverseCallback::FileInfo& output)
    {
        output.fileSize      = fileInfo.fileSize.QuadPart;
        output.lastWriteTime = getModTime(fileInfo);
        output.id            = extractFileID(volumeSerial, fileInfo.fileId);
    }

    static Int64           getModTime      (const FindData& fileInfo) { return toTimeT(fileInfo.lastWriteTime); }
    static const FILETIME& getModTimeRaw   (const FindData& fileInfo) { return fileInfo.lastWriteTime; }
    static const FILETIME& getCreateTimeRaw(const FindData& fileInfo) { return fileInfo.creationTime; }
    static const wchar_t*  getShortName    (const FindData& fileInfo) { return fileInfo.shortName; }
    static bool            isDirectory     (const FindData& fileInfo) { return (fileInfo.fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }
    static bool            isSymlink       (const FindData& fileInfo) { return zen::isSymlink(fileInfo.fileAttributes, fileInfo.reparseTag); } //[!] keep namespace
};


class DirTraverser
{
public:
    DirTraverser(const Zstring& baseDirectory, TraverseCallback& sink, DstHackCallback* dstCallback) :
        needDstHack(dstCallback ? dst::isFatDrive(baseDirectory) : false)
    {
        try //traversing certain folders with restricted permissions requires this privilege! (but copying these files may still fail)
        {
            activatePrivilege(SE_BACKUP_NAME); //throw FileError
        }
        catch (FileError&) {} //don't cause issues in user mode

        if (::openDir && ::readDir && ::closeDir)
            traverse<FilePlusTraverser>(baseDirectory, sink, retrieveVolumeSerial(baseDirectory)); //retrieveVolumeSerial returns 0 on error
        else //fallback
            traverse<Win32Traverser>(baseDirectory, sink, 0);

        //apply daylight saving time hack AFTER file traversing, to give separate feedback to user
        if (needDstHack)
            if (dstCallback) //bound if "needDstHack == true"
                applyDstHack(*dstCallback);
    }

private:
    DirTraverser(const DirTraverser&);
    DirTraverser& operator=(const DirTraverser&);

    template <class Trav>
    void traverse(const Zstring& dirname, zen::TraverseCallback& sink, DWORD volumeSerial /*may be 0!*/)
    {
        //no need to check for endless recursion: Windows seems to have an internal path limit of about 700 chars

        typename Trav::DirHandle searchHandle;

        if (!tryReportingDirError([&]
    {
        typedef Trav Trav; //f u VS!
        Trav::create(dirname, searchHandle); //throw FileError
        }, sink))
        return; //ignored error
        ZEN_ON_SCOPE_EXIT(typedef Trav Trav; Trav::destroy(searchHandle));

        typename Trav::FindData findData = {};

        auto fallback = [&] { this->traverse<Win32Traverser>(dirname, sink, volumeSerial); }; //help VS2010 a little by avoiding too deeply nested lambdas

        for (;;)
        {
            bool gotEntry = false;
            tryReportingDirError([&] { typedef Trav Trav; /*VS 2010 bug*/ gotEntry = Trav::getEntry(searchHandle, dirname, findData, fallback); }, sink); //throw FileError
            if (!gotEntry) //no more items or ignored error
                return;

            //skip "." and ".."
            const Zchar* const shortName = Trav::getShortName(findData);
            if (shortName[0] == L'.' &&
                (shortName[1] == 0 || (shortName[1] == L'.' && shortName[2] == 0)))
                continue;

            const Zstring& fullName = appendSeparator(dirname) + shortName;

            if (Trav::isSymlink(findData)) //check first!
            {
                TraverseCallback::SymlinkInfo linkInfo;
                linkInfo.lastWriteTime = Trav::getModTime (findData);

                switch (sink.onSymlink(shortName, fullName, linkInfo))
                {
                    case TraverseCallback::LINK_FOLLOW:
                        if (Trav::isDirectory(findData))
                        {
                            if (TraverseCallback* trav = sink.onDir(shortName, fullName))
                            {
                                ZEN_ON_SCOPE_EXIT(sink.releaseDirTraverser(trav));
                                traverse<Trav>(fullName, *trav, retrieveVolumeSerial(fullName)); //symlink may link to different volume => redetermine volume serial!
                            }
                        }
                        else //a file
                        {
                            TraverseCallback::FileInfo targetInfo;
                            const bool validLink = tryReportingItemError([&] //try to resolve symlink (and report error on failure!!!)
                            {
                                getInfoFromFileSymlink(fullName, targetInfo); //throw FileError
                                targetInfo.symlinkInfo = &linkInfo;
                            }, sink, shortName);

                            if (validLink)
                                sink.onFile(shortName, fullName, targetInfo);
                            // else //broken symlink -> ignore: it's client's responsibility to handle error!
                        }
                        break;

                    case TraverseCallback::LINK_SKIP:
                        break;
                }
            }
            else if (Trav::isDirectory(findData))
            {
                if (TraverseCallback* trav = sink.onDir(shortName, fullName))
                {
                    ZEN_ON_SCOPE_EXIT(sink.releaseDirTraverser(trav));
                    traverse<Trav>(fullName, *trav, volumeSerial);
                }
            }
            else //a file
            {
                TraverseCallback::FileInfo fileInfo;
                Trav::extractFileInfo(findData, volumeSerial, fileInfo);

                //####################################### DST hack ###########################################
                if (needDstHack)
                {
                    const dst::RawTime rawTime(Trav::getCreateTimeRaw(findData), Trav::getModTimeRaw(findData));

                    if (dst::fatHasUtcEncoded(rawTime)) //throw std::runtime_error
                        fileInfo.lastWriteTime = toTimeT(dst::fatDecodeUtcTime(rawTime)); //return real UTC time; throw std::runtime_error
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

        for (auto it = markForDstHack.begin(); it != markForDstHack.end(); ++it)
        {
            if (failedAttempts >= 10) //some cloud storages don't support changing creation/modification times => don't waste (a lot of) time trying to
                return;

            dstCallback.requestUiRefresh(it->first);

            try
            {
                //set modification time including DST hack: this function is too clever to not introduce this dependency
                setFileTime(it->first, it->second, SYMLINK_FOLLOW); //throw FileError
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
                const dst::RawTime encodedTime = dst::fatEncodeUtcTime(tofiletime(it->second)); //throw std::runtime_error

                //dst hack: verify data written; attention: this check may fail for "sync.ffs_lock"
                WIN32_FILE_ATTRIBUTE_DATA debugeAttr = {};
                ::GetFileAttributesEx(zen::applyLongPathPrefix(it->first).c_str(), //__in   LPCTSTR lpFileName,
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

    const bool needDstHack;
    typedef std::vector<std::pair<Zstring, Int64>> FilenameTimeList;
    FilenameTimeList markForDstHack;
    //####################################### DST hack ###########################################
};

#elif defined ZEN_LINUX || defined ZEN_MAC
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
        const size_t nameMax = std::max<long>(::pathconf(directoryFormatted.c_str(), _PC_NAME_MAX), 10000); //::pathconf may return long(-1)
        buffer.resize(offsetof(struct ::dirent, d_name) + nameMax + 1);

        traverse(directoryFormatted, sink);
    }

private:
    DirTraverser(const DirTraverser&);
    DirTraverser& operator=(const DirTraverser&);

    void traverse(const Zstring& dirname, zen::TraverseCallback& sink)
    {
        //no need to check for endless recursion: Linux has a fixed limit on the number of symbolic links in a path

        DIR* dirObj = nullptr;
        if (!tryReportingDirError([&]
    {
        dirObj = ::opendir(dirname.c_str()); //directory must NOT end with path separator, except "/"
            if (!dirObj)
                throw FileError(replaceCpy(_("Cannot open directory %x."), L"%x", fmtFileName(dirname)), formatSystemError(L"opendir", getLastError()));
        }, sink))
        return; //ignored error
        ZEN_ON_SCOPE_EXIT(::closedir(dirObj)); //never close nullptr handles! -> crash

        for (;;)
        {
            struct ::dirent* dirEntry = nullptr;
            tryReportingDirError([&]
            {
                if (::readdir_r(dirObj, reinterpret_cast< ::dirent*>(&buffer[0]), &dirEntry) != 0)
                    throw FileError(replaceCpy(_("Cannot enumerate directory %x."), L"%x", fmtFileName(dirname)), formatSystemError(L"readdir_r", getLastError()));
            }, sink);
            if (!dirEntry) //no more items or ignored error
                return;

            //don't return "." and ".."
            const char* shortName = dirEntry->d_name; //evaluate dirEntry *before* going into recursion => we use a single "buffer"!
            if (shortName[0] == '.' &&
                (shortName[1] == 0 || (shortName[1] == '.' && shortName[2] == 0)))
                continue;
#ifdef ZEN_MAC
            //some file system abstraction layers fail to properly return decomposed UTF8: http://developer.apple.com/library/mac/#qa/qa1173/_index.html
            //so we need to do it ourselves; perf: ~600 ns per conversion
            //note: it's not sufficient to apply this in z_impl::compareFilenamesNoCase: if UTF8 forms differ, FFS assumes a rename in case sensitivity and
            //   will try to propagate the rename => this won't work if target drive reports a particular UTF8 form only!
            if (CFStringRef cfStr = osx::createCFString(shortName))
            {
                ZEN_ON_SCOPE_EXIT(::CFRelease(cfStr));

                CFIndex lenMax = ::CFStringGetMaximumSizeOfFileSystemRepresentation(cfStr); //"could be much larger than the actual space required" => don't store in Zstring
                if (lenMax > 0)
                {
                    bufferUtfDecomposed.resize(lenMax);
                    if (::CFStringGetFileSystemRepresentation(cfStr, &bufferUtfDecomposed[0], lenMax)) //get decomposed UTF form (verified!) despite ambiguous documentation
                        shortName = &bufferUtfDecomposed[0];
                }
            }
            //const char* sampleDecomposed  = "\x6f\xcc\x81.txt";
            //const char* samplePrecomposed = "\xc3\xb3.txt";
#endif
            const Zstring& fullName = appendSeparator(dirname) + shortName;

            struct ::stat statData = {};
            if (!tryReportingItemError([&]
        {
            if (::lstat(fullName.c_str(), &statData) != 0) //lstat() does not resolve symlinks
                    throw FileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(fullName)), formatSystemError(L"lstat", getLastError()));
            }, sink, shortName))
            continue; //ignore error: skip file

            if (S_ISLNK(statData.st_mode)) //on Linux there is no distinction between file and directory symlinks!
            {
                TraverseCallback::SymlinkInfo linkInfo;
                linkInfo.lastWriteTime = statData.st_mtime; //UTC time (ANSI C format); unit: 1 second

                switch (sink.onSymlink(shortName, fullName, linkInfo))
                {
                    case TraverseCallback::LINK_FOLLOW:
                    {
                        //try to resolve symlink (and report error on failure!!!)
                        struct ::stat statDataTrg = {};
                        bool validLink = tryReportingItemError([&]
                        {
                            if (::stat(fullName.c_str(), &statDataTrg) != 0)
                                throw FileError(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(fullName)), formatSystemError(L"stat", getLastError()));
                        }, sink, shortName);

                        if (validLink)
                        {
                            if (S_ISDIR(statDataTrg.st_mode)) //a directory
                            {
                                if (TraverseCallback* trav = sink.onDir(shortName, fullName))
                                {
                                    ZEN_ON_SCOPE_EXIT(sink.releaseDirTraverser(trav));
                                    traverse(fullName, *trav);
                                }
                            }
                            else //a file or named pipe, ect.
                            {
                                TraverseCallback::FileInfo fileInfo;
                                fileInfo.fileSize      = zen::UInt64(statDataTrg.st_size);
                                fileInfo.lastWriteTime = statDataTrg.st_mtime; //UTC time (time_t format); unit: 1 second
                                fileInfo.id            = extractFileID(statDataTrg);
                                fileInfo.symlinkInfo   = &linkInfo;
                                sink.onFile(shortName, fullName, fileInfo);
                            }
                        }
                        // else //broken symlink -> ignore: it's client's responsibility to handle error!
                    }
                    break;

                    case TraverseCallback::LINK_SKIP:
                        break;
                }
            }
            else if (S_ISDIR(statData.st_mode)) //a directory
            {
                if (TraverseCallback* trav = sink.onDir(shortName, fullName))
                {
                    ZEN_ON_SCOPE_EXIT(sink.releaseDirTraverser(trav));
                    traverse(fullName, *trav);
                }
            }
            else //a file or named pipe, ect.
            {
                TraverseCallback::FileInfo fileInfo;
                fileInfo.fileSize      = zen::UInt64(statData.st_size);
                fileInfo.lastWriteTime = statData.st_mtime; //UTC time (time_t format); unit: 1 second
                fileInfo.id            = extractFileID(statData);

                sink.onFile(shortName, fullName, fileInfo);
            }
            /*
            It may be a good idea to not check "S_ISREG(statData.st_mode)" explicitly and to not issue an error message on other types to support these scenarios:
            - RTS setup watch (essentially wants to read directories only)
            - removeDirectory (wants to delete everything; pipes can be deleted just like files via "unlink")

            However an "open" on a pipe will block (https://sourceforge.net/p/freefilesync/bugs/221/), so the copy routines need to be smarter!!
            */
        }
    }

    std::vector<char> buffer;
#ifdef ZEN_MAC
    std::vector<char> bufferUtfDecomposed;
#endif
};
#endif
}


void zen::traverseFolder(const Zstring& dirname, TraverseCallback& sink, DstHackCallback* dstCallback)
{
    DirTraverser(dirname, sink, dstCallback);
}
