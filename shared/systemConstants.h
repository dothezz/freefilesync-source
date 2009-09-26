#ifndef SYSTEMCONSTANTS_H_INCLUDED
#define SYSTEMCONSTANTS_H_INCLUDED

#include "zstring.h"


namespace globalFunctions
{
//------------------------------------------------
//      GLOBALS
//------------------------------------------------
#ifdef FFS_WIN
    const DefaultChar FILE_NAME_SEPARATOR = '\\';
    static const DefaultChar* const LINE_BREAK = L"\r\n"; //internal linkage
#elif defined FFS_LINUX
    const DefaultChar FILE_NAME_SEPARATOR = '/';
    static const DefaultChar* const LINE_BREAK = "\n";
#endif
}


#endif // SYSTEMCONSTANTS_H_INCLUDED
