// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef XMLPROCESSING_H_INCLUDED
#define XMLPROCESSING_H_INCLUDED

#include <vector>
#include <wx/string.h>
#include <zen/zstring.h>
#include "../lib/xml_base.h"


namespace xmlAccess
{
struct XmlRealConfig
{
    XmlRealConfig() : delay(10) {}
    std::vector<wxString> directories;
    wxString commandline;
    size_t delay;
};

void readRealConfig(const Zstring& filename, XmlRealConfig& config);        //throw FfsXmlError
void writeRealConfig(const XmlRealConfig& config, const Zstring& filename); //throw FfsXmlError
}

#endif // XMLPROCESSING_H_INCLUDED
