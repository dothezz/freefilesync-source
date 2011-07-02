// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef XMLBASE_H_INCLUDED
#define XMLBASE_H_INCLUDED

#include <xml_error.h>
#include <zenXml/zenxml.h>

namespace xmlAccess
{
void saveXmlDocument(const zen::XmlDoc& doc, const wxString& filename); //throw (FfsXmlError)
void loadXmlDocument(const wxString& filename, zen::XmlDoc& doc); //throw FfsXmlError()

const wxString getErrorMessageFormatted(const zen::XmlIn& in);
}

#endif // XMLBASE_H_INCLUDED
