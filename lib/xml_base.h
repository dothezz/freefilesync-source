// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef XMLBASE_H_INCLUDED
#define XMLBASE_H_INCLUDED

#include <zenxml/xml.h>
#include <zen/zstring.h>
#include <zen/file_error.h>

//bind zenxml and zen file handling together

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

    explicit FfsXmlError(const std::wstring& message, Severity sev = FATAL) : errorMessage(message), m_severity(sev) {}

    const std::wstring& toString() const { return errorMessage; }
    Severity getSeverity() const { return m_severity; }
private:
    const std::wstring errorMessage;
    const Severity m_severity;
};

void saveXmlDocument(const zen::XmlDoc& doc, const Zstring& filename); //throw FfsXmlError
void loadXmlDocument(const Zstring& filename, zen::XmlDoc& doc); //throw FfsXmlError

const std::wstring getErrorMessageFormatted(const zen::XmlIn& in);
}

#endif // XMLBASE_H_INCLUDED
