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
#include <boost/thread/once.hpp>
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

void moveToWindowsRecycler(const std::vector<Zstring>& filesToDelete)  //throw FileError
{
    if (filesToDelete.empty())
        return;

    static bool useIFileOperation = false;
    static boost::once_flag once = BOOST_ONCE_INIT; //caveat: function scope static initialization is not thread-safe in VS 2010!
    boost::call_once(once, [] { useIFileOperation = vistaOrLater(); });

    if (useIFileOperation) //new recycle bin usage: available since Vista
    {
        std::vector<const wchar_t*> fileNames;
        std::transform(filesToDelete.begin(), filesToDelete.end(),
                       std::back_inserter(fileNames), std::mem_fun_ref(&Zstring::c_str));

        using namespace fileop;
        const DllFun<MoveToRecycleBinFct> moveToRecycler(getDllName(), moveToRecycleBinFctName);
        const DllFun<GetLastErrorFct>     getLastError  (getDllName(), getLastErrorFctName);

        if (!moveToRecycler || !getLastError)
            throw FileError(replaceCpy(_("Unable to move %x to the Recycle Bin!"), L"%x", std::wstring(L"\"") + fileNames[0] + L"\"") + L"\n\n" + //report first file only... better than nothing
                            replaceCpy(_("Cannot load file %x."), L"%x", std::wstring(L"\"") + getDllName() + L"\""));

        //#warning moving long file paths to recycler does not work! clarify!
        //        std::vector<Zstring> temp;
        //        std::transform(filesToDelete.begin(), filesToDelete.end(),
        //                       std::back_inserter(temp), std::ptr_fun(zen::removeLongPathPrefix)); //::IFileOperation() can't handle \\?\-prefix!

        if (!moveToRecycler(&fileNames[0], //array must not be empty
                            fileNames.size()))
        {
            std::vector<wchar_t> msgBuffer(2000);
            getLastError(&msgBuffer[0], msgBuffer.size());

            throw FileError(replaceCpy(_("Unable to move %x to the Recycle Bin!"), L"%x", std::wstring(L"\"") + fileNames[0] + L"\"") + L"\n\n" + //report first file only... better than nothing
                            &msgBuffer[0]);
        }
    }
    else //regular recycle bin usage: available since XP
    {
        Zstring filenameDoubleNull;
        for (std::vector<Zstring>::const_iterator i = filesToDelete.begin(); i != filesToDelete.end(); ++i)
        {
            //#warning moving long file paths to recycler does not work! clarify!
            //filenameDoubleNull += removeLongPathPrefix(*i); //::SHFileOperation() can't handle \\?\-prefix!
            //You should use fully-qualified path names with this function. Using it with relative path names is not thread safe.
            filenameDoubleNull += *i; //::SHFileOperation() can't handle \\?\-prefix!
            filenameDoubleNull += L'\0';
        }

        SHFILEOPSTRUCT fileOp = {};
        fileOp.hwnd   = nullptr;
        fileOp.wFunc  = FO_DELETE;
        fileOp.pFrom  = filenameDoubleNull.c_str();
        fileOp.pTo    = nullptr;
        fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
        fileOp.fAnyOperationsAborted = false;
        fileOp.hNameMappings         = nullptr;
        fileOp.lpszProgressTitle     = nullptr;

        if (::SHFileOperation(&fileOp) != 0 || fileOp.fAnyOperationsAborted)
        {
            throw FileError(replaceCpy(_("Unable to move %x to the Recycle Bin!"), L"%x", std::wstring(L"\"") + filenameDoubleNull.c_str() + L"\"")); //report first file only... better than nothing
        }
    }
}
#endif
}


bool zen::moveToRecycleBin(const Zstring& filename)  //throw FileError
{
    if (!somethingExists(filename))
        return false; //neither file nor any other object with that name existing: no error situation, manual deletion relies on it!

#ifdef FFS_WIN
    //::SetFileAttributes(applyLongPathPrefix(filename).c_str(), FILE_ATTRIBUTE_NORMAL);
    //both SHFileOperation and IFileOperation are not able to delete a folder named "System Volume Information" with normal attributes but shamelessly report success

    std::vector<Zstring> fileNames;
    fileNames.push_back(filename);
    ::moveToWindowsRecycler(fileNames);  //throw FileError

#elif defined FFS_LINUX
    GFile* file = g_file_new_for_path(filename.c_str()); //never fails according to docu
    ZEN_ON_SCOPE_EXIT(g_object_unref(file);)

    GError* error = nullptr;
    ZEN_ON_SCOPE_EXIT(if (error) g_error_free(error););

    if (!g_file_trash(file, nullptr, &error))
    {
        if (!error)
            throw FileError(replaceCpy(_("Unable to move %x to the Recycle Bin!"), L"%x", L"\"" + filename + L"\"") + L"\n\n" +
                            L"Unknown error.");

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

        //assemble error message
        const std::wstring errorMessage = L"Glib Error Code " + numberTo<std::wstring>(error->code) + /* L", " +
                                          g_quark_to_string(error->domain) + */ L": " + utf8CvrtTo<std::wstring>(error->message);

        throw FileError(replaceCpy(_("Unable to move %x to the Recycle Bin!"), L"%x", L"\"" + filename + L"\"") + L"\n\n" +
                        errorMessage);
    }
#endif
    return true;
}


#ifdef FFS_WIN
zen::StatusRecycler zen::recycleBinStatus(const Zstring& pathName)
{
    const DWORD bufferSize = MAX_PATH + 1;
    std::vector<wchar_t> buffer(bufferSize);
    if (::GetVolumePathName(pathName.c_str(), //__in   LPCTSTR lpszFileName,
                            &buffer[0],       //__out  LPTSTR lpszVolumePathName,
                            bufferSize))      //__in   DWORD cchBufferLength
    {
        Zstring rootPath = &buffer[0];
        if (!endsWith(rootPath, FILE_NAME_SEPARATOR)) //a trailing backslash is required
            rootPath += FILE_NAME_SEPARATOR;

        SHQUERYRBINFO recInfo = {};
        recInfo.cbSize = sizeof(recInfo);
        HRESULT rv = ::SHQueryRecycleBin(rootPath.c_str(), //__in_opt  LPCTSTR pszRootPath,
                                         &recInfo);        //__inout   LPSHQUERYRBINFO pSHQueryRBInfo
        return rv == S_OK ? STATUS_REC_EXISTS : STATUS_REC_MISSING;
    }
    return STATUS_REC_UNKNOWN;
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
