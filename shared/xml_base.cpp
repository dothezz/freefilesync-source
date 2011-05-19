// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "xml_base.h"
#include "file_io.h"
#include "i18n.h"
#include "string_conv.h"
#include "system_constants.h"
#include "file_handling.h"

using namespace zen;
using namespace xmlAccess;


namespace
{
//convert (0xD, 0xA) and (0xD) to 0xA: just like in TiXmlDocument::LoadFile(); not sure if actually needed
void normalize(std::vector<char>& stream)
{
    std::vector<char> tmp;
    for (std::vector<char>::const_iterator i = stream.begin(); i != stream.end(); ++i)
        switch (*i)
        {
            case '\xD':
                tmp.push_back('\xA');
                if (i + 1 != stream.end() && *(i + 1) == '\xA')
                    ++i;
                break;
            default:
                tmp.push_back(*i);
        }

    stream.swap(tmp);
}


void parseRawXmlDocument(const wxString& filename, TiXmlDocument& document) //throw XmlError()
{
    const zen::UInt64 fs = zen::getFilesize(wxToZ(filename));

    try
    {
        {
            //quick test whether input is an XML: avoid loading large binary files up front!
            std::string xmlBegin = "<?xml version=";
            std::vector<char> buffer(xmlBegin.size());

            FileInput inputFile(wxToZ(filename)); //throw (FileError);
            const size_t bytesRead = inputFile.read(&buffer[0], buffer.size()); //throw (FileError)
            if (bytesRead < xmlBegin.size() || !std::equal(buffer.begin(), buffer.end(), xmlBegin.begin()))
                throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\""));
        }

        std::vector<char> buffer(to<size_t>(fs) + 1); //inc. null-termination (already set!)

        FileInput inputFile(wxToZ(filename)); //throw (FileError);
        const size_t bytesRead = inputFile.read(&buffer[0], to<size_t>(fs)); //throw (FileError)
        if (bytesRead < to<size_t>(fs))
        {
            wxString errorMessage = wxString(_("Error reading file:")) + wxT("\n\"") + filename + wxT("\"");
            throw XmlError(errorMessage + wxT("\n\n"));
        }

        //convert (0xD, 0xA) and (0xD) to (0xA): just like in TiXmlDocument::LoadFile(); not sure if actually needed
        normalize(buffer);

        document.Parse(&buffer[0], 0, TIXML_ENCODING_UTF8); //respect null-termination!
    }
    catch (const FileError& error) //more detailed error messages than with wxWidgets
    {
        throw XmlError(error.msg());
    }
}
}


void xmlAccess::loadXmlDocument(const wxString& filename, TiXmlDocument& document) //throw XmlError()
{
    TiXmlBase::SetCondenseWhiteSpace(false); //do not condense whitespace characters

    ::parseRawXmlDocument(filename, document); //throw XmlError()

    TiXmlElement* root = document.RootElement();
    if (!root)
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\""));
}


namespace
{
bool saveNecessary(const Zstring& filename, const std::string& dataToWrite) //throw()
{
    try
    {
        if (zen::getFilesize(filename) != static_cast<unsigned long>(dataToWrite.size())) //throw (FileError);
            return true;

        std::vector<char> inputBuffer(dataToWrite.size() + 1); //+ 1 in order to test for end of file!

        FileInput inputFile(filename); //throw (FileError);

        const size_t bytesRead = inputFile.read(&inputBuffer[0], inputBuffer.size()); //throw (FileError)
        if (bytesRead != dataToWrite.size()) //implicit test for eof!
            return true;

        return ::memcmp(&inputBuffer[0], dataToWrite.c_str(), dataToWrite.size()) != 0;
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
            FileOutput outputFile(wxToZ(filename), FileOutput::ACC_OVERWRITE);            //throw (FileError)
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
        //load elements
        const TiXmlElement* xmlList = parent->FirstChildElement(name);
        if (xmlList)
        {
            output.clear();
            for (const TiXmlElement* item = xmlList->FirstChildElement("Item"); item != NULL; item = item->NextSiblingElement())
            {
                const char* text = item->GetText();
                if (text) //may be NULL!!
                    output.push_back(wxString::FromUTF8(text));
                else
                    output.push_back(wxEmptyString);
            }
            return true;
        }
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
    if (parent)
    {
        TiXmlElement* xmlList= new TiXmlElement(name);
        parent->LinkEndChild(xmlList);

        for (std::vector<wxString>::const_iterator i = value.begin(); i != value.end(); ++i)
            addXmlElement("Item", std::string(i->ToUTF8()), xmlList);
    }
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


using xmlAccess::XmlErrorLogger;

void XmlErrorLogger::logError(const std::string& nodeName)
{
    failedNodes.push_back(wxString::FromUTF8(nodeName.c_str()));
}


const wxString XmlErrorLogger::getErrorMessageFormatted() const
{
    wxString errorMessage = wxString(_("Could not read values for the following XML nodes:")) + wxT("\n");
    for (std::vector<wxString>::const_iterator i = failedNodes.begin(); i != failedNodes.end(); ++i)
        errorMessage += wxString(wxT("<")) + *i + wxT(">") + ((i - failedNodes.begin() + 1) % 3 == 0 ? wxT("\n") : wxT(" "));
    return errorMessage;
}

