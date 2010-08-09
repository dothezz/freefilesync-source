// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "recycler.h"
#include "string_conv.h"
#include <wx/intl.h>
#include <stdexcept>
#include <iterator>

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


namespace
{
#ifdef FFS_WIN
const std::wstring& getRecyclerDllName()
{
    static const std::wstring filename(
        util::is64BitBuild ?
        L"FileOperation_x64.dll":
        L"FileOperation_Win32.dll");

    assert_static(util::is32BitBuild || util::is64BitBuild);

    return filename;
}


bool vistaOrLater()
{
    OSVERSIONINFO osvi;
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
    using ffs3::FileError;

    if (filesToDelete.empty())
        return;

    static const bool useIFileOperation = vistaOrLater();

    if (useIFileOperation) //new recycle bin usage: available since Vista
    {
                std::vector<const wchar_t*> fileNames;
        std::transform(filesToDelete.begin(), filesToDelete.end(),
                       std::back_inserter(fileNames), std::mem_fun_ref(&Zstring::c_str));

        using namespace FileOp;

        static const MoveToRecycleBinFct moveToRecycler =
            util::loadDllFunction<MoveToRecycleBinFct>(getRecyclerDllName().c_str(), moveToRecycleBinFctName);

        static const GetLastErrorFct getLastError =
            util::loadDllFunction<GetLastErrorFct>(getRecyclerDllName().c_str(), getLastErrorFctName);

        if (moveToRecycler == NULL || getLastError == NULL)
            throw FileError(wxString(_("Error moving to Recycle Bin:")) + wxT("\n\"") + fileNames[0] + wxT("\"\n\n") + //report first file only... better than nothing
                            wxString(_("Could not load a required DLL:")) + wxT(" \"") + getRecyclerDllName().c_str() + wxT("\""));

        //#warning moving long file paths to recycler does not work! clarify!
//        std::vector<Zstring> temp;
//        std::transform(filesToDelete.begin(), filesToDelete.end(),
//                       std::back_inserter(temp), std::ptr_fun(ffs3::removeLongPathPrefix)); //::IFileOperation() can't handle \\?\-prefix!

        if (!(*moveToRecycler)(&fileNames[0], //array must not be empty
                               fileNames.size()))
        {
            wchar_t errorMessage[2000];
            (*getLastError)(errorMessage, 2000);
            throw FileError(wxString(_("Error moving to Recycle Bin:")) + wxT("\n\"") + fileNames[0] + wxT("\"\n\n") + //report first file only... better than nothing
                            wxT("(") + errorMessage + wxT(")"));
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
            filenameDoubleNull += DefaultChar(0);
        }

        SHFILEOPSTRUCT fileOp;
        fileOp.hwnd   = NULL;
        fileOp.wFunc  = FO_DELETE;
        fileOp.pFrom  = filenameDoubleNull.c_str();
        fileOp.pTo    = NULL;
        fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
        fileOp.fAnyOperationsAborted = false;
        fileOp.hNameMappings         = NULL;
        fileOp.lpszProgressTitle     = NULL;

        if (SHFileOperation(&fileOp) != 0 || fileOp.fAnyOperationsAborted)
        {
            throw FileError(wxString(_("Error moving to Recycle Bin:")) + wxT("\n\"") + filenameDoubleNull.c_str() + wxT("\"")); //report first file only... better than nothing
        }
    }
}
#endif
}


void ffs3::moveToRecycleBin(const Zstring& fileToDelete)  //throw (FileError)
{
#ifdef FFS_WIN
    const Zstring filenameFmt = applyLongPathPrefix(fileToDelete);
    if (::GetFileAttributes(filenameFmt.c_str()) == INVALID_FILE_ATTRIBUTES)
        return; //neither file nor any other object with that name existing: no error situation, manual deletion relies on it!

    std::vector<Zstring> fileNames;
    fileNames.push_back(fileToDelete);
    ::moveToWindowsRecycler(fileNames);  //throw (FileError)

#elif defined FFS_LINUX
    struct stat fileInfo;
    if (::lstat(fileToDelete.c_str(), &fileInfo) != 0)
        return; //neither file nor any other object with that name existing: no error situation, manual deletion relies on it!


    Glib::RefPtr<Gio::File> fileObj = Gio::File::create_for_path(fileToDelete.c_str()); //never fails

    try
    {
        if (!fileObj->trash())
        throw FileError(wxString(_("Error moving to Recycle Bin:")) + wxT("\n\"") + zToWx(fileToDelete) + wxT("\"\n\n") +
                        wxT("(") + wxT("unknown error") + wxT(")"));
    }
    catch (const Glib::Error& errorObj)
    {
        //assemble error message
        const wxString errorMessage = wxString(wxT("Glib Error Code ")) + wxString::Format(wxT("%i"), errorObj.code()) + wxT(", ") +
                                      wxString::FromUTF8(g_quark_to_string(errorObj.domain())) + wxT(": ") + wxString::FromUTF8(errorObj.what().c_str());

        throw FileError(wxString(_("Error moving to Recycle Bin:")) + wxT("\n\"") + zToWx(fileToDelete) + wxT("\"\n\n") +
                        wxT("(") + errorMessage + wxT(")"));
    }
#endif
}


bool ffs3::recycleBinExists()
{
#ifdef FFS_WIN
    return true;
#elif defined FFS_LINUX
    return true;
#endif
}
