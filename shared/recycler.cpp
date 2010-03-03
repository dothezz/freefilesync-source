// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "recycler.h"
#include "stringConv.h"
#include <wx/intl.h>
#include <stdexcept>

#ifdef FFS_WIN
#include "dllLoader.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "buildInfo.h"
#include "staticAssert.h"
#include <algorithm>
#include <functional>
#include <vector>
#include "longPathPrefix.h"

#elif defined FFS_LINUX
#include <sys/stat.h>

#ifdef RECYCLER_GIO
#include <gio/gio.h>
#include <boost/shared_ptr.hpp>
#endif

#endif


namespace
{
#ifdef FFS_WIN
const std::wstring& getRecyclerDllName()
{
    static const std::wstring filename(
        Utility::is64BitBuild ?
        L"Recycler_x64.dll":
        L"Recycler_Win32.dll");

    assert_static(Utility::is32BitBuild || Utility::is64BitBuild);

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
    using FreeFileSync::FileError;

    if (filesToDelete.empty())
        return;

    static const bool useIFileOperation = vistaOrLater();

    if (useIFileOperation) //new recycle bin usage: available since Vista
    {
        typedef bool (*MoveToRecycleBinFunc)(
            const wchar_t* fileNames[],
            size_t         fileNo, //size of fileNames array
            wchar_t*	   errorMessage,
            size_t         errorBufferLen);

        static const MoveToRecycleBinFunc moveToRecycler =
            Utility::loadDllFunction<MoveToRecycleBinFunc>(getRecyclerDllName().c_str(), "moveToRecycleBin");

        if (moveToRecycler == NULL)
            throw FileError(wxString(_("Could not load a required DLL:")) + wxT(" \"") + getRecyclerDllName().c_str() + wxT("\""));

        //#warning moving long file paths to recycler does not work! clarify!
//        std::vector<Zstring> temp;
//        std::transform(filesToDelete.begin(), filesToDelete.end(),
//                       std::back_inserter(temp), std::ptr_fun(FreeFileSync::removeLongPathPrefix)); //::IFileOperation() can't handle \\?\-prefix!

        std::vector<const wchar_t*> fileNames;
        std::transform(filesToDelete.begin(), filesToDelete.end(),
                       std::back_inserter(fileNames), std::mem_fun_ref(&Zstring::c_str));

        wchar_t errorMessage[2000];
        if (!(*moveToRecycler)(&fileNames[0], //array must not be empty
                               fileNames.size(),
                               errorMessage,
                               2000))
        {
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


void FreeFileSync::moveToRecycleBin(const Zstring& fileToDelete)  //throw (FileError)
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

#ifdef RECYCLER_GIO
    boost::shared_ptr<GFile> fileObj(g_file_new_for_path(fileToDelete.c_str()), &g_object_unref);
    GError* error = NULL;
    if (g_file_trash(fileObj.get(), NULL, &error) == FALSE)
    {
        if (!error)
            throw std::runtime_error("Recycle Bin failed but did not provide error information!");

        boost::shared_ptr<GError> errorObj(error, &g_error_free);

        //assemble error message
        const wxString errorMessage = wxString(wxT("Error Code ")) + wxString::Format(wxT("%i"), errorObj->code) +
                                      + wxT(", ") + wxString::FromUTF8(g_quark_to_string(errorObj->domain)) + wxT(": ") + wxString::FromUTF8(errorObj->message);
        throw FileError(wxString(_("Error moving to Recycle Bin:")) + wxT("\n\"") + zToWx(fileToDelete) + wxT("\"\n\n") +
                        wxT("(") + errorMessage + wxT(")"));
    }

#elif defined RECYCLER_NONE
    throw std::logic_error("No Recycler for this Linux version available at the moment!"); //user will never arrive here: recycler option cannot be activated in this case!
#endif

#endif
}


bool FreeFileSync::recycleBinExists()
{
#ifdef FFS_WIN
    return true;
#elif defined FFS_LINUX

#ifdef RECYCLER_GIO
    return true;
#elif defined RECYCLER_NONE
    return false;
#else
you have to choose a recycler!
#endif

#endif
}
