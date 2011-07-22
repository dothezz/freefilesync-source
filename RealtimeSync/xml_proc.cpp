// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "xml_proc.h"
#include <wx/filefn.h>
#include "../shared/i18n.h"
#include <file_handling.h>
#include <string_conv.h>
#include <xml_base.h>

using namespace zen;
using namespace xmlAccess;


namespace
{
void readConfig(const XmlIn& in, XmlRealConfig& config)
{
    in["Directories"](config.directories);
    in["Commandline"](config.commandline);
    in["Delay"      ](config.delay);
}


bool isXmlTypeRTS(const XmlDoc& doc) //throw()
{
    if (doc.root().getNameAs<std::string>() == "FreeFileSync")
    {
        std::string type;
        if (doc.root().getAttribute("XmlType", type))
        {
            if (type == "REAL")
                return true;
        }
    }
    return false;
}
}


void xmlAccess::readRealConfig(const wxString& filename, XmlRealConfig& config)
{
    if (!fileExists(toZ(filename)))
        throw FfsXmlError(wxString(_("File does not exist:")) + wxT("\n\"") + filename + wxT("\""));

    XmlDoc doc;
    loadXmlDocument(filename, doc);  //throw (FfsXmlError)

    if (!isXmlTypeRTS(doc))
        throw FfsXmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\""));

    XmlIn in(doc);
    ::readConfig(in, config);

    if (in.errorsOccured())
        throw FfsXmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\"\n\n") +
                          getErrorMessageFormatted(in), FfsXmlError::WARNING);
}


namespace
{
void writeConfig(const XmlRealConfig& config, XmlOut& out)
{
    out["Directories"](config.directories);
    out["Commandline"](config.commandline);
    out["Delay"      ](config.delay);
}
}


void xmlAccess::writeRealConfig(const XmlRealConfig& config, const wxString& filename)
{
    XmlDoc doc("FreeFileSync");
    doc.root().setAttribute("XmlType", "REAL");

    XmlOut out(doc);
    writeConfig(config, out);

    saveXmlDocument(doc, filename); //throw (FfsXmlError)
}
