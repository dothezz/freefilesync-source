#include "xmlFreeFileSync.h"
#include "../shared/standardPaths.h"
#include "../shared/globalFunctions.h"
#include "../shared/zstring.h"
#include "functions.h"
#include "../shared/xmlBase.h"

//include FreeFileSync xml headers
#include "../library/processXml.h"


#ifdef FFS_WIN
class CmpNoCase
{
public:
    bool operator()(const wxString& a, const wxString& b)
    {
        return FreeFileSync::compareStringsWin32(a.c_str(), b.c_str(), a.length(), b.length()) < 0;
    }
};
#endif


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
        if (e.getSeverity() != xmlAccess::XmlError::WARNING) //ignore parsing errors
            throw;
    }

#ifdef FFS_WIN
    std::set<wxString, CmpNoCase> uniqueFolders;
#elif defined FFS_LINUX
    std::set<wxString> uniqueFolders;
#endif

    for (std::vector<FreeFileSync::FolderPair>::const_iterator i = batchCfg.directoryPairs.begin(); i != batchCfg.directoryPairs.end(); ++i)
    {
        uniqueFolders.insert(i->leftDirectory.c_str());
        uniqueFolders.insert(i->rightDirectory.c_str());
    }

    config.directories.insert(config.directories.end(), uniqueFolders.begin(), uniqueFolders.end());

    config.commandline = FreeFileSync::getInstallationDir() + globalFunctions::FILE_NAME_SEPARATOR + wxT("FreeFileSync.exe ") +
                         wxT("\"") + filename + wxT("\"");
}


int RealtimeSync::getProgramLanguage()
{
    xmlAccess::XmlGlobalSettings settings;
    xmlAccess::readGlobalSettings(settings);
    return settings.programLanguage;
}
