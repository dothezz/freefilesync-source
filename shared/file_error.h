// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef FILEERROR_H_INCLUDED
#define FILEERROR_H_INCLUDED

#include <wx/string.h>


namespace zen
{
class FileError //Exception base class used to notify file/directory copy/delete errors
{
public:
    FileError(const wxString& message) : errorMessage(message) {}
    virtual ~FileError() {}

    const wxString& msg() const
    {
        return errorMessage;
    }

private:
    const wxString errorMessage;
};

#define DEFINE_NEW_FILE_ERROR(X) struct X : public FileError { X(const wxString& message) : FileError(message) {} };

DEFINE_NEW_FILE_ERROR(ErrorNotExisting);
DEFINE_NEW_FILE_ERROR(ErrorTargetExisting);
DEFINE_NEW_FILE_ERROR(ErrorTargetPathMissing);
DEFINE_NEW_FILE_ERROR(ErrorFileLocked);
}

#endif // FILEERROR_H_INCLUDED
