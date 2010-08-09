// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "xml_ffs.h"
#include "../shared/standard_paths.h"
#include "../shared/global_func.h"
#include "../shared/zstring.h"
#include "functions.h"
#include "../shared/xml_base.h"
#include "../shared/string_conv.h"

//include FreeFileSync xml headers
#include "../library/process_xml.h"

using namespace ffs3;


#ifdef FFS_WIN
struct CmpNoCase
{
    bool operator()(const wxString& a, const wxString& b) const
    {
        return a.CmpNoCase(b) < 0;
    }
};
#endif


xmlAccess::XmlRealConfig convertBatchToReal(const xmlAccess::XmlBatchConfig& batchCfg, const wxString& filename)
{
    xmlAccess::XmlRealConfig output;

#ifdef FFS_WIN
    std::set<wxString, CmpNoCase> uniqueFolders;
#elif defined FFS_LINUX
    std::set<wxString> uniqueFolders;
#endif

    //add main folders
    uniqueFolders.insert(zToWx(batchCfg.mainCfg.firstPair.leftDirectory));
    uniqueFolders.insert(zToWx(batchCfg.mainCfg.firstPair.rightDirectory));

    //additional folders
    for (std::vector<ffs3::FolderPairEnh>::const_iterator i = batchCfg.mainCfg.additionalPairs.begin();
            i != batchCfg.mainCfg.additionalPairs.end(); ++i)
    {
        uniqueFolders.insert(zToWx(i->leftDirectory));
        uniqueFolders.insert(zToWx(i->rightDirectory));
    }

    output.directories.insert(output.directories.end(), uniqueFolders.begin(), uniqueFolders.end());

    output.commandline = ffs3::getBinaryDir() +
#ifdef FFS_WIN
                         wxT("FreeFileSync.exe") +
#elif defined FFS_LINUX
                         wxT("FreeFileSync") +
#endif
                         wxT(" \"") + filename + wxT("\"");

    return output;
}


void rts::readRealOrBatchConfig(const wxString& filename, xmlAccess::XmlRealConfig& config)  //throw (xmlAccess::XmlError);
{
    if (xmlAccess::getXmlType(filename) != xmlAccess::XML_BATCH_CONFIG)
    {
        xmlAccess::readRealConfig(filename, config);
        return;
    }

    //convert batch config to RealtimeSync config
    xmlAccess::XmlBatchConfig batchCfg;
    try
    {
        xmlAccess::readConfig(filename, batchCfg); //throw (xmlAccess::XmlError);
    }
    catch (const xmlAccess::XmlError& e)
    {
        if (e.getSeverity() == xmlAccess::XmlError::WARNING)
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
    catch (const xmlAccess::XmlError&) {} //user default language if error occured

    return settings.programLanguage;
}
