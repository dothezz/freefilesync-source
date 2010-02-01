#include "recycler.h"
#include "dllLoader.h"
#include <wx/intl.h>
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "buildInfo.h"
#include "staticAssert.h"
#include <algorithm>
#include <functional>
//#include "../shared/longPathPrefix.h"

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

void FreeFileSync::moveToWindowsRecycler(const Zstring& fileToDelete)  //throw (FileError)
{
    std::vector<Zstring> fileNames;
    fileNames.push_back(fileToDelete);
    moveToWindowsRecycler(fileNames);  //throw (FileError)
}


void FreeFileSync::moveToWindowsRecycler(const std::vector<Zstring>& filesToDelete)  //throw (FileError)
{
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
            throw FileError(wxString(_("Error moving to Recycle Bin:")) + wxT("\n\"") + fileNames[0] + wxT("\"") + //report first file only... better than nothing
                            + wxT("\n\n") +
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

