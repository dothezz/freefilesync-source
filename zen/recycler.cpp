// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "recycler.h"
//#include <stdexcept>
//#include <iterator>
#include <zen/file_handling.h>

#ifdef ZEN_WIN
//#include <algorithm>
//#include <functional>
#include <zen/dll.h>
#include <zen/assert_static.h>
#include <zen/win_ver.h>
#include <zen/long_path_prefix.h>
#include "IFileOperation/file_op.h"

#elif defined ZEN_LINUX
#include <zen/scope_guard.h>
#include <sys/stat.h>
#include <gio/gio.h>

#elif defined ZEN_MAC
#include <CoreServices/CoreServices.h>
#endif

using namespace zen;


#ifdef ZEN_WIN
namespace
{
/*
Performance test: delete 1000 files
------------------------------------
SHFileOperation - single file     33s
SHFileOperation - multiple files  2,1s
IFileOperation  - single file     33s
IFileOperation  - multiple files  2,1s

=> SHFileOperation and IFileOperation have nearly IDENTICAL performance characteristics!

Nevertheless, let's use IFileOperation for better error reporting (including details on locked files)!
*/
const bool useIFileOperation = vistaOrLater(); //caveat: function scope static initialization is not thread-safe in VS 2010!

struct CallbackData
{
    CallbackData(CallbackRecycling* cb) :
        userCallback(cb),
        exceptionInUserCallback(false) {}

    CallbackRecycling* const userCallback; //optional!
    bool exceptionInUserCallback;
};

bool recyclerCallback(const wchar_t* filename, void* sink)
{
    CallbackData& cbd = *static_cast<CallbackData*>(sink); //sink is NOT optional here

    if (cbd.userCallback)
        try
        {
            cbd.userCallback->updateStatus(filename); //throw ?
        }
        catch (...)
        {
            cbd.exceptionInUserCallback = true; //try again outside the C call stack!
            return false;
        }
    return true;
}
}

void zen::recycleOrDelete(const std::vector<Zstring>& filenames, CallbackRecycling* callback)
{
    if (filenames.empty())
        return;
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
            throw FileError(replaceCpy(_("Unable to move %x to the recycle bin."), L"%x", fmtFileName(filenames[0])),
                            replaceCpy(_("Cannot load file %x."), L"%x", fmtFileName(getDllName())));

        std::vector<const wchar_t*> cNames;
        for (auto it = filenames.begin(); it != filenames.end(); ++it) //CAUTION: to not create temporary strings here!!
            cNames.push_back(it->c_str());

        CallbackData cbd(callback);
        if (!moveToRecycler(&cNames[0], cNames.size(), recyclerCallback, &cbd))
        {
            if (cbd.exceptionInUserCallback) //now we may throw...
                callback->updateStatus(Zstring()); //should throw again!

            std::wstring filenameFmt = fmtFileName(filenames[0]); //probably not the correct file name for file lists larger than 1!
            if (filenames.size() > 1)
                filenameFmt += L", ..."; //give at least some hint that there are multiple files, and the error need not be related to the first one

            throw FileError(replaceCpy(_("Unable to move %x to the recycle bin."), L"%x", filenameFmt), getLastError()); //already includes details about locking errors!
        }
    }
    else //regular recycle bin usage: available since XP
    {
        Zstring filenamesDoubleNull;
        for (const Zstring& filename : filenames)
        {
            filenamesDoubleNull += filename;
            filenamesDoubleNull += L'\0';
        }

        SHFILEOPSTRUCT fileOp = {};
        fileOp.hwnd   = nullptr;
        fileOp.wFunc  = FO_DELETE;
        fileOp.pFrom  = filenamesDoubleNull.c_str();
        fileOp.pTo    = nullptr;
        fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
        fileOp.fAnyOperationsAborted = false;
        fileOp.hNameMappings         = nullptr;
        fileOp.lpszProgressTitle     = nullptr;

        //"You should use fully-qualified path names with this function. Using it with relative path names is not thread safe."
        if (::SHFileOperation(&fileOp) != 0 || fileOp.fAnyOperationsAborted)
        {
            throw FileError(replaceCpy(_("Unable to move %x to the recycle bin."), L"%x", fmtFileName(filenames[0]))); //probably not the correct file name for file list larger than 1!
        }
    }
}
#endif


bool zen::recycleOrDelete(const Zstring& filename) //throw FileError
{
    if (!somethingExists(filename)) //[!] do not optimize away, OS X needs this for reliable detection of "recycle bin missing"
        return false; //neither file nor any other object with that name existing: no error situation, manual deletion relies on it!

#ifdef ZEN_WIN
    std::vector<Zstring> filenames;
    filenames.push_back(filename);
    recycleOrDelete(filenames, nullptr); //throw FileError

#elif defined ZEN_LINUX
    GFile* file = ::g_file_new_for_path(filename.c_str()); //never fails according to docu
    ZEN_ON_SCOPE_EXIT(g_object_unref(file);)

    GError* error = nullptr;
    ZEN_ON_SCOPE_EXIT(if (error) ::g_error_free(error););

    if (!::g_file_trash(file, nullptr, &error))
    {
        const std::wstring errorMsg = replaceCpy(_("Unable to move %x to the recycle bin."), L"%x", fmtFileName(filename));

        if (!error)
            throw FileError(errorMsg, L"Unknown error."); //user should never see this

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

        throw FileError(errorMsg, replaceCpy<std::wstring>(L"Glib Error Code %x:", L"%x", numberTo<std::wstring>(error->code)) + L" " + utfCvrtTo<std::wstring>(error->message));
        //g_quark_to_string(error->domain)
    }

#elif defined ZEN_MAC
    //we cannot use FSPathMoveObjectToTrashSync directly since it follows symlinks!

    assert_static(sizeof(Zchar) == sizeof(char));
    const UInt8* filenameUtf8 = reinterpret_cast<const UInt8*>(filename.c_str());

    auto throwFileError = [&](OSStatus oss)
    {
        const std::wstring errorMsg = replaceCpy(_("Unable to move %x to the recycle bin."), L"%x", fmtFileName(filename));
        std::wstring errorDescr = L"OSStatus Code " + numberTo<std::wstring>(oss);

        if (const char* description = ::GetMacOSStatusCommentString(oss)) //found no documentation for proper use of GetMacOSStatusCommentString
            errorDescr += L": " + utfCvrtTo<std::wstring>(description);
        throw FileError(errorMsg, errorDescr);
    };

    FSRef objectRef;
    OSStatus rv = ::FSPathMakeRefWithOptions(filenameUtf8, //const UInt8 *path,
                                             kFSPathMakeRefDoNotFollowLeafSymlink, //OptionBits options,
                                             &objectRef,  //FSRef *ref,
                                             nullptr);    //Boolean *isDirectory
    if (rv != noErr)
        throwFileError(rv);

    //deprecated since OS X 10.8!!! "trashItemAtURL" should be used instead
    OSStatus rv2 = ::FSMoveObjectToTrashSync(&objectRef, //const FSRef *source,
                                             nullptr,    //FSRef *target,
                                             kFSFileOperationDefaultOptions); //OptionBits options
    if (rv2 != noErr)
    {
        //implement same behavior as in Windows: if recycler is not existing, delete permanently
        if (rv2 == -120) //=="Directory not found or incomplete pathname." but should really be "recycle bin directory not found"!
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

        throwFileError(rv2);
    }
#endif
    return true;
}


#ifdef ZEN_WIN
StatusRecycler zen::recycleBinStatus(const Zstring& pathName)
{
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

    return rv == S_OK ? STATUS_REC_EXISTS : STATUS_REC_MISSING;
    //1. excessive: traverses whole C:\$Recycle.Bin directory tree each time!!!! But it's safe and correct.

    //2. we would prefer to use CLSID_RecycleBinManager beginning with Vista... if only this interface were documented!!!

    //3. check directory existence of "C:\$Recycle.Bin, C:\RECYCLER, C:\RECYCLED"
    // -> not upward-compatible, wrong result for subst-alias: recycler assumed existing, although it is not!

    //4. alternative approach a'la Raymond Chen: http://blogs.msdn.com/b/oldnewthing/archive/2008/09/18/8956382.aspx
    //caveat: might not be reliable, e.g. "subst"-alias of volume contains "$Recycle.Bin" although it is not available!

    /*
        Zstring rootPathPf = appendSeparator(&buffer[0]);

    	const bool canUseFastCheckForRecycler = winXpOrLater();
        if (!canUseFastCheckForRecycler) //== "checkForRecycleBin"
            return STATUS_REC_UNKNOWN;

        using namespace fileop;
        const DllFun<FunType_checkRecycler> checkRecycler(getDllName(), funName_checkRecycler);

        if (!checkRecycler)
            return STATUS_REC_UNKNOWN; //actually an error since we're >= XP

        //search root directories for recycle bin folder...

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
                (dataRoot.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM   ) != 0 && //risky to rely on these attributes!!!
                (dataRoot.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN   ) != 0)   //
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
}

#elif defined ZEN_LINUX || defined ZEN_MAC
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
