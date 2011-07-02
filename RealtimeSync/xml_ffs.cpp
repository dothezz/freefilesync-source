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
    for (std::vector<zen::FolderPairEnh>::const_iterator i = batchCfg.mainCfg.additionalPairs.begin();
         i != batchCfg.mainCfg.additionalPairs.end(); ++i)
    {
        uniqueFolders.insert(zToWx(i->leftDirectory));
        uniqueFolders.insert(zToWx(i->rightDirectory));
    }

    uniqueFolders.erase(wxString());

    output.directories.insert(output.directories.end(), uniqueFolders.begin(), uniqueFolders.end());

    output.commandline = wxT("\"") + zen::getLauncher() + wxT("\"") +
                         wxT(" \"") + filename + wxT("\"");

    return output;
}


void rts::readRealOrBatchConfig(const wxString& filename, xmlAccess::XmlRealConfig& config)  //throw (xmlAccess::FfsXmlError);
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
        xmlAccess::readConfig(filename, batchCfg); //throw (xmlAccess::FfsXmlError);
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
