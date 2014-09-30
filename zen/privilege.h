// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef PRIVILEGE_H_INCLUDED
#define PRIVILEGE_H_INCLUDED

#include "file_error.h"

namespace zen
{
void activatePrivilege(const wchar_t* privilege); //throw FileError; thread-safe!!!
}

#endif // PRIVILEGE_H_INCLUDED
