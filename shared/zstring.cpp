// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "zstring.h"
#include <stdexcept>
#include <boost/thread/once.hpp>

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#undef min
#undef max
#include "dll_loader.h"
#endif  //FFS_WIN

#ifndef NDEBUG
#include <wx/string.h>
#include <iostream>
#include <cstdlib>
#endif


#ifndef NDEBUG
LeakChecker::~LeakChecker()
{
    if (activeStrings.size() > 0)
    {
        int rowCount = 0;
        std::string leakingStrings;

        for (VoidPtrSizeMap::const_iterator i = activeStrings.begin();
             i != activeStrings.end() && ++rowCount <= 20;
             ++i)
            leakingStrings += "\"" + rawMemToString(i->first, i->second) + "\"\n";

        const std::string message = std::string("Memory leak detected!") + "\n\n"
                                    + "Candidates:\n" + leakingStrings;
#ifdef FFS_WIN
        MessageBoxA(NULL, message.c_str(), "Error", 0);
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
struct Dummy { Dummy() { LeakChecker::instance(); }} blah;
}


std::string LeakChecker::rawMemToString(const void* ptr, size_t size)
{
    std::string output = std::string(reinterpret_cast<const char*>(ptr), size);
    output.erase(std::remove(output.begin(), output.end(), 0), output.end()); //remove intermediate 0-termination
    if (output.size() > 100)
        output.resize(100);
    return output;
}


void LeakChecker::reportProblem(const std::string& message) //throw (std::logic_error)
{
#ifdef FFS_WIN
    ::MessageBoxA(NULL, message.c_str(), "Error", 0);
#else
    std::cerr << message;
#endif
    throw std::logic_error("Memory leak! " + message);
}
#endif //NDEBUG


#ifdef FFS_WIN
namespace
{
bool hasInvariantLocale()
{
    OSVERSIONINFO osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    //invariant locale has been introduced with XP
    if (GetVersionEx(&osvi))
        return osvi.dwMajorVersion > 5 ||
               (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 1); //XP    has majorVersion == 5, minorVersion == 1
    //overview: http://msdn.microsoft.com/en-us/library/ms724834(VS.85).aspx
    return false;
}

#ifndef LOCALE_INVARIANT
#define LOCALE_INVARIANT                0x007f
#endif


//warning: LOCALE_INVARIANT is NOT available with Windows 2000, so we have to make yet another distinction...
const LCID ZSTRING_INVARIANT_LOCALE = hasInvariantLocale() ?
                                      LOCALE_INVARIANT :
                                      MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT); //see: http://msdn.microsoft.com/en-us/goglobal/bb688122.aspx


//try to call "CompareStringOrdinal" for low-level string comparison: unfortunately available not before Windows Vista!
//by a factor ~3 faster than old string comparison using "LCMapString"
typedef int (WINAPI* CompareStringOrdinalFunc)(LPCWSTR lpString1,
                                               int     cchCount1,
                                               LPCWSTR lpString2,
                                               int     cchCount2,
                                               BOOL    bIgnoreCase);
util::DllFun<CompareStringOrdinalFunc> ordinalCompare; //caveat: function scope static initialization is not thread-safe in VS 2010!
boost::once_flag initCmpStrOrdOnce = BOOST_ONCE_INIT;
}


int z_impl::compareFilenamesWin(const wchar_t* a, const wchar_t* b, size_t sizeA, size_t sizeB)
{
    boost::call_once(initCmpStrOrdOnce, []() { ordinalCompare = util::DllFun<CompareStringOrdinalFunc>(L"kernel32.dll", "CompareStringOrdinal"); });

    if (ordinalCompare) //this additional test has no noticeable performance impact
    {
        const int rv = ordinalCompare(a,  	      //pointer to first string
                                      static_cast<int>(sizeA),	  //size, in bytes or characters, of first string
                                      b,	      //pointer to second string
                                      static_cast<int>(sizeB),     //size, in bytes or characters, of second string
                                      true); 	  //ignore case
        if (rv == 0)
            throw std::runtime_error("Error comparing strings (ordinal)!");
        else
            return rv - 2; //convert to C-style string compare result
    }
    else //fallback
    {
        //do NOT use "CompareString"; this function is NOT accurate (even with LOCALE_INVARIANT and SORT_STRINGSORT): for example "weiß" == "weiss"!!!
        //the only reliable way to compare filenames (with XP) is to call "CharUpper" or "LCMapString":

        const size_t minSize = std::min(sizeA, sizeB);

        if (minSize == 0) //LCMapString does not allow input sizes of 0!
            return static_cast<int>(sizeA) - static_cast<int>(sizeB);

        int rv = 0; //always initialize...
        if (minSize <= 5000) //performance optimization: stack
        {
            wchar_t bufferA[5000];
            wchar_t bufferB[5000];

            if (::LCMapString(   //faster than CharUpperBuff + wmemcpy or CharUpper + wmemcpy and same speed like ::CompareString()
                    ZSTRING_INVARIANT_LOCALE, //__in   LCID Locale,
                    LCMAP_UPPERCASE,  //__in   DWORD dwMapFlags,
                    a,                //__in   LPCTSTR lpSrcStr,
                    static_cast<int>(minSize), //__in   int cchSrc,
                    bufferA,          //__out  LPTSTR lpDestStr,
                    5000              //__in   int cchDest
                ) == 0)
                throw std::runtime_error("Error comparing strings! (LCMapString)");

            if (::LCMapString(ZSTRING_INVARIANT_LOCALE, LCMAP_UPPERCASE, b, static_cast<int>(minSize), bufferB, 5000) == 0)
                throw std::runtime_error("Error comparing strings! (LCMapString)");

            rv = ::wmemcmp(bufferA, bufferB, minSize);
        }
        else //use freestore
        {
            std::vector<wchar_t> bufferA(minSize);
            std::vector<wchar_t> bufferB(minSize);

            if (::LCMapString(ZSTRING_INVARIANT_LOCALE, LCMAP_UPPERCASE, a, static_cast<int>(minSize), &bufferA[0], static_cast<int>(minSize)) == 0)
                throw std::runtime_error("Error comparing strings! (LCMapString: FS)");

            if (::LCMapString(ZSTRING_INVARIANT_LOCALE, LCMAP_UPPERCASE, b, static_cast<int>(minSize), &bufferB[0], static_cast<int>(minSize)) == 0)
                throw std::runtime_error("Error comparing strings! (LCMapString: FS)");

            rv = ::wmemcmp(&bufferA[0], &bufferB[0], minSize);
        }

        return rv == 0 ?
               static_cast<int>(sizeA) - static_cast<int>(sizeB) :
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
