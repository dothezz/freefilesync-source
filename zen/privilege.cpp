#include "privilege.h"
#include <map>
//#include <mutex>
#include "win.h" //includes "windows.h"
#include "thread.h"
#include "zstring.h"
#include "scope_guard.h"
#include "win_ver.h"

using namespace zen;


namespace
{
bool privilegeIsActive(const wchar_t* privilege) //throw FileError
{
    HANDLE hToken = nullptr;
    if (!::OpenProcessToken(::GetCurrentProcess(), //__in   HANDLE ProcessHandle,
                            TOKEN_QUERY,           //__in   DWORD DesiredAccess,
                            &hToken))              //__out  PHANDLE TokenHandle
        throwFileError(replaceCpy(_("Cannot set privilege %x."), L"%x", std::wstring(L"\"") + privilege +  L"\""), L"OpenProcessToken", getLastError());
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hToken));

    LUID luid = {};
    if (!::LookupPrivilegeValue(nullptr,   //__in_opt  LPCTSTR lpSystemName,
                                privilege, //__in      LPCTSTR lpName,
                                &luid ))   //__out     PLUID lpLuid
        throwFileError(replaceCpy(_("Cannot set privilege %x."), L"%x", std::wstring(L"\"") + privilege +  L"\""), L"LookupPrivilegeValue", getLastError());

    PRIVILEGE_SET priv  = {};
    priv.PrivilegeCount = 1;
    priv.Control        = PRIVILEGE_SET_ALL_NECESSARY;
    priv.Privilege[0].Luid = luid;
    priv.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL alreadyGranted = FALSE;
    if (!::PrivilegeCheck(hToken,           //__in     HANDLE ClientToken,
                          &priv,            //__inout  PPRIVILEGE_SET RequiredPrivileges,
                          &alreadyGranted)) //__out    LPBOOL pfResult
        throwFileError(replaceCpy(_("Cannot set privilege %x."), L"%x", std::wstring(L"\"") + privilege +  L"\""), L"PrivilegeCheck", getLastError());

    return alreadyGranted != FALSE;
}


void setPrivilege(const wchar_t* privilege, bool enable) //throw FileError
{
    HANDLE hToken = nullptr;
    if (!::OpenProcessToken(::GetCurrentProcess(),   //__in   HANDLE ProcessHandle,
                            TOKEN_ADJUST_PRIVILEGES, //__in   DWORD DesiredAccess,
                            &hToken))                //__out  PHANDLE TokenHandle
        throwFileError(replaceCpy(_("Cannot set privilege %x."), L"%x", std::wstring(L"\"") + privilege +  L"\""), L"OpenProcessToken", getLastError());
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hToken));

    LUID luid = {};
    if (!::LookupPrivilegeValue(nullptr,   //__in_opt  LPCTSTR lpSystemName,
                                privilege, //__in      LPCTSTR lpName,
                                &luid ))   //__out     PLUID lpLuid
        throwFileError(replaceCpy(_("Cannot set privilege %x."), L"%x", std::wstring(L"\"") + privilege +  L"\""), L"LookupPrivilegeValue", getLastError());

    TOKEN_PRIVILEGES tp = {};
    tp.PrivilegeCount   = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

    if (!::AdjustTokenPrivileges(hToken,   //__in       HANDLE TokenHandle,
                                 false,    //__in       BOOL DisableAllPrivileges,
                                 &tp,      //__in_opt   PTOKEN_PRIVILEGES NewState,
                                 0,        //__in       DWORD BufferLength,
                                 nullptr,  //__out_opt  PTOKEN_PRIVILEGES PreviousState,
                                 nullptr)) //__out_opt  PDWORD ReturnLength
        throwFileError(replaceCpy(_("Cannot set privilege %x."), L"%x", std::wstring(L"\"") + privilege +  L"\""), L"AdjustTokenPrivileges", getLastError());

    DWORD lastError = ::GetLastError(); //copy before directly or indirectly making other system calls!
    if (lastError == ERROR_NOT_ALL_ASSIGNED) //check although previous function returned with success!
    {
#ifdef __MINGW32__ //Shobjidl.h
#define ERROR_ELEVATION_REQUIRED         740L
#endif
        if (vistaOrLater()) //replace this useless error code with what it *really* means!
            lastError = ERROR_ELEVATION_REQUIRED;

        throwFileError(replaceCpy(_("Cannot set privilege %x."), L"%x", std::wstring(L"\"") + privilege +  L"\""), L"AdjustTokenPrivileges", lastError);
    }
}


class Privileges
{
public:
    static Privileges& getInstance()
    {
		        //meyers singleton: avoid static initialization order problem in global namespace!
        static Privileges inst;
        return inst;
    }

    void ensureActive(const wchar_t* privilege) //throw FileError
    {
		    boost::lock_guard<boost::mutex> dummy(lockPrivileges);

        if (activePrivileges.find(privilege) != activePrivileges.end())
            return; //privilege already active

        if (privilegeIsActive(privilege)) //privilege was already active before starting this tool
            activePrivileges.insert(std::make_pair(privilege, false));
        else
        {
            setPrivilege(privilege, true);
            activePrivileges.insert(std::make_pair(privilege, true));
        }
    }

private:
    Privileges() {}
    Privileges           (const Privileges&) = delete;
    Privileges& operator=(const Privileges&) = delete;

    ~Privileges() //clean up: deactivate all privileges that have been activated by this application
    {
        for (const auto& priv : activePrivileges)
            if (priv.second)
            {
                try
                {
                    setPrivilege(priv.first.c_str(), false); //throw FileError
                }
                catch (FileError&) {}
            }
    }

    std::map<Zstring, bool> activePrivileges; //bool: enabled by this application
boost::mutex lockPrivileges;
};

//caveat: function scope static initialization is not thread-safe in VS 2010!
auto& dummy = Privileges::getInstance();
}


void zen::activatePrivilege(const wchar_t* privilege) //throw FileError
{
    Privileges::getInstance().ensureActive(privilege);
}
