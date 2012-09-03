// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "xml_proc.h"
#include <wx/filefn.h>
#include <zen/file_handling.h>
#include <wx+/string_conv.h>

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
            return type == "REAL";
    }
    return false;
}
}


void xmlAccess::readRealConfig(const Zstring& filename, XmlRealConfig& config)
{
    XmlDoc doc;
    loadXmlDocument(filename, doc); //throw FfsXmlError

    if (!isXmlTypeRTS(doc))
        throw FfsXmlError(replaceCpy(_("File %x does not contain a valid configuration."), L"%x", fmtFileName(filename)));

    XmlIn in(doc);
    ::readConfig(in, config);

    if (in.errorsOccured())
        throw FfsXmlError(replaceCpy(_("Configuration file %x loaded partially only."), L"%x", fmtFileName(filename)) + L"\n\n" +
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


void xmlAccess::writeRealConfig(const XmlRealConfig& config, const Zstring& filename)
{
    XmlDoc doc("FreeFileSync");
    doc.root().setAttribute("XmlType", "REAL");

    XmlOut out(doc);
    writeConfig(config, out);

    saveXmlDocument(doc, filename); //throw (FfsXmlError)
}
