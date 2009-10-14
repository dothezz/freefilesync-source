#ifndef SYSTEMFUNCTIONS_H_INCLUDED
#define SYSTEMFUNCTIONS_H_INCLUDED

#include <wx/string.h>


namespace FreeFileSync
{

#ifdef FFS_WIN
wxString getLastErrorFormatted(unsigned long lastError = 0); //try to get additional Windows error information
#elif defined FFS_LINUX
wxString getLastErrorFormatted(int lastError = 0); //try to get additional Linux error information
#endif
}


#endif // SYSTEMFUNCTIONS_H_INCLUDED
