// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "xml_base.h"
#include <zen/file_handling.h>
#include <zen/file_io.h>

using namespace zen;


//loadXmlDocument vs loadStream:
//1. better error reporting
//2. quick exit if (potentially large) input file is not an XML
XmlDoc xmlAccess::loadXmlDocument(const Zstring& filename) //throw FfsXmlError
{
    std::string stream;
    try
    {
        FileInput inputFile(filename); //throw FileError
        {
            //quick test whether input is an XML: avoid loading large binary files up front!
            const std::string xmlBegin = "<?xml version=";
            stream.resize(strLength(zen::BYTE_ORDER_MARK_UTF8) + xmlBegin.size());

            const size_t bytesRead = inputFile.read(&stream[0], stream.size()); //throw FileError
            stream.resize(bytesRead);

            if (!startsWith(stream, xmlBegin) &&
                !startsWith(stream, zen::BYTE_ORDER_MARK_UTF8 + xmlBegin)) //respect BOM!
                throw FfsXmlError(replaceCpy(_("File %x does not contain a valid configuration."), L"%x", fmtFileName(filename)));
        }

        const size_t blockSize = 128 * 1024;
        do
        {
            stream.resize(stream.size() + blockSize);

            const size_t bytesRead = inputFile.read(&*stream.begin() + stream.size() - blockSize, blockSize); //throw FileError
            if (bytesRead < blockSize)
                stream.resize(stream.size() - (blockSize - bytesRead)); //caveat: unsigned arithmetics
        }
        while (!inputFile.eof());
    }
    catch (const FileError& error)
    {
        throw FfsXmlError(error.toString());
    }

    try
    {
        return zen::parse(stream); //throw XmlParsingError
    }
    catch (const XmlParsingError& e)
    {
        throw FfsXmlError(
            replaceCpy(replaceCpy(replaceCpy(_("Error parsing file %x, row %y, column %z."),
                                             L"%x", fmtFileName(filename)),
                                  L"%y", numberTo<std::wstring>(e.row + 1)),
                       L"%z", numberTo<std::wstring>(e.col + 1)));
    }
}


const std::wstring xmlAccess::getErrorMessageFormatted(const std::vector<std::wstring>& failedElements)
{
    std::wstring msg;

    if (!failedElements.empty())
    {
        msg = _("Cannot read the following XML elements:") + L"\n";
        std::for_each(failedElements.begin(), failedElements.end(), [&](const std::wstring& elem) { msg += L"\n" + elem; });
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
