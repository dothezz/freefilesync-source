// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef OSX_STRING_1873641732143214324
#define OSX_STRING_1873641732143214324

#include <CoreFoundation/CoreFoundation.h> //CFString
#include "zstring.h"

namespace osx
{
Zstring cfStringToZstring(const CFStringRef& cfStr);

CFStringRef        createCFString       (const char* utf8Str); //returns nullptr on error
CFMutableStringRef createMutableCFString(const char* utf8Str); //pass ownership! => ZEN_ON_SCOPE_EXIT(::CFRelease(str));













//################# implementation #####################
inline
Zstring cfStringToZstring(const CFStringRef& cfStr)
{
    if (cfStr)
    {
        //perf: try to get away cheap:
        if (const char* utf8Str = ::CFStringGetCStringPtr(cfStr, kCFStringEncodingUTF8))
            return utf8Str;

        CFIndex length = ::CFStringGetLength(cfStr);
        if (length > 0)
        {
            CFIndex bufferSize = ::CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
            Zstring buffer;
            buffer.resize(bufferSize);

            if (::CFStringGetCString(cfStr, &*buffer.begin(), bufferSize, kCFStringEncodingUTF8))
            {
                buffer.resize(zen::strLength(buffer.c_str())); //caveat: memory consumption of returned string!
                return buffer;
            }
        }
    }
    return Zstring();
}


inline
CFStringRef createCFString(const char* utf8Str)
{
    //don't bother with CFStringCreateWithBytes: it's slightly slower, despite passing length info
    return ::CFStringCreateWithCString(nullptr,                //CFAllocatorRef alloc,
                                       utf8Str,                //const char *cStr,
                                       kCFStringEncodingUTF8); //CFStringEncoding encoding
}


inline
CFMutableStringRef createMutableCFString(const char* utf8Str)
{
    if (CFMutableStringRef strRef = ::CFStringCreateMutable(NULL, 0))
    {
        ::CFStringAppendCString(strRef, utf8Str, kCFStringEncodingUTF8);
        return strRef;
    }
    return nullptr;
}
}

#endif //OSX_STRING_1873641732143214324
