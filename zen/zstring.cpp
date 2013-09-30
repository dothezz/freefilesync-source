// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "zstring.h"
#include <stdexcept>

#ifdef ZEN_WIN
#include "dll.h"
#include "win_ver.h"

#elif defined ZEN_MAC
//#include <zen/scope_guard.h>
#include <ctype.h> //toupper()
#endif

#ifndef NDEBUG
#include "thread.h" //includes <boost/thread.hpp>
#include <iostream>
#endif

using namespace zen;


#ifndef NDEBUG
namespace
{
class LeakChecker //small test for memory leaks
{
public:
    void insert(const void* ptr, size_t size)
    {
        boost::lock_guard<boost::mutex> dummy(lockActStrings);
        if (!activeStrings.insert(std::make_pair(ptr, size)).second)
            reportProblem("Serious Error: New memory points into occupied space: " + rawMemToString(ptr, size));
    }

    void remove(const void* ptr)
    {
        boost::lock_guard<boost::mutex> dummy(lockActStrings);
        if (activeStrings.erase(ptr) != 1)
            reportProblem("Serious Error: No memory available for deallocation at this location!");
    }

    static LeakChecker& instance() { static LeakChecker inst; return inst; }

private:
    LeakChecker() {}
    ~LeakChecker()
    {
        if (!activeStrings.empty())
        {
            std::string leakingStrings;

            int items = 0;
            for (auto it = activeStrings.begin(); it != activeStrings.end() && items < 20; ++it, ++items)
                leakingStrings += "\"" + rawMemToString(it->first, it->second) + "\"\n";

            const std::string message = std::string("Memory leak detected!") + "\n\n"
                                        + "Candidates:\n" + leakingStrings;
#ifdef ZEN_WIN
            MessageBoxA(nullptr, message.c_str(), "Error", 0);
#else
            std::cerr << message;
            std::abort();
#endif
        }
    }

    LeakChecker(const LeakChecker&);
    LeakChecker& operator=(const LeakChecker&);

    static std::string rawMemToString(const void* ptr, size_t size)
    {
        std::string output(reinterpret_cast<const char*>(ptr), std::min<size_t>(size, 100));
        std::replace(output.begin(), output.end(), '\0', ' '); //don't stop at 0-termination
        return output;
    }

    void reportProblem(const std::string& message) //throw std::logic_error
    {
#ifdef ZEN_WIN
        ::MessageBoxA(nullptr, message.c_str(), "Error", 0);
#else
        std::cerr << message;
#endif
        throw std::logic_error("Memory leak! " + message);
    }

    boost::mutex lockActStrings;
    zen::hash_map<const void*, size_t> activeStrings;
};

//caveat: function scope static initialization is not thread-safe in VS 2010! => make sure to call at app start!
const LeakChecker& dummy = LeakChecker::instance();
}

void z_impl::leakCheckerInsert(const void* ptr, size_t size) { LeakChecker::instance().insert(ptr, size); }
void z_impl::leakCheckerRemove(const void* ptr             ) { LeakChecker::instance().remove(ptr); }
#endif //NDEBUG


/*
Perf test: compare strings 10 mio times; 64 bit build
-----------------------------------------------------
	string a = "Fjk84$%kgfj$%T\\\\Gffg\\gsdgf\\fgsx----------d-"
	string b = "fjK84$%kgfj$%T\\\\gfFg\\gsdgf\\fgSy----------dfdf"

Windows (UTF16 wchar_t)
  4 ns | wcscmp
 67 ns | CompareStringOrdinalFunc+ + bIgnoreCase
314 ns | LCMapString + wmemcmp

OS X (UTF8 char)
   6 ns | strcmp
  98 ns | strcasecmp
 120 ns | strncasecmp + std::min(sizeLhs, sizeRhs);
 856 ns | CFStringCreateWithCString       + CFStringCompare(kCFCompareCaseInsensitive)
1110 ns | CFStringCreateWithCStringNoCopy + CFStringCompare(kCFCompareCaseInsensitive)
________________________
time per call | function
*/


#ifdef ZEN_WIN
namespace
{
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


int z_impl::compareFilenamesNoCase(const wchar_t* lhs, const wchar_t* rhs, size_t sizeLhs, size_t sizeRhs)
{
    //caveat: function scope static initialization is not thread-safe in VS 2010!
    if (compareStringOrdinal) //this additional test has no noticeable performance impact
    {
        const int rv = compareStringOrdinal(lhs,                       //__in  LPCWSTR lpString1,
                                            static_cast<int>(sizeLhs), //__in  int cchCount1,
                                            rhs,                       //__in  LPCWSTR lpString2,
                                            static_cast<int>(sizeRhs), //__in  int cchCount2,
                                            true);                     //__in  BOOL bIgnoreCase
        if (rv <= 0)
            throw std::runtime_error("Error comparing strings (CompareStringOrdinal).");
        else
            return rv - 2; //convert to C-style string compare result
    }
    else //fallback
    {
        //do NOT use "CompareString"; this function is NOT accurate (even with LOCALE_INVARIANT and SORT_STRINGSORT): for example "weiß" == "weiss"!!!
        //the only reliable way to compare filenames (with XP) is to call "CharUpper" or "LCMapString":

        const auto minSize = static_cast<unsigned int>(std::min(sizeLhs, sizeRhs));

        if (minSize > 0) //LCMapString does not allow input sizes of 0!
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
                    throw std::runtime_error("Error comparing strings (LCMapString).");

                if (::LCMapString(ZSTRING_INVARIANT_LOCALE, LCMAP_UPPERCASE, rhs, minSize, bufferB, MAX_PATH) == 0)
                    throw std::runtime_error("Error comparing strings (LCMapString).");

                const int rv = ::wmemcmp(bufferA, bufferB, minSize);
                if (rv != 0)
                    return rv;
            }
            else //use freestore
            {
                std::vector<wchar_t> bufferA(minSize);
                std::vector<wchar_t> bufferB(minSize);

                if (::LCMapString(ZSTRING_INVARIANT_LOCALE, LCMAP_UPPERCASE, lhs, minSize, &bufferA[0], minSize) == 0)
                    throw std::runtime_error("Error comparing strings (LCMapString: FS).");

                if (::LCMapString(ZSTRING_INVARIANT_LOCALE, LCMAP_UPPERCASE, rhs, minSize, &bufferB[0], minSize) == 0)
                    throw std::runtime_error("Error comparing strings (LCMapString: FS).");

                const int rv = ::wmemcmp(&bufferA[0], &bufferB[0], minSize);
                if (rv != 0)
                    return rv;
            }
        }
        return static_cast<int>(sizeLhs) - static_cast<int>(sizeRhs);
    }
}


void z_impl::makeFilenameUpperCase(wchar_t* str, size_t size)
{
    if (size == 0) //LCMapString does not allow input sizes of 0!
        return;

    //use Windows' upper case conversion: faster than ::CharUpper()
    if (::LCMapString(ZSTRING_INVARIANT_LOCALE, LCMAP_UPPERCASE, str, static_cast<int>(size), str, static_cast<int>(size)) == 0)
        throw std::runtime_error("Error converting to upper case! (LCMapString)");
}

#elif defined ZEN_MAC
int z_impl::compareFilenamesNoCase(const char* lhs, const char* rhs, size_t sizeLhs, size_t sizeRhs)
{
    return ::strcasecmp(lhs, rhs); //locale-dependent!
}


void z_impl::makeFilenameUpperCase(char* str, size_t size)
{
    std::for_each(str, str + size, [](char& c) { c = static_cast<char>(::toupper(static_cast<unsigned char>(c))); }); //locale-dependent!
    //result of toupper() is an unsigned char mapped to int range, so the char representation is in the last 8 bits and we need not care about signedness!
    //this should work for UTF-8, too: all chars >= 128 are mapped upon themselves!
}
#endif
