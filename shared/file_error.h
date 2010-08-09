// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef FILEERROR_H_INCLUDED
#define FILEERROR_H_INCLUDED

#include <wx/string.h>


namespace ffs3
{
class FileError //Exception base class used to notify file/directory copy/delete errors
{
public:
    FileError(const wxString& message) :
        errorMessage(message) {}

    virtual ~FileError() {}

    const wxString& msg() const
    {
        return errorMessage;
    }

private:
    const wxString errorMessage;
};


class ErrorNotExisting : public FileError
{
public:
    ErrorNotExisting(const wxString& message) : FileError(message) {}
};
}

#endif // FILEERROR_H_INCLUDED
