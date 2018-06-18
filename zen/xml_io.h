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

//combine zen::Xml and zen file i/o
//-> loadXmlDocument vs loadStream:
//1. better error reporting
//2. quick exit if (potentially large) input file is not an XML

namespace zen
{
XmlDoc loadXmlDocument(const Zstring& filename); //throw FileError
void checkForMappingErrors(const XmlIn& xmlInput, const Zstring& filename); //throw FileError

void saveXmlDocument(const XmlDoc& doc, const Zstring& filename); //throw FileError
}

#endif // XMLBASE_H_INCLUDED
