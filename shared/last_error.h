// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef SYSTEMFUNCTIONS_H_INCLUDED
#define SYSTEMFUNCTIONS_H_INCLUDED

#include <string>


namespace zen
{
//evaluate GetLastError()/errno and assemble specific error message
#ifdef FFS_WIN
std::wstring getLastErrorFormatted(unsigned long lastError = 0);
#elif defined FFS_LINUX
std::wstring getLastErrorFormatted(int lastError = 0);
#endif
}


#endif // SYSTEMFUNCTIONS_H_INCLUDED
