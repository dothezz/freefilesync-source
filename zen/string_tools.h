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
#include <vector>
#include <sstream>
#include "stl_tools.h"
#include "string_traits.h"


//enhance arbitray string class with useful non-member functions:
namespace zen
{
template <class Char> bool cStringIsWhiteSpace(Char ch);
template <class Char> bool cStringIsDigit     (Char ch); //not exactly the same as "std::isdigit" -> we consider '0'-'9' only!


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

//high-performance conversion between numbers and strings
template <class S, class T, class Num> S printNumber(const T& format, const Num& number); //format a single number using std::snprintf()

template <class S,   class Num> S   toString(const Num& number);
template <class Num, class S  > Num toNumber(const S&   str);

//string to string conversion: converts string-like type into char-compatible target string class
template <class T, class S> T copyStringTo(const S& str);
































//---------------------- implementation ----------------------
template <> inline
bool cStringIsWhiteSpace(char ch)
{
    //caveat 1: std::isspace() takes an int, but expects an unsigned char
    //caveat 2: some parts of UTF-8 chars are erroneously seen as whitespace, e.g. the a0 from "\xec\x8b\xa0" (MSVC)
    return static_cast<unsigned char>(ch) < 128 &&
           std::isspace(static_cast<unsigned char>(ch)) != 0;
}

template <> inline bool cStringIsWhiteSpace(wchar_t ch) { return std::iswspace(ch) != 0; }


template <class Char> inline
bool cStringIsDigit(Char ch) //similar to implmenetation of std::::isdigit()!
{
    assert_static((IsSameType<Char, char>::result || IsSameType<Char, wchar_t>::result));
    return static_cast<Char>('0') <= ch && ch <= static_cast<Char>('9');
}


template <class S, class T> inline
bool startsWith(const S& str, const T& prefix)
{
    assert_static(IsStringLike<S>::result);
    assert_static(IsStringLike<T>::result);
    typedef typename GetCharType<S>::Result CharType;

    const size_t pfLength = strLength(prefix);
    if (strLength(str) < pfLength)
        return false;

    const CharType* const strFirst = strBegin(str);
    return std::equal(strFirst, strFirst + pfLength,
                      strBegin(prefix));
}


template <class S, class T> inline
bool endsWith(const S& str, const T& postfix)
{
    assert_static(IsStringLike<S>::result);
    assert_static(IsStringLike<T>::result);
    typedef typename GetCharType<S>::Result CharType;

    const size_t strLen = strLength(str);
    const size_t pfLen  = strLength(postfix);
    if (strLen < pfLen)
        return false;

    const CharType* const cmpFirst = strBegin(str) + strLen - pfLen;
    return std::equal(cmpFirst, cmpFirst + pfLen,
                      strBegin(postfix));
}


//returns the whole string if ch not found
template <class S, class T> inline
S afterLast(const S& str, const T& ch)
{
    assert_static(IsStringLike<T>::result);
    typedef typename GetCharType<S>::Result CharType;

    const size_t chLen = strLength(ch);

    const CharType* const strFirst = strBegin(str);
    const CharType* const strLast  = strFirst + strLength(str);
    const CharType* const chFirst  = strBegin(ch);

    const CharType* iter = search_last(strFirst, strLast,
                                       chFirst, chFirst + chLen);
    if (iter == strLast)
        return str;

    iter += chLen;
    return S(iter, strLast - iter);
}


//returns empty string if ch not found
template <class S, class T> inline
S beforeLast(const S& str, const T& ch)
{
    assert_static(IsStringLike<T>::result);
    typedef typename GetCharType<S>::Result CharType;

    const CharType* const strFirst = strBegin(str);
    const CharType* const strLast  = strFirst + strLength(str);
    const CharType* const chFirst  = strBegin(ch);

    const CharType* iter = search_last(strFirst, strLast,
                                       chFirst,  chFirst + strLength(ch));
    if (iter == strLast)
        return S();

    return S(strFirst, iter - strFirst);
}


//returns empty string if ch not found
template <class S, class T> inline
S afterFirst(const S& str, const T& ch)
{
    assert_static(IsStringLike<T>::result);
    typedef typename GetCharType<S>::Result CharType;

    const size_t chLen = strLength(ch);
    const CharType* const strFirst = strBegin(str);
    const CharType* const strLast  = strFirst + strLength(str);
    const CharType* const chFirst  = strBegin(ch);

    const CharType* iter = std::search(strFirst, strLast,
                                       chFirst,  chFirst + chLen);
    if (iter == strLast)
        return S();
    iter += chLen;

    return S(iter, strLast - iter);
}


//returns the whole string if ch not found
template <class S, class T> inline
S beforeFirst(const S& str, const T& ch)
{
    assert_static(IsStringLike<T>::result);
    typedef typename GetCharType<S>::Result CharType;

    const CharType* const strFirst = strBegin(str);
    const CharType* const chFirst  = strBegin(ch);

    return S(strFirst, std::search(strFirst, strFirst + strLength(str),
                                   chFirst,  chFirst  + strLength(ch)) - strFirst);
}


template <class S, class T> inline
std::vector<S> split(const S& str, const T& delimiter)
{
    assert_static(IsStringLike<T>::result);
    typedef typename GetCharType<S>::Result CharType;

    std::vector<S> output;

    const size_t delimLen = strLength(delimiter);

    if (delimLen == 0)
        output.push_back(str);
    else
    {
        const CharType* const delimFirst = strBegin(delimiter);
        const CharType* const delimLast  = delimFirst + delimLen;

        const CharType* blockStart    = strBegin(str);
        const CharType* const strLast = blockStart + strLength(str);

        for (;;)
        {
            const CharType* const blockEnd = std::search(blockStart, strLast,
                                                         delimFirst, delimLast);

            output.push_back(S(blockStart, blockEnd - blockStart));
            if (blockEnd == strLast)
                break;
            blockStart = blockEnd + delimLen;
        }
    }
    return output;
}


template <class S> inline
void truncate(S& str, size_t newLen)
{
    if (newLen < str.length())
        str.resize(newLen);
}


namespace implementation
{
ZEN_INIT_DETECT_MEMBER(append);

//either call operator+=(S(str, len)) or append(str, len)
template <class S, class Char> inline
typename EnableIf<HasMember_append<S>::result>::Result stringAppend(S& str, const Char* other, size_t len) { str.append(other, len); }

template <class S, class Char> inline
typename EnableIf<!HasMember_append<S>::result>::Result stringAppend(S& str, const Char* other, size_t len) { str += S(other, len); }
}


template <class S, class T, class U> inline
S replaceCpy(const S& str, const T& oldOne, const U& newOne, bool replaceAll)
{
    assert_static(IsStringLike<T>::result);
    assert_static(IsStringLike<U>::result);

    typedef typename GetCharType<S>::Result CharType;

    const size_t oldLen = strLength(oldOne);
    const size_t newLen = strLength(newOne);

    S output;

    const CharType*       strPos = strBegin(str);
    const CharType* const strEnd = strPos + strLength(str);

    const CharType* const oldBegin = strBegin(oldOne);
    const CharType* const newBegin = strBegin(newOne);

    for (;;)
    {
        const CharType* ptr = std::search(strPos, strEnd,
                                          oldBegin, oldBegin + oldLen);
        if (ptr == strEnd)
            break;

        implementation::stringAppend(output, strPos, ptr - strPos);
        implementation::stringAppend(output, newBegin, newLen);

        strPos = ptr + oldLen;

        if (!replaceAll)
            break;
    }
    implementation::stringAppend(output, strPos, strEnd - strPos);

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

    typedef typename GetCharType<S>::Result CharType; //don't use value_type! (wxString, Glib::ustring)

    const CharType* const oldBegin = str.c_str();
    const CharType* newBegin = oldBegin;
    const CharType* newEnd   = oldBegin + str.length();

    if (fromRight)
        while (newBegin != newEnd && cStringIsWhiteSpace(newEnd[-1]))
            --newEnd;

    if (fromLeft)
        while (newBegin != newEnd && cStringIsWhiteSpace(*newBegin))
            ++newBegin;

    const size_t newLength = newEnd - newBegin;
    if (newLength != str.length())
    {
        if (newBegin != oldBegin)
            str = S(newBegin, newLength); //minor inefficiency: in case "str" is not shared, we could save an allocation and do a memory move only
        else
            str.resize(newLength);
    }
}


namespace implementation
{
template <class S, class T>
struct CopyStringToString
{
    T copy(const S& src) const { return T(strBegin(src), strLength(src)); }
};

template <class S>
struct CopyStringToString<S, S> //perf: we don't need a deep copy if string types match
{
    const S& copy(const S& src) const { return src; }
};
}

template <class T, class S> inline
T copyStringTo(const S& str) { return implementation::CopyStringToString<S, T>().copy(str); }


namespace implementation
{
template <class Num> inline
int saferPrintf(char* buffer, size_t bufferSize, const char* format, const Num& number) //there is no such thing as a "safe" printf ;)
{
#ifdef _MSC_VER
    return ::_snprintf(buffer, bufferSize, format, number); //VS2010 doesn't respect ISO C
#else
    return std::snprintf(buffer, bufferSize, format, number); //C99
#endif
}

template <class Num> inline
int saferPrintf(wchar_t* buffer, size_t bufferSize, const wchar_t* format, const Num& number)
{
#ifdef __MINGW32__
    return ::snwprintf(buffer, bufferSize, format, number); //MinGW doesn't respect ISO C
#else
    return std::swprintf(buffer, bufferSize, format, number); //C99
#endif
}
}

template <class S, class T, class Num> inline
S printNumber(const T& format, const Num& number) //format a single number using ::sprintf
{
    assert_static(IsStringLike<T>::result);
    assert_static((IsSameType<
                   typename GetCharType<S>::Result,
                   typename GetCharType<T>::Result>::result));

    typedef typename GetCharType<S>::Result CharType;

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


template <class S, class Num> inline
S toString(const Num& number, Int2Type<NUM_TYPE_OTHER>) //default number to string conversion using streams: convenient, but SLOW, SLOW, SLOW!!!! (~ factor of 20)
{
    typedef typename GetCharType<S>::Result CharType;

    std::basic_ostringstream<CharType> ss;
    ss << number;
    return copyStringTo<S>(ss.str());
}


template <class S, class Num> inline S floatToString(const Num& number, char   ) { return printNumber<S>( "%g", static_cast<double>(number)); }
template <class S, class Num> inline S floatToString(const Num& number, wchar_t) { return printNumber<S>(L"%g", static_cast<double>(number)); }

template <class S, class Num> inline
S toString(const Num& number, Int2Type<NUM_TYPE_FLOATING_POINT>)
{
    return floatToString<S>(number, typename GetCharType<S>::Result());
}


/*
perf: integer to string: (executed 10 mio. times)
	std::stringstream - 14796 ms
	std::sprintf      -  3086 ms
	formatInteger     -   778 ms
*/

template <class S, class Num> inline
S formatInteger(Num n, bool hasMinus)
{
    typedef typename GetCharType<S>::Result CharType;

    assert(n >= 0);
    S output;
    do
    {
        output += static_cast<CharType>('0' + n % 10);
        n /= 10;
    }
    while (n != 0);
    if (hasMinus)
        output += static_cast<CharType>('-');

    std::reverse(output.begin(), output.end());
    return output;
}

template <class S, class Num> inline
S toString(const Num& number, Int2Type<NUM_TYPE_SIGNED_INT>)
{
    return formatInteger<S>(number < 0 ? -number : number, number < 0);
}


template <class S, class Num> inline
S toString(const Num& number, Int2Type<NUM_TYPE_UNSIGNED_INT>)
{
    return formatInteger<S>(number, false);
}

//--------------------------------------------------------------------------------


template <class Num, class S> inline
Num toNumber(const S& str, Int2Type<NUM_TYPE_OTHER>) //default string to number conversion using streams: convenient, but SLOW
{
    typedef typename GetCharType<S>::Result CharType;
    Num number = 0;
    std::basic_istringstream<CharType>(copyStringTo<std::basic_string<CharType> >(str)) >> number;
    return number;
}


template <class Num> inline Num stringToFloat(const char*    str) { return std::strtod(str, NULL); }
template <class Num> inline Num stringToFloat(const wchar_t* str) { return std::wcstod(str, NULL); }

template <class Num, class S> inline
Num toNumber(const S& str, Int2Type<NUM_TYPE_FLOATING_POINT>)
{
    return stringToFloat<Num>(strBegin(str));
}

template <class Num, class S>
Num extractInteger(const S& str, bool& hasMinusSign) //very fast conversion to integers: slightly faster than std::atoi, but more importantly: generic
{
    typedef typename GetCharType<S>::Result CharType;

    const CharType* first = strBegin(str);
    const CharType* last  = first + strLength(str);

    while (first != last && cStringIsWhiteSpace(*first)) //skip leading whitespace
        ++first;

    //handle minus sign
    hasMinusSign = false;
    if (first != last)
    {
        if (*first == static_cast<CharType>('-'))
        {
            hasMinusSign = true;
            ++first;
        }
        else if (*first == static_cast<CharType>('+'))
            ++first;
    }

    Num number = 0;
    for (const CharType* iter = first; iter != last; ++iter)
    {
        const CharType c = *iter;
        if (static_cast<CharType>('0') <= c && c <= static_cast<CharType>('9'))
        {
            number *= 10;
            number += c - static_cast<CharType>('0');
        }
        else
        {
            //rest of string should contain whitespace only
            //assert(std::find_if(iter, last, std::not1(std::ptr_fun(&cStringIsWhiteSpace<CharType>))) == last); -> this is NO assert situation
            break;
        }
    }
    return number;
}


template <class Num, class S> inline
Num toNumber(const S& str, Int2Type<NUM_TYPE_SIGNED_INT>)
{
    bool hasMinusSign = false; //handle minus sign
    const Num number = extractInteger<Num>(str, hasMinusSign);
    return hasMinusSign ? -number : number;
}


template <class Num, class S> inline
Num toNumber(const S& str, Int2Type<NUM_TYPE_UNSIGNED_INT>) //very fast conversion to integers: slightly faster than std::atoi, but more importantly: generic
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
}


template <class S, class Num> inline
S toString(const Num& number) //convert number to string the C++ way
{
    typedef Int2Type<
    IsSignedInt  <Num>::result ? implementation::NUM_TYPE_SIGNED_INT :
    IsUnsignedInt<Num>::result ? implementation::NUM_TYPE_UNSIGNED_INT :
    IsFloat      <Num>::result ? implementation::NUM_TYPE_FLOATING_POINT :
    implementation::NUM_TYPE_OTHER> TypeTag;

    return implementation::toString<S>(number, TypeTag());
}


template <class Num, class S> inline
Num toNumber(const S& str) //convert string to number the C++ way
{
    typedef Int2Type<
    IsSignedInt  <Num>::result ? implementation::NUM_TYPE_SIGNED_INT :
    IsUnsignedInt<Num>::result ? implementation::NUM_TYPE_UNSIGNED_INT :
    IsFloat      <Num>::result ? implementation::NUM_TYPE_FLOATING_POINT :
    implementation::NUM_TYPE_OTHER> TypeTag;

    return implementation::toNumber<Num>(str, TypeTag());
}

}

#endif //STRING_TOOLS_HEADER_213458973046
