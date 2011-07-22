// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "recycler.h"
#include <stdexcept>
#include <iterator>
#include "i18n.h"

#ifdef FFS_WIN
#include "dll_loader.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "build_info.h"
#include "assert_static.h"
#include <algorithm>
#include <functional>
#include <vector>
#include "long_path_prefix.h"
#include "IFileOperation/file_op.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#include <giomm/file.h>
#endif

using namespace zen;


namespace
{
#ifdef FFS_WIN
inline
std::wstring getRecyclerDllName()
{
    assert_static(util::is32BitBuild || util::is64BitBuild);

    return util::is64BitBuild ?
           L"FileOperation_x64.dll":
           L"FileOperation_Win32.dll";
}


bool vistaOrLater()
{
    OSVERSIONINFO osvi = {};
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    //IFileOperation is supported with Vista and later
    if (GetVersionEx(&osvi))
        return osvi.dwMajorVersion > 5;
    //XP    has majorVersion == 5, minorVersion == 1
    //Vista has majorVersion == 6, minorVersion == 0
    //version overview: http://msdn.microsoft.com/en-us/library/ms724834(VS.85).aspx
    return false;
}

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

void moveToWindowsRecycler(const std::vector<Zstring>& filesToDelete)  //throw (FileError)
{
    using zen::FileError;

    if (filesToDelete.empty())
        return;

    static const bool useIFileOperation = vistaOrLater();

    if (useIFileOperation) //new recycle bin usage: available since Vista
    {
        std::vector<const wchar_t*> fileNames;
        std::transform(filesToDelete.begin(), filesToDelete.end(),
                       std::back_inserter(fileNames), std::mem_fun_ref(&Zstring::c_str));

        using namespace fileop;

        static MoveToRecycleBinFct moveToRecycler = NULL;
        if (!moveToRecycler)
            moveToRecycler = util::getDllFun<MoveToRecycleBinFct>(getRecyclerDllName(), moveToRecycleBinFctName);

        static GetLastErrorFct getLastError = NULL;
        if (!getLastError)
            getLastError = util::getDllFun<GetLastErrorFct>(getRecyclerDllName(), getLastErrorFctName);

        if (moveToRecycler == NULL || getLastError == NULL)
            throw FileError(_("Error moving to Recycle Bin:") + "\n\"" + fileNames[0] + "\"" + //report first file only... better than nothing
                            "\n\n" + _("Could not load a required DLL:") + " \"" + getRecyclerDllName() + "\"");

        //#warning moving long file paths to recycler does not work! clarify!
        //        std::vector<Zstring> temp;
        //        std::transform(filesToDelete.begin(), filesToDelete.end(),
        //                       std::back_inserter(temp), std::ptr_fun(zen::removeLongPathPrefix)); //::IFileOperation() can't handle \\?\-prefix!

        if (!moveToRecycler(&fileNames[0], //array must not be empty
                            fileNames.size()))
        {
            wchar_t errorMessage[2000];
            getLastError(errorMessage, 2000);
            throw FileError(_("Error moving to Recycle Bin:") + "\n\"" + fileNames[0] + "\"" + //report first file only... better than nothing
                            "\n\n" + "(" + errorMessage + ")");
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
            filenameDoubleNull += Zchar(0);
        }

        SHFILEOPSTRUCT fileOp = {};
        fileOp.hwnd   = NULL;
        fileOp.wFunc  = FO_DELETE;
        fileOp.pFrom  = filenameDoubleNull.c_str();
        fileOp.pTo    = NULL;
        fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
        fileOp.fAnyOperationsAborted = false;
        fileOp.hNameMappings         = NULL;
        fileOp.lpszProgressTitle     = NULL;

        if (::SHFileOperation(&fileOp) != 0 || fileOp.fAnyOperationsAborted)
        {
            throw FileError(_("Error moving to Recycle Bin:") + "\n\"" + filenameDoubleNull.c_str() + "\""); //report first file only... better than nothing
        }
    }
}
#endif
}


bool zen::moveToRecycleBin(const Zstring& fileToDelete)  //throw (FileError)
{
#ifdef FFS_WIN
    const Zstring filenameFmt = applyLongPathPrefix(fileToDelete);

    const DWORD attr = ::GetFileAttributes(filenameFmt.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES)
        return false; //neither file nor any other object with that name existing: no error situation, manual deletion relies on it!
    //::SetFileAttributes(filenameFmt.c_str(), FILE_ATTRIBUTE_NORMAL);

    //both SHFileOperation and useIFileOperation are not able to delete a folder named "System Volume Information" with normal attributes but shamelessly report success

    std::vector<Zstring> fileNames;
    fileNames.push_back(fileToDelete);
    ::moveToWindowsRecycler(fileNames);  //throw (FileError)

#elif defined FFS_LINUX
    struct stat fileInfo = {};
    if (::lstat(fileToDelete.c_str(), &fileInfo) != 0)
        return false; //neither file nor any other object with that name existing: no error situation, manual deletion relies on it!

    Glib::RefPtr<Gio::File> fileObj = Gio::File::create_for_path(fileToDelete.c_str()); //never fails
    try
    {
        if (!fileObj->trash())
            throw FileError(_("Error moving to Recycle Bin:") + "\n\"" + fileToDelete + "\"" +
                            "\n\n" + "(unknown error)");
    }
    catch (const Glib::Error& errorObj)
    {
        //assemble error message
        const std::wstring errorMessage = L"Glib Error Code " + toString<std::wstring>(errorObj.code()) + ", " +
                                          g_quark_to_string(errorObj.domain()) + ": " + errorObj.what();

        throw FileError(_("Error moving to Recycle Bin:") + "\n\"" + fileToDelete + "\"" +
                        "\n\n" + "(" + errorMessage + ")");
    }
#endif
    return true;
}


#ifdef FFS_WIN
zen::StatusRecycler zen::recycleBinExists(const Zstring& pathName)
{
    std::vector<wchar_t> buffer(MAX_PATH + 1);
    if (::GetVolumePathName(applyLongPathPrefix(pathName).c_str(), //__in   LPCTSTR lpszFileName,
                            &buffer[0],                            //__out  LPTSTR lpszVolumePathName,
                            static_cast<DWORD>(buffer.size())))    //__in   DWORD cchBufferLength
    {
        Zstring rootPath =&buffer[0];
        if (!rootPath.EndsWith(FILE_NAME_SEPARATOR)) //a trailing backslash is required
            rootPath += FILE_NAME_SEPARATOR;

        SHQUERYRBINFO recInfo = {};
        recInfo.cbSize = sizeof(recInfo);
        HRESULT rv = ::SHQueryRecycleBin(rootPath.c_str(), //__in_opt  LPCTSTR pszRootPath,
                                         &recInfo);        //__inout   LPSHQUERYRBINFO pSHQueryRBInfo
        return rv == S_OK ? STATUS_REC_EXISTS : STATUS_REC_MISSING;
    }
    return STATUS_REC_UNKNOWN;
}
#endif
