// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef STRING_TOOLS_HEADER_213458973046
#define STRING_TOOLS_HEADER_213458973046

#include <cstddef> //size_t
#include <cctype>  //isspace
#include <cwctype> //iswspace
#include <algorithm>
#include <cassert>
#include <sstream>
#include <functional>
#include <vector>
#include "loki/TypeTraits.h"


//enhance arbitray string class with useful non-member functions:
namespace zen
{
template <class C> size_t cStringLength(const C* input); //strlen()
template <class C> bool cStringIsWhiteSpace(C ch);
template <class C> bool cStringIsDigit(C ch);

template <class S>          bool startsWith(const S& str, typename S::value_type prefix);
template <class S>          bool startsWith(const S& str, const typename S::value_type* prefix);
template <class S, class T> bool startsWith(const S& str, const T& prefix, typename T::value_type dummy = 0); //SFINAE: T must be a "string"
template <class S>          bool endsWith(const S& str, typename S::value_type postfix);
template <class S>          bool endsWith(const S& str, const typename S::value_type* postfix);
template <class S, class T> bool endsWith(const S& str, const T& postfix, typename T::value_type dummy = 0); //SFINAE: T must be a "string"

template <class S> S afterLast  (const S& str, typename S::value_type ch); //returns the whole string if ch not found
template <class S> S beforeLast (const S& str, typename S::value_type ch); //returns empty string if ch not found
template <class S> S afterFirst (const S& str, typename S::value_type ch); //returns empty string if ch not found
template <class S> S beforeFirst(const S& str, typename S::value_type ch); //returns the whole string if ch not found

template <class S, class T> std::vector<S> split(const S& str, const T& delimiter);
template <class S> void truncate(S& str, size_t newLen);
template <class S> void replace(S& str, const typename S::value_type* old, const typename S::value_type* replacement, bool replaceAll);
template <class S> void trim(S& str, bool fromLeft = true, bool fromRight = true);

//formatted number conversion the C++ way
template <class S, class Num> S toString(const Num& number);
template <class Num, class S> Num toNumber(const S& str);








































//---------------------- implementation ----------------------

template <class C>
inline
size_t cStringLength(const C* input) //strlen()
{
    const C* iter = input;
    while (*iter != 0)
        ++iter;
    return iter - input;
}


template <>
inline
bool cStringIsWhiteSpace(char ch)
{
    const unsigned char usc = ch; //caveat 1: std::isspace() takes an int, but expects an unsigned char
    return usc < 128 &&           //caveat 2: some parts of UTF-8 chars are erroneously seen as whitespace, e.g. the a0 from "\xec\x8b\xa0" (MSVC)
           std::isspace(usc) != 0;
}

template <>
inline
bool cStringIsWhiteSpace(wchar_t ch)
{
    return std::iswspace(ch) != 0; //some compilers (e.g. VC++ 6.0) return true for a call to isspace('\xEA'); but no issue with newer versions of MSVC
}


template <>
inline
bool cStringIsDigit(char ch)
{
    return std::isdigit(static_cast<unsigned char>(ch)) != 0; //caveat: takes an int, but expects an unsigned char
}


template <>
inline
bool cStringIsDigit(wchar_t ch)
{
    return std::iswdigit(ch) != 0;
}


template <class S>
inline
bool startsWith(const S& str, typename S::value_type prefix)
{
    return str.length() != 0 && str.operator[](0) == prefix;
}


template <class S>
inline
bool startsWith(const S& str, const typename S::value_type* prefix)
{
    const size_t pfLength = cStringLength(prefix);
    if (str.length() < pfLength)
        return false;

    return std::equal(str.c_str(), str.c_str() + pfLength,
                      prefix);
}


template <class S, class T>
inline
bool startsWith(const S& str, const T& prefix, typename T::value_type)
{
    if (str.length() < prefix.length())
        return false;

    return std::equal(str.c_str(), str.c_str() + prefix.length(),
                      prefix.c_str());
}


template <class S>
inline
bool endsWith(const S& str, typename S::value_type postfix)
{
    const size_t len = str.length();
    return len != 0 && str.operator[](len - 1) == postfix;
}


template <class S>
inline
bool endsWith(const S& str, const typename S::value_type* postfix)
{
    const size_t pfLength  = cStringLength(postfix);
    if (str.length() < pfLength)
        return false;

    const typename S::value_type* cmpBegin = str.c_str() + str.length() - pfLength;
    return std::equal(cmpBegin, cmpBegin + pfLength,
                      postfix);
}


template <class S, class T>
inline
bool endsWith(const S& str, const T& postfix, typename T::value_type)
{
    if (str.length() < postfix.length())
        return false;

    const typename S::value_type* cmpBegin = str.c_str() + str.length() - postfix.length();
    return std::equal(cmpBegin, cmpBegin + postfix.length(),
                      postfix.c_str());
}


// get all characters after the last occurence of ch
// (returns the whole string if ch not found)
template <class S>
inline
S afterLast(const S& str, typename S::value_type ch)
{
    const size_t pos = str.rfind(ch);
    if (pos != S::npos)
        return S(str.c_str() + pos + 1, str.length() - pos - 1);
    else
        return str;
}


// get all characters before the last occurence of ch
// (returns empty string if ch not found)
template <class S>
inline
S beforeLast(const S& str, typename S::value_type ch)
{
    const size_t pos = str.rfind(ch);
    if (pos != S::npos)
        return S(str.c_str(), pos); //data is non-empty string in this context: else ch would not have been found!
    else
        return S();
}


//returns empty string if ch not found
template <class S>
inline
S afterFirst(const S& str, typename S::value_type ch)
{
    const size_t pos = str.find(ch);
    if (pos != S::npos)
        return S(str.c_str() + pos + 1, str.length() - pos - 1);
    else
        return S();

}


//returns the whole string if ch not found
template <class S>
inline
S beforeFirst(const S& str, typename S::value_type ch)
{
    const size_t pos = str.find(ch, 0);
    if (pos != S::npos)
        return S(str.c_str(), pos); //data is non-empty string in this context: else ch would not have been found!
    else
        return str;
}


template <class S, class T>
inline
std::vector<S> split(const S& str, const T& delimiter)
{
    const S delim = S() + delimiter; //handle char, char* and string
    std::vector<S> output;
    size_t bockStart = 0;
    if (!delim.empty())
    {
        for (size_t blockEnd = str.find(delim, bockStart);
             blockEnd != S::npos;
             bockStart = blockEnd + delim.size(), blockEnd = str.find(delim, bockStart))
        {
            output.push_back(S(str.c_str() + bockStart, blockEnd - bockStart));
        }
    }
    output.push_back(S(str.c_str() + bockStart, str.length() - bockStart));
    return output;
}


template <class S>
inline
void truncate(S& str, size_t newLen)
{
    if (newLen < str.length())
        str.resize(newLen);
}


template <class S>
inline
void replace(S& str, const typename S::value_type* old, const typename S::value_type* replacement, bool replaceAll)
{
    const size_t oldLen         = cStringLength(old);
    const size_t replacementLen = cStringLength(replacement);

    size_t pos = 0;
    while ((pos = str.find(old, pos)) != S::npos)
    {
        str.replace(pos, oldLen, replacement, replacementLen);
        pos += replacementLen; //move past the string that was replaced

        if (!replaceAll)
            break;
    }
}


template <class S>
inline
void trim(S& str, bool fromLeft, bool fromRight)
{
    assert(fromLeft || fromRight);

    typedef typename S::value_type CharType;

    const CharType* newBegin = str.c_str();
    const CharType* newEnd   = str.c_str() + str.length();

    if (fromRight)
        while (newBegin != newEnd && cStringIsWhiteSpace(newEnd[-1]))
            --newEnd;

    if (fromLeft)
        while (newBegin != newEnd && cStringIsWhiteSpace(*newBegin))
            ++newBegin;

    const size_t newLength = newEnd - newBegin;
    if (newLength != str.length())
    {
        if (newBegin != str.c_str())
            str = S(newBegin, newLength); //minor inefficiency: in case "str" is not shared, we could save an allocation and do a memory move only
        else
            str.resize(newLength);
    }
}


namespace
{
template <class S, class T>
struct CnvtStringToString
{
    T convert(const S& src) const { return T(src.c_str(), src.size()); }
};

template <class S>
struct CnvtStringToString<S, S> //optimization: for "basic_string -> basic_string" we don't need a deep copy
{
    S convert(const S& src) const { return src; }
};


template <class S, class Num>
struct CvrtNumberToString
{
    S convert(const Num& number) const //convert string to number using streams: convenient, but SLOW
    {
        typedef typename S::value_type CharType;

        std::basic_ostringstream<CharType> ss;
        ss << number;
        return CnvtStringToString<std::basic_string<CharType>, S>().convert(ss.str());
    }
};


template <class S, class Num, bool isIntegral>
struct CvrtStringToNumber
{
    Num convert(const S& str) const //convert number to string using streams: convenient, but SLOW
    {
        typedef typename S::value_type CharType;

        Num number = 0;
        std::basic_istringstream<CharType>(CnvtStringToString<S, std::basic_string<CharType> >().convert(str)) >> number;
        return number;
    }
};


template <class S, class Num>
struct CvrtStringToNumber<S, Num, true>
{
    Num convert(const S& str) const //very fast conversion to integers: slightly faster than std::atoi, but more importantly: generic
    {
        typedef typename S::value_type CharType;

        const CharType* first = str.c_str();
        const CharType* last  = first + str.size();

        while (first != last && cStringIsWhiteSpace(*first)) //skip leading whitespace
            ++first;

        bool hasMinusSign = false; //handle minus sign
        if (first != last)
        {
            if (*first == '-')
            {
                hasMinusSign = true;
                ++first;
            }
            else if (*first == '+')
                ++first;
        }

        Num number = 0;
        for (const CharType* iter = first; iter != last; ++iter)
        {
            const CharType c = *iter;
            if ('0' <= c && c <= '9')
            {
                number *= 10;
                number += c - '0';
            }
            else
            {
                assert(std::find_if(iter, last, std::not1(std::ptr_fun(&cStringIsWhiteSpace<CharType>))) == last); //rest of string should contain whitespace only
                break;
            }
        }

        return hasMinusSign ? -number : number;
    }
};
}


template <class S, class Num>
inline
S toString(const Num& number) //convert number to string the C++ way
{
    return CvrtNumberToString<S, Num>().convert(number);
}


template <class Num, class S>
inline
Num toNumber(const S& str) //convert number to string the C++ way
{
    return CvrtStringToNumber<S, Num, Loki::TypeTraits<Num>::isIntegral>().convert(str);
}

}

#endif //STRING_TOOLS_HEADER_213458973046
