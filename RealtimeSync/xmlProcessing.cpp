#include "xmlProcessing.h"
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
        throw XmlError(wxString(_("File does not exist:")) + wxT(" \"") + filename + wxT("\""));

    TiXmlDocument doc;
    if (!loadXmlDocument(filename, XML_REAL_CONFIG, doc))
        throw XmlError(wxString(_("Error reading file:")) + wxT(" \"") + filename + wxT("\""));

    RtsXmlParser parser(doc.RootElement());
    parser.readXmlRealConfig(config); //read GUI layout configuration

    if (parser.errorsOccured())
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT(" \"") + filename + wxT("\"\n\n") +
                       parser.getErrorMessageFormatted(), XmlError::WARNING);
}


void xmlAccess::writeRealConfig(const XmlRealConfig& outputCfg, const wxString& filename)
{
    TiXmlDocument doc;
    getDefaultXmlDocument(XML_REAL_CONFIG, doc);

    //populate and write XML tree
    if (    !writeXmRealSettings(outputCfg, doc) || //add GUI layout configuration settings
            !saveXmlDocument(filename, doc)) //save XML
        throw XmlError(wxString(_("Error writing file:")) + wxT(" \"") + filename + wxT("\""));
    return;
}

//--------------------------------------------------------------------------------


void RtsXmlParser::readXmlRealConfig(xmlAccess::XmlRealConfig& outputCfg)
{
    //read directories for monitoring
    const TiXmlElement* directoriesToWatch = TiXmlHandleConst(root).FirstChild("Directories").ToElement();

    readXmlElementLogging("Folder", directoriesToWatch, outputCfg.directories);

    //commandline to execute
    readXmlElementLogging("Commandline", root, outputCfg.commandline);

    //delay
    readXmlElementLogging("Delay", root, outputCfg.delay);
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
