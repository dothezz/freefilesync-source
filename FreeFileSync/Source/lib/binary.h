// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef BINARY_H_INCLUDED
#define BINARY_H_INCLUDED

#include <functional>
#include <zen/zstring.h>
#include <zen/file_error.h>
#include <zen/int64.h>

namespace zen
{
bool filesHaveSameContent(const Zstring& filename1,  //throw FileError
                          const Zstring& filename2,
                          const std::function<void(Int64 bytesDelta)>& onUpdateStatus); //may be nullptr
}

#endif // BINARY_H_INCLUDED
