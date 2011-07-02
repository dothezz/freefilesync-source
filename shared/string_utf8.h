// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License, Version 1.0. See accompanying file             *
// * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.       *
// * Copyright (C) 2011 ZenJu (zhnmju123 AT gmx.de)                         *
// **************************************************************************

#ifndef STRING_UTF8_HEADER_01832479146991573473545
#define STRING_UTF8_HEADER_01832479146991573473545

#include <iterator>
#include "loki/TypeManip.h"
#include "string_tools.h"
#include "assert_static.h"

namespace zen
{
//Example: std::string tmp = toUtf8<std::string>(L"abc");
template <class CharString, class WideString>
CharString wideToUtf8(const WideString& str);

//Example: std::wstring tmp = utf8To<std::wstring>("abc");
template <class WideString, class CharString>
WideString utf8ToWide(const CharString& str);

const char BYTE_ORDER_MARK_UTF8[] = "\xEF\xBB\xBF";

//convert any(!) "string-like" object into a UTF8 encoded std::string
template <class String> std::string toStdString(const String& str);
//convert a UTF8 encoded std::string to any(!) string-class
template <class String> String      stdStringTo(const std::string& str);






























//----------------------- implementation ----------------------------------
namespace implementation
{
typedef unsigned int CodePoint;

const CodePoint CODE_POINT_MAX     = 0x10ffff;

const CodePoint HIGH_SURROGATE     = 0xd800;
const CodePoint HIGH_SURROGATE_MAX = 0xdbff;

const CodePoint LOW_SURROGATE      = 0xdc00;
const CodePoint LOW_SURROGATE_MAX  = 0xdfff;


template <class OutputIterator> inline
OutputIterator codePointToUtf16(CodePoint cp, OutputIterator result) //http://en.wikipedia.org/wiki/UTF-16
{
    typedef unsigned short Char16; //this isn't necessarily 16 bit, but all we need is an unsigned type

    assert(cp < HIGH_SURROGATE || LOW_SURROGATE_MAX < cp); //code points [0xd800, 0xdfff] are not allowed for UTF-16
    assert(cp <= CODE_POINT_MAX);

    if (cp < 0x10000)
        *result++ = static_cast<Char16>(cp);
    else
    {
        cp -= 0x10000;
        *result++ = static_cast<Char16>((cp >> 10) + HIGH_SURROGATE);
        *result++ = static_cast<Char16>((cp & 0x3ff) + LOW_SURROGATE);
    }
    return result;
}


template <class CharIterator, class Function> inline
Function utf16ToCodePoint(CharIterator first, CharIterator last, Function f) //f is a unary function taking a CodePoint as single parameter
{
    assert_static(sizeof(typename std::iterator_traits<CharIterator>::value_type) == 2);
    typedef unsigned short Char16; //this isn't necessarily 16 bit, but all we need is an unsigned type

    for ( ; first != last; ++first)
    {
        CodePoint cp = static_cast<Char16>(*first);
        if (HIGH_SURROGATE <= cp && cp <= HIGH_SURROGATE_MAX)
        {
            if (++first == last)
            {
                assert(false); //low surrogate expected
                break;
            }
            assert(LOW_SURROGATE <= static_cast<Char16>(*first) && static_cast<Char16>(*first) <= LOW_SURROGATE_MAX); //low surrogate expected
            cp = ((cp - HIGH_SURROGATE) << 10) + static_cast<Char16>(*first) - LOW_SURROGATE + 0x10000;
        }
        else
            assert(cp < LOW_SURROGATE || LOW_SURROGATE_MAX < cp); //NO low surrogate expected

        f(cp);
    }
    return f;
}


template <class OutputIterator> inline
OutputIterator codePointToUtf8(CodePoint cp, OutputIterator result) //http://en.wikipedia.org/wiki/UTF-8
{
    typedef unsigned char Char8;

    assert(cp <= CODE_POINT_MAX);

    if (cp < 0x80)
        *result++ = static_cast<Char8>(cp);
    else if (cp < 0x800)
    {
        *result++ = static_cast<Char8>((cp >> 6   )| 0xc0);
        *result++ = static_cast<Char8>((cp & 0x3f )| 0x80);
    }
    else if (cp < 0x10000)
    {
        *result++ = static_cast<Char8>((cp >> 12         )| 0xe0);
        *result++ = static_cast<Char8>(((cp >> 6) & 0x3f )| 0x80);
        *result++ = static_cast<Char8>((cp & 0x3f        )| 0x80);
    }
    else
    {
        *result++ = static_cast<Char8>((cp >> 18          )| 0xf0);
        *result++ = static_cast<Char8>(((cp >> 12) & 0x3f )| 0x80);
        *result++ = static_cast<Char8>(((cp >> 6)  & 0x3f )| 0x80);
        *result++ = static_cast<Char8>((cp & 0x3f         )| 0x80);
    }
    return result;
}


inline
size_t getUtf8Len(unsigned char ch)
{
    if (ch < 0x80)
        return 1;
    if (ch >> 5 == 0x6)
        return 2;
    if (ch >> 4 == 0xe)
        return 3;
    if (ch >> 3 == 0x1e)
        return 4;

    assert(false); //no valid begin of UTF8 encoding
    return 1;
}


template <class CharIterator, class Function> inline
Function utf8ToCodePoint(CharIterator first, CharIterator last, Function f) //f is a unary function taking a CodePoint as single parameter
{
    assert_static(sizeof(typename std::iterator_traits<CharIterator>::value_type) == 1);
    typedef unsigned char Char8;

    for ( ; first != last; ++first)
    {
        auto getChar = [&](Char8& ch ) -> bool
        {
            if (++first == last)
            {
                assert(false); //low surrogate expected
                return false;
            }
            ch = static_cast<Char8>(*first);
            assert(ch >> 6 == 0x2);
            return true;
        };

        CodePoint cp = static_cast<Char8>(*first);
        switch (getUtf8Len(cp))
        {
            case 1:
                break;
            case 2:
            {
                cp = (cp & 0x1f) << 6;
                Char8 ch;
                if (!getChar(ch)) continue;
                cp += ch & 0x3f;
            }
            break;
            case 3:
            {
                cp = (cp & 0xf) << 12;
                Char8 ch;
                if (!getChar(ch)) continue;
                cp += (ch & 0x3f) << 6;
                if (!getChar(ch)) continue;
                cp += ch & 0x3f;

            }
            break;
            case 4:
            {
                cp = (cp & 0x7) << 18;
                Char8 ch;
                if (!getChar(ch)) continue;
                cp += (ch & 0x3f) << 12;
                if (!getChar(ch)) continue;
                cp += (ch & 0x3f) << 6;
                if (!getChar(ch)) continue;
                cp += ch & 0x3f;
            }
            break;
            default:
                assert(false);
        }
        f(cp);
    }
    return f;
}


template <class String>
class AppendStringIterator: public std::iterator<std::output_iterator_tag, void, void, void, void>
{
public:
    explicit AppendStringIterator (String& x) : str(x) {}
    AppendStringIterator& operator= (typename String::value_type value) { str += value; return *this; }
    AppendStringIterator& operator*  ()    { return *this; }
    AppendStringIterator& operator++ ()    { return *this; }
    AppendStringIterator  operator++ (int) { return *this; }
private:
    String& str;
};


template <class WideString, class CharString> inline
WideString utf8ToWide(const CharString& str, Loki::Int2Type<2>) //windows: convert utf8 to utf16 wchar_t
{
    WideString output;
    utf8ToCodePoint(strBegin(str), strBegin(str) + strLength(str),
    [&](CodePoint cp) { codePointToUtf16(cp, AppendStringIterator<WideString>(output)); });
    return output;
}


template <class WideString, class CharString> inline
WideString utf8ToWide(const CharString& str, Loki::Int2Type<4>) //other OS: convert utf8 to utf32 wchar_t
{
    WideString output;
    utf8ToCodePoint(strBegin(str), strBegin(str) + strLength(str),
    [&](CodePoint cp) { output += cp; });
    return output;
}


template <class CharString, class WideString> inline
CharString wideToUtf8(const WideString& str, Loki::Int2Type<2>) //windows: convert utf16-wchar_t to utf8
{
    CharString output;
    utf16ToCodePoint(strBegin(str), strBegin(str) + strLength(str),
    [&](CodePoint cp) { codePointToUtf8(cp, AppendStringIterator<CharString>(output)); });
    return output;
}


template <class CharString, class WideString> inline
CharString wideToUtf8(const WideString& str, Loki::Int2Type<4>) //other OS: convert utf32-wchar_t to utf8
{
    CharString output;
    std::for_each(strBegin(str), strBegin(str) + strLength(str),
    [&](CodePoint cp) { codePointToUtf8(cp, AppendStringIterator<CharString>(output)); });
    return output;
}
}


template <class WideString, class CharString> inline
WideString utf8ToWide(const CharString& str)
{
    assert_static((Loki::IsSameType<typename StringTraits<CharString>::CharType, char   >::value));
    assert_static((Loki::IsSameType<typename StringTraits<WideString>::CharType, wchar_t>::value));

    return implementation::utf8ToWide<WideString>(str, Loki::Int2Type<sizeof(wchar_t)>());
}


template <class CharString, class WideString> inline
CharString wideToUtf8(const WideString& str)
{
    assert_static((Loki::IsSameType<typename StringTraits<CharString>::CharType, char   >::value));
    assert_static((Loki::IsSameType<typename StringTraits<WideString>::CharType, wchar_t>::value));

    return implementation::wideToUtf8<CharString>(str, Loki::Int2Type<sizeof(wchar_t)>());
}


//-------------------------------------------------------------------------------------------
template <class String> inline
std::string toStdString(const String& str, wchar_t) { return wideToUtf8<std::string>(str); } //convert wide character string to UTF8

template <class String> inline
std::string toStdString(const String& str, char) { return cvrtString<std::string>(str); } //directly process string without UTF8 conversion

template <class String> inline
std::string toStdString(const String& str) { return toStdString(str, typename StringTraits<String>::CharType()); }
//-------------------------------------------------------------------------------------------

template <class String> inline
String stdStringTo(const std::string& str, wchar_t) { return utf8ToWide<String>(str); } //convert UTF8 to wide character string

template <class String> inline
String stdStringTo(const std::string& str, char) { return cvrtString<String>(str); } //directly process string without UTF8 conversion

template <class String> inline
String stdStringTo(const std::string& str) { return stdStringTo<String>(str, typename StringTraits<String>::CharType()); }
}

#endif //STRING_UTF8_HEADER_01832479146991573473545
