// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
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

            FileInput inputFile(filename); //throw FileError
            const size_t bytesRead = inputFile.read(&buffer[0], buffer.size()); //throw FileError

            const std::string fileBegin(&buffer[0], bytesRead);

            if (!startsWith(fileBegin, xmlBegin) &&
                !startsWith(fileBegin, zen::BYTE_ORDER_MARK_UTF8 + xmlBegin)) //respect BOM!
                throw FfsXmlError(replaceCpy(_("File %x does not contain a valid configuration."), L"%x", fmtFileName(filename)));
        }

        const zen::UInt64 fs = zen::getFilesize(filename); //throw FileError
        stream.resize(to<size_t>(fs));

        FileInput inputFile(filename); //throw FileError
        const size_t bytesRead = inputFile.read(&stream[0], stream.size()); //throw FileError
        if (bytesRead < to<size_t>(fs))
            throw FfsXmlError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(filename)));
    }
    catch (const FileError& error)
    {
        throw FfsXmlError(error.toString());
    }

    try
    {
        zen::parse(stream, doc); //throw XmlParsingError
    }
    catch (const XmlParsingError& e)
    {
        throw FfsXmlError(
            replaceCpy(replaceCpy(replaceCpy(_("Error parsing file %x, row %y, column %z."),
                                             L"%x", fmtFileName(filename)),
                                  L"%y", numberTo<std::wstring>(e.row)),
                       L"%z", numberTo<std::wstring>(e.col)));
    }
}


const std::wstring xmlAccess::getErrorMessageFormatted(const XmlIn& in)
{
    std::wstring msg;

    const auto& failedElements = in.getErrorsAs<std::wstring>();
    if (!failedElements.empty())
    {
        msg = _("Cannot read the following XML elements:") + L"\n";
        std::for_each(failedElements.begin(), failedElements.end(),
        [&](const std::wstring& str) { msg += str + L'\n'; });
    }

    return msg;
}


void xmlAccess::saveXmlDocument(const zen::XmlDoc& doc, const Zstring& filename) //throw FfsXmlError
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
            outputFile.write(stream.c_str(), stream.length());          //
        }
        catch (const FileError& error) //more detailed error messages than with wxWidgets
        {
            throw FfsXmlError(error.toString());
        }
}
