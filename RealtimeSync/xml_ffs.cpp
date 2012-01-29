// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "xml_ffs.h"
#include "../lib/ffs_paths.h"
#include <zen/zstring.h>
#include <wx+/string_conv.h>

//include FreeFileSync xml headers
#include "../lib/process_xml.h"

using namespace zen;


xmlAccess::XmlRealConfig convertBatchToReal(const xmlAccess::XmlBatchConfig& batchCfg, const Zstring& filename)
{
    xmlAccess::XmlRealConfig output;

    std::set<Zstring, LessFilename> uniqueFolders;

    //add main folders
    uniqueFolders.insert(batchCfg.mainCfg.firstPair.leftDirectory);
    uniqueFolders.insert(batchCfg.mainCfg.firstPair.rightDirectory);

    //additional folders
    std::for_each(batchCfg.mainCfg.additionalPairs.begin(), batchCfg.mainCfg.additionalPairs.end(),
                  [&](const FolderPairEnh & fp)
    {
        uniqueFolders.insert(fp.leftDirectory);
        uniqueFolders.insert(fp.rightDirectory);
    });

    uniqueFolders.erase(Zstring());

    output.directories.clear();
    std::transform(uniqueFolders.begin(), uniqueFolders.end(), std::back_inserter(output.directories),
    [](const Zstring & fn) { return toWx(fn); });

    output.commandline = std::wstring(L"\"") + zen::getLauncher() + L"\"" +
                         L" \"" + filename + L"\"";

    return output;
}


void rts::readRealOrBatchConfig(const wxString& filename, xmlAccess::XmlRealConfig& config)  //throw xmlAccess::FfsXmlError;
{
    if (xmlAccess::getXmlType(filename) != xmlAccess::XML_TYPE_BATCH)
    {
        xmlAccess::readRealConfig(filename, config);
        return;
    }

    //convert batch config to RealtimeSync config
    xmlAccess::XmlBatchConfig batchCfg;
    try
    {
        xmlAccess::readConfig(filename, batchCfg); //throw xmlAccess::FfsXmlError;
    }
    catch (const xmlAccess::FfsXmlError& e)
    {
        if (e.getSeverity() == xmlAccess::FfsXmlError::WARNING)
            config = convertBatchToReal(batchCfg, toZ(filename)); //do work despite parsing errors, then re-throw

        throw;                                 //
    }
    config = convertBatchToReal(batchCfg, toZ(filename));
}


int rts::getProgramLanguage()
{
    xmlAccess::XmlGlobalSettings settings;

    try
    {
        xmlAccess::readConfig(settings);
    }
    catch (const xmlAccess::FfsXmlError&) {} //user default language if error occured

    return settings.programLanguage;
}
