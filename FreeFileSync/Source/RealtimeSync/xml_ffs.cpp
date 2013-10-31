// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "xml_ffs.h"
#include "../lib/ffs_paths.h"
#include <zen/zstring.h>
//#include <wx+/string_conv.h>

//include FreeFileSync xml headers
#include "../lib/process_xml.h"

using namespace zen;


xmlAccess::XmlRealConfig convertBatchToReal(const xmlAccess::XmlBatchConfig& batchCfg, const Zstring& filename)
{
    std::set<Zstring, LessFilename> uniqueFolders;

    //add main folders
    uniqueFolders.insert(batchCfg.mainCfg.firstPair.dirnamePhraseLeft);
    uniqueFolders.insert(batchCfg.mainCfg.firstPair.dirnamePhraseRight);

    //additional folders
    for (const FolderPairEnh& fp : batchCfg.mainCfg.additionalPairs)
    {
        uniqueFolders.insert(fp.dirnamePhraseLeft);
        uniqueFolders.insert(fp.dirnamePhraseRight);
    }

    uniqueFolders.erase(Zstring());

    xmlAccess::XmlRealConfig output;
    output.directories.assign(uniqueFolders.begin(), uniqueFolders.end());
    output.commandline = Zstr("\"") + zen::getFreeFileSyncLauncher() + Zstr("\" \"") + filename + Zstr("\"");
    return output;
}


void rts::readRealOrBatchConfig(const Zstring& filename, xmlAccess::XmlRealConfig& config)  //throw xmlAccess::FfsXmlError;
{
    using namespace xmlAccess;

    if (getXmlType(filename) != XML_TYPE_BATCH) //throw FfsXmlError
        return readRealConfig(filename, config); //throw FfsXmlError

    //convert batch config to RealtimeSync config
    XmlBatchConfig batchCfg;
    try
    {
        readConfig(filename, batchCfg); //throw FfsXmlError;
    }
    catch (const FfsXmlError& e)
    {
        if (e.getSeverity() == FfsXmlError::WARNING)
            config = convertBatchToReal(batchCfg, filename); //do work despite parsing errors, then re-throw

        throw;                                 //
    }
    config = convertBatchToReal(batchCfg, filename);
}


int rts::getProgramLanguage()
{
    xmlAccess::XmlGlobalSettings settings;

    try
    {
        xmlAccess::readConfig(settings);
    }
    catch (const xmlAccess::FfsXmlError&) {} //user default language if error occurred

    return settings.programLanguage;
}
