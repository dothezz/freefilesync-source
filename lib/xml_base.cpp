// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "xml_base.h"
#include <zen/file_handling.h>
#include <zen/file_io.h>

using namespace zen;


//loadXmlDocument vs loadStream:
//1. better error reporting
//2. quick exit if (potentially large) input file is not an XML
void xmlAccess::loadXmlDocument(const Zstring& filename, XmlDoc& doc) //throw FfsXmlError
{
    std::string stream;
    try
    {
        {
            //quick test whether input is an XML: avoid loading large binary files up front!
            const std::string xmlBegin = "<?xml version=";
            std::vector<char> buffer(xmlBegin.size() + sizeof(zen::BYTE_ORDER_MARK_UTF8));

            FileInput inputFile(filename); //throw FileError;
            const size_t bytesRead = inputFile.read(&buffer[0], buffer.size()); //throw FileError

            const std::string fileBegin(&buffer[0], bytesRead);

            if (!startsWith(fileBegin, xmlBegin) &&
                !startsWith(fileBegin, zen::BYTE_ORDER_MARK_UTF8 + xmlBegin)) //respect BOM!
                throw FfsXmlError(_("Error parsing configuration file:") + L"\n\"" + filename + L"\"");
        }

        const zen::UInt64 fs = zen::getFilesize(filename); //throw FileError
        stream.resize(to<size_t>(fs));

        FileInput inputFile(filename); //throw FileError
        const size_t bytesRead = inputFile.read(&stream[0], stream.size()); //throw FileError
        if (bytesRead < to<size_t>(fs))
            throw FfsXmlError(_("Error reading file:") + L"\n\"" + filename + L"\"");
    }
    catch (const FileError& error)
    {
        if (!fileExists(filename))
            throw FfsXmlError(_("File does not exist:") + L"\n\"" + filename+ L"\"");

        throw FfsXmlError(error.toString());
    }

    try
    {
        zen::parse(stream, doc); //throw XmlParsingError
    }
    catch (const XmlParsingError&)
    {
        throw FfsXmlError(_("Error parsing configuration file:") + L"\n\"" + filename + L"\"");
    }
}


const std::wstring xmlAccess::getErrorMessageFormatted(const XmlIn& in)
{
    std::wstring errorMessage = _("Could not read values for the following XML nodes:") + L"\n";

    std::vector<std::wstring> failedNodes = in.getErrorsAs<std::wstring>();
    std::for_each(failedNodes.begin(), failedNodes.end(),
    [&](const std::wstring& str) { errorMessage += str + L'\n'; });

    return errorMessage;
}


void xmlAccess::saveXmlDocument(const zen::XmlDoc& doc, const Zstring& filename) //throw (FfsXmlError)
{
    std::string stream = serialize(doc); //throw ()

    bool saveNecessary = true;
    try
    {
        if (zen::getFilesize(filename) == stream.size()) //throw FileError
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
            FileOutput outputFile(filename, FileOutput::ACC_OVERWRITE); //throw FileError
            outputFile.write(stream.c_str(), stream.length());                 //
        }
        catch (const FileError& error) //more detailed error messages than with wxWidgets
        {
            throw FfsXmlError(error.toString());
        }
}
