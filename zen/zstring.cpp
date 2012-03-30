// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "zstring.h"
#include <stdexcept>
#include <zen/stl_tools.h>

#ifdef FFS_WIN
#include "dll.h"
#include "win_ver.h"
#endif

#ifndef NDEBUG
#include <iostream>
#include <cstdlib>
#endif

using namespace zen;


#ifndef NDEBUG
LeakChecker::~LeakChecker()
{
    if (!activeStrings.empty())
    {
        std::string leakingStrings;

        int items = 0;
        for (auto iter = activeStrings.begin(); iter != activeStrings.end() && items < 20; ++iter, ++items)
            leakingStrings += "\"" + rawMemToString(iter->first, iter->second) + "\"\n";

        const std::string message = std::string("Memory leak detected!") + "\n\n"
                                    + "Candidates:\n" + leakingStrings;
#ifdef FFS_WIN
        MessageBoxA(nullptr, message.c_str(), "Error", 0);
#else
        std::cerr << message;
        std::abort();
#endif
    }
}


LeakChecker& LeakChecker::instance()
{
    static LeakChecker inst;
    return inst;
}

//caveat: function scope static initialization is not thread-safe in VS 2010! => make sure to call at app start!
namespace
{
const LeakChecker& dummy = LeakChecker::instance();
}


std::string LeakChecker::rawMemToString(const void* ptr, size_t size)
{
    std::string output = std::string(reinterpret_cast<const char*>(ptr), size);
    vector_remove_if(output, [](char& c) { return c == 0; }); //remove intermediate 0-termination
    if (output.size() > 100)
        output.resize(100);
    return output;
}


void LeakChecker::reportProblem(const std::string& message) //throw (std::logic_error)
{
#ifdef FFS_WIN
    ::MessageBoxA(nullptr, message.c_str(), "Error", 0);
#else
    std::cerr << message;
#endif
    throw std::logic_error("Memory leak! " + message);
}
#endif //NDEBUG


#ifdef FFS_WIN
namespace
{
#ifndef LOCALE_INVARIANT //invariant locale has been introduced with XP
#define LOCALE_INVARIANT                0x007f
#endif


//warning: LOCALE_INVARIANT is NOT available with Windows 2000, so we have to make yet another distinction...
const LCID ZSTRING_INVARIANT_LOCALE = zen::winXpOrLater() ?
                                      LOCALE_INVARIANT :
                                      MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT); //see: http://msdn.microsoft.com/en-us/goglobal/bb688122.aspx


//try to call "CompareStringOrdinal" for low-level string comparison: unfortunately available not before Windows Vista!
//by a factor ~3 faster than old string comparison using "LCMapString"
typedef int (WINAPI* CompareStringOrdinalFunc)(LPCWSTR lpString1,
                                               int     cchCount1,
                                               LPCWSTR lpString2,
                                               int     cchCount2,
                                               BOOL    bIgnoreCase);
const SysDllFun<CompareStringOrdinalFunc> compareStringOrdinal = SysDllFun<CompareStringOrdinalFunc>(L"kernel32.dll", "CompareStringOrdinal");
}


int z_impl::compareFilenamesWin(const wchar_t* lhs, const wchar_t* rhs, size_t sizeLhs, size_t sizeRhs)
{
    //caveat: function scope static initialization is not thread-safe in VS 2010!
    if (compareStringOrdinal) //this additional test has no noticeable performance impact
    {
        const int rv = compareStringOrdinal(lhs,                       //__in  LPCWSTR lpString1,
                                            static_cast<int>(sizeLhs), //__in  int cchCount1,
                                            rhs,                       //__in  LPCWSTR lpString2,
                                            static_cast<int>(sizeRhs), //__in  int cchCount2,
                                            true);                     //__in  BOOL bIgnoreCase
        if (rv == 0)
            throw std::runtime_error("Error comparing strings (ordinal)!");
        else
            return rv - 2; //convert to C-style string compare result
    }
    else //fallback
    {
        //do NOT use "CompareString"; this function is NOT accurate (even with LOCALE_INVARIANT and SORT_STRINGSORT): for example "weiß" == "weiss"!!!
        //the only reliable way to compare filenames (with XP) is to call "CharUpper" or "LCMapString":

        const auto minSize = static_cast<unsigned int>(std::min(sizeLhs, sizeRhs));

        int rv = 0;
        if (minSize >= 0) //LCMapString does not allow input sizes of 0!
        {
            if (minSize <= MAX_PATH) //performance optimization: stack
            {
                wchar_t bufferA[MAX_PATH];
                wchar_t bufferB[MAX_PATH];

                //faster than CharUpperBuff + wmemcpy or CharUpper + wmemcpy and same speed like ::CompareString()
                if (::LCMapString(ZSTRING_INVARIANT_LOCALE, //__in   LCID Locale,
                                  LCMAP_UPPERCASE,          //__in   DWORD dwMapFlags,
                                  lhs,                      //__in   LPCTSTR lpSrcStr,
                                  minSize,                  //__in   int cchSrc,
                                  bufferA,                  //__out  LPTSTR lpDestStr,
                                  MAX_PATH) == 0)           //__in   int cchDest
                    throw std::runtime_error("Error comparing strings! (LCMapString)");

                if (::LCMapString(ZSTRING_INVARIANT_LOCALE, LCMAP_UPPERCASE, rhs, minSize, bufferB, MAX_PATH) == 0)
                    throw std::runtime_error("Error comparing strings! (LCMapString)");

                rv = ::wmemcmp(bufferA, bufferB, minSize);
            }
            else //use freestore
            {
                std::vector<wchar_t> bufferA(minSize);
                std::vector<wchar_t> bufferB(minSize);

                if (::LCMapString(ZSTRING_INVARIANT_LOCALE, LCMAP_UPPERCASE, lhs, minSize, &bufferA[0], minSize) == 0)
                    throw std::runtime_error("Error comparing strings! (LCMapString: FS)");

                if (::LCMapString(ZSTRING_INVARIANT_LOCALE, LCMAP_UPPERCASE, rhs, minSize, &bufferB[0], minSize) == 0)
                    throw std::runtime_error("Error comparing strings! (LCMapString: FS)");

                rv = ::wmemcmp(&bufferA[0], &bufferB[0], minSize);
            }
        }

        return rv == 0 ?
               static_cast<int>(sizeLhs) - static_cast<int>(sizeRhs) :
               rv;
    }

    //        const int rv = CompareString(
    //                           invariantLocale,    //locale independent
    //                           NORM_IGNORECASE | SORT_STRINGSORT, //comparison-style options
    //                           a,  	                //pointer to first string
    //                           aCount,	            //size, in bytes or characters, of first string
    //                           b,	                //pointer to second string
    //                           bCount); 	        //size, in bytes or characters, of second string
    //
    //        if (rv == 0)
    //            throw std::runtime_error("Error comparing strings!");
    //        else
    //            return rv - 2; //convert to C-style string compare result
}


void z_impl::makeUpperCaseWin(wchar_t* str, size_t size)
{
    if (size == 0) //LCMapString does not allow input sizes of 0!
        return;

    //use Windows' upper case conversion: faster than ::CharUpper()
    if (::LCMapString(ZSTRING_INVARIANT_LOCALE, LCMAP_UPPERCASE, str, static_cast<int>(size), str, static_cast<int>(size)) == 0)
        throw std::runtime_error("Error converting to upper case! (LCMapString)");
}


/*
#include <fstream>
 extern std::wofstream statShared;
extern std::wofstream statLowCapacity;
extern std::wofstream statOverwrite;

std::wstring p(ptr);
p.erase(std::remove(p.begin(), p.end(), L'\n'), p.end());
p.erase(std::remove(p.begin(), p.end(), L','), p.end());

 if (descr(ptr)->refCount > 1)
    statShared <<
               minCapacity          << L"," <<
               descr(ptr)->refCount << L"," <<
               descr(ptr)->length   << L"," <<
               descr(ptr)->capacity << L"," <<
               p << L"\n";
else if (minCapacity > descr(ptr)->capacity)
    statLowCapacity <<
                    minCapacity          << L"," <<
                    descr(ptr)->refCount << L"," <<
                    descr(ptr)->length   << L"," <<
                    descr(ptr)->capacity << L"," <<
                    p << L"\n";
else
    statOverwrite <<
                  minCapacity          << L"," <<
                  descr(ptr)->refCount << L"," <<
                  descr(ptr)->length   << L"," <<
                  descr(ptr)->capacity << L"," <<
                  p << L"\n";
*/

#endif //FFS_WIN
