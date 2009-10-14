#include "xmlBase.h"
#include "globalFunctions.h"
#include <wx/ffile.h>
#include <wx/intl.h>


std::string getTypeName(xmlAccess::XmlType type)
{
    switch (type)
    {
    case xmlAccess::XML_GUI_CONFIG:
        return "GUI";
    case xmlAccess::XML_BATCH_CONFIG:
        return "BATCH";
    case xmlAccess::XML_GLOBAL_SETTINGS:
        return "GLOBAL";
    case xmlAccess::XML_REAL_CONFIG:
        return "REAL";
    case xmlAccess::XML_OTHER:
        break;
    }
    assert(false);
    return "OTHER";
}


xmlAccess::XmlType xmlAccess::getXmlType(const wxString& filename)
{
    if (!wxFileExists(filename))
        return XML_OTHER;

    //workaround to get a FILE* from a unicode filename
    wxFFile configFile(filename, wxT("rb"));
    if (!configFile.IsOpened())
        return XML_OTHER;

    FILE* inputFile = configFile.fp();

    TiXmlDocument doc;
    try
    {
        if (!doc.LoadFile(inputFile)) //fails if inputFile is no proper XML
            return XML_OTHER;
    }
    catch (const std::exception&)
    {
        //unfortunately TiXml isn't very smart and tries to allocate space for the complete file: length_error exception is thrown for large files!
        return XML_OTHER;
    }

    TiXmlElement* root = doc.RootElement();

    if (!root || (root->ValueStr() != std::string("FreeFileSync"))) //check for FFS configuration xml
        return XML_OTHER;

    const char* cfgType = root->Attribute("XmlType");
    if (!cfgType)
        return XML_OTHER;

    const std::string type(cfgType);

    if (type == getTypeName(XML_GUI_CONFIG))
        return XML_GUI_CONFIG;
    else if (type == getTypeName(XML_BATCH_CONFIG))
        return XML_BATCH_CONFIG;
    else if (type == getTypeName(XML_GLOBAL_SETTINGS))
        return XML_GLOBAL_SETTINGS;
    else if (type == getTypeName(XML_REAL_CONFIG))
        return XML_REAL_CONFIG;

    return XML_OTHER;
}


bool xmlAccess::loadXmlDocument(const wxString& fileName, const xmlAccess::XmlType type, TiXmlDocument& document)
{
    if (!wxFileExists(fileName)) //avoid wxWidgets error message when wxFFile receives not existing file
        return false;

    //workaround to get a FILE* from a unicode filename
    wxFFile dummyFile(fileName, wxT("rb")); //binary mode needed for TiXml to work correctly in this case
    if (dummyFile.IsOpened())
    {
        FILE* inputFile = dummyFile.fp();

        TiXmlBase::SetCondenseWhiteSpace(false); //do not condense whitespace characters

        if (document.LoadFile(inputFile)) //load XML; fails if inputFile is no proper XML
        {
            TiXmlElement* root = document.RootElement();

            if (root && (root->ValueStr() == std::string("FreeFileSync"))) //check for FFS configuration xml
            {
                const char* cfgType = root->Attribute("XmlType");
                if (cfgType)
                    return std::string(cfgType) == getTypeName(type);
            }
        }
    }

    return false;
}


void xmlAccess::getDefaultXmlDocument(const XmlType type, TiXmlDocument& document)
{
    TiXmlBase::SetCondenseWhiteSpace(false); //do not condense whitespace characters

    TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "UTF-8", ""); //delete won't be necessary later; ownership passed to TiXmlDocument!
    document.LinkEndChild(decl);

    TiXmlElement* root = new TiXmlElement("FreeFileSync");

    root->SetAttribute("XmlType", getTypeName(type)); //xml configuration type

    document.LinkEndChild(root);
}


bool xmlAccess::saveXmlDocument(const wxString& fileName, const TiXmlDocument& document)
{
    //workaround to get a FILE* from a unicode filename
    wxFFile dummyFile(fileName, wxT("w")); //no need for "binary" mode here
    if (!dummyFile.IsOpened())
        return false;

    FILE* outputFile = dummyFile.fp();

    if (!document.SaveFile(outputFile)) //save XML
        return false;

    return dummyFile.Flush(); //flush data to disk! (think of multiple batch jobs writing/reading)
}


//################################################################################################################


bool xmlAccess::readXmlElement(const std::string& name, const TiXmlElement* parent, std::string& output)
{
    if (parent)
    {
        const TiXmlElement* child = parent->FirstChildElement(name);
        if (child)
        {
            const char* text = child->GetText();
            if (text) //may be NULL!!
                output = text;
            else //NULL means empty string
                output.clear();
            return true;
        }
    }

    return false;
}


bool xmlAccess::readXmlElement(const std::string& name, const TiXmlElement* parent, wxString& output)
{
    std::string tempString;
    if (!readXmlElement(name, parent, tempString))
        return false;

    output = wxString::FromUTF8(tempString.c_str());
    return true;
}


bool xmlAccess::readXmlElement(const std::string& name, const TiXmlElement* parent, int& output)
{
    std::string temp;
    if (!readXmlElement(name, parent, temp))
        return false;

    output = globalFunctions::stringToInt(temp);
    return true;
}


bool xmlAccess::readXmlElement(const std::string& name, const TiXmlElement* parent, unsigned int& output)
{
    int dummy = 0;
    if (!xmlAccess::readXmlElement(name, parent, dummy))
        return false;

    output = static_cast<unsigned int>(dummy);
    return true;
}


bool xmlAccess::readXmlElement(const std::string& name, const TiXmlElement* parent, long& output)
{
    std::string temp;
    if (readXmlElement(name, parent, temp))
    {
        output = globalFunctions::stringToLong(temp);
        return true;
    }
    else
        return false;
}


bool xmlAccess::readXmlElement(const std::string& name, const TiXmlElement* parent, bool& output)
{
    std::string dummy;
    if (readXmlElement(name, parent, dummy))
    {
        output = dummy == "true";
        return true;
    }
    else
        return false;
}


bool xmlAccess::readXmlElement(const std::string& name, const TiXmlElement* parent, std::vector<wxString>& output)
{
    if (parent)
    {
        output.clear();

        //load elements
        const TiXmlElement* element = parent->FirstChildElement(name);
        while (element)
        {
            const char* text = element->GetText();
            if (text) //may be NULL!!
                output.push_back(wxString::FromUTF8(text));
            else
                output.push_back(wxEmptyString);

            element = element->NextSiblingElement();
        }
        return true;
    }

    return false;
}


bool xmlAccess::readXmlAttribute(const std::string& name, const TiXmlElement* node, std::string& output)
{
    if (node)
    {
        const std::string* attrib = node->Attribute(name);
        if (attrib) //may be NULL!
        {
            output = *attrib;
            return true;
        }
    }
    return false;
}


bool xmlAccess::readXmlAttribute(const std::string& name, const TiXmlElement* node, wxString& output)
{
    std::string tempString;
    if (!readXmlAttribute(name, node, tempString))
        return false;

    output = wxString::FromUTF8(tempString.c_str());
    return true;
}


bool xmlAccess::readXmlAttribute(const std::string& name, const TiXmlElement* node, int& output)
{
    std::string dummy;
    if (readXmlAttribute(name, node, dummy))
    {
        output = globalFunctions::stringToInt(dummy);
        return true;
    }
    else
        return false;
}


bool xmlAccess::readXmlAttribute(const std::string& name, const TiXmlElement* node, unsigned int& output)
{
    std::string dummy;
    if (readXmlAttribute(name, node, dummy))
    {
        output = globalFunctions::stringToInt(dummy);
        return true;
    }
    else
        return false;
}


bool xmlAccess::readXmlAttribute(const std::string& name, const TiXmlElement* node, bool& output)
{
    std::string dummy;
    if (readXmlAttribute(name, node, dummy))
    {
        output = dummy == "true";
        return true;
    }
    else
        return false;
}


//################################################################################################################


void xmlAccess::addXmlElement(const std::string& name, const std::string& value, TiXmlElement* parent)
{
    if (parent)
    {
        TiXmlElement* subElement = new TiXmlElement(name);
        parent->LinkEndChild(subElement);
        subElement->LinkEndChild(new TiXmlText(value));
    }
}


void xmlAccess::addXmlElement(const std::string& name, const wxString& value, TiXmlElement* parent)
{
    addXmlElement(name, std::string(value.ToUTF8()), parent);
}


void xmlAccess::addXmlElement(const std::string& name, const int value, TiXmlElement* parent)
{
    addXmlElement(name, globalFunctions::numberToString(value), parent);
}


void xmlAccess::addXmlElement(const std::string& name, const unsigned int value, TiXmlElement* parent)
{
    addXmlElement(name, static_cast<int>(value), parent);
}


void xmlAccess::addXmlElement(const std::string& name, const long value, TiXmlElement* parent)
{
    addXmlElement(name, globalFunctions::numberToString(value), parent);
}


void xmlAccess::addXmlElement(const std::string& name, const bool value, TiXmlElement* parent)
{
    if (value)
        addXmlElement(name, std::string("true"), parent);
    else
        addXmlElement(name, std::string("false"), parent);
}


void xmlAccess::addXmlElement(const std::string& name, const std::vector<wxString>& value, TiXmlElement* parent)
{
    for (std::vector<wxString>::const_iterator i = value.begin(); i != value.end(); ++i)
        addXmlElement(name, std::string(i->ToUTF8()), parent);
}


void xmlAccess::addXmlAttribute(const std::string& name, const std::string& value, TiXmlElement* node)
{
    if (node)
        node->SetAttribute(name, value);
}


void xmlAccess::addXmlAttribute(const std::string& name, const wxString& value, TiXmlElement* node)
{
    addXmlAttribute(name, std::string(value.ToUTF8()), node);
}


void xmlAccess::addXmlAttribute(const std::string& name, const int value, TiXmlElement* node)
{
    addXmlAttribute(name, globalFunctions::numberToString(value), node);
}


void xmlAccess::addXmlAttribute(const std::string& name, const unsigned int value, TiXmlElement* node)
{
    addXmlAttribute(name, globalFunctions::numberToString(value), node);
}


void xmlAccess::addXmlAttribute(const std::string& name, const bool value, TiXmlElement* node)
{
    if (value)
        addXmlAttribute(name, std::string("true"), node);
    else
        addXmlAttribute(name, std::string("false"), node);
}


using xmlAccess::XmlParser;

void XmlParser::logError(const std::string& nodeName)
{
    failedNodes.push_back(wxString::FromUTF8(nodeName.c_str()));
}


bool XmlParser::errorsOccured() const
{
    return !failedNodes.empty();
}


const wxString XmlParser::getErrorMessageFormatted() const
{
    wxString errorMessage = wxString(_("Could not read values for the following XML nodes:")) + wxT("\n");
    for (std::vector<wxString>::const_iterator i = failedNodes.begin(); i != failedNodes.end(); ++i)
        errorMessage += wxString(wxT("<")) + *i + wxT(">") + ((i - failedNodes.begin() + 1) % 3 == 0 ? wxT("\n") : wxT(" "));
    return errorMessage;
}

