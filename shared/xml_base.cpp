// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "xml_base.h"
#include <wx/intl.h>
#include "file_io.h"
#include "string_conv.h"
#include "system_constants.h"
#include <boost/scoped_array.hpp>
#include "file_handling.h"

using namespace ffs3;


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


namespace
{
//convert (0xD, 0xA) and (0xD) to 0xA: just like in TiXmlDocument::LoadFile(); not sure if actually needed
void normalize(std::vector<char>& stream)
{
    std::vector<char> tmp;
    for (std::vector<char>::const_iterator i = stream.begin(); i != stream.end(); ++i)
        switch (*i)
        {
            case 0xD:
                tmp.push_back(0xA);
                if (i + 1 != stream.end() && *(i + 1) == 0xA)
                    ++i;
                break;
            default:
                tmp.push_back(*i);
        }

    stream.swap(tmp);
}


void loadRawXmlDocument(const wxString& filename, TiXmlDocument& document) //throw XmlError()
{
    using xmlAccess::XmlError;

    const size_t BUFFER_SIZE = 2 * 1024 * 1024; //maximum size of a valid FreeFileSync XML file!

    std::vector<char> inputBuffer;
    inputBuffer.resize(BUFFER_SIZE);

    try
    {
        FileInput inputFile(wxToZ(filename)); //throw FileError();
        const size_t bytesRead = inputFile.read(&inputBuffer[0], inputBuffer.size()); //throw FileError()

        if (bytesRead == 0 || bytesRead >= inputBuffer.size()) //treat XML files larger than 2 MB as erroneous: loading larger files just wastes CPU + memory
            throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\""));

        inputBuffer.resize(bytesRead + 1);
        inputBuffer[bytesRead] = 0; //set null-termination!!!!
    }
    catch (const FileError& error) //more detailed error messages than with wxWidgets
    {
        throw XmlError(error.msg());
    }

    //convert (0xD, 0xA) and (0xD) to (0xA): just like in TiXmlDocument::LoadFile(); not sure if actually needed
    normalize(inputBuffer);

    document.Parse(&inputBuffer[0], 0, TIXML_DEFAULT_ENCODING); //respect null-termination!

    TiXmlElement* root = document.RootElement();

    if (root && (root->ValueStr() == std::string("FreeFileSync"))) //check for FFS configuration xml
        return; //finally... success!

    throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\""));
}
}


xmlAccess::XmlType xmlAccess::getXmlType(const wxString& filename) //throw()
{
    try
    {
        TiXmlDocument doc;
        ::loadRawXmlDocument(filename, doc); //throw XmlError()

        TiXmlElement* root = doc.RootElement();
        if (root)
        {
            const char* cfgType = root->Attribute("XmlType");
            if (cfgType)
            {
                const std::string type(cfgType);

                if (type == getTypeName(XML_GUI_CONFIG))
                    return XML_GUI_CONFIG;
                else if (type == getTypeName(XML_BATCH_CONFIG))
                    return XML_BATCH_CONFIG;
                else if (type == getTypeName(XML_GLOBAL_SETTINGS))
                    return XML_GLOBAL_SETTINGS;
                else if (type == getTypeName(XML_REAL_CONFIG))
                    return XML_REAL_CONFIG;
            }
        }
    }
    catch (const XmlError&) {}

    return XML_OTHER;
}


void xmlAccess::loadXmlDocument(const wxString& filename, const xmlAccess::XmlType type, TiXmlDocument& document) //throw XmlError()
{
    TiXmlBase::SetCondenseWhiteSpace(false); //do not condense whitespace characters

    ::loadRawXmlDocument(filename, document); //throw XmlError()

    TiXmlElement* root = document.RootElement();
    if (root)
    {
        const char* cfgType = root->Attribute("XmlType");
        if (cfgType && std::string(cfgType) == getTypeName(type))
            return; //finally... success!
    }

    throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\""));
}


void xmlAccess::getDefaultXmlDocument(const XmlType type, TiXmlDocument& document)
{
    TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "UTF-8", ""); //delete won't be necessary later; ownership passed to TiXmlDocument!
    document.LinkEndChild(decl);

    TiXmlElement* root = new TiXmlElement("FreeFileSync");
    root->SetAttribute("XmlType", getTypeName(type)); //xml configuration type

    document.LinkEndChild(root);
}


namespace
{
bool saveNecessary(const Zstring& filename, const std::string& dataToWrite) //throw()
{
    try
    {
        if (ffs3::getFilesize(filename) != static_cast<unsigned long>(dataToWrite.size())) //throw FileError();
            return true;

        boost::scoped_array<char> inputBuffer(new char[dataToWrite.size() + 1]); //+ 1 in order to test for end of file!

        FileInput inputFile(filename); //throw FileError();

        const size_t bytesRead = inputFile.read(inputBuffer.get(), dataToWrite.size() + 1); //throw FileError()
        if (bytesRead != dataToWrite.size()) //implicit test for eof!
            return true;

        return ::memcmp(inputBuffer.get(), dataToWrite.c_str(), dataToWrite.size()) != 0;
    }
    catch (const FileError&)
    {
        return true;
    }
}
}


void xmlAccess::saveXmlDocument(const wxString& filename, const TiXmlDocument& document) //throw (XmlError)
{
    TiXmlBase::SetCondenseWhiteSpace(false); //do not condense whitespace characters

    //convert XML into continuous byte sequence
    TiXmlPrinter printer;
    printer.SetLineBreak(wxString(common::LINE_BREAK).ToUTF8());
    document.Accept(&printer);
    const std::string buffer = printer.Str();

    if (saveNecessary(wxToZ(filename), buffer)) //only write file if different data will be written
    {
        try
        {
            FileOutput outputFile(wxToZ(filename));            //throw FileError()
            outputFile.write(buffer.c_str(), buffer.length()); //
        }
        catch (const FileError& error) //more detailed error messages than with wxWidgets
        {
            throw XmlError(error.msg());
        }
    }
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


bool XmlParser::errorsOccurred() const
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

