#ifndef SYSTEMCONSTANTS_H_INCLUDED
#define SYSTEMCONSTANTS_H_INCLUDED

#include "zstring.h"
#include <wx/string.h>

namespace globalFunctions
{
//------------------------------------------------
//      GLOBALS
//------------------------------------------------
#ifdef FFS_WIN
const DefaultChar FILE_NAME_SEPARATOR = '\\';
static const wxChar* const LINE_BREAK = wxT("\r\n"); //internal linkage
#elif defined FFS_LINUX
const DefaultChar FILE_NAME_SEPARATOR = '/';
static const wxChar* const LINE_BREAK = wxT("\n");
#endif
}


#endif // SYSTEMCONSTANTS_H_INCLUDED
