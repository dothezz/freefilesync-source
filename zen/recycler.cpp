// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "recycler.h"
#include <stdexcept>
#include <iterator>
#include <zen/file_handling.h>

#ifdef FFS_WIN
#include <algorithm>
#include <functional>
#include <vector>
#include <zen/dll.h>
#include <zen/win.h> //includes "windows.h"
#include <zen/assert_static.h>
#include <zen/win_ver.h>
#include <zen/long_path_prefix.h>
#include "IFileOperation/file_op.h"

#elif defined FFS_LINUX
#include <zen/scope_guard.h>
#include <sys/stat.h>
#include <gio/gio.h>
#endif

using namespace zen;


namespace
{
#ifdef FFS_WIN
/*
Performance test: delete 1000 files
------------------------------------
SHFileOperation - single file     33s
SHFileOperation - multiple files  2,1s
IFileOperation  - single file     33s
IFileOperation  - multiple files  2,1s

=> SHFileOperation and IFileOperation have nearly IDENTICAL performance characteristics!

Nevertheless, let's use IFileOperation for better error reporting!
*/
const bool useIFileOperation = vistaOrLater(); //caveat: function scope static initialization is not thread-safe in VS 2010!

//(try to) enhance error messages by showing which processed lock the file
Zstring getLockingProcessNames(const Zstring& filename) //throw(), empty string if none found or error occurred
{
    if (vistaOrLater())
    {
        using namespace fileop;
        const DllFun<FunType_getLockingProcesses> getLockingProcesses(getDllName(), funName_getLockingProcesses);
        const DllFun<FunType_freeString>          freeString         (getDllName(), funName_freeString);

        if (getLockingProcesses && freeString)
        {
            const wchar_t* procList = nullptr;
            if (getLockingProcesses(filename.c_str(), procList))
            {
                ZEN_ON_SCOPE_EXIT(freeString(procList));
                return procList;
            }
        }
    }
    return Zstring();
}
#endif
}


bool zen::moveToRecycleBin(const Zstring& filename)  //throw FileError
{
    if (!somethingExists(filename))
        return false; //neither file nor any other object with that name existing: no error situation, manual deletion relies on it!

#ifdef FFS_WIN
    //::SetFileAttributes(applyLongPathPrefix(filename).c_str(), FILE_ATTRIBUTE_NORMAL);
    //warning: moving long file paths to recycler does not work!
    //both ::SHFileOperation() and ::IFileOperation() cannot delete a folder named "System Volume Information" with normal attributes but shamelessly report success
    //both ::SHFileOperation() and ::IFileOperation() can't handle \\?\-prefix!

    if (useIFileOperation) //new recycle bin usage: available since Vista
    {
        using namespace fileop;
        const DllFun<FunType_moveToRecycleBin> moveToRecycler(getDllName(), funName_moveToRecycleBin);
        const DllFun<FunType_getLastError>     getLastError  (getDllName(), funName_getLastError);

        if (!moveToRecycler || !getLastError)
            throw FileError(replaceCpy(_("Unable to move %x to the Recycle Bin!"), L"%x", fmtFileName(filename)) + L"\n\n" +
                            replaceCpy(_("Cannot load file %x."), L"%x", fmtFileName(getDllName())));

        std::vector<const wchar_t*> filenames;
        filenames.push_back(filename.c_str());

        if (!moveToRecycler(&filenames[0], filenames.size()))
        {
            const std::wstring shortMsg = replaceCpy(_("Unable to move %x to the Recycle Bin!"), L"%x", fmtFileName(filename));

            //if something is locking our file -> emit better error message!
            const Zstring procList = getLockingProcessNames(filename); //throw()
            if (!procList.empty())
                throw FileError(shortMsg + L"\n\n" + _("The file is locked by another process:") + L"\n" + procList);

            throw FileError(shortMsg + L"\n\n" + getLastError());
        }
    }
    else //regular recycle bin usage: available since XP
    {
        const Zstring& filenameDoubleNull = filename + L'\0';

        SHFILEOPSTRUCT fileOp = {};
        fileOp.hwnd   = nullptr;
        fileOp.wFunc  = FO_DELETE;
        fileOp.pFrom  = filenameDoubleNull.c_str();
        fileOp.pTo    = nullptr;
        fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
        fileOp.fAnyOperationsAborted = false;
        fileOp.hNameMappings         = nullptr;
        fileOp.lpszProgressTitle     = nullptr;

        //"You should use fully-qualified path names with this function. Using it with relative path names is not thread safe."
        if (::SHFileOperation(&fileOp) != 0 || fileOp.fAnyOperationsAborted)
        {
            throw FileError(replaceCpy(_("Unable to move %x to the Recycle Bin!"), L"%x", fmtFileName(filename)));
        }
    }

#elif defined FFS_LINUX
    GFile* file = g_file_new_for_path(filename.c_str()); //never fails according to docu
    ZEN_ON_SCOPE_EXIT(g_object_unref(file);)

    GError* error = nullptr;
    ZEN_ON_SCOPE_EXIT(if (error) g_error_free(error););

    if (!g_file_trash(file, nullptr, &error))
    {
        const std::wstring shortMsg = replaceCpy(_("Unable to move %x to the Recycle Bin!"), L"%x", fmtFileName(filename));

        if (!error)
            throw FileError(shortMsg + L"\n\n" + L"Unknown error.");

        //implement same behavior as in Windows: if recycler is not existing, delete permanently
        if (error->code == G_IO_ERROR_NOT_SUPPORTED)
        {
            struct stat fileInfo = {};
            if (::lstat(filename.c_str(), &fileInfo) != 0)
                return false;

            if (S_ISLNK(fileInfo.st_mode) || S_ISREG(fileInfo.st_mode))
                removeFile(filename); //throw FileError
            else if (S_ISDIR(fileInfo.st_mode))
                removeDirectory(filename); //throw FileError
            return true;
        }

        throw FileError(shortMsg + L"\n\n" + L"Glib Error Code " + numberTo<std::wstring>(error->code) + /* L", " +
                                          g_quark_to_string(error->domain) + */ L": " + utfCvrtTo<std::wstring>(error->message));
    }
#endif
    return true;
}


#ifdef FFS_WIN
zen::StatusRecycler zen::recycleBinStatus(const Zstring& pathName)
{
    warn_static("fix XP not working + finish");

    /*
    const bool canUseFastCheckForRecycler = winXpOrLater();
        if (!canUseFastCheckForRecycler) //== "checkForRecycleBin"
            return STATUS_REC_UNKNOWN;

        using namespace fileop;
        const DllFun<FunType_checkRecycler> checkRecycler(getDllName(), funName_checkRecycler);

        if (!checkRecycler)
            return STATUS_REC_UNKNOWN; //actually an error since we're >= XP

        const DWORD bufferSize = MAX_PATH + 1;
        std::vector<wchar_t> buffer(bufferSize);
        if (!::GetVolumePathName(pathName.c_str(), //__in   LPCTSTR lpszFileName,
                                 &buffer[0],       //__out  LPTSTR lpszVolumePathName,
                                 bufferSize))      //__in   DWORD cchBufferLength
            return STATUS_REC_UNKNOWN;

        Zstring rootPathPf = appendSeparator(&buffer[0]);

        //search root directories for recycle bin folder...
        //we would prefer to use CLSID_RecycleBinManager beginning with Vista... if this interface were documented

        //caveat: "subst"-alias of volume contains "$Recycle.Bin" although it is not available!!!
        warn_static("check")

        WIN32_FIND_DATA dataRoot = {};
        HANDLE hFindRoot = ::FindFirstFile(applyLongPathPrefix(rootPathPf + L'*').c_str(), &dataRoot);
        if (hFindRoot == INVALID_HANDLE_VALUE)
            return STATUS_REC_UNKNOWN;
        ZEN_ON_SCOPE_EXIT(FindClose(hFindRoot));

        auto shouldSkip = [](const Zstring& shortname) { return shortname == L"."  || shortname == L".."; };

        do
        {
            if (!shouldSkip(dataRoot.cFileName) &&
                (dataRoot.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
                (dataRoot.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM   ) != 0 && //maybe a little risky to rely on these attributes, there may be a recycler
                (dataRoot.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN   ) != 0)   //(in obscure cases) we don't find, but that's better than the other way round
            {
                WIN32_FIND_DATA dataChild = {};
                const Zstring childDirPf = rootPathPf + dataRoot.cFileName + L"\\";

                HANDLE hFindChild = ::FindFirstFile(applyLongPathPrefix(childDirPf + L'*').c_str(), &dataChild);
                if (hFindChild != INVALID_HANDLE_VALUE) //if we can't access a subdir, it's probably not the recycler
                {
                    ZEN_ON_SCOPE_EXIT(FindClose(hFindChild));
                    do
                    {
                        if (!shouldSkip(dataChild.cFileName) &&
                            (dataChild.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
                        {
                            bool isRecycler = false;
                            if (checkRecycler((childDirPf + dataChild.cFileName).c_str(), isRecycler))
                            {
                                if (isRecycler)
                                    return STATUS_REC_EXISTS;
                            }
                            else assert(false);
                        }
                    }
                    while (::FindNextFile(hFindChild, &dataChild)); //ignore errors other than ERROR_NO_MORE_FILES
                }
            }
        }
        while (::FindNextFile(hFindRoot, &dataRoot)); //

        return STATUS_REC_MISSING;

    */

    const DWORD bufferSize = MAX_PATH + 1;
    std::vector<wchar_t> buffer(bufferSize);
    if (!::GetVolumePathName(pathName.c_str(), //__in   LPCTSTR lpszFileName,
                             &buffer[0],       //__out  LPTSTR lpszVolumePathName,
                             bufferSize))      //__in   DWORD cchBufferLength
        return STATUS_REC_UNKNOWN;

    const Zstring rootPathPf = appendSeparator(&buffer[0]);

    SHQUERYRBINFO recInfo = {};
    recInfo.cbSize = sizeof(recInfo);
    HRESULT rv = ::SHQueryRecycleBin(rootPathPf.c_str(), //__in_opt  LPCTSTR pszRootPath,
                                     &recInfo);          //__inout   LPSHQUERYRBINFO pSHQueryRBInfo
    //traverses whole C:\$Recycle.Bin directory each time!!!!

    return rv == S_OK ? STATUS_REC_EXISTS : STATUS_REC_MISSING;
}

#elif defined FFS_LINUX
/*
We really need access to a similar function to check whether a directory supports trashing and emit a warning if it does not!

The following function looks perfect, alas it is restricted to local files and to the implementation of GIO only:

    gboolean _g_local_file_has_trash_dir(const char* dirname, dev_t dir_dev);
    See: http://www.netmite.com/android/mydroid/2.0/external/bluetooth/glib/gio/glocalfileinfo.h

    Just checking for "G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH" is not correct, since we find in
    http://www.netmite.com/android/mydroid/2.0/external/bluetooth/glib/gio/glocalfileinfo.c

            g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH,
                                           writable && parent_info->has_trash_dir);

    => We're NOT interested in whether the specified folder can be trashed, but whether it supports thrashing its child elements! (Only support, not actual write access!)
    This renders G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH useless for this purpose.
*/
#endif
