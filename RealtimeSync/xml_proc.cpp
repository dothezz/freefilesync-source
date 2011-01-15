// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "xml_proc.h"
#include <wx/filefn.h>
#include <wx/intl.h>


class RtsXmlParser : public xmlAccess::XmlParser
{
public:
    RtsXmlParser(const TiXmlElement* rootElement) : xmlAccess::XmlParser(rootElement) {}

    void readXmlRealConfig(xmlAccess::XmlRealConfig& outputCfg);
};



void readXmlRealConfig(const TiXmlDocument& doc, xmlAccess::XmlRealConfig& outputCfg);
bool writeXmRealSettings(const xmlAccess::XmlRealConfig& outputCfg, TiXmlDocument& doc);


void xmlAccess::readRealConfig(const wxString& filename, XmlRealConfig& config)
{
    //load XML
    if (!wxFileExists(filename))
        throw XmlError(wxString(_("File does not exist:")) + wxT("\n\"") + filename + wxT("\""));

    TiXmlDocument doc;
    loadXmlDocument(filename, XML_REAL_CONFIG, doc); //throw (XmlError)

    RtsXmlParser parser(doc.RootElement());
    parser.readXmlRealConfig(config); //read GUI layout configuration

    if (parser.errorsOccurred())
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\"\n\n") +
                       parser.getErrorMessageFormatted(), XmlError::WARNING);
}


void xmlAccess::writeRealConfig(const XmlRealConfig& outputCfg, const wxString& filename)
{
    TiXmlDocument doc;
    getDefaultXmlDocument(XML_REAL_CONFIG, doc);

    //populate and write XML tree
    if (!writeXmRealSettings(outputCfg, doc)) //add GUI layout configuration settings
        throw XmlError(wxString(_("Error writing file:")) + wxT("\n\"") + filename + wxT("\""));

    saveXmlDocument(filename, doc); //throw (XmlError)
}

//--------------------------------------------------------------------------------


void RtsXmlParser::readXmlRealConfig(xmlAccess::XmlRealConfig& outputCfg)
{
    //read directories for monitoring
    const TiXmlElement* directoriesToWatch = TiXmlHandleConst(getRoot()).FirstChild("Directories").ToElement();

    readXmlElementLogging("Folder", directoriesToWatch, outputCfg.directories);

    //commandline to execute
    readXmlElementLogging("Commandline", getRoot(), outputCfg.commandline);

    //delay
    readXmlElementLogging("Delay", getRoot(), outputCfg.delay);
}


bool writeXmRealSettings(const xmlAccess::XmlRealConfig& outputCfg, TiXmlDocument& doc)
{
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    //directories to monitor
    TiXmlElement* directoriesToWatch = new TiXmlElement("Directories");
    root->LinkEndChild(directoriesToWatch);
    xmlAccess::addXmlElement("Folder", outputCfg.directories, directoriesToWatch);

    //commandline to execute
    xmlAccess::addXmlElement("Commandline", outputCfg.commandline, root);

    //delay
    xmlAccess::addXmlElement("Delay", outputCfg.delay, root);

    return true;
}
