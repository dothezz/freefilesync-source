// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef FILEERROR_H_INCLUDED
#define FILEERROR_H_INCLUDED

#include <wx/string.h>


namespace FreeFileSync
{
class FileError //Exception class used to notify file/directory copy/delete errors
{
public:
    FileError(const wxString& message) :
        errorMessage(message) {}

    const wxString& show() const
    {
        return errorMessage;
    }

private:
    wxString errorMessage;
};
}

#endif // FILEERROR_H_INCLUDED
