// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0           *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#ifndef ZSTRING_H_73425873425789
#define ZSTRING_H_73425873425789

#include "string_base.h"


    using Zchar = char;
    #define Zstr(x) x
    const Zchar FILE_NAME_SEPARATOR = '/';

//"The reason for all the fuss above" - Loki/SmartPtr
//a high-performance string for interfacing with native OS APIs in multithreaded contexts
using Zstring = zen::Zbase<Zchar>;


int cmpStringNoCase(const wchar_t* lhs, size_t lhsLen, const wchar_t* rhs, size_t rhsLen);
    int cmpStringNoCase(const char*    lhs, size_t lhsLen, const char*    rhs, size_t rhsLen);

template <class S, class T> inline
bool equalNoCase(const S& lhs, const T& rhs) { using namespace zen; return cmpStringNoCase(strBegin(lhs), strLength(lhs), strBegin(rhs), strLength(rhs)) == 0;  }

template <class S>
S makeUpperCopy(S str);


//Compare filepaths: Windows/OS X does NOT distinguish between upper/lower-case, while Linux DOES
int cmpFilePath(const wchar_t* lhs, size_t lhsLen, const wchar_t* rhs, size_t rhsLen);
    int cmpFilePath(const char* lhs, size_t lhsLen, const char* rhs, size_t rhsLen);

template <class S, class T> inline
bool equalFilePath(const S& lhs, const T& rhs) { using namespace zen; return cmpFilePath(strBegin(lhs), strLength(lhs), strBegin(rhs), strLength(rhs)) == 0;  }

struct LessFilePath
{
    template <class S, class T>
    bool operator()(const S& lhs, const T& rhs) const { using namespace zen; return cmpFilePath(strBegin(lhs), strLength(lhs), strBegin(rhs), strLength(rhs)) < 0; }
};



inline
Zstring appendSeparator(Zstring path) //support rvalue references!
{
    return zen::endsWith(path, FILE_NAME_SEPARATOR) ? path : (path += FILE_NAME_SEPARATOR); //returning a by-value parameter implicitly converts to r-value!
}


inline
Zstring getFileExtension(const Zstring& filePath)
{
    const Zstring shortName = afterLast(filePath, FILE_NAME_SEPARATOR, zen::IF_MISSING_RETURN_ALL);
    return afterLast(shortName, Zchar('.'), zen::IF_MISSING_RETURN_NONE);
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


template <class S, class T, class U>
S pathReplaceCpy(const S& str, const T& oldTerm, const U& newTerm, bool replaceAll = true);




//################################# inline implementation ########################################
inline
void makeUpperInPlace(wchar_t* str, size_t strLen)
{
    std::for_each(str, str + strLen, [](wchar_t& c) { c = std::towupper(c); }); //locale-dependent!
}


inline
void makeUpperInPlace(char* str, size_t strLen)
{
    std::for_each(str, str + strLen, [](char& c) { c = std::toupper(static_cast<unsigned char>(c)); }); //locale-dependent!
    //result of toupper() is an unsigned char mapped to int range, so the char representation is in the last 8 bits and we need not care about signedness!
    //this should work for UTF-8, too: all chars >= 128 are mapped upon themselves!
}


inline
int cmpStringNoCase(const wchar_t* lhs, size_t lhsLen, const wchar_t* rhs, size_t rhsLen)
{
    assert(std::find(lhs, lhs + lhsLen, 0) == lhs + lhsLen); //don't expect embedded nulls!
    assert(std::find(rhs, rhs + rhsLen, 0) == rhs + rhsLen); //

    const int rv = ::wcsncasecmp(lhs, rhs, std::min(lhsLen, rhsLen)); //locale-dependent!
    if (rv != 0)
        return rv;
    return static_cast<int>(lhsLen) - static_cast<int>(rhsLen);
}


inline
int cmpStringNoCase(const char* lhs, size_t lhsLen, const char* rhs, size_t rhsLen)
{
    assert(std::find(lhs, lhs + lhsLen, 0) == lhs + lhsLen); //don't expect embedded nulls!
    assert(std::find(rhs, rhs + rhsLen, 0) == rhs + rhsLen); //

    const int rv = ::strncasecmp(lhs, rhs, std::min(lhsLen, rhsLen)); //locale-dependent!
    if (rv != 0)
        return rv;
    return static_cast<int>(lhsLen) - static_cast<int>(rhsLen);
}


template <class S> inline
S makeUpperCopy(S str)
{
    const size_t len = str.length(); //we assert S is a string type!
    if (len > 0)
        makeUpperInPlace(&*str.begin(), len);

    return std::move(str); //"str" is an l-value parameter => no copy elision!
}


inline
int cmpFilePath(const wchar_t* lhs, size_t lhsLen, const wchar_t* rhs, size_t rhsLen)
{
    assert(std::find(lhs, lhs + lhsLen, 0) == lhs + lhsLen); //don't expect embedded nulls!
    assert(std::find(rhs, rhs + rhsLen, 0) == rhs + rhsLen); //

    const int rv = std::wcsncmp(lhs, rhs, std::min(lhsLen, rhsLen));
    if (rv != 0)
        return rv;
    return static_cast<int>(lhsLen) - static_cast<int>(rhsLen);
}


inline
int cmpFilePath(const char* lhs, size_t lhsLen, const char* rhs, size_t rhsLen)
{
    assert(std::find(lhs, lhs + lhsLen, 0) == lhs + lhsLen); //don't expect embedded nulls!
    assert(std::find(rhs, rhs + rhsLen, 0) == rhs + rhsLen); //

    const int rv = std::strncmp(lhs, rhs, std::min(lhsLen, rhsLen));
    if (rv != 0)
        return rv;
    return static_cast<int>(lhsLen) - static_cast<int>(rhsLen);
}


template <class S, class T, class U> inline
S pathReplaceCpy(const S& str, const T& oldTerm, const U& newTerm, bool replaceAll)
{
    assert(!contains(str, Zchar('\0')));

    return replaceCpy(str, oldTerm, newTerm, replaceAll);
}


//---------------------------------------------------------------------------
//ZEN macro consistency checks:


#endif //ZSTRING_H_73425873425789
