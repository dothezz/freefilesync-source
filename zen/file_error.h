// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef FILEERROR_H_INCLUDED
#define FILEERROR_H_INCLUDED

#include <string>
#include "zstring.h"
#include "utf.h"
#include "last_error.h" //we'll need this later anyway!

namespace zen
{
class FileError //Exception base class used to notify file/directory copy/delete errors
{
public:
    explicit FileError(const std::wstring& message) : errorMessage(message) {}
    virtual ~FileError() {}

    const std::wstring& toString() const { return errorMessage; }

private:
    std::wstring errorMessage;
};

#define DEFINE_NEW_FILE_ERROR(X) struct X : public FileError { X(const std::wstring& message) : FileError(message) {} };

DEFINE_NEW_FILE_ERROR(ErrorNotExisting);
DEFINE_NEW_FILE_ERROR(ErrorTargetExisting);
DEFINE_NEW_FILE_ERROR(ErrorTargetPathMissing);
DEFINE_NEW_FILE_ERROR(ErrorFileLocked);
DEFINE_NEW_FILE_ERROR(ErrorDifferentVolume);



//----------- facilitate usage of std::wstring for error messages --------------------

//allow implicit UTF8 conversion: since std::wstring models a GUI string, convenience is more important than performance
inline
std::wstring operator+(const std::wstring& lhs, const Zstring& rhs) { return std::wstring(lhs) += utfCvrtTo<std::wstring>(rhs); }

//we musn't put our overloads in namespace std, but namespace zen (+ using directive) is sufficient


inline
std::wstring fmtFileName(const Zstring& filename)
{
    std::wstring output;
    output += L'\'';
    output += utfCvrtTo<std::wstring>(filename);
    output += L'\'';
    return output;
}
}

#endif // FILEERROR_H_INCLUDED
