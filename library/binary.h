// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef BINARY_H_INCLUDED
#define BINARY_H_INCLUDED

#include "../shared/zstring.h"
#include "../shared/file_error.h"
#include "../shared/int64.h"

namespace zen
{

//callback functionality for status updates while comparing
class CompareCallback
{
public:
    virtual ~CompareCallback() {}
    virtual void updateCompareStatus(zen::UInt64 totalBytesTransferred) = 0;
};

bool filesHaveSameContent(const Zstring& filename1, const Zstring& filename2, CompareCallback& callback); //throw FileError
}

#endif // BINARY_H_INCLUDED
