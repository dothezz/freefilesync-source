#include "processXml.h"
#include "../shared/xmlBase.h"
#include <wx/filefn.h>
#include <wx/intl.h>
#include "../shared/globalFunctions.h"
#include "../shared/fileHandling.h"
#include "../shared/standardPaths.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#endif

using namespace FreeFileSync;


class FfsXmlParser : public xmlAccess::XmlParser
{
public:
    FfsXmlParser(const TiXmlElement* rootElement) : xmlAccess::XmlParser(rootElement) {}

    //read gui settings, all values retrieved are optional, so check for initial values! (== -1)
    void readXmlGuiConfig(xmlAccess::XmlGuiConfig& outputCfg);
    //read batch settings, all values retrieved are optional
    void readXmlBatchConfig(xmlAccess::XmlBatchConfig& outputCfg);
    //read global settings, valid for both GUI and batch mode, independent from configuration
    void readXmlGlobalSettings(xmlAccess::XmlGlobalSettings& outputCfg);

private:
    //read basic FreefileSync settings (used by commandline and GUI), return true if ALL values have been retrieved successfully
    void readXmlMainConfig(MainConfiguration& mainCfg, std::vector<FolderPair>& directoryPairs);
};


//write gui settings
bool writeXmlGuiConfig(const xmlAccess::XmlGuiConfig& outputCfg, TiXmlDocument& doc);
//write batch settings
bool writeXmlBatchConfig(const xmlAccess::XmlBatchConfig& outputCfg, TiXmlDocument& doc);
//write global settings
bool writeXmlGlobalSettings(const xmlAccess::XmlGlobalSettings& outputCfg, TiXmlDocument& doc);
//write basic FreefileSync settings (used by commandline and GUI), return true if everything was written successfully
bool writeXmlMainConfig(const MainConfiguration& mainCfg, const std::vector<FolderPair>& directoryPairs, TiXmlDocument& doc);


void xmlAccess::readGuiConfig(const wxString& filename, xmlAccess::XmlGuiConfig& config)
{
    //load XML
    if (!wxFileExists(filename))
        throw XmlError(wxString(_("File does not exist:")) + wxT(" \"") + filename + wxT("\""));

    TiXmlDocument doc;
    if (!loadXmlDocument(filename, XML_GUI_CONFIG, doc))
        throw XmlError(wxString(_("Error reading file:")) + wxT(" \"") + filename + wxT("\""));

    FfsXmlParser parser(doc.RootElement());
    parser.readXmlGuiConfig(config); //read GUI layout configuration

    if (parser.errorsOccured())
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT(" \"") + filename + wxT("\"\n\n") +
                       parser.getErrorMessageFormatted(), XmlError::WARNING);
}


void xmlAccess::readBatchConfig(const wxString& filename, xmlAccess::XmlBatchConfig& config)
{
    //load XML
    if (!wxFileExists(filename))
        throw XmlError(wxString(_("File does not exist:")) + wxT(" \"") + filename + wxT("\""));

    TiXmlDocument doc;
    if (!loadXmlDocument(filename, XML_BATCH_CONFIG, doc))
        throw XmlError(wxString(_("Error reading file:")) + wxT(" \"") + filename + wxT("\""));

    FfsXmlParser parser(doc.RootElement());
    parser.readXmlBatchConfig(config); //read GUI layout configuration

    if (parser.errorsOccured())
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT(" \"") + filename + wxT("\"\n\n") +
                       parser.getErrorMessageFormatted(), XmlError::WARNING);
}


void xmlAccess::readGlobalSettings(xmlAccess::XmlGlobalSettings& config)
{
    //load XML
    if (!wxFileExists(FreeFileSync::getGlobalConfigFile()))
        throw XmlError(wxString(_("File does not exist:")) + wxT(" \"") + FreeFileSync::getGlobalConfigFile() + wxT("\""));

    TiXmlDocument doc;
    if (!loadXmlDocument(FreeFileSync::getGlobalConfigFile(), XML_GLOBAL_SETTINGS, doc))
        throw XmlError(wxString(_("Error reading file:")) + wxT(" \"") + FreeFileSync::getGlobalConfigFile() + wxT("\""));

    FfsXmlParser parser(doc.RootElement());
    parser.readXmlGlobalSettings(config); //read GUI layout configuration

    if (parser.errorsOccured())
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT(" \"") + FreeFileSync::getGlobalConfigFile() + wxT("\"\n\n") +
                       parser.getErrorMessageFormatted(), XmlError::WARNING);
}


void xmlAccess::writeGuiConfig(const XmlGuiConfig& outputCfg, const wxString& filename)
{
    TiXmlDocument doc;
    getDefaultXmlDocument(XML_GUI_CONFIG, doc);

    //populate and write XML tree
    if (    !writeXmlGuiConfig(outputCfg, doc) || //add GUI layout configuration settings
            !saveXmlDocument(filename, doc)) //save XML
        throw XmlError(wxString(_("Error writing file:")) + wxT(" \"") + filename + wxT("\""));
    return;
}


void xmlAccess::writeBatchConfig(const XmlBatchConfig& outputCfg, const wxString& filename)
{
    TiXmlDocument doc;
    getDefaultXmlDocument(XML_BATCH_CONFIG, doc);

    //populate and write XML tree
    if (    !writeXmlBatchConfig(outputCfg, doc) || //add batch configuration settings
            !saveXmlDocument(filename, doc)) //save XML
        throw XmlError(wxString(_("Error writing file:")) + wxT(" \"") + filename + wxT("\""));
    return;
}


void xmlAccess::writeGlobalSettings(const XmlGlobalSettings& outputCfg)
{
    TiXmlDocument doc;
    getDefaultXmlDocument(XML_GLOBAL_SETTINGS, doc);

    //populate and write XML tree
    if (    !writeXmlGlobalSettings(outputCfg, doc) || //add GUI layout configuration settings
            !saveXmlDocument(FreeFileSync::getGlobalConfigFile(), doc)) //save XML
        throw XmlError(wxString(_("Error writing file:")) + wxT(" \"") + FreeFileSync::getGlobalConfigFile() + wxT("\""));
    return;
}


bool readXmlElement(const std::string& name, const TiXmlElement* parent, CompareVariant& output)
{
    std::string dummy;
    if (xmlAccess::readXmlElement(name, parent, dummy))
    {
        if (dummy == "ByTimeAndSize")
            output = FreeFileSync::CMP_BY_TIME_SIZE;
        else if (dummy == "ByContent")
            output = FreeFileSync::CMP_BY_CONTENT;
        else
            return false;

        return true;
    }
    else
        return false;
}


bool readXmlElement(const std::string& name, const TiXmlElement* parent, SyncDirectionCfg& output)
{
    std::string dummy;
    if (xmlAccess::readXmlElement(name, parent, dummy))
    {
        if (dummy == "left")
            output = SYNC_DIR_CFG_LEFT;
        else if (dummy == "right")
            output = SYNC_DIR_CFG_RIGHT;
        else //treat all other input as "none"
            output = SYNC_DIR_CFG_NONE;

        return true;
    }
    else
        return false;
}


bool readXmlElement(const std::string& name, const TiXmlElement* parent, xmlAccess::OnError& output)
{
    std::string dummy;
    if (xmlAccess::readXmlElement(name, parent, dummy))
    {
        if (dummy == "Ignore")
            output = xmlAccess::ON_ERROR_IGNORE;
        else if (dummy == "Exit")
            output = xmlAccess::ON_ERROR_EXIT;
        else //treat all other input as popup
            output = xmlAccess::ON_ERROR_POPUP;

        return true;
    }
    else
        return false;
}


bool readXmlElement(const std::string& name, const TiXmlElement* parent , FreeFileSync::DeletionPolicy& output)
{
    std::string dummy;
    if (xmlAccess::readXmlElement(name, parent, dummy))
    {
        if (dummy == "DeletePermanently")
            output = FreeFileSync::DELETE_PERMANENTLY;
        else if (dummy == "MoveToRecycleBin")
            output = FreeFileSync::MOVE_TO_RECYCLE_BIN;
        else if (dummy == "MoveToCustomDirectory")
            output = FreeFileSync::MOVE_TO_CUSTOM_DIRECTORY;
        else
            return false;

        return true;
    }

    return false;
}


bool readXmlAttribute(const std::string& name, const TiXmlElement* node, xmlAccess::ColumnTypes& output)
{
    int dummy;
    if (xmlAccess::readXmlAttribute(name, node, dummy))
    {
        output = static_cast<xmlAccess::ColumnTypes>(dummy);
        return true;
    }
    else
        return false;
}


//################################################################################################################
void FfsXmlParser::readXmlMainConfig(MainConfiguration& mainCfg, std::vector<FolderPair>& directoryPairs)
{
    TiXmlHandleConst hRoot(root); //custom const handle: TiXml API seems broken in this regard

    const TiXmlElement* cmpSettings  = hRoot.FirstChild("MainConfig").FirstChild("Comparison").ToElement();
    const TiXmlElement* syncConfig   = hRoot.FirstChild("MainConfig").FirstChild("Synchronization").FirstChild("Directions").ToElement();
    const TiXmlElement* miscSettings = hRoot.FirstChild("MainConfig").FirstChild("Miscellaneous").ToElement();
    const TiXmlElement* filter       = TiXmlHandleConst(miscSettings).FirstChild("Filter").ToElement();

//###########################################################
    //read compare variant
    readXmlElementLogging("Variant", cmpSettings, mainCfg.compareVar);

    //read folder pair(s)
    const TiXmlElement* folderPair = TiXmlHandleConst(cmpSettings).FirstChild("Folders").FirstChild("Pair").ToElement();

    //read folder pairs
    directoryPairs.clear();
    while (folderPair)
    {
        wxString temp;
        wxString temp2;
        readXmlElementLogging("Left",  folderPair, temp);
        readXmlElementLogging("Right", folderPair, temp2);
        directoryPairs.push_back(FolderPair(temp.c_str(), temp2.c_str()));

        folderPair = folderPair->NextSiblingElement();
    }

//###########################################################
    //read sync configuration
    readXmlElementLogging("LeftOnly",   syncConfig, mainCfg.syncConfiguration.exLeftSideOnly);
    readXmlElementLogging("RightOnly",  syncConfig, mainCfg.syncConfiguration.exRightSideOnly);
    readXmlElementLogging("LeftNewer",  syncConfig, mainCfg.syncConfiguration.leftNewer);
    readXmlElementLogging("RightNewer", syncConfig, mainCfg.syncConfiguration.rightNewer);
    readXmlElementLogging("Different",  syncConfig, mainCfg.syncConfiguration.different);

//###########################################################
    //read filter settings
    readXmlElementLogging("Active",  filter, mainCfg.filterIsActive);
    readXmlElementLogging("Include", filter, mainCfg.includeFilter);
    readXmlElementLogging("Exclude", filter, mainCfg.excludeFilter);
//###########################################################
    //other
    readXmlElementLogging("DeletionPolicy", miscSettings, mainCfg.handleDeletion);
    readXmlElementLogging("CustomDeletionFolder", miscSettings, mainCfg.customDeletionDirectory);
}


void FfsXmlParser::readXmlGuiConfig(xmlAccess::XmlGuiConfig& outputCfg)
{
    //read main config
    readXmlMainConfig(outputCfg.mainCfg, outputCfg.directoryPairs);

    //read GUI specific config data
    const TiXmlElement* guiConfig = TiXmlHandleConst(root).FirstChild("GuiConfig").ToElement();

    readXmlElementLogging("HideFiltered", guiConfig, outputCfg.hideFilteredElements);

    xmlAccess::OnError errorHand;
    readXmlElementLogging("HandleError", guiConfig, errorHand);
    outputCfg.ignoreErrors = errorHand == xmlAccess::ON_ERROR_IGNORE;

    readXmlElementLogging("SyncPreviewActive", guiConfig, outputCfg.syncPreviewEnabled);
}


void FfsXmlParser::readXmlBatchConfig(xmlAccess::XmlBatchConfig& outputCfg)
{
    //read main config
    readXmlMainConfig(outputCfg.mainCfg, outputCfg.directoryPairs);

    //read batch specific config
    const TiXmlElement* batchConfig = TiXmlHandleConst(root).FirstChild("BatchConfig").ToElement();

    readXmlElementLogging("Silent", batchConfig, outputCfg.silent);
    readXmlElementLogging("LogfileDirectory", batchConfig, outputCfg.logFileDirectory);
    readXmlElementLogging("HandleError", batchConfig, outputCfg.handleError);
}


void FfsXmlParser::readXmlGlobalSettings(xmlAccess::XmlGlobalSettings& outputCfg)
{
    //read global settings
    const TiXmlElement* global = TiXmlHandleConst(root).FirstChild("Shared").ToElement();

    //try to read program language setting
    readXmlElementLogging("Language", global, outputCfg.programLanguage);

    //max. allowed file time deviation
    readXmlElementLogging("FileTimeTolerance", global, outputCfg.fileTimeTolerance);

    //ignore +/- 1 hour due to DST change
    readXmlElementLogging("IgnoreOneHourDifference", global, outputCfg.ignoreOneHourDiff);

    //traverse into symbolic links (to folders)
    readXmlElementLogging("TraverseDirectorySymlinks", global, outputCfg.traverseDirectorySymlinks);

    //copy symbolic links to files
    readXmlElementLogging("CopyFileSymlinks", global, outputCfg.copyFileSymlinks);

    //last update check
    readXmlElementLogging("LastCheckForUpdates", global, outputCfg.lastUpdateCheck);


    const TiXmlElement* warnings = TiXmlHandleConst(root).FirstChild("Shared").FirstChild("Warnings").ToElement();

    //folder dependency check
    readXmlElementLogging("CheckForDependentFolders", warnings, outputCfg.warnings.warningDependentFolders);
    //significant difference check
    readXmlElementLogging("CheckForSignificantDifference", warnings, outputCfg.warnings.warningSignificantDifference);
    //check free disk space
    readXmlElementLogging("CheckForFreeDiskSpace", warnings, outputCfg.warnings.warningNotEnoughDiskSpace);
    //check for unresolved conflicts
    readXmlElementLogging("CheckForUnresolvedConflicts", warnings, outputCfg.warnings.warningUnresolvedConflicts);

    //gui specific global settings (optional)
    const TiXmlElement* mainWindow = TiXmlHandleConst(root).FirstChild("Gui").FirstChild("Windows").FirstChild("Main").ToElement();

    //read application window size and position
    readXmlElementLogging("Width",     mainWindow, outputCfg.gui.widthNotMaximized);
    readXmlElementLogging("Height",    mainWindow, outputCfg.gui.heightNotMaximized);
    readXmlElementLogging("PosX",      mainWindow, outputCfg.gui.posXNotMaximized);
    readXmlElementLogging("PosY",      mainWindow, outputCfg.gui.posYNotMaximized);
    readXmlElementLogging("Maximized", mainWindow, outputCfg.gui.isMaximized);

    readXmlElementLogging("ManualDeletionOnBothSides", mainWindow, outputCfg.gui.deleteOnBothSides);
    readXmlElementLogging("ManualDeletionUseRecycler", mainWindow, outputCfg.gui.useRecyclerForManualDeletion);
    readXmlElementLogging("ShowFileIconsLeft",         mainWindow, outputCfg.gui.showFileIconsLeft);
    readXmlElementLogging("ShowFileIconsRight",        mainWindow, outputCfg.gui.showFileIconsRight);
    readXmlElementLogging("PopupOnConfigChange",       mainWindow, outputCfg.gui.popupOnConfigChange);
    readXmlElementLogging("SummaryBeforeSync",         mainWindow, outputCfg.gui.showSummaryBeforeSync);

//###########################################################
    //read column attributes
    readXmlAttributeLogging("AutoAdjust", TiXmlHandleConst(mainWindow).FirstChild("LeftColumns").ToElement(), outputCfg.gui.autoAdjustColumnsLeft);

    const TiXmlElement* leftColumn = TiXmlHandleConst(mainWindow).FirstChild("LeftColumns").FirstChild("Column").ToElement();
    unsigned int colPos = 0;
    while (leftColumn)
    {
        xmlAccess::ColumnAttrib newAttrib;
        newAttrib.position = colPos++;
        readXmlAttributeLogging("Type",    leftColumn, newAttrib.type);
        readXmlAttributeLogging("Visible", leftColumn, newAttrib.visible);
        readXmlAttributeLogging("Width",   leftColumn, newAttrib.width);
        outputCfg.gui.columnAttribLeft.push_back(newAttrib);

        leftColumn = leftColumn->NextSiblingElement();
    }

    readXmlAttributeLogging("AutoAdjust", TiXmlHandleConst(mainWindow).FirstChild("RightColumns").ToElement(), outputCfg.gui.autoAdjustColumnsRight);

    const TiXmlElement* rightColumn = TiXmlHandleConst(mainWindow).FirstChild("RightColumns").FirstChild("Column").ToElement();
    colPos = 0;
    while (rightColumn)
    {
        xmlAccess::ColumnAttrib newAttrib;
        newAttrib.position = colPos++;
        readXmlAttributeLogging("Type",    rightColumn, newAttrib.type);
        readXmlAttributeLogging("Visible", rightColumn, newAttrib.visible);
        readXmlAttributeLogging("Width",   rightColumn, newAttrib.width);
        outputCfg.gui.columnAttribRight.push_back(newAttrib);

        rightColumn = rightColumn->NextSiblingElement();
    }

    //load folder history elements
    const TiXmlElement* historyLeft = TiXmlHandleConst(mainWindow).FirstChild("FolderHistoryLeft").ToElement();
    //load max. history size
    readXmlAttributeLogging("MaximumSize", historyLeft, outputCfg.gui.folderHistLeftMax);
    //load config history elements
    readXmlElementLogging("Folder", historyLeft, outputCfg.gui.folderHistoryLeft);


    const TiXmlElement* historyRight = TiXmlHandleConst(mainWindow).FirstChild("FolderHistoryRight").ToElement();
    //load max. history size
    readXmlAttributeLogging("MaximumSize", historyRight, outputCfg.gui.folderHistRightMax);
    //load config history elements
    readXmlElementLogging("Folder", historyRight, outputCfg.gui.folderHistoryRight);


    readXmlElementLogging("SelectedTabBottomLeft", mainWindow, outputCfg.gui.selectedTabBottomLeft);


    const TiXmlElement* gui = TiXmlHandleConst(root).FirstChild("Gui").ToElement();

    //commandline for file manager integration
    readXmlElementLogging("FileManager", gui, outputCfg.gui.commandLineFileManager);


    //load config file history
    const TiXmlElement* cfgHistory = TiXmlHandleConst(root).FirstChild("Gui").FirstChild("ConfigHistory").ToElement();

    //load max. history size
    readXmlAttributeLogging("MaximumSize", cfgHistory, outputCfg.gui.cfgHistoryMax);

    //load config history elements
    readXmlElementLogging("File", cfgHistory, outputCfg.gui.cfgFileHistory);


    //batch specific global settings
    //const TiXmlElement* batch = TiXmlHandleConst(root).FirstChild("Batch").ToElement();
}


void addXmlElement(const std::string& name, const CompareVariant variant, TiXmlElement* parent)
{
    switch (variant)
    {
    case FreeFileSync::CMP_BY_TIME_SIZE:
        xmlAccess::addXmlElement(name, std::string("ByTimeAndSize"), parent);
        break;
    case FreeFileSync::CMP_BY_CONTENT:
        xmlAccess::addXmlElement(name, std::string("ByContent"), parent);
        break;
    }
}


void addXmlElement(TiXmlElement* parent, const std::string& name, const SyncDirectionCfg value)
{
    switch (value)
    {
    case SYNC_DIR_CFG_LEFT:
        xmlAccess::addXmlElement(name, std::string("left"), parent);
        break;
    case SYNC_DIR_CFG_RIGHT:
        xmlAccess::addXmlElement(name, std::string("right"), parent);
        break;
    case SYNC_DIR_CFG_NONE:
        xmlAccess::addXmlElement(name, std::string("none"), parent);
        break;
    }
}


void addXmlElement(const std::string& name, const xmlAccess::OnError value, TiXmlElement* parent)
{
    switch (value)
    {
    case xmlAccess::ON_ERROR_IGNORE:
        xmlAccess::addXmlElement(name, std::string("Ignore"), parent);
        break;
    case xmlAccess::ON_ERROR_EXIT:
        xmlAccess::addXmlElement(name, std::string("Exit"), parent);
        break;
    case xmlAccess::ON_ERROR_POPUP:
        xmlAccess::addXmlElement(name, std::string("Popup"), parent);
        break;
    }
}


void addXmlElement(const std::string& name, const FreeFileSync::DeletionPolicy value, TiXmlElement* parent)
{
    switch (value)
    {
    case FreeFileSync::DELETE_PERMANENTLY:
        xmlAccess::addXmlElement(name, std::string("DeletePermanently"), parent);
        break;
    case FreeFileSync::MOVE_TO_RECYCLE_BIN:
        xmlAccess::addXmlElement(name, std::string("MoveToRecycleBin"), parent);
        break;
    case FreeFileSync::MOVE_TO_CUSTOM_DIRECTORY:
        xmlAccess::addXmlElement(name, std::string("MoveToCustomDirectory"), parent);
        break;
    }
}


void addXmlAttribute(const std::string& name, const xmlAccess::ColumnTypes value, TiXmlElement* node)
{
    xmlAccess::addXmlAttribute(name, static_cast<int>(value), node);
}


bool writeXmlMainConfig(const MainConfiguration& mainCfg, const std::vector<FolderPair>& directoryPairs, TiXmlDocument& doc)
{
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlElement* settings = new TiXmlElement("MainConfig");
    root->LinkEndChild(settings);

//###########################################################
    TiXmlElement* cmpSettings = new TiXmlElement("Comparison");
    settings->LinkEndChild(cmpSettings);

    //write compare algorithm
    ::addXmlElement("Variant", mainCfg.compareVar, cmpSettings);

    //write folder pair(s)
    TiXmlElement* folders = new TiXmlElement("Folders");
    cmpSettings->LinkEndChild(folders);

    //write folder pairs
    for (std::vector<FolderPair>::const_iterator i = directoryPairs.begin(); i != directoryPairs.end(); ++i)
    {
        TiXmlElement* folderPair = new TiXmlElement("Pair");
        folders->LinkEndChild(folderPair);

        xmlAccess::addXmlElement("Left",  wxString(i->leftDirectory.c_str()),  folderPair);
        xmlAccess::addXmlElement("Right", wxString(i->rightDirectory.c_str()), folderPair);
    }

//###########################################################
    TiXmlElement* syncSettings = new TiXmlElement("Synchronization");
    settings->LinkEndChild(syncSettings);

    //write sync configuration
    TiXmlElement* syncConfig = new TiXmlElement("Directions");
    syncSettings->LinkEndChild(syncConfig);

    ::addXmlElement(syncConfig, "LeftOnly",   mainCfg.syncConfiguration.exLeftSideOnly);
    ::addXmlElement(syncConfig, "RightOnly",  mainCfg.syncConfiguration.exRightSideOnly);
    ::addXmlElement(syncConfig, "LeftNewer",  mainCfg.syncConfiguration.leftNewer);
    ::addXmlElement(syncConfig, "RightNewer", mainCfg.syncConfiguration.rightNewer);
    ::addXmlElement(syncConfig, "Different",  mainCfg.syncConfiguration.different);

//###########################################################
    TiXmlElement* miscSettings = new TiXmlElement("Miscellaneous");
    settings->LinkEndChild(miscSettings);

    //write filter settings
    TiXmlElement* filter = new TiXmlElement("Filter");
    miscSettings->LinkEndChild(filter);

    xmlAccess::addXmlElement("Active",  mainCfg.filterIsActive, filter);
    xmlAccess::addXmlElement("Include", mainCfg.includeFilter,  filter);
    xmlAccess::addXmlElement("Exclude", mainCfg.excludeFilter,  filter);

    //other
    addXmlElement("DeletionPolicy", mainCfg.handleDeletion, miscSettings);
    xmlAccess::addXmlElement("CustomDeletionFolder", mainCfg.customDeletionDirectory, miscSettings);
//###########################################################

    return true;
}


bool writeXmlGuiConfig(const xmlAccess::XmlGuiConfig& inputCfg, TiXmlDocument& doc)
{
    //write main config
    if (!writeXmlMainConfig(inputCfg.mainCfg, inputCfg.directoryPairs, doc))
        return false;

    //write GUI specific config
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlElement* guiConfig = new TiXmlElement("GuiConfig");
    root->LinkEndChild(guiConfig);

    xmlAccess::addXmlElement("HideFiltered",      inputCfg.hideFilteredElements, guiConfig);

    ::addXmlElement("HandleError", inputCfg.ignoreErrors ? xmlAccess::ON_ERROR_IGNORE : xmlAccess::ON_ERROR_POPUP, guiConfig);

    xmlAccess::addXmlElement("SyncPreviewActive", inputCfg.syncPreviewEnabled,   guiConfig);

    return true;
}


bool writeXmlBatchConfig(const xmlAccess::XmlBatchConfig& inputCfg, TiXmlDocument& doc)
{
    //write main config
    if (!writeXmlMainConfig(inputCfg.mainCfg, inputCfg.directoryPairs, doc))
        return false;

    //write GUI specific config
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlElement* batchConfig = new TiXmlElement("BatchConfig");
    root->LinkEndChild(batchConfig);

    xmlAccess::addXmlElement("Silent", inputCfg.silent, batchConfig);
    xmlAccess::addXmlElement("LogfileDirectory", inputCfg.logFileDirectory, batchConfig);
    ::addXmlElement("HandleError", inputCfg.handleError, batchConfig);

    return true;
}


bool writeXmlGlobalSettings(const xmlAccess::XmlGlobalSettings& inputCfg, TiXmlDocument& doc)
{
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    //###################################################################
    //write global settings
    TiXmlElement* global = new TiXmlElement("Shared");
    root->LinkEndChild(global);

    //program language
    xmlAccess::addXmlElement("Language", inputCfg.programLanguage, global);

    //max. allowed file time deviation
    xmlAccess::addXmlElement("FileTimeTolerance", inputCfg.fileTimeTolerance, global);

    //ignore +/- 1 hour due to DST change
    xmlAccess::addXmlElement("IgnoreOneHourDifference", inputCfg.ignoreOneHourDiff, global);

    //traverse into symbolic links (to folders)
    xmlAccess::addXmlElement("TraverseDirectorySymlinks", inputCfg.traverseDirectorySymlinks, global);

    //copy symbolic links to files
    xmlAccess::addXmlElement("CopyFileSymlinks", inputCfg.copyFileSymlinks, global);

    //last update check
    xmlAccess::addXmlElement("LastCheckForUpdates", inputCfg.lastUpdateCheck, global);

    //warnings
    TiXmlElement* warnings = new TiXmlElement("Warnings");
    global->LinkEndChild(warnings);

    //warning: dependent folders
    xmlAccess::addXmlElement("CheckForDependentFolders", inputCfg.warnings.warningDependentFolders, warnings);

    //significant difference check
    xmlAccess::addXmlElement("CheckForSignificantDifference", inputCfg.warnings.warningSignificantDifference, warnings);

    //check free disk space
    xmlAccess::addXmlElement("CheckForFreeDiskSpace", inputCfg.warnings.warningNotEnoughDiskSpace, warnings);

    //check for unresolved conflicts
    xmlAccess::addXmlElement("CheckForUnresolvedConflicts", inputCfg.warnings.warningUnresolvedConflicts, warnings);


    //###################################################################
    //write global gui settings
    TiXmlElement* gui = new TiXmlElement("Gui");
    root->LinkEndChild(gui);

    TiXmlElement* windows = new TiXmlElement("Windows");
    gui->LinkEndChild(windows);

    TiXmlElement* mainWindow = new TiXmlElement("Main");
    windows->LinkEndChild(mainWindow);

    //window size
    xmlAccess::addXmlElement("Width",  inputCfg.gui.widthNotMaximized,  mainWindow);
    xmlAccess::addXmlElement("Height", inputCfg.gui.heightNotMaximized, mainWindow);

    //window position
    xmlAccess::addXmlElement("PosX",      inputCfg.gui.posXNotMaximized, mainWindow);
    xmlAccess::addXmlElement("PosY",      inputCfg.gui.posYNotMaximized, mainWindow);
    xmlAccess::addXmlElement("Maximized", inputCfg.gui.isMaximized,      mainWindow);

    xmlAccess::addXmlElement("ManualDeletionOnBothSides", inputCfg.gui.deleteOnBothSides,            mainWindow);
    xmlAccess::addXmlElement("ManualDeletionUseRecycler", inputCfg.gui.useRecyclerForManualDeletion, mainWindow);
    xmlAccess::addXmlElement("ShowFileIconsLeft",         inputCfg.gui.showFileIconsLeft,            mainWindow);
    xmlAccess::addXmlElement("ShowFileIconsRight",        inputCfg.gui.showFileIconsRight,           mainWindow);
    xmlAccess::addXmlElement("PopupOnConfigChange",       inputCfg.gui.popupOnConfigChange,          mainWindow);
    xmlAccess::addXmlElement("SummaryBeforeSync",         inputCfg.gui.showSummaryBeforeSync,        mainWindow);

    //write column attributes
    TiXmlElement* leftColumn = new TiXmlElement("LeftColumns");
    mainWindow->LinkEndChild(leftColumn);

    xmlAccess::addXmlAttribute("AutoAdjust", inputCfg.gui.autoAdjustColumnsLeft, leftColumn);

    xmlAccess::ColumnAttributes columnAtrribLeftCopy = inputCfg.gui.columnAttribLeft; //can't change const vector
    sort(columnAtrribLeftCopy.begin(), columnAtrribLeftCopy.end(), xmlAccess::sortByPositionOnly);
    for (unsigned int i = 0; i < columnAtrribLeftCopy.size(); ++i)
    {
        TiXmlElement* subElement = new TiXmlElement("Column");
        leftColumn->LinkEndChild(subElement);

        const xmlAccess::ColumnAttrib& colAttrib = columnAtrribLeftCopy[i];
        addXmlAttribute("Type",    colAttrib.type,    subElement);
        xmlAccess::addXmlAttribute("Visible", colAttrib.visible, subElement);
        xmlAccess::addXmlAttribute("Width",   colAttrib.width,   subElement);
    }

    TiXmlElement* rightColumn = new TiXmlElement("RightColumns");
    mainWindow->LinkEndChild(rightColumn);

    xmlAccess::addXmlAttribute("AutoAdjust", inputCfg.gui.autoAdjustColumnsRight, rightColumn);

    xmlAccess::ColumnAttributes columnAtrribRightCopy = inputCfg.gui.columnAttribRight;
    sort(columnAtrribRightCopy.begin(), columnAtrribRightCopy.end(), xmlAccess::sortByPositionOnly);
    for (unsigned int i = 0; i < columnAtrribRightCopy.size(); ++i)
    {
        TiXmlElement* subElement = new TiXmlElement("Column");
        rightColumn->LinkEndChild(subElement);

        const xmlAccess::ColumnAttrib& colAttrib = columnAtrribRightCopy[i];
        addXmlAttribute("Type",    colAttrib.type,    subElement);
        xmlAccess::addXmlAttribute("Visible", colAttrib.visible, subElement);
        xmlAccess::addXmlAttribute("Width",   colAttrib.width,   subElement);
    }

    //write folder history elements
    TiXmlElement* historyLeft = new TiXmlElement("FolderHistoryLeft");
    mainWindow->LinkEndChild(historyLeft);
    TiXmlElement* historyRight = new TiXmlElement("FolderHistoryRight");
    mainWindow->LinkEndChild(historyRight);

    xmlAccess::addXmlAttribute("MaximumSize", inputCfg.gui.folderHistLeftMax,  historyLeft);
    xmlAccess::addXmlAttribute("MaximumSize", inputCfg.gui.folderHistRightMax, historyRight);

    xmlAccess::addXmlElement("Folder", inputCfg.gui.folderHistoryLeft, historyLeft);
    xmlAccess::addXmlElement("Folder", inputCfg.gui.folderHistoryRight, historyRight);

    xmlAccess::addXmlElement("SelectedTabBottomLeft", inputCfg.gui.selectedTabBottomLeft, mainWindow);

    //commandline for file manager integration
    xmlAccess::addXmlElement("FileManager", inputCfg.gui.commandLineFileManager, gui);

    //write config file history
    TiXmlElement* cfgHistory = new TiXmlElement("ConfigHistory");
    gui->LinkEndChild(cfgHistory);

    xmlAccess::addXmlAttribute("MaximumSize", inputCfg.gui.cfgHistoryMax, cfgHistory);
    xmlAccess::addXmlElement("File", inputCfg.gui.cfgFileHistory, cfgHistory);

    //###################################################################
    //write global batch settings

    TiXmlElement* batch = new TiXmlElement("Batch");
    root->LinkEndChild(batch);

    return true;
}


int xmlAccess::retrieveSystemLanguage()
{
    return wxLocale::GetSystemLanguage();
}


bool xmlAccess::recycleBinAvailable()
{
    return FreeFileSync::recycleBinExists();
}


void xmlAccess::WarningMessages::resetWarnings()
{
    warningDependentFolders        = true;
    warningSignificantDifference   = true;
    warningNotEnoughDiskSpace      = true;
    warningUnresolvedConflicts     = true;
}
