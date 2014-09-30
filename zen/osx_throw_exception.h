// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef OSX_EXCEPTION_89274305834255
#define OSX_EXCEPTION_89274305834255

#import <Cocoa/Cocoa.h>
#include "sys_error.h"
#include "utf.h"

namespace osx
{
//for use in Objective C implementation files only!
void throwSysError(NSException* e); //throw SysError

#define ZEN_OSX_ASSERT(obj) ZEN_OSX_ASSERT_IMPL(obj, #obj) //throw SysError
/*
Example: ZEN_OSX_ASSERT(obj);

Equivalent to:
    if (!obj)
		throw zen::SysError(L"Assertion failed: \"obj\".");
*/






//######################## implmentation ############################
inline
void throwSysError(NSException* e) //throw SysError
{
    std::string msg;
    if (const char* name  = [[e name  ] cStringUsingEncoding:NSUTF8StringEncoding]) //"const char*" NOT owned by us!
        msg += name;
    if (const char* descr = [[e reason] cStringUsingEncoding:NSUTF8StringEncoding])
    {
        msg += "\n";
        msg += descr;
    }
    throw zen::SysError(zen::utfCvrtTo<std::wstring>(msg));
    /*
    e.g.
    NSInvalidArgumentException
    *** +[NSString stringWithCString:encoding:]: NULL cString
    */
}
}

#define ZEN_OSX_ASSERT_IMPL(obj, txt) if (!(obj)) throw zen::SysError(std::wstring(L"Assertion failed: \"") + L ## txt + L"\".");

#endif //OSX_EXCEPTION_89274305834255
