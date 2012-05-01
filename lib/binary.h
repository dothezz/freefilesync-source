// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef BINARY_H_INCLUDED
#define BINARY_H_INCLUDED

#include <zen/zstring.h>
#include <zen/file_error.h>
#include <zen/int64.h>

namespace zen
{

//callback functionality for status updates while comparing
class CompareCallback
{
public:
    virtual ~CompareCallback() {}
    virtual void updateCompareStatus(zen::UInt64 totalBytes) = 0;
};

bool filesHaveSameContent(const Zstring& filename1, const Zstring& filename2, CompareCallback& callback); //throw FileError
}

#endif // BINARY_H_INCLUDED