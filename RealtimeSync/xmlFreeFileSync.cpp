#include "xmlFreeFileSync.h"
#include "../shared/standardPaths.h"
#include "../shared/globalFunctions.h"
#include "../shared/zstring.h"
#include "functions.h"
#include "../shared/xmlBase.h"
#include "../shared/stringConv.h"

//include FreeFileSync xml headers
#include "../library/processXml.h"

using namespace FreeFileSync;


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
    uniqueFolders.insert(zToWx(batchCfg.mainCfg.mainFolderPair.leftDirectory));
    uniqueFolders.insert(zToWx(batchCfg.mainCfg.mainFolderPair.rightDirectory));

    //additional folders
    for (std::vector<FreeFileSync::FolderPairEnh>::const_iterator i = batchCfg.mainCfg.additionalPairs.begin();
            i != batchCfg.mainCfg.additionalPairs.end(); ++i)
    {
        uniqueFolders.insert(zToWx(i->leftDirectory));
        uniqueFolders.insert(zToWx(i->rightDirectory));
    }

    output.directories.insert(output.directories.end(), uniqueFolders.begin(), uniqueFolders.end());

    output.commandline = FreeFileSync::getInstallationDir() + zToWx(globalFunctions::FILE_NAME_SEPARATOR) + wxT("FreeFileSync.exe ") +
                         wxT("\"") + filename + wxT("\"");

    return output;
}


void RealtimeSync::readRealOrBatchConfig(const wxString& filename, xmlAccess::XmlRealConfig& config)  //throw (xmlAccess::XmlError);
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
        xmlAccess::readBatchConfig(filename, batchCfg); //throw (xmlAccess::XmlError);
    }
    catch (const xmlAccess::XmlError& e)
    {
        if (e.getSeverity() != xmlAccess::XmlError::WARNING)
            throw;

        config = convertBatchToReal(batchCfg, filename); //do work despite parsing errors, then re-throw
        throw;                                 //
    }
        config = convertBatchToReal(batchCfg, filename);
}


int RealtimeSync::getProgramLanguage()
{
    xmlAccess::XmlGlobalSettings settings;

    try
    {
        xmlAccess::readGlobalSettings(settings);
    }
    catch (const xmlAccess::XmlError&)
        {} //user default language if error occured

    return settings.programLanguage;
}
