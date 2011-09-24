// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "xml_base.h"
#include <file_handling.h>
#include <string_conv.h>
#include <file_io.h>
#include <i18n.h>

using namespace zen;


//loadXmlDocument vs loadStream:
//1. better error reporting
//2. quick exit if (potentially large) input file is not an XML
void xmlAccess::loadXmlDocument(const wxString& filename, XmlDoc& doc) //throw FfsXmlError
{
    std::string stream;
    try
    {
        {
            //quick test whether input is an XML: avoid loading large binary files up front!
            const std::string xmlBegin = "<?xml version=";
            std::vector<char> buffer(xmlBegin.size() + sizeof(zen::BYTE_ORDER_MARK_UTF8));

            FileInput inputFile(toZ(filename)); //throw FileError;
            const size_t bytesRead = inputFile.read(&buffer[0], buffer.size()); //throw FileError

            const std::string fileBegin(&buffer[0], bytesRead);

            if (!startsWith(fileBegin, xmlBegin) &&
                !startsWith(fileBegin, zen::BYTE_ORDER_MARK_UTF8 + xmlBegin)) //respect BOM!
                throw FfsXmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\""));
        }

        const zen::UInt64 fs = zen::getFilesize(toZ(filename)); //throw FileError
        stream.resize(to<size_t>(fs));

        FileInput inputFile(toZ(filename)); //throw FileError
        const size_t bytesRead = inputFile.read(&stream[0], stream.size()); //throw FileError
        if (bytesRead < to<size_t>(fs))
        {
            wxString errorMessage = wxString(_("Error reading file:")) + wxT("\n\"") + filename + wxT("\"");
            throw FfsXmlError(errorMessage + wxT("\n\n"));
        }
    }
    catch (const FileError& error) //more detailed error messages than with wxWidgets
    {
        throw FfsXmlError(error.msg());
    }

    try
    {
        zen::parse(stream, doc); //throw XmlParsingError
    }
    catch (const XmlParsingError&)
    {
        throw FfsXmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\""));
    }
}


const wxString xmlAccess::getErrorMessageFormatted(const XmlIn& in)
{
    wxString errorMessage = wxString(_("Could not read values for the following XML nodes:")) + wxT("\n");

    std::vector<wxString> failedNodes = in.getErrorsAs<wxString>();
    std::for_each(failedNodes.begin(), failedNodes.end(),
    [&](const wxString& str) { errorMessage += str + wxT('\n'); });

    return errorMessage;
}


void xmlAccess::saveXmlDocument(const zen::XmlDoc& doc, const wxString& filename) //throw (FfsXmlError)
{
    std::string stream = serialize(doc); //throw ()

    bool saveNecessary = true;
    try
    {
        if (zen::getFilesize(toZ(filename)) == stream.size()) //throw FileError
            try
            {
                if (zen::loadStream(filename) == stream) //throw XmlFileError
                    saveNecessary = false;
            }
            catch (const zen::XmlFileError&) {}
    }
    catch (FileError&) {}

    if (saveNecessary)
        try
        {
            FileOutput outputFile(toZ(filename), FileOutput::ACC_OVERWRITE); //throw FileError
            outputFile.write(stream.c_str(), stream.length());                 //
        }
        catch (const FileError& error) //more detailed error messages than with wxWidgets
        {
            throw FfsXmlError(error.msg());
        }
}
