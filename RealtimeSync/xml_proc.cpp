// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "xml_proc.h"
#include <wx/filefn.h>
#include "../shared/i18n.h"


using namespace xmlAccess;

class RtsXmlErrorLogger : public xmlAccess::XmlErrorLogger
{
public:
    void readConfig(const TiXmlElement* root, xmlAccess::XmlRealConfig& outputCfg);
};


//--------------------------------------------------------------------------------


void RtsXmlErrorLogger::readConfig(const TiXmlElement* root, xmlAccess::XmlRealConfig& outputCfg)
{
    //read directories for monitoring
    const TiXmlElement* directoriesToWatch = TiXmlHandleConst(root).FirstChild("Directories").ToElement();

    readXmlElementLogging("Folder", directoriesToWatch, outputCfg.directories);

    //commandline to execute
    readXmlElementLogging("Commandline", root, outputCfg.commandline);

    //delay
    readXmlElementLogging("Delay", root, outputCfg.delay);
}


void writeConfig(const xmlAccess::XmlRealConfig& outputCfg, TiXmlElement& root)
{
    //directories to monitor
    TiXmlElement* directoriesToWatch = new TiXmlElement("Directories");
    root.LinkEndChild(directoriesToWatch);
    xmlAccess::addXmlElement("Folder", outputCfg.directories, directoriesToWatch);

    //commandline to execute
    xmlAccess::addXmlElement("Commandline", outputCfg.commandline, &root);

    //delay
    xmlAccess::addXmlElement("Delay", outputCfg.delay, &root);
}


bool isXmlTypeRTS(const TiXmlDocument& doc) //throw()
{
    const TiXmlElement* root = doc.RootElement();
    if (root && root->ValueStr() == std::string("RealtimeSync"))
    {
        const char* cfgType = root->Attribute("XmlType");
        if (cfgType)
            return std::string(cfgType) == "REAL";
    }
    return false;
}


void initXmlDocument(TiXmlDocument& doc) //throw()
{
    TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "UTF-8", ""); //delete won't be necessary later; ownership passed to TiXmlDocument!
    doc.LinkEndChild(decl);

    TiXmlElement* root = new TiXmlElement("RealtimeSync");
    doc.LinkEndChild(root);

    addXmlAttribute("XmlType", "REAL", doc.RootElement());
}


void xmlAccess::readRealConfig(const wxString& filename, XmlRealConfig& config)
{
    //load XML
    if (!wxFileExists(filename))
        throw XmlError(wxString(_("File does not exist:")) + wxT("\n\"") + filename + wxT("\""));

    TiXmlDocument doc;
    loadXmlDocument(filename, doc); //throw (XmlError)

    if (!isXmlTypeRTS(doc))
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\""));

    RtsXmlErrorLogger parser;
    parser.readConfig(doc.RootElement(), config); //read GUI layout configuration

    if (parser.errorsOccurred())
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\"\n\n") +
                       parser.getErrorMessageFormatted(), XmlError::WARNING);
}


void xmlAccess::writeRealConfig(const XmlRealConfig& outputCfg, const wxString& filename)
{
    TiXmlDocument doc;
    initXmlDocument(doc); //throw()

    if (!doc.RootElement())
        throw XmlError(wxString(_("Error writing file:")) + wxT("\n\"") + filename + wxT("\""));

    writeConfig(outputCfg, *doc.RootElement()); //add GUI layout configuration settings

    saveXmlDocument(filename, doc); //throw (XmlError)
}
