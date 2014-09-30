// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "shadow.h"
#include <stdexcept>
#include <zen/win.h> //includes "windows.h"
#include <zen/dll.h>
#include <zen/win_ver.h>
#include <zen/assert_static.h>
#include <zen/long_path_prefix.h>
#include <zen/symlink_target.h>
#include "../dll/ShadowCopy/shadow.h"
#include <zen/scope_guard.h>

using namespace zen;
using namespace shadow;


namespace
{
bool runningWOW64() //test if process is running under WOW64 (reference http://msdn.microsoft.com/en-us/library/ms684139(VS.85).aspx)
{
    typedef BOOL (WINAPI* IsWow64ProcessFun)(HANDLE hProcess, PBOOL Wow64Process);

    const SysDllFun<IsWow64ProcessFun> isWow64Process(L"kernel32.dll", "IsWow64Process");
    if (isWow64Process)
    {
        BOOL isWow64 = FALSE;
        if (isWow64Process(::GetCurrentProcess(), &isWow64))
            return isWow64 != FALSE;
    }
    return false;
}
}

//#############################################################################################################

class ShadowCopy::ShadowVolume
{
public:
    ShadowVolume(const Zstring& volumeNamePf) : //throw FileError
        createShadowCopy   (getDllName(), funName_createShadowCopy),
        releaseShadowCopy  (getDllName(), funName_releaseShadowCopy),
        getShadowVolume    (getDllName(), funName_getShadowVolume),
        getLastErrorMessage(getDllName(), funName_getLastErrorMessage),
        backupHandle(nullptr)
    {
        //VSS does not support running under WOW64 except for Windows XP and Windows Server 2003
        //reference: http://msdn.microsoft.com/en-us/library/aa384627(VS.85).aspx
        if (runningWOW64())
            throw FileError(_("Cannot access the Volume Shadow Copy Service."),
                            _("Please use FreeFileSync 64-bit version to create shadow copies on this system."));

        //check if shadow copy dll was loaded correctly
        if (!createShadowCopy || !releaseShadowCopy || !getShadowVolume || !getLastErrorMessage)
            throw FileError(_("Cannot access the Volume Shadow Copy Service."),
                            replaceCpy(_("Cannot load file %x."), L"%x", fmtFileName(getDllName())));

        //---------------------------------------------------------------------------------------------------------
        //start volume shadow copy service:
        backupHandle = createShadowCopy(volumeNamePf.c_str());
        if (!backupHandle)
            throw FileError(_("Cannot access the Volume Shadow Copy Service."), getLastErrorMessage() + std::wstring(L" Volume: ") + fmtFileName(volumeNamePf));

        shadowVolPf = appendSeparator(getShadowVolume(backupHandle)); //shadowVolName NEVER has a trailing backslash
    }

    ~ShadowVolume() { releaseShadowCopy(backupHandle); } //fast! no performance optimization necessary

    Zstring geNamePf() const { return shadowVolPf; } //with trailing path separator

private:
    ShadowVolume           (const ShadowVolume&) = delete;
    ShadowVolume& operator=(const ShadowVolume&) = delete;

    const DllFun<FunType_createShadowCopy   > createShadowCopy;
    const DllFun<FunType_releaseShadowCopy  > releaseShadowCopy;
    const DllFun<FunType_getShadowVolume    > getShadowVolume;
    const DllFun<FunType_getLastErrorMessage> getLastErrorMessage;

    Zstring shadowVolPf;
    ShadowHandle backupHandle;
};

//#############################################################################################################

Zstring ShadowCopy::makeShadowCopy(const Zstring& inputFile, const std::function<void(const Zstring& volumeNamePf)>& onBeforeMakeVolumeCopy)
{
    Zstring filepathFinal = inputFile;

    //try to resolve symlinks and junctions:
    //1. symlinks: we need to retrieve the target path, else we would just return a symlink on a VSS volume while the target outside were still locked!
    //2. junctions: C:\Users\<username> is a junction that may link to e.g. D:\Users\<username>, so GetVolumePathName() returns "D:\" => "Volume name %x not part of file name %y."
    if (vistaOrLater())
        filepathFinal = getResolvedFilePath(inputFile); //throw FileError; requires Vista or later!
    //-> returns paths with \\?\ prefix! => make sure to avoid duplicate shadow copies for volume paths with/without prefix

    DWORD bufferSize = 10000;
    std::vector<wchar_t> volBuffer(bufferSize);
    if (!::GetVolumePathName(filepathFinal.c_str(), //__in   LPCTSTR lpszFileName,
                             &volBuffer[0],         //__out  LPTSTR lpszVolumePathName,
                             bufferSize))           //__in   DWORD cchBufferLength
        throwFileError(replaceCpy(_("Cannot determine volume name for %x."), L"%x", fmtFileName(filepathFinal)), L"GetVolumePathName", ::GetLastError());

    const Zstring volumeNamePf = appendSeparator(&volBuffer[0]); //msdn: if buffer is 1 char too short, GetVolumePathName() may skip last separator without error!

    //input file is always absolute! directory formatting takes care of this! Therefore volume name can always be found.
    const size_t pos = filepathFinal.find(volumeNamePf); //filepathFinal needs NOT to begin with volumeNamePf: consider \\?\ prefix!
    if (pos == Zstring::npos)
        throw FileError(replaceCpy(replaceCpy(_("Volume name %x is not part of file path %y."),
                                              L"%x", fmtFileName(volumeNamePf)),
                                   L"%y", fmtFileName(filepathFinal)));

    //get or create instance of shadow volume
    auto it = shadowVol.find(volumeNamePf);
    if (it == shadowVol.end())
    {
        onBeforeMakeVolumeCopy(volumeNamePf); //notify client before (potentially) blocking some time
        auto newEntry = std::make_shared<ShadowVolume>(volumeNamePf); //throw FileError
        it = shadowVol.insert(std::make_pair(volumeNamePf, newEntry)).first;
    }

    //return filepath alias on shadow copy volume
    return it->second->geNamePf() + Zstring(filepathFinal.c_str() + pos + volumeNamePf.length());
}
