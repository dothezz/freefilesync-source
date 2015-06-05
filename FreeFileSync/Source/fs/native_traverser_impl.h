// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include <zen/sys_error.h>
#include <zen/symlink_target.h>
#include <zen/int64.h>

#include <cstddef> //offsetof
#include <sys/stat.h>
#include <dirent.h>

//implementation header for native.cpp, not for reuse!!!

namespace
{
using namespace zen;
using ABF = AbstractBaseFolder;


inline
ABF::FileId convertToAbstractFileId(const zen::FileId& fid)
{
    if (fid == zen::FileId())
        return ABF::FileId();

    ABF::FileId out(reinterpret_cast<const char*>(&fid.first), sizeof(fid.first));
    out.append(reinterpret_cast<const char*>(&fid.second), sizeof(fid.second));
    return out;
}


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
};
}
