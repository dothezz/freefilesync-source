// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef BASE64_HEADER_08473021856321840873021487213453214
#define BASE64_HEADER_08473021856321840873021487213453214

#ifndef NDEBUG //no release build dependencies!
#include <iterator>
#include <cassert>
#endif

namespace zen
{
//http://en.wikipedia.org/wiki/Base64
/*
Usage:
	const std::string input = "Sample text";
	std::string output;
	zen::encodeBase64(input.begin(), input.end(), std::back_inserter(output));
	//output contains "U2FtcGxlIHRleHQ="
*/
template <class InputIterator, class OutputIterator>
OutputIterator encodeBase64(InputIterator first, InputIterator last, OutputIterator result); //nothrow!

template <class InputIterator, class OutputIterator>
OutputIterator decodeBase64(InputIterator first, InputIterator last, OutputIterator result); //nothrow!












//------------------------- implementation -------------------------------
namespace implementation
{
//64 chars for base64 encoding + padding char
const char ENCODING_MIME[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
const signed char DECODING_MIME[] =
{
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, 64, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
};
const unsigned char INDEX_PAD = 64; //"="
}


//http://en.wikipedia.org/wiki/Base64
template <class InputIterator, class OutputIterator> inline
OutputIterator encodeBase64(InputIterator first, InputIterator last, OutputIterator result)
{
    using namespace implementation;
    static_assert(sizeof(typename std::iterator_traits<InputIterator>::value_type) == 1, "");
    static_assert(sizeof(ENCODING_MIME) == 65 + 1, "");

    while (first != last)
    {
        const unsigned char a = static_cast<unsigned char>(*first++);
        *result++ = ENCODING_MIME[a >> 2];

        if (first == last)
        {
            *result++ = ENCODING_MIME[((a & 0x3) << 4)];
            *result++ = ENCODING_MIME[INDEX_PAD];
            *result++ = ENCODING_MIME[INDEX_PAD];
            break;
        }
        const unsigned char b = static_cast<unsigned char>(*first++);
        *result++ = ENCODING_MIME[((a & 0x3) << 4) | (b >> 4)];

        if (first == last)
        {
            *result++ = ENCODING_MIME[((b & 0xf) << 2)];
            *result++ = ENCODING_MIME[INDEX_PAD];
            break;
        }
        const unsigned char c = static_cast<unsigned char>(*first++);
        *result++ = ENCODING_MIME[((b & 0xf) << 2) | (c >> 6)];
        *result++ = ENCODING_MIME[c & 0x3f];
    }

    return result;
}


template <class InputIterator, class OutputIterator> inline
OutputIterator decodeBase64(InputIterator first, InputIterator last, OutputIterator result)
{
    using namespace implementation;
    static_assert(sizeof(typename std::iterator_traits<InputIterator>::value_type) == 1, "");
    static_assert(sizeof(DECODING_MIME) == 128, "");

    const unsigned char INDEX_END = INDEX_PAD + 1;

    auto readIndex = [&]() -> unsigned char //return index within [0, 64] or INDEX_END if end of input
    {
        while (true)
        {
            if (first == last)
                return INDEX_END;

            const unsigned char ch = static_cast<unsigned char>(*first++);
            if (ch < 128) //we're in lower ASCII table half
            {
                const int index = implementation::DECODING_MIME[ch];
                if (0 <= index && index <= static_cast<int>(INDEX_PAD)) //skip all unknown characters (including carriage return, line-break, tab)
                    return index;
            }
        }
    };

    for (;;)
    {
        const unsigned char index1 = readIndex();
        const unsigned char index2 = readIndex();
        if (index1 >= INDEX_PAD || index2 >= INDEX_PAD)
        {
            assert(index1 == INDEX_END && index2 == INDEX_END);
            break;
        }
        *result++ = static_cast<char>((index1 << 2) | (index2 >> 4));

        const unsigned char index3 = readIndex();
        if (index3 >= INDEX_PAD) //padding
        {
            assert(index3 == INDEX_PAD);
            break;
        }
        *result++ = static_cast<char>(((index2 & 0xf) << 4) | (index3 >> 2));

        const unsigned char index4 = readIndex();
        if (index4 >= INDEX_PAD) //padding
        {
            assert(index4 == INDEX_PAD);
            break;
        }
        *result++ = static_cast<char>(((index3 & 0x3) << 6) | index4);
    }
    return result;
}
}

#endif //BASE64_HEADER_08473021856321840873021487213453214
