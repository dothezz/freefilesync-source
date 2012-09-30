// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License: http://www.boost.org/LICENSE_1_0.txt           *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef STRING_UTF8_HEADER_01832479146991573473545
#define STRING_UTF8_HEADER_01832479146991573473545

#include <cstdint>
#include <iterator>
#include "string_tools.h" //copyStringTo

namespace zen
{
//convert all(!) char- and wchar_t-based "string-like" objects applying a UTF8 conversions (but only if necessary!)
template <class TargetString, class SourceString>
TargetString utfCvrtTo(const SourceString& str);

const char BYTE_ORDER_MARK_UTF8[] = "\xEF\xBB\xBF";

//---- explicit conversion: wide <-> utf8 ----
template <class CharString, class WideString>
CharString wideToUtf8(const WideString& str); //example: std::string tmp = wideToUtf8<std::string>(L"abc");

template <class WideString, class CharString>
WideString utf8ToWide(const CharString& str); //std::wstring tmp = utf8ToWide<std::wstring>("abc");

//access unicode characters in UTF-encoded string (char- or wchar_t-based)
template <class UtfString>
size_t unicodeLength(const UtfString& str); //return number of code points for UTF-encoded string

template <class UtfString>
size_t findUnicodePos(const UtfString& str, size_t unicodePos); //return position of unicode char in UTF-encoded string






























//----------------------- implementation ----------------------------------
namespace implementation
{
typedef std::uint_fast32_t CodePoint; //must be at least four bytes
typedef std::uint_fast16_t Char16;    //we need an unsigned type
typedef unsigned char Char8;

const CodePoint CODE_POINT_MAX     = 0x10ffff;

const CodePoint HIGH_SURROGATE     = 0xd800;
const CodePoint HIGH_SURROGATE_MAX = 0xdbff;

const CodePoint LOW_SURROGATE      = 0xdc00;
const CodePoint LOW_SURROGATE_MAX  = 0xdfff;


template <class Function> inline
void codePointToUtf16(CodePoint cp, Function writeOutput) //"writeOutput" is a unary function taking a Char16
{
    //http://en.wikipedia.org/wiki/UTF-16
    assert(cp < HIGH_SURROGATE || LOW_SURROGATE_MAX < cp); //code points [0xd800, 0xdfff] are not allowed for UTF-16
    assert(cp <= CODE_POINT_MAX);

    if (cp < 0x10000)
        writeOutput(static_cast<Char16>(cp));
    else
    {
        cp -= 0x10000;
        writeOutput(static_cast<Char16>((cp >> 10) + HIGH_SURROGATE));
        writeOutput(static_cast<Char16>((cp & 0x3ff) + LOW_SURROGATE));
    }
}


inline
size_t getUtf16Len(Char16 ch) //ch must be first code unit!
{
    const CodePoint cp = ch;

    if (HIGH_SURROGATE <= cp && cp <= HIGH_SURROGATE_MAX)
        return 2;
    else
    {
        assert(cp < LOW_SURROGATE || LOW_SURROGATE_MAX < cp); //NO low surrogate expected
        return 1;
    }
}


template <class CharIterator, class Function> inline
void utf16ToCodePoint(CharIterator first, CharIterator last, Function writeOutput) //"writeOutput" is a unary function taking a CodePoint
{
    assert_static(sizeof(typename std::iterator_traits<CharIterator>::value_type) == 2);

    for ( ; first != last; ++first)
    {
        CodePoint cp = static_cast<Char16>(*first);
        if (HIGH_SURROGATE <= cp && cp <= HIGH_SURROGATE_MAX)
        {
            if (++first == last)
            {
                assert(false); //low surrogate expected
                return;
            }
            assert(LOW_SURROGATE <= static_cast<Char16>(*first) && static_cast<Char16>(*first) <= LOW_SURROGATE_MAX); //low surrogate expected
            cp = ((cp - HIGH_SURROGATE) << 10) + static_cast<Char16>(*first) - LOW_SURROGATE + 0x10000;
        }
        else
            assert(cp < LOW_SURROGATE || LOW_SURROGATE_MAX < cp); //NO low surrogate expected

        writeOutput(cp);
    }
}


template <class Function> inline
void codePointToUtf8(CodePoint cp, Function writeOutput) //"writeOutput" is a unary function taking a Char8
{
    //http://en.wikipedia.org/wiki/UTF-8

    if (cp < 0x80)
        writeOutput(static_cast<Char8>(cp));
    else if (cp < 0x800)
    {
        writeOutput(static_cast<Char8>((cp >> 6  ) | 0xc0));
        writeOutput(static_cast<Char8>((cp & 0x3f) | 0x80));
    }
    else if (cp < 0x10000)
    {
        writeOutput(static_cast<Char8>((cp >> 12        ) | 0xe0));
        writeOutput(static_cast<Char8>(((cp >> 6) & 0x3f) | 0x80));
        writeOutput(static_cast<Char8>((cp & 0x3f       ) | 0x80));
    }
    else
    {
        assert(cp <= CODE_POINT_MAX);
        writeOutput(static_cast<Char8>((cp >> 18         ) | 0xf0));
        writeOutput(static_cast<Char8>(((cp >> 12) & 0x3f) | 0x80));
        writeOutput(static_cast<Char8>(((cp >> 6)  & 0x3f) | 0x80));
        writeOutput(static_cast<Char8>((cp & 0x3f        ) | 0x80));
    }
}


inline
size_t getUtf8Len(unsigned char ch) //ch must be first code unit!
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
void utf8ToCodePoint(CharIterator first, CharIterator last, Function writeOutput) //"writeOutput" is a unary function taking a CodePoint
{
    assert_static(sizeof(typename std::iterator_traits<CharIterator>::value_type) == 1);

    for ( ; first != last; ++first)
    {
        auto getChar = [&](Char8& ch) -> bool
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

        Char8 ch = static_cast<Char8>(*first);
        switch (getUtf8Len(ch))
        {
            case 1:
                writeOutput(ch);
                break;
            case 2:
            {
                CodePoint cp = (ch & 0x1f) << 6;
                if (!getChar(ch)) return;
                cp += ch & 0x3f;
                writeOutput(cp);
            }
            break;
            case 3:
            {
                CodePoint cp = (ch & 0xf) << 12;
                if (!getChar(ch)) return;
                cp += (ch & 0x3f) << 6;
                if (!getChar(ch)) return;
                cp += ch & 0x3f;
                writeOutput(cp);
            }
            break;
            case 4:
            {
                CodePoint cp = (ch & 0x7) << 18;
                if (!getChar(ch)) return;
                cp += (ch & 0x3f) << 12;
                if (!getChar(ch)) return;
                cp += (ch & 0x3f) << 6;
                if (!getChar(ch)) return;
                cp += ch & 0x3f;
                writeOutput(cp);
            }
            break;
            default:
                assert(false);
        }
    }
}


template <class CharString> inline
size_t unicodeLength(const CharString& str, char) //utf8
{
    typedef typename GetCharType<CharString>::Type CharType;

    const CharType*       strFirst  = strBegin(str);
    const CharType* const strLast   = strFirst + strLength(str);

    size_t len = 0;
    while (strFirst < strLast) //[!]
    {
        ++len;
        strFirst += getUtf8Len(*strFirst); //[!]
    }
    return len;
}


template <class WideString> inline
size_t unicodeLengthWide(const WideString& str, Int2Type<2>) //windows: utf16-wchar_t
{
    typedef typename GetCharType<WideString>::Type CharType;

    const CharType*       strFirst = strBegin(str);
    const CharType* const strLast  = strFirst + strLength(str);

    size_t len = 0;
    while (strFirst < strLast) //[!]
    {
        ++len;
        strFirst += getUtf16Len(*strFirst); //[!]
    }
    return len;
}


template <class WideString> inline
size_t unicodeLengthWide(const WideString& str, Int2Type<4>) //other OS: utf32-wchar_t
{
    return strLength(str);
}


template <class WideString> inline
size_t unicodeLength(const WideString& str, wchar_t)
{
    return unicodeLengthWide(str, Int2Type<sizeof(wchar_t)>());
}
}


template <class UtfString> inline
size_t unicodeLength(const UtfString& str) //return number of code points
{
    return implementation::unicodeLength(str, typename GetCharType<UtfString>::Type());
}


namespace implementation
{
template <class CharString> inline
size_t findUnicodePos(const CharString& str, size_t unicodePos, char) //utf8-char
{
    typedef typename GetCharType<CharString>::Type CharType;

    const CharType* strFirst = strBegin(str);
    const size_t strLen = strLength(str);

    size_t utfPos = 0;
    while (unicodePos-- > 0)
    {
        utfPos += getUtf8Len(strFirst[utfPos]);

        if (utfPos >= strLen)
            return strLen;
    }
    return utfPos;
}


template <class WideString> inline
size_t findUnicodePosWide(const WideString& str, size_t unicodePos, Int2Type<2>) //windows: utf16-wchar_t
{
    typedef typename GetCharType<WideString>::Type CharType;

    const CharType* strFirst = strBegin(str);
    const size_t strLen = strLength(str);

    size_t utfPos = 0;
    while (unicodePos-- > 0)
    {
        utfPos += getUtf16Len(strFirst[utfPos]);

        if (utfPos >= strLen)
            return strLen;
    }
    return utfPos;
}


template <class WideString> inline
size_t findUnicodePosWide(const WideString& str, size_t unicodePos, Int2Type<4>) //other OS: utf32-wchar_t
{
    return std::min(strLength(str), unicodePos);
}


template <class UtfString> inline
size_t findUnicodePos(const UtfString& str, size_t unicodePos, wchar_t)
{
    return findUnicodePosWide(str, unicodePos, Int2Type<sizeof(wchar_t)>());
}
}


template <class UtfString> inline
size_t findUnicodePos(const UtfString& str, size_t unicodePos) //return position of unicode char in UTF-encoded string
{
    return implementation::findUnicodePos(str, unicodePos, typename GetCharType<UtfString>::Type());
}

//-------------------------------------------------------------------------------------------

namespace implementation
{
template <class WideString, class CharString> inline
WideString utf8ToWide(const CharString& str, Int2Type<2>) //windows: convert utf8 to utf16-wchar_t
{
    WideString output;
    utf8ToCodePoint(strBegin(str), strBegin(str) + strLength(str),
    [&](CodePoint cp) { codePointToUtf16(cp, [&](Char16 c) { output += static_cast<wchar_t>(c); }); });
    return output;
}


template <class WideString, class CharString> inline
WideString utf8ToWide(const CharString& str, Int2Type<4>) //other OS: convert utf8 to utf32-wchar_t
{
    WideString output;
    utf8ToCodePoint(strBegin(str), strBegin(str) + strLength(str),
    [&](CodePoint cp) { output += static_cast<wchar_t>(cp); });
    return output;
}


template <class CharString, class WideString> inline
CharString wideToUtf8(const WideString& str, Int2Type<2>) //windows: convert utf16-wchar_t to utf8
{
    CharString output;
    utf16ToCodePoint(strBegin(str), strBegin(str) + strLength(str),
    [&](CodePoint cp) { codePointToUtf8(cp, [&](char c) { output += c; }); });
    return output;
}


template <class CharString, class WideString> inline
CharString wideToUtf8(const WideString& str, Int2Type<4>) //other OS: convert utf32-wchar_t to utf8
{
    CharString output;
    std::for_each(strBegin(str), strBegin(str) + strLength(str),
    [&](CodePoint cp) { codePointToUtf8(cp, [&](char c) { output += c; }); });
    return output;
}
}


template <class WideString, class CharString> inline
WideString utf8ToWide(const CharString& str)
{
    assert_static((IsSameType<typename GetCharType<CharString>::Type, char   >::value));
    assert_static((IsSameType<typename GetCharType<WideString>::Type, wchar_t>::value));

    return implementation::utf8ToWide<WideString>(str, Int2Type<sizeof(wchar_t)>());
}


template <class CharString, class WideString> inline
CharString wideToUtf8(const WideString& str)
{
    assert_static((IsSameType<typename GetCharType<CharString>::Type, char   >::value));
    assert_static((IsSameType<typename GetCharType<WideString>::Type, wchar_t>::value));

    return implementation::wideToUtf8<CharString>(str, Int2Type<sizeof(wchar_t)>());
}

//-------------------------------------------------------------------------------------------

template <class TargetString, class SourceString> inline
TargetString utfCvrtTo(const SourceString& str, char, wchar_t) { return utf8ToWide<TargetString>(str); }

template <class TargetString, class SourceString> inline
TargetString utfCvrtTo(const SourceString& str, wchar_t, char) { return wideToUtf8<TargetString>(str); }

template <class TargetString, class SourceString> inline
TargetString utfCvrtTo(const SourceString& str, char, char) { return copyStringTo<TargetString>(str); }

template <class TargetString, class SourceString> inline
TargetString utfCvrtTo(const SourceString& str, wchar_t, wchar_t) { return copyStringTo<TargetString>(str); }

template <class TargetString, class SourceString> inline
TargetString utfCvrtTo(const SourceString& str)
{
    return utfCvrtTo<TargetString>(str,
                                   typename GetCharType<SourceString>::Type(),
                                   typename GetCharType<TargetString>::Type());
}
}

#endif //STRING_UTF8_HEADER_01832479146991573473545
