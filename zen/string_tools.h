// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License, Version 1.0. See accompanying file             *
// * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.       *
// * Copyright (C) 2011 ZenJu (zhnmju123 AT gmx.de)                         *
// **************************************************************************

#ifndef STRING_TOOLS_HEADER_213458973046
#define STRING_TOOLS_HEADER_213458973046

#include <cctype>  //isspace
#include <cwctype> //iswspace
#include <cstdio>  //sprintf
#include <cwchar>  //swprintf
#include <algorithm>
#include <cassert>
#include <sstream>
#include <vector>
#include "string_traits.h"
#include "type_traits.h"


//enhance arbitray string class with useful non-member functions:
namespace zen
{
template <class C> bool cStringIsWhiteSpace(C ch);
template <class C> bool cStringIsDigit(C ch);

template <class S, class T> bool startsWith(const S& str, const T& prefix);  //both S and T can be strings or char/wchar_t arrays or simple char/wchar_t
template <class S, class T> bool endsWith  (const S& str, const T& postfix); //

template <class S, class T> S afterLast  (const S& str, const T& ch); //returns the whole string if ch not found
template <class S, class T> S beforeLast (const S& str, const T& ch); //returns empty string if ch not found
template <class S, class T> S afterFirst (const S& str, const T& ch); //returns empty string if ch not found
template <class S, class T> S beforeFirst(const S& str, const T& ch); //returns the whole string if ch not found

template <class S, class T> std::vector<S> split(const S& str, const T& delimiter);
template <class S> void truncate(S& str, size_t newLen);
template <class S> void trim(S& str, bool fromLeft = true, bool fromRight = true);
template <class S, class T, class U> void replace   (      S& str, const T& oldOne, const U& newOne, bool replaceAll = true);
template <class S, class T, class U> S    replaceCpy(const S& str, const T& oldOne, const U& newOne, bool replaceAll = true);

//high-performance conversion from numbers to strings
template <class S, class T, class Num> S printNumber(const T& format, const Num& number); //format a single number using ::sprintf

template <class S,   class Num> S   toString(const Num& number);
template <class Num, class S  > Num toNumber(const S&   str);

//string to string conversion: converst string-like type into char-compatible target string class
template <class T, class S> T cvrtString(const S& str);
































//---------------------- implementation ----------------------
template <> inline
bool cStringIsWhiteSpace(char ch)
{
    //caveat 1: std::isspace() takes an int, but expects an unsigned char
    //caveat 2: some parts of UTF-8 chars are erroneously seen as whitespace, e.g. the a0 from "\xec\x8b\xa0" (MSVC)
    return static_cast<unsigned char>(ch) < 128 &&
           std::isspace(static_cast<unsigned char>(ch)) != 0;
}

//template <> inline bool cStringIsWhiteSpace(unsigned char ch) { return cStringIsWhiteSpace<char>(ch); } -> not character types!
//template <> inline bool cStringIsWhiteSpace(signed char ch)   { return cStringIsWhiteSpace<char>(ch); }
template <> inline bool cStringIsWhiteSpace(wchar_t ch)       { return std::iswspace(ch) != 0; }

template <> inline
bool cStringIsDigit(char ch)
{
    return std::isdigit(static_cast<unsigned char>(ch)) != 0; //caveat: takes an int, but expects an unsigned char
}


template <> inline
bool cStringIsDigit(wchar_t ch)
{
    return std::iswdigit(ch) != 0;
}


template <class S, class T> inline
bool startsWith(const S& str, const T& prefix)
{
    assert_static(StringTraits<S>::isStringLike);
    assert_static(StringTraits<T>::isStringLike);

    const size_t pfLength = strLength(prefix);
    if (strLength(str) < pfLength)
        return false;

    return std::equal(strBegin(str), strBegin(str) + pfLength,
                      strBegin(prefix));
}


template <class S, class T> inline
bool endsWith(const S& str, const T& postfix)
{
    assert_static(StringTraits<S>::isStringLike);
    assert_static(StringTraits<T>::isStringLike);

    size_t strLen = strLength(str);
    size_t pfLen  = strLength(postfix);
    if (strLen < pfLen)
        return false;

    typedef typename StringTraits<S>::CharType CharType;

    const CharType* cmpBegin = strBegin(str) + strLen - pfLen;
    return std::equal(cmpBegin, cmpBegin + pfLen,
                      strBegin(postfix));
}


//returns the whole string if ch not found
template <class S, class T> inline
S afterLast(const S& str, const T& ch)
{
    assert_static(StringTraits<T>::isStringLike);

    const size_t pos = str.rfind(ch);
    if (pos != S::npos)
    {
        size_t chLen = strLength(ch);
        return S(str.c_str() + pos + chLen, str.length() - pos - chLen);
    }
    else
        return str;
}


//returns empty string if ch not found
template <class S, class T> inline
S beforeLast(const S& str, const T& ch)
{
    assert_static(StringTraits<T>::isStringLike);

    const size_t pos = str.rfind(ch);
    if (pos != S::npos)
        return S(str.c_str(), pos); //data is non-empty string in this context: else ch would not have been found!
    else
        return S();
}


//returns empty string if ch not found
template <class S, class T> inline
S afterFirst(const S& str, const T& ch)
{
    assert_static(StringTraits<T>::isStringLike);

    const size_t pos = str.find(ch);
    if (pos != S::npos)
    {
        size_t chLen = strLength(ch);
        return S(str.c_str() + pos + chLen, str.length() - pos - chLen);
    }
    else
        return S();

}


//returns the whole string if ch not found
template <class S, class T> inline
S beforeFirst(const S& str, const T& ch)
{
    assert_static(StringTraits<T>::isStringLike);

    const size_t pos = str.find(ch);
    if (pos != S::npos)
        return S(str.c_str(), pos); //data is non-empty string in this context: else ch would not have been found!
    else
        return str;
}


template <class S, class T> inline
std::vector<S> split(const S& str, const T& delimiter)
{
    assert_static(StringTraits<T>::isStringLike);

    std::vector<S> output;
    size_t bockStart = 0;
    size_t delimLen = strLength(delimiter);
    if (delimLen != 0)
    {
        for (size_t blockEnd = str.find(delimiter, bockStart);
             blockEnd != S::npos;
             bockStart = blockEnd + delimLen, blockEnd = str.find(delimiter, bockStart))
        {
            output.push_back(S(str.c_str() + bockStart, blockEnd - bockStart));
        }
    }
    output.push_back(S(str.c_str() + bockStart, str.length() - bockStart));
    return output;
}


template <class S> inline
void truncate(S& str, size_t newLen)
{
    if (newLen < str.length())
        str.resize(newLen);
}


template <class S, class T, class U> inline
S replaceCpy(const S& str, const T& oldOne, const U& newOne, bool replaceAll)
{
    assert_static(StringTraits<T>::isStringLike);
    assert_static(StringTraits<U>::isStringLike);

    typedef typename StringTraits<S>::CharType CharType;

    const size_t oldLen = strLength(oldOne);
    const size_t newLen = strLength(newOne);

    S output;

    const CharType* strPos = strBegin(str);
    const CharType* strEnd = strPos + strLength(str);

    for (;;)
    {
        const CharType* ptr = std::search(strPos, strEnd,
                                          strBegin(oldOne), strBegin(oldOne) + oldLen);
        if (ptr == strEnd)
            break;

        output += S(strPos, ptr - strPos);
        output += S(strBegin(newOne), newLen);

        strPos = ptr + oldLen;

        if (!replaceAll)
            break;
    }
    output += S(strPos, strEnd - strPos);

    return output;
}


template <class S, class T, class U> inline
void replace(S& str, const T& oldOne, const U& newOne, bool replaceAll)
{
    str = replaceCpy(str, oldOne, newOne, replaceAll);
}


template <class S> inline
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


namespace implementation
{
template <class S, class T>
struct CnvtStringToString
{
    T convert(const S& src) const { return T(strBegin(src), strLength(src)); }
};

template <class S>
struct CnvtStringToString<S, S> //perf: we don't need a deep copy if string types match
{
    const S& convert(const S& src) const { return src; }
};
}

template <class T, class S> inline
T cvrtString(const S& str) { return implementation::CnvtStringToString<S, T>().convert(str); }


namespace implementation
{
template <class Num> inline
int saferPrintf(char* buffer, size_t bufferSize, const char* format, const Num& number) //there is no such thing as a "safe" printf ;)
{
#ifdef _MSC_VER
    return ::_snprintf(buffer, bufferSize, format, number);  //VS2010 doesn't respect ISO C
#else
    return std::snprintf(buffer, bufferSize, format, number); //C99
#endif
}

template <class Num> inline
int saferPrintf(wchar_t* buffer, size_t bufferSize, const wchar_t* format, const Num& number)
{
#ifdef __MINGW32__ //MinGW doesn't respect ISO C
    return ::snwprintf(buffer, bufferSize, format, number);
#else
    return std::swprintf(buffer, bufferSize, format, number); //C99
#endif
}
}

template <class S, class T, class Num> inline
S printNumber(const T& format, const Num& number) //format a single number using ::sprintf
{
    assert_static(StringTraits<T>::isStringLike);
    assert_static((IsSameType<
                   typename StringTraits<S>::CharType,
                   typename StringTraits<T>::CharType>::result));

    typedef typename StringTraits<S>::CharType CharType;

    const int BUFFER_SIZE = 128;
    CharType buffer[BUFFER_SIZE];
    const int charsWritten = implementation::saferPrintf(buffer, BUFFER_SIZE, format, number);

    return charsWritten > 0 ? S(buffer, charsWritten) : S();
}


namespace implementation
{
enum NumberType
{
    NUM_TYPE_SIGNED_INT,
    NUM_TYPE_UNSIGNED_INT,
    NUM_TYPE_FLOATING_POINT,
    NUM_TYPE_OTHER,
};


template <class S, class Num, NumberType>
struct CvrtNumberToString
{
    S convert(const Num& number) const //default number to string conversion using streams: convenient, but SLOW, SLOW, SLOW!!!! (~ factor of 20)
    {
        typedef typename StringTraits<S>::CharType CharType;

        std::basic_ostringstream<CharType> ss;
        ss << number;
        return cvrtString<S>(ss.str());
    }
};


template <class S, class Num>
struct CvrtNumberToString<S, Num, NUM_TYPE_FLOATING_POINT>
{
    S convert(const Num& number) const { return convertFloat(number, typename StringTraits<S>::CharType()); }

private:
    S convertFloat(const Num& number, char   ) const { return printNumber<S>( "%g", static_cast<double>(number)); }
    S convertFloat(const Num& number, wchar_t) const { return printNumber<S>(L"%g", static_cast<double>(number)); }
};

/*
perf: integer to string: (executed 10 mio. times)
	std::stringstream - 14796 ms
	std::sprintf      -  3086 ms
	formatInteger     -   778 ms
*/

template <class S, class Num> inline
S formatInteger(Num n, bool hasMinus)
{
    assert(n >= 0);
    S output;
    do
    {
        output += '0' + n % 10;
        n /= 10;
    }
    while (n != 0);
    if (hasMinus)
        output += '-';

    std::reverse(output.begin(), output.end());
    return output;
}

template <class S, class Num>
struct CvrtNumberToString<S, Num, NUM_TYPE_SIGNED_INT>
{
    S convert(const Num& number) const { return formatInteger<S>(number < 0 ? -number : number, number < 0); }
};

template <class S, class Num>
struct CvrtNumberToString<S, Num, NUM_TYPE_UNSIGNED_INT>
{
    S convert(const Num& number) const { return formatInteger<S>(number, false); }
};

//--------------------------------------------------------------------------------

template <class S, class Num, NumberType>
struct CvrtStringToNumber
{
    Num convert(const S& str) const //default string to number conversion using streams: convenient, but SLOW
    {
        typedef typename StringTraits<S>::CharType CharType;
        Num number = 0;
        std::basic_istringstream<CharType>(cvrtString<std::basic_string<CharType> >(str)) >> number;
        return number;
    }
};


template <class S, class Num>
struct CvrtStringToNumber<S, Num, NUM_TYPE_FLOATING_POINT>
{
    Num convert(const S& str) const { return convertFloat(strBegin(str)); }

private:
    Num convertFloat(const char*    str) const { return std::strtod(str, NULL); }
    Num convertFloat(const wchar_t* str) const { return std::wcstod(str, NULL); }
};

template <class Num, class S>
Num extractInteger(const S& str, bool& hasMinusSign) //very fast conversion to integers: slightly faster than std::atoi, but more importantly: generic
{
    typedef typename StringTraits<S>::CharType CharType;

    const CharType* first = strBegin(str);
    const CharType* last  = first + strLength(str);

    while (first != last && cStringIsWhiteSpace(*first)) //skip leading whitespace
        ++first;

    hasMinusSign = false; //handle minus sign
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
    return number;
}


template <class S, class Num>
struct CvrtStringToNumber<S, Num, NUM_TYPE_SIGNED_INT>
{
    Num convert(const S& str) const
    {
        bool hasMinusSign = false; //handle minus sign
        const Num number = extractInteger<Num>(str, hasMinusSign);
        return hasMinusSign ? -number : number;
    }
};


template <class S, class Num>
struct CvrtStringToNumber<S, Num, NUM_TYPE_UNSIGNED_INT>
{
    Num convert(const S& str) const //very fast conversion to integers: slightly faster than std::atoi, but more importantly: generic
    {
        bool hasMinusSign = false; //handle minus sign
        const Num number = extractInteger<Num>(str, hasMinusSign);
        if (hasMinusSign)
        {
            assert(false);
            return 0U;
        }
        return number;
    }
};
}


template <class S, class Num>
inline
S toString(const Num& number) //convert number to string the C++ way
{
    using namespace implementation;
    return CvrtNumberToString<S, Num,
           IsSignedInt  <Num>::result ? NUM_TYPE_SIGNED_INT :
           IsUnsignedInt<Num>::result ? NUM_TYPE_UNSIGNED_INT :
           IsFloat      <Num>::result ? NUM_TYPE_FLOATING_POINT :
           NUM_TYPE_OTHER
           >().convert(number);
}


template <class Num, class S>
inline
Num toNumber(const S& str) //convert string to number the C++ way
{
    using namespace implementation;
    return CvrtStringToNumber<S, Num,
           IsSignedInt  <Num>::result ? NUM_TYPE_SIGNED_INT :
           IsUnsignedInt<Num>::result ? NUM_TYPE_UNSIGNED_INT :
           IsFloat      <Num>::result ? NUM_TYPE_FLOATING_POINT :
           NUM_TYPE_OTHER
           >().convert(str);
}

}

#endif //STRING_TOOLS_HEADER_213458973046
