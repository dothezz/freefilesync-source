// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef XMLERROR_H_INCLUDED
#define XMLERROR_H_INCLUDED

#include <wx/string.h>


namespace xmlAccess
{

class FfsXmlError //Exception class
{
public:
    enum Severity
    {
        WARNING = 77,
        FATAL
    };

    FfsXmlError(const wxString& message, Severity sev = FATAL) : errorMessage(message), m_severity(sev) {}

    const wxString& msg() const { return errorMessage; }
    Severity getSeverity() const { return m_severity; }
private:
    const wxString errorMessage;
    const Severity m_severity;
};
}

#endif // XMLERROR_H_INCLUDED
