// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "xml_ffs.h"
#include "../shared/standard_paths.h"
#include "../shared/global_func.h"
#include "../shared/zstring.h"
//#include "functions.h"
#include "../shared/xml_base.h"
#include "../shared/string_conv.h"

//include FreeFileSync xml headers
#include "../library/process_xml.h"

using namespace zen;


xmlAccess::XmlRealConfig convertBatchToReal(const xmlAccess::XmlBatchConfig& batchCfg, const wxString& filename)
{
    xmlAccess::XmlRealConfig output;

    std::set<Zstring, LessFilename> uniqueFolders;

    //add main folders
    uniqueFolders.insert(batchCfg.mainCfg.firstPair.leftDirectory);
    uniqueFolders.insert(batchCfg.mainCfg.firstPair.rightDirectory);

    //additional folders
    std::for_each(batchCfg.mainCfg.additionalPairs.begin(), batchCfg.mainCfg.additionalPairs.end(),
                  [&](const FolderPairEnh& fp)
    {
        uniqueFolders.insert(fp.leftDirectory);
        uniqueFolders.insert(fp.rightDirectory);
    });

    uniqueFolders.erase(Zstring());

    output.directories.clear();
    std::transform(uniqueFolders.begin(), uniqueFolders.end(), std::back_inserter(output.directories),
    [](const Zstring& fn) { return toWx(fn); });

    output.commandline = wxT("\"") + zen::getLauncher() + wxT("\"") +
                         wxT(" \"") + filename + wxT("\"");

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
    catch (const xmlAccess::FfsXmlError&) {} //user default language if error occured

    return settings.programLanguage;
}
