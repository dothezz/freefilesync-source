// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License: http://www.boost.org/LICENSE_1_0.txt           *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
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
template <class Char> bool isWhiteSpace(Char ch);
template <class Char> bool isDigit     (Char ch); //not exactly the same as "std::isdigit" -> we consider '0'-'9' only!

template <class S, class T> bool startsWith(const S& str, const T& prefix);  //
template <class S, class T> bool endsWith  (const S& str, const T& postfix); //both S and T can be strings or char/wchar_t arrays or simple char/wchar_t
template <class S, class T> bool contains  (const S& str, const T& term);    //

template <class S, class T> S afterLast  (const S& str, const T& term); //returns the whole string if term not found
template <class S, class T> S beforeLast (const S& str, const T& term); //returns empty string if term not found
template <class S, class T> S afterFirst (const S& str, const T& term); //returns empty string if term not found
template <class S, class T> S beforeFirst(const S& str, const T& term); //returns the whole string if term not found

template <class S, class T> std::vector<S> split(const S& str, const T& delimiter);
template <class S> void trim(S& str, bool fromLeft = true, bool fromRight = true);
template <class S, class T, class U> void replace   (      S& str, const T& oldTerm, const U& newTerm, bool replaceAll = true);
template <class S, class T, class U> S    replaceCpy(const S& str, const T& oldTerm, const U& newTerm, bool replaceAll = true);

//high-performance conversion between numbers and strings
template <class S, class T, class Num> S printNumber(const T& format, const Num& number); //format a single number using std::snprintf()

template <class S,   class Num> S   numberTo(const Num& number);
template <class Num, class S  > Num stringTo(const S&   str);

//string to string conversion: converts string-like type into char-compatible target string class
template <class T, class S> T copyStringTo(const S& str);
































//---------------------- implementation ----------------------
template <> inline
bool isWhiteSpace(char ch)
{
    //caveat 1: std::isspace() takes an int, but expects an unsigned char
    //caveat 2: some parts of UTF-8 chars are erroneously seen as whitespace, e.g. the a0 from "\xec\x8b\xa0" (MSVC)
    return static_cast<unsigned char>(ch) < 128 &&
           std::isspace(static_cast<unsigned char>(ch)) != 0;
}

template <> inline
bool isWhiteSpace(wchar_t ch) { return std::iswspace(ch) != 0; }


template <class Char> inline
bool isDigit(Char ch) //similar to implmenetation of std::::isdigit()!
{
    assert_static((IsSameType<Char, char>::value || IsSameType<Char, wchar_t>::value));
    return static_cast<Char>('0') <= ch && ch <= static_cast<Char>('9');
}


template <class S, class T> inline
bool startsWith(const S& str, const T& prefix)
{
    assert_static(IsStringLike<S>::value && IsStringLike<T>::value);
    typedef typename GetCharType<S>::Type CharType;

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
    assert_static(IsStringLike<S>::value && IsStringLike<T>::value);
    typedef typename GetCharType<S>::Type CharType;

    const size_t strLen = strLength(str);
    const size_t pfLen  = strLength(postfix);
    if (strLen < pfLen)
        return false;

    const CharType* const cmpFirst = strBegin(str) + strLen - pfLen;
    return std::equal(cmpFirst, cmpFirst + pfLen,
                      strBegin(postfix));
}


template <class S, class T> inline
bool contains(const S& str, const T& term)
{
    assert_static(IsStringLike<S>::value && IsStringLike<T>::value);
    typedef typename GetCharType<S>::Type CharType;

    const size_t strLen  = strLength(str);
    const size_t termLen = strLength(term);
    if (strLen < termLen)
        return false;

    const CharType* const strFirst  = strBegin(str);
    const CharType* const strLast   = strFirst + strLen;
    const CharType* const termFirst = strBegin(term);

    return std::search(strFirst, strLast,
                       termFirst, termFirst + termLen) != strLast;
}


//returns the whole string if term not found
template <class S, class T> inline
S afterLast(const S& str, const T& term)
{
    assert_static(IsStringLike<T>::value);
    typedef typename GetCharType<S>::Type CharType;

    const size_t termLen = strLength(term);

    const CharType* const strFirst  = strBegin(str);
    const CharType* const strLast   = strFirst + strLength(str);
    const CharType* const termFirst = strBegin(term);

    const CharType* iter = search_last(strFirst, strLast,
                                       termFirst, termFirst + termLen);
    if (iter == strLast)
        return str;

    iter += termLen;
    return S(iter, strLast - iter);
}


//returns empty string if term not found
template <class S, class T> inline
S beforeLast(const S& str, const T& term)
{
    assert_static(IsStringLike<T>::value);
    typedef typename GetCharType<S>::Type CharType;

    const CharType* const strFirst  = strBegin(str);
    const CharType* const strLast   = strFirst + strLength(str);
    const CharType* const termFirst = strBegin(term);

    const CharType* iter = search_last(strFirst, strLast,
                                       termFirst, termFirst + strLength(term));
    if (iter == strLast)
        return S();

    return S(strFirst, iter - strFirst);
}


//returns empty string if term not found
template <class S, class T> inline
S afterFirst(const S& str, const T& term)
{
    assert_static(IsStringLike<T>::value);
    typedef typename GetCharType<S>::Type CharType;

    const size_t termLen = strLength(term);
    const CharType* const strFirst  = strBegin(str);
    const CharType* const strLast   = strFirst + strLength(str);
    const CharType* const termFirst = strBegin(term);

    const CharType* iter = std::search(strFirst, strLast,
                                       termFirst, termFirst + termLen);
    if (iter == strLast)
        return S();
    iter += termLen;

    return S(iter, strLast - iter);
}


//returns the whole string if term not found
template <class S, class T> inline
S beforeFirst(const S& str, const T& term)
{
    assert_static(IsStringLike<T>::value);
    typedef typename GetCharType<S>::Type CharType;

    const CharType* const strFirst  = strBegin(str);
    const CharType* const termFirst = strBegin(term);

    return S(strFirst, std::search(strFirst, strFirst + strLength(str),
                                   termFirst,  termFirst  + strLength(term)) - strFirst);
}


template <class S, class T> inline
std::vector<S> split(const S& str, const T& delimiter)
{
    assert_static(IsStringLike<T>::value);
    typedef typename GetCharType<S>::Type CharType;

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


namespace implementation
{
ZEN_INIT_DETECT_MEMBER(append);

//either call operator+=(S(str, len)) or append(str, len)
template <class S, class Char> inline
typename EnableIf<HasMember_append<S>::value>::Type stringAppend(S& str, const Char* other, size_t len) { str.append(other, len); }

template <class S, class Char> inline
typename EnableIf<!HasMember_append<S>::value>::Type stringAppend(S& str, const Char* other, size_t len) { str += S(other, len); }
}


template <class S, class T, class U> inline
S replaceCpy(const S& str, const T& oldTerm, const U& newTerm, bool replaceAll)
{
    assert_static(IsStringLike<T>::value && IsStringLike<U>::value);
    typedef typename GetCharType<S>::Type CharType;

    const size_t oldLen = strLength(oldTerm);
    if (oldLen == 0)
    {
        assert(false);
        return str;
    }

    const CharType*       strPos = strBegin(str);
    const CharType* const strEnd = strPos + strLength(str);

    const CharType* const oldBegin = strBegin(oldTerm);
    const CharType* const oldEnd   = oldBegin + oldLen;

    //optimize "oldTerm not found"
    const CharType* strMatch = std::search(strPos, strEnd,
                                           oldBegin, oldEnd);
    if (strMatch == strEnd)
        return str;

    const size_t newLen = strLength(newTerm);
    const CharType* const newBegin = strBegin(newTerm);
    S output;

    for (;;)
    {
        implementation::stringAppend(output, strPos, strMatch - strPos);
        implementation::stringAppend(output, newBegin, newLen);

        strPos = strMatch + oldLen;

        if (!replaceAll)
            break;

        strMatch = std::search(strPos, strEnd,
                               oldBegin, oldEnd);
        if (strMatch == strEnd)
            break;
    }
    implementation::stringAppend(output, strPos, strEnd - strPos);

    return output;
}


template <class S, class T, class U> inline
void replace(S& str, const T& oldTerm, const U& newTerm, bool replaceAll)
{
    str = replaceCpy(str, oldTerm, newTerm, replaceAll);
}


template <class S> inline
void trim(S& str, bool fromLeft, bool fromRight)
{
    assert(fromLeft || fromRight);
    typedef typename GetCharType<S>::Type CharType; //don't use value_type! (wxString, Glib::ustring)

    const CharType* const oldBegin = str.c_str();
    const CharType* newBegin = oldBegin;
    const CharType* newEnd   = oldBegin + str.length();

    if (fromRight)
        while (newBegin != newEnd && isWhiteSpace(newEnd[-1]))
            --newEnd;

    if (fromLeft)
        while (newBegin != newEnd && isWhiteSpace(*newBegin))
            ++newBegin;

    if (newBegin != oldBegin)
        str = S(newBegin, newEnd - newBegin); //minor inefficiency: in case "str" is not shared, we could save an allocation and do a memory move only
    else
        str.resize(newEnd - newBegin);
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
#if defined _MSC_VER || defined __MINGW32__
    return ::_snprintf(buffer, bufferSize, format, number); //by factor 10 faster than "std::snprintf" on Mingw and on par with std::sprintf()!!!
#else
    return std::snprintf(buffer, bufferSize, format, number); //C99
#endif
}

template <class Num> inline
int saferPrintf(wchar_t* buffer, size_t bufferSize, const wchar_t* format, const Num& number)
{
#ifdef __MINGW32__
    return ::_snwprintf(buffer, bufferSize, format, number); //MinGW doesn't respect ISO C
#else
    return std::swprintf(buffer, bufferSize, format, number); //C99
#endif
}
}

template <class S, class T, class Num> inline
S printNumber(const T& format, const Num& number) //format a single number using ::sprintf
{
    assert_static(IsStringLike<T>::value);
    assert_static((IsSameType<
                   typename GetCharType<S>::Type,
                   typename GetCharType<T>::Type>::value));

    typedef typename GetCharType<S>::Type CharType;

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
S numberTo(const Num& number, Int2Type<NUM_TYPE_OTHER>) //default number to string conversion using streams: convenient, but SLOW, SLOW, SLOW!!!! (~ factor of 20)
{
    typedef typename GetCharType<S>::Type CharType;

    std::basic_ostringstream<CharType> ss;
    ss << number;
    return copyStringTo<S>(ss.str());
}


template <class S, class Num> inline S floatToString(const Num& number, char   ) { return printNumber<S>( "%g", static_cast<double>(number)); }
template <class S, class Num> inline S floatToString(const Num& number, wchar_t) { return printNumber<S>(L"%g", static_cast<double>(number)); }

template <class S, class Num> inline
S numberTo(const Num& number, Int2Type<NUM_TYPE_FLOATING_POINT>)
{
    return floatToString<S>(number, typename GetCharType<S>::Type());
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
    assert(n >= 0);
    typedef typename GetCharType<S>::Type CharType;
    CharType buffer[2 + 5 * sizeof(Num) / 2]; //it's generally faster to use a buffer than to rely on String::operator+=() (in)efficiency
    //minimum required chars (+ sign char): 1 + ceil(ln_10 (256^sizeof(n))) =~ 1 + ceil(sizeof(n) * 2.4082) < 2 + floor(sizeof(n) * 2.5)

    auto iter = std::end(buffer);
    do
    {
        const Num tmp = n / 10;
        *--iter = static_cast<CharType>('0' + (n - tmp * 10)); //8% faster than using modulus operator!
        n = tmp;
    }
    while (n != 0);

    if (hasMinus)
        *--iter = static_cast<CharType>('-');

    return S(&*iter, std::end(buffer) - iter);
}

template <class S, class Num> inline
S numberTo(const Num& number, Int2Type<NUM_TYPE_SIGNED_INT>)
{
    return formatInteger<S>(number < 0 ? -number : number, number < 0);
    //bug for "INT_MIN"! technically -INT_MIN == INT_MIN -> not worth the trouble
}


template <class S, class Num> inline
S numberTo(const Num& number, Int2Type<NUM_TYPE_UNSIGNED_INT>)
{
    return formatInteger<S>(number, false);
}

//--------------------------------------------------------------------------------

template <class Num, class S> inline
Num stringTo(const S& str, Int2Type<NUM_TYPE_OTHER>) //default string to number conversion using streams: convenient, but SLOW
{
    typedef typename GetCharType<S>::Type CharType;
    Num number = 0;
    std::basic_istringstream<CharType>(copyStringTo<std::basic_string<CharType> >(str)) >> number;
    return number;
}


template <class Num> inline Num stringToFloat(const char*    str) { return std::strtod(str, nullptr); }
template <class Num> inline Num stringToFloat(const wchar_t* str) { return std::wcstod(str, nullptr); }

template <class Num, class S> inline
Num stringTo(const S& str, Int2Type<NUM_TYPE_FLOATING_POINT>)
{
    return stringToFloat<Num>(strBegin(str));
}

template <class Num, class S>
Num extractInteger(const S& str, bool& hasMinusSign) //very fast conversion to integers: slightly faster than std::atoi, but more importantly: generic
{
    typedef typename GetCharType<S>::Type CharType;

    const CharType* first = strBegin(str);
    const CharType* last  = first + strLength(str);

    while (first != last && isWhiteSpace(*first)) //skip leading whitespace
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
            //assert(std::all_of(iter, last, &isWhiteSpace<CharType>)); -> this is NO assert situation
            break;
        }
    }
    return number;
}


template <class Num, class S> inline
Num stringTo(const S& str, Int2Type<NUM_TYPE_SIGNED_INT>)
{
    bool hasMinusSign = false; //handle minus sign
    const Num number = extractInteger<Num>(str, hasMinusSign);
    return hasMinusSign ? -number : number;
}


template <class Num, class S> inline
Num stringTo(const S& str, Int2Type<NUM_TYPE_UNSIGNED_INT>) //very fast conversion to integers: slightly faster than std::atoi, but more importantly: generic
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
S numberTo(const Num& number)
{
    typedef Int2Type<
    IsSignedInt  <Num>::value ? implementation::NUM_TYPE_SIGNED_INT :
    IsUnsignedInt<Num>::value ? implementation::NUM_TYPE_UNSIGNED_INT :
    IsFloat      <Num>::value ? implementation::NUM_TYPE_FLOATING_POINT :
    implementation::NUM_TYPE_OTHER> TypeTag;

    return implementation::numberTo<S>(number, TypeTag());
}


template <class Num, class S> inline
Num stringTo(const S& str)
{
    typedef Int2Type<
    IsSignedInt  <Num>::value ? implementation::NUM_TYPE_SIGNED_INT :
    IsUnsignedInt<Num>::value ? implementation::NUM_TYPE_UNSIGNED_INT :
    IsFloat      <Num>::value ? implementation::NUM_TYPE_FLOATING_POINT :
    implementation::NUM_TYPE_OTHER> TypeTag;

    return implementation::stringTo<Num>(str, TypeTag());
}

}

#endif //STRING_TOOLS_HEADER_213458973046
