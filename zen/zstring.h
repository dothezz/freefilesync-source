// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef ZSTRING_H_INCLUDED_73425873425789
#define ZSTRING_H_INCLUDED_73425873425789

#include "string_base.h"

#ifdef ZEN_LINUX
    #include <cstring> //strncmp
#endif



#ifdef ZEN_WIN //Windows encodes Unicode as UTF-16 wchar_t
    typedef wchar_t Zchar;
    #define Zstr(x) L ## x
    const Zchar FILE_NAME_SEPARATOR = L'\\';

#elif defined ZEN_LINUX || defined ZEN_MAC //Linux uses UTF-8
    typedef char Zchar;
    #define Zstr(x) x
    const Zchar FILE_NAME_SEPARATOR = '/';
#endif

//"The reason for all the fuss above" - Loki/SmartPtr
//a high-performance string for interfacing with native OS APIs and multithreaded contexts
typedef zen::Zbase<Zchar, zen::StorageRefCountThreadSafe, zen::AllocatorOptimalSpeed> Zstring;



//Compare filepaths: Windows does NOT distinguish between upper/lower-case, while Linux DOES
int cmpFilePath(const Zchar* lhs, size_t lhsLen, const Zchar* rhs, size_t rhsLen);


struct LessFilePath //case-insensitive on Windows, case-sensitive on Linux
{
    template <class S, class T>
    bool operator()(const S& lhs, const T& rhs) const { using namespace zen; return cmpFilePath(strBegin(lhs), strLength(lhs), strBegin(rhs), strLength(rhs)) < 0; }
};

struct EqualFilePath //case-insensitive on Windows, case-sensitive on Linux
{
    template <class S, class T>
    bool operator()(const S& lhs, const T& rhs) const { using namespace zen; return cmpFilePath(strBegin(lhs), strLength(lhs), strBegin(rhs), strLength(rhs)) == 0; }
};


#if defined ZEN_WIN || defined ZEN_MAC
    Zstring makeUpperCopy(const Zstring& str);
#endif


inline
Zstring appendSeparator(Zstring path) //support rvalue references!
{
    return zen::endsWith(path, FILE_NAME_SEPARATOR) ? path : (path += FILE_NAME_SEPARATOR); //returning a by-value parameter implicitly converts to r-value!
}


inline
Zstring getFileExtension(const Zstring& filePath)
{
    const Zstring shortName = afterLast(filePath, FILE_NAME_SEPARATOR); //returns the whole string if term not found

    return contains(shortName, Zchar('.')) ?
           afterLast(filePath, Zchar('.')) :
           Zstring();
}


template <class S, class T> inline
bool pathStartsWith(const S& str, const T& prefix)
{
    using namespace zen;
    const size_t pfLen = strLength(prefix);
    if (strLength(str) < pfLen)
        return false;

    return cmpFilePath(strBegin(str), pfLen, strBegin(prefix), pfLen) == 0;
}


template <class S, class T> inline
bool pathEndsWith(const S& str, const T& postfix)
{
    using namespace zen;
    const size_t strLen = strLength(str);
    const size_t pfLen  = strLength(postfix);
    if (strLen < pfLen)
        return false;

    return cmpFilePath(strBegin(str) + strLen - pfLen, pfLen, strBegin(postfix), pfLen) == 0;
}





//################################# inline implementation ########################################

#ifdef ZEN_LINUX
inline
int cmpFilePath(const Zchar* lhs, size_t lhsLen, const Zchar* rhs, size_t rhsLen)
{
    assert(std::find(lhs, lhs + lhsLen, 0) == lhs + lhsLen); //don't expect embedded nulls!
    assert(std::find(rhs, rhs + rhsLen, 0) == rhs + rhsLen); //

    const int rv = std::strncmp(lhs, rhs, std::min(lhsLen, rhsLen));
    if (rv != 0)
        return rv;
    return static_cast<int>(lhsLen) - static_cast<int>(rhsLen);
}
#endif


//---------------------------------------------------------------------------
//ZEN macro consistency checks:
#ifdef ZEN_WIN
    #if defined ZEN_LINUX || defined ZEN_MAC
        #error more than one target platform defined
    #endif

    #ifdef ZEN_WIN_VISTA_AND_LATER
        #ifdef ZEN_WIN_PRE_VISTA
            #error choose only one of the two variants
        #endif
    #elif defined ZEN_WIN_PRE_VISTA
        #ifdef ZEN_WIN_VISTA_AND_LATER
            #error choose only one of the two variants
        #endif
    #else
        #error choose one of the two variants
    #endif

#elif defined ZEN_LINUX
    #if defined ZEN_WIN || defined ZEN_MAC
        #error more than one target platform defined
    #endif

#elif defined ZEN_MAC
    #if defined ZEN_WIN || defined ZEN_LINUX
        #error more than one target platform defined
    #endif

#else
    #error no target platform defined
#endif

#endif //ZSTRING_H_INCLUDED_73425873425789
