// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef XMLPROCESSING_H_INCLUDED
#define XMLPROCESSING_H_INCLUDED

#include <vector>
#include <wx/string.h>
#include "../shared/xml_base.h"


namespace xmlAccess
{
struct XmlRealConfig
{
    XmlRealConfig() : delay(5) {}
    std::vector<wxString> directories;
    wxString commandline;
    size_t delay;
};

void readRealConfig(const wxString& filename, XmlRealConfig& config);           //throw (xmlAccess::XmlError);
void writeRealConfig(const XmlRealConfig& outputCfg, const wxString& filename); //throw (xmlAccess::XmlError);
}

#endif // XMLPROCESSING_H_INCLUDED
