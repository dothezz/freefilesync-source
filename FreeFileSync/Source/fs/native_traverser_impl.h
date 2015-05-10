// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include <zen/sys_error.h>
#include <zen/symlink_target.h>
#include <zen/int64.h>

#ifdef ZEN_WIN
    #include <zen/win_ver.h>
    #include <zen/long_path_prefix.h>
    #include <zen/file_access.h>
    #include <zen/dll.h>
    #include "../dll/FindFilePlus/find_file_plus.h"

#elif defined ZEN_MAC
    #include <zen/osx_string.h>
#endif

#if defined ZEN_LINUX || defined ZEN_MAC
    #include <cstddef> //offsetof
    #include <sys/stat.h>
    #include <dirent.h>
#endif

using namespace zen;
using ABF = AbstractBaseFolder;


namespace
{
inline
ABF::FileId convertToAbstractFileId(const zen::FileId& fid)
{
    if (fid == zen::FileId())
        return ABF::FileId();

    ABF::FileId out(reinterpret_cast<const char*>(&fid.first), sizeof(fid.first));
    out.append(reinterpret_cast<const char*>(&fid.second), sizeof(fid.second));
    return out;
}


#ifdef ZEN_WIN
struct SymlinkTargetInfo
{
    std::uint64_t fileSize;
    std::int64_t  lastWriteTime;
    FileId        id;
};

SymlinkTargetInfo getInfoFromFileSymlink(const Zstring& linkName) //throw FileError
{
    //open handle to target of symbolic link
    HANDLE hFile = ::CreateFile(zen::applyLongPathPrefix(linkName).c_str(),              //_In_      LPCTSTR lpFileName,
                                0,                                                       //_In_      DWORD dwDesiredAccess,
                                FILE_SHARE_READ  | FILE_SHARE_WRITE | FILE_SHARE_DELETE, //_In_      DWORD dwShareMode,
                                nullptr,                    //_In_opt_  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                OPEN_EXISTING,              //_In_      DWORD dwCreationDisposition,
                                FILE_FLAG_BACKUP_SEMANTICS, //_In_      DWORD dwFlagsAndAttributes,
                                //needed to open a directory -> keep it even if we expect to open a file! See comment below
                                nullptr);                   //_In_opt_  HANDLE hTemplateFile
    if (hFile == INVALID_HANDLE_VALUE)
        throwFileError(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(linkName)), L"CreateFile", getLastError());
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hFile));

    BY_HANDLE_FILE_INFORMATION fileInfo = {};
    if (!::GetFileInformationByHandle(hFile, &fileInfo))
        throwFileError(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(linkName)), L"GetFileInformationByHandle", getLastError());

    //a file symlink may incorrectly point to a directory, but both CreateFile() and GetFileInformationByHandle() will succeed and return garbage!
    //- if we did not use FILE_FLAG_BACKUP_SEMANTICS above, CreateFile() would error out with an even less helpful ERROR_ACCESS_DENIED!
    //- reinterpreting the link as a directory symlink would still fail during traversal, so just show an error here
    //- OTOH a directory symlink that points to a file fails immediately in ::FindFirstFile() with ERROR_DIRECTORY! -> nothing to do in this case
    if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        throw FileError(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(linkName)), formatSystemError(L"GetFileInformationByHandle", static_cast<DWORD>(ERROR_FILE_INVALID)));

    return { get64BitUInt(fileInfo.nFileSizeLow, fileInfo.nFileSizeHigh),
             filetimeToTimeT(fileInfo.ftLastWriteTime),
             extractFileId(fileInfo) }; //consider detection of moved files: allow for duplicate file ids, renaming affects symlink, not target, ...
}


DWORD retrieveVolumeSerial(const Zstring& pathName) //returns 0 on error or if serial is not supported!
{
    //this works for:
    //- root paths "C:\", "D:\"
    //- network shares: \\share\dirname
    //- indirection: subst S: %USERPROFILE%
    //   -> GetVolumePathName() + GetVolumeInformation() OTOH incorrectly resolves "S:\Desktop\somedir" to "S:\Desktop\" - nice try...
    const HANDLE hDir = ::CreateFile(zen::applyLongPathPrefix(pathName).c_str(),             //_In_      LPCTSTR lpFileName,
                                     0,                                                      //_In_      DWORD dwDesiredAccess,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, //_In_      DWORD dwShareMode,
                                     nullptr,                    //_In_opt_  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                     OPEN_EXISTING,              //_In_      DWORD dwCreationDisposition,
                                     // FILE_FLAG_OPEN_REPARSE_POINT -> no, we follow symlinks!
                                     FILE_FLAG_BACKUP_SEMANTICS, //_In_      DWORD dwFlagsAndAttributes,
                                     /*needed to open a directory*/
                                     nullptr);                   //_In_opt_  HANDLE hTemplateFile
    if (hDir == INVALID_HANDLE_VALUE)
        return 0;
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hDir));

    BY_HANDLE_FILE_INFORMATION fileInfo = {};
    if (!::GetFileInformationByHandle(hDir, &fileInfo))
        return 0;

    return fileInfo.dwVolumeSerialNumber;
}


const bool isXpOrLater = winXpOrLater(); //VS2010 compiled DLLs are not supported on Win 2000 and show popup dialog "DecodePointer not found"

#define DEF_DLL_FUN(name) const auto name = isXpOrLater ? DllFun<findplus::FunType_##name>(findplus::getDllName(), findplus::funName_##name) : DllFun<findplus::FunType_##name>();
DEF_DLL_FUN(openDir);   //
DEF_DLL_FUN(readDir);   //load at startup: avoid pre C++11 static initialization MT issues
DEF_DLL_FUN(closeDir);  //
#undef DEF_DLL_FUN

/*
Common C-style interface for Win32 FindFirstFile(), FindNextFile() and FileFilePlus openDir(), closeDir():
struct TraverserPolicy //see "policy based design"
{
typedef ... DirHandle;
typedef ... FindData;

static DirHandle create(const Zstring& directory); //throw FileError - don't follow FindFirstFile() design: open handle only, *no* return of data!
static void destroy(DirHandle hnd); //throw()

static bool getEntry(DirHandle hnd, const Zstring& directory, FindData& fileInfo) //throw FileError, NeedFallbackToWin32Traverser -> fallback to FindFirstFile()/FindNextFile()

//FindData "member" functions
static const wchar_t*  getItemName(const FindData& fileInfo);
static std::uint64_t   getFileSize(const FindData& fileInfo);
static std::int64_t    getModTime (const FindData& fileInfo);
static FileId          getFileId  (const FindData& fileInfo, DWORD volumeSerial); //volumeSerial may be 0 if not available!
static bool            isDirectory(const FindData& fileInfo);
static bool            isSymlink  (const FindData& fileInfo);
}

Note: Win32 FindFirstFile(), FindNextFile() is a weaker abstraction than FileFilePlus openDir(), readDir(), closeDir() and Unix opendir(), closedir(), stat()
*/


struct Win32Traverser
{
    struct DirHandle
    {
        DirHandle(HANDLE hnd, const WIN32_FIND_DATA& d) : searchHandle(hnd), haveData(true), data(d) {}
        explicit DirHandle(HANDLE hnd)                  : searchHandle(hnd), haveData(false) {}

        HANDLE searchHandle;
        bool haveData;
        WIN32_FIND_DATA data;
    };

    typedef WIN32_FIND_DATA FindData;

    static DirHandle create(const Zstring& dirPath) //throw FileError
    {
        const Zstring& dirPathPf = appendSeparator(dirPath);

        WIN32_FIND_DATA fileData = {};
        HANDLE hnd = ::FindFirstFile(applyLongPathPrefix(dirPathPf + L'*').c_str(), &fileData);
        //no noticable performance difference compared to FindFirstFileEx with FindExInfoBasic, FIND_FIRST_EX_CASE_SENSITIVE and/or FIND_FIRST_EX_LARGE_FETCH
        if (hnd == INVALID_HANDLE_VALUE)
        {
            const DWORD lastError = ::GetLastError(); //copy before making other system calls!
            if (lastError == ERROR_FILE_NOT_FOUND)
            {
                //1. directory may not exist *or* 2. it is completely empty: not all directories contain "., .." entries, e.g. a drive's root directory; NetDrive
                // -> FindFirstFile() is a nice example of violation of API design principle of single responsibility
                if (dirExists(dirPath)) //yes, a race-condition, still the best we can do
                    return DirHandle(hnd);
            }
            throwFileError(replaceCpy(_("Cannot open directory %x."), L"%x", fmtFileName(dirPath)), L"FindFirstFile", lastError);
        }
        return DirHandle(hnd, fileData);
    }

    static void destroy(const DirHandle& hnd) { ::FindClose(hnd.searchHandle); } //throw()

    static bool getEntry(DirHandle& hnd, const Zstring& dirPath, FindData& fileInfo) //throw FileError
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
            const DWORD lastError = ::GetLastError(); //copy before making other system calls!
            if (lastError == ERROR_NO_MORE_FILES) //not an error situation
                return false;
            //else we have a problem... report it:
            throwFileError(replaceCpy(_("Cannot enumerate directory %x."), L"%x", fmtFileName(dirPath)), L"FindNextFile", lastError);
        }
        return true;
    }

    static const wchar_t*  getItemName(const FindData& fileInfo) { return fileInfo.cFileName; }
    static std::uint64_t   getFileSize(const FindData& fileInfo) { return get64BitUInt(fileInfo.nFileSizeLow, fileInfo.nFileSizeHigh); }
    static std::int64_t    getModTime (const FindData& fileInfo) { return filetimeToTimeT(fileInfo.ftLastWriteTime); }
    static FileId          getFileId  (const FindData& fileInfo, DWORD volumeSerial) { return FileId(); }
    static bool            isDirectory(const FindData& fileInfo) { return (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }
    static bool            isSymlink  (const FindData& fileInfo) { return zen::isSymlink(fileInfo); } //[!] keep namespace
};


class NeedFallbackToWin32Traverser {}; //special exception class


struct FilePlusTraverser
{
    struct DirHandle
    {
        explicit DirHandle(findplus::FindHandle hnd) : searchHandle(hnd) {}

        findplus::FindHandle searchHandle;
    };

    typedef findplus::FileInformation FindData;

    static DirHandle create(const Zstring& dirPath) //throw FileError
    {
        const findplus::FindHandle hnd = ::openDir(applyLongPathPrefix(dirPath).c_str());
        if (!hnd)
            throwFileError(replaceCpy(_("Cannot open directory %x."), L"%x", fmtFileName(dirPath)), L"openDir", getLastError());

        return DirHandle(hnd);
    }

    static void destroy(DirHandle hnd) { ::closeDir(hnd.searchHandle); } //throw()

    static bool getEntry(DirHandle hnd, const Zstring& dirPath, FindData& fileInfo) //throw FileError, NeedFallbackToWin32Traverser
    {
        if (!::readDir(hnd.searchHandle, fileInfo))
        {
            const DWORD lastError = ::GetLastError(); //copy before directly or indirectly making other system calls!
            if (lastError == ERROR_NO_MORE_FILES) //not an error situation
                return false;

            /*
            fallback to default directory query method, if FileIdBothDirectoryInformation is not properly implemented
            this is required for NetDrive mounted Webdav, e.g. www.box.net and NT4, 2000 remote drives, et al.
            */
            if (lastError == ERROR_NOT_SUPPORTED)
                throw NeedFallbackToWin32Traverser();
            //fallback should apply to whole directory sub-tree! => client needs to handle duplicate file notifications!

            //else we have a problem... report it:
            throwFileError(replaceCpy(_("Cannot enumerate directory %x."), L"%x", fmtFileName(dirPath)), L"readDir", lastError);
        }

        return true;
    }

    static const wchar_t*  getItemName(const FindData& fileInfo) { return fileInfo.shortName; }
    static std::uint64_t   getFileSize(const FindData& fileInfo) { return fileInfo.fileSize; }
    static std::int64_t    getModTime (const FindData& fileInfo) { return filetimeToTimeT(fileInfo.lastWriteTime); }
    static FileId          getFileId  (const FindData& fileInfo, DWORD volumeSerial) { return extractFileId(volumeSerial, fileInfo.fileId); }
    static bool            isDirectory(const FindData& fileInfo) { return (fileInfo.fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }
    static bool            isSymlink  (const FindData& fileInfo) { return zen::isSymlink(fileInfo.fileAttributes, fileInfo.reparseTag); } //[!] keep namespace
};


class DirTraverser
{
public:
    static void execute(const Zstring& baseDirectory, ABF::TraverserCallback& sink)
    {
        DirTraverser(baseDirectory, sink);
    }

private:
    DirTraverser(const Zstring& baseDirectory, ABF::TraverserCallback& sink);
    DirTraverser           (const DirTraverser&) = delete;
    DirTraverser& operator=(const DirTraverser&) = delete;

    template <class Trav>
    void traverse(const Zstring& dirPath, ABF::TraverserCallback& sink, DWORD volumeSerial);

    template <class Trav>
    void traverseWithException(const Zstring& dirPath, ABF::TraverserCallback& sink, DWORD volumeSerial /*may be 0!*/); //throw FileError, NeedFallbackToWin32Traverser
};


template <> inline
void DirTraverser::traverse<Win32Traverser>(const Zstring& dirPath, ABF::TraverserCallback& sink, DWORD volumeSerial)
{
    tryReportingDirError([&]
    {
        traverseWithException<Win32Traverser>(dirPath, sink, 0); //throw FileError
    }, sink);
}


template <> inline
void DirTraverser::traverse<FilePlusTraverser>(const Zstring& dirPath, ABF::TraverserCallback& sink, DWORD volumeSerial)
{
    try
    {
        tryReportingDirError([&]
        {
            traverseWithException<FilePlusTraverser>(dirPath, sink, volumeSerial); //throw FileError, NeedFallbackToWin32Traverser
        }, sink);
    }
    catch (NeedFallbackToWin32Traverser&) { traverse<Win32Traverser>(dirPath, sink, 0); }
}


inline
DirTraverser::DirTraverser(const Zstring& baseDirectory, ABF::TraverserCallback& sink)
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
}


template <class Trav>
void DirTraverser::traverseWithException(const Zstring& dirPath, ABF::TraverserCallback& sink, DWORD volumeSerial /*may be 0!*/) //throw FileError, NeedFallbackToWin32Traverser
{
    //no need to check for endless recursion: Windows seems to have an internal path limit of about 700 chars

    typename Trav::DirHandle searchHandle = Trav::create(dirPath); //throw FileError
    ZEN_ON_SCOPE_EXIT(Trav::destroy(searchHandle));

    typename Trav::FindData findData = {};

    while (Trav::getEntry(searchHandle, dirPath, findData)) //throw FileError, NeedFallbackToWin32Traverser
        //don't retry but restart dir traversal on error! http://blogs.msdn.com/b/oldnewthing/archive/2014/06/12/10533529.aspx
    {
        //skip "." and ".."
        const Zchar* const shortName = Trav::getItemName(findData);

        if (shortName[0] == 0) throw FileError(replaceCpy(_("Cannot enumerate directory %x."), L"%x", fmtFileName(dirPath)), L"Trav::getEntry: Data corruption, found item without name."); //show error instead of endless recursion!!!
        if (shortName[0] == L'.' &&
            (shortName[1] == 0 || (shortName[1] == L'.' && shortName[2] == 0)))
            continue;

        const Zstring& itempath = appendSeparator(dirPath) + shortName;

        if (Trav::isSymlink(findData)) //check first!
        {
            const ABF::TraverserCallback::SymlinkInfo linkInfo = { shortName, Trav::getModTime(findData) };

            switch (sink.onSymlink(linkInfo))
            {
                case ABF::TraverserCallback::LINK_FOLLOW:
                    if (Trav::isDirectory(findData))
                    {
                        if (ABF::TraverserCallback* trav = sink.onDir({ shortName }))
                        {
                            ZEN_ON_SCOPE_EXIT(sink.releaseDirTraverser(trav));
                            traverse<Trav>(itempath, *trav, retrieveVolumeSerial(itempath)); //symlink may link to different volume => redetermine volume serial!
                        }
                    }
                    else //a file
                    {
                        SymlinkTargetInfo targetInfo= {};
                        const bool validLink = tryReportingItemError([&] //try to resolve symlink (and report error on failure!!!)
                        {
                            targetInfo = getInfoFromFileSymlink(itempath); //throw FileError
                        }, sink, shortName);

                        if (validLink)
                        {
                            ABF::TraverserCallback::FileInfo fi = { shortName, targetInfo.fileSize, targetInfo.lastWriteTime, convertToAbstractFileId(targetInfo.id), &linkInfo };
                            sink.onFile(fi);
                        }
                        // else //broken symlink -> ignore: it's client's responsibility to handle error!
                    }
                    break;

                case ABF::TraverserCallback::LINK_SKIP:
                    break;
            }
        }
        else if (Trav::isDirectory(findData))
        {
            if (ABF::TraverserCallback* trav = sink.onDir({ shortName }))
            {
                ZEN_ON_SCOPE_EXIT(sink.releaseDirTraverser(trav));
                traverse<Trav>(itempath, *trav, volumeSerial);
            }
        }
        else //a file
        {
            ABF::TraverserCallback::FileInfo fi = { shortName, Trav::getFileSize(findData), Trav::getModTime(findData), convertToAbstractFileId(Trav::getFileId(findData, volumeSerial)), nullptr };
            sink.onFile(fi); //init "fi" on separate line: VC 2013 compiler crash when using temporary!!!
        }
    }
}


#elif defined ZEN_LINUX || defined ZEN_MAC
class DirTraverser
{
public:
    static void execute(const Zstring& baseDirectory, ABF::TraverserCallback& sink)
    {
        DirTraverser(baseDirectory, sink);
    }

private:
    DirTraverser(const Zstring& baseDirectory, ABF::TraverserCallback& sink)
    {
        /* quote: "Since POSIX.1 does not specify the size of the d_name field, and other nonstandard fields may precede
                   that field within the dirent structure, portable applications that use readdir_r() should allocate
                   the buffer whose address is passed in entry as follows:
                       len = offsetof(struct dirent, d_name) + pathconf(dirPath, _PC_NAME_MAX) + 1
                       entryp = malloc(len); */
        const size_t nameMax = std::max<long>(::pathconf(baseDirectory.c_str(), _PC_NAME_MAX), 10000); //::pathconf may return long(-1)
        buffer.resize(offsetof(struct ::dirent, d_name) + nameMax + 1);

        traverse(baseDirectory, sink);
    }

    DirTraverser           (const DirTraverser&) = delete;
    DirTraverser& operator=(const DirTraverser&) = delete;

    void traverse(const Zstring& dirPath, ABF::TraverserCallback& sink)
    {
        tryReportingDirError([&]
        {
            traverseWithException(dirPath, sink); //throw FileError
        }, sink);
    }

    void traverseWithException(const Zstring& dirPath, ABF::TraverserCallback& sink) //throw FileError
    {
        //no need to check for endless recursion: Linux has a fixed limit on the number of symbolic links in a path

        DIR* dirObj = ::opendir(dirPath.c_str()); //directory must NOT end with path separator, except "/"
        if (!dirObj)
            throwFileError(replaceCpy(_("Cannot open directory %x."), L"%x", fmtFileName(dirPath)), L"opendir", getLastError());
        ZEN_ON_SCOPE_EXIT(::closedir(dirObj)); //never close nullptr handles! -> crash

        for (;;)
        {
            struct ::dirent* dirEntry = nullptr;
            if (::readdir_r(dirObj, reinterpret_cast< ::dirent*>(&buffer[0]), &dirEntry) != 0)
                throwFileError(replaceCpy(_("Cannot enumerate directory %x."), L"%x", fmtFileName(dirPath)), L"readdir_r", getLastError());
            //don't retry but restart dir traversal on error! http://blogs.msdn.com/b/oldnewthing/archive/2014/06/12/10533529.aspx

            if (!dirEntry) //no more items
                return;

            //don't return "." and ".."
            const char* shortName = dirEntry->d_name; //evaluate dirEntry *before* going into recursion => we use a single "buffer"!

            if (shortName[0] == 0) throw FileError(replaceCpy(_("Cannot enumerate directory %x."), L"%x", fmtFileName(dirPath)), L"readdir_r: Data corruption, found item without name.");
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
                        shortName = &bufferUtfDecomposed[0]; //attention: => don't access "shortName" after recursion in "traverse"!
                }
            }
            //const char* sampleDecomposed  = "\x6f\xcc\x81.txt";
            //const char* samplePrecomposed = "\xc3\xb3.txt";
#endif
            const Zstring& itempath = appendSeparator(dirPath) + shortName;

            struct ::stat statData = {};
            if (!tryReportingItemError([&]
        {
            if (::lstat(itempath.c_str(), &statData) != 0) //lstat() does not resolve symlinks
                    throwFileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(itempath)), L"lstat", getLastError());
            }, sink, shortName))
            continue; //ignore error: skip file

            if (S_ISLNK(statData.st_mode)) //on Linux there is no distinction between file and directory symlinks!
            {
                const ABF::TraverserCallback::SymlinkInfo linkInfo = { shortName, statData.st_mtime };

                switch (sink.onSymlink(linkInfo))
                {
                    case ABF::TraverserCallback::LINK_FOLLOW:
                    {
                        //try to resolve symlink (and report error on failure!!!)
                        struct ::stat statDataTrg = {};
                        bool validLink = tryReportingItemError([&]
                        {
                            if (::stat(itempath.c_str(), &statDataTrg) != 0)
                                throwFileError(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(itempath)), L"stat", getLastError());
                        }, sink, shortName);

                        if (validLink)
                        {
                            if (S_ISDIR(statDataTrg.st_mode)) //a directory
                            {
                                if (ABF::TraverserCallback* trav = sink.onDir({ shortName }))
                                {
                                    ZEN_ON_SCOPE_EXIT(sink.releaseDirTraverser(trav));
                                    traverse(itempath, *trav);
                                }
                            }
                            else //a file or named pipe, ect.
                            {
                                ABF::TraverserCallback::FileInfo fi = { shortName, makeUnsigned(statDataTrg.st_size), statDataTrg.st_mtime, convertToAbstractFileId(extractFileId(statDataTrg)), &linkInfo };
                                sink.onFile(fi);
                            }
                        }
                        // else //broken symlink -> ignore: it's client's responsibility to handle error!
                    }
                    break;

                    case ABF::TraverserCallback::LINK_SKIP:
                        break;
                }
            }
            else if (S_ISDIR(statData.st_mode)) //a directory
            {
                if (ABF::TraverserCallback* trav = sink.onDir({ shortName }))
                {
                    ZEN_ON_SCOPE_EXIT(sink.releaseDirTraverser(trav));
                    traverse(itempath, *trav);
                }
            }
            else //a file or named pipe, ect.
            {
                ABF::TraverserCallback::FileInfo fi = { shortName, makeUnsigned(statData.st_size), statData.st_mtime, convertToAbstractFileId(extractFileId(statData)), nullptr };
                sink.onFile(fi);
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
