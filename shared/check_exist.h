// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef CHECKEXIST_H_INCLUDED
#define CHECKEXIST_H_INCLUDED

#include "zstring.h"

namespace util
{
enum ResultExist
{
    EXISTING_TRUE,
    EXISTING_FALSE,
    EXISTING_TIMEOUT
};

ResultExist fileExists(const Zstring& filename, size_t timeout); //timeout in ms
ResultExist dirExists( const Zstring& dirname,  size_t timeout); //timeout in ms
}

#endif // CHECKEXIST_H_INCLUDED
