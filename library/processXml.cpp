// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "processXml.h"
#include "../shared/xmlBase.h"
#include <wx/intl.h>
#include <wx/filefn.h>
#include "../shared/globalFunctions.h"
#include "../shared/standardPaths.h"
#include "../shared/stringConv.h"

using namespace FreeFileSync;
using namespace xmlAccess; //functionally needed!!!


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
    //read alternate configuration (optional) => might point to NULL
    void readXmlLocalConfig(const TiXmlElement& folderPair, FolderPairEnh& enhPair);

    //read basic FreefileSync settings (used by commandline and GUI), return true if ALL values have been retrieved successfully
    void readXmlMainConfig(MainConfiguration& mainCfg);
};


//write gui settings
bool writeXmlGuiConfig(const xmlAccess::XmlGuiConfig& outputCfg, TiXmlDocument& doc);
//write batch settings
bool writeXmlBatchConfig(const xmlAccess::XmlBatchConfig& outputCfg, TiXmlDocument& doc);
//write global settings
bool writeXmlGlobalSettings(const xmlAccess::XmlGlobalSettings& outputCfg, TiXmlDocument& doc);
//write alternate configuration (optional) => might point to NULL
void writeXmlLocalConfig(const FolderPairEnh& enhPair, TiXmlElement& parent);
//write basic FreefileSync settings (used by commandline and GUI), return true if everything was written successfully
bool writeXmlMainConfig(const MainConfiguration& mainCfg, TiXmlDocument& doc);


void xmlAccess::readGuiConfig(const wxString& filename, xmlAccess::XmlGuiConfig& config)
{
    //load XML
    if (!wxFileExists(filename))
        throw XmlError(wxString(_("File does not exist:")) + wxT("\n\"") + filename + wxT("\""));

    TiXmlDocument doc;
    loadXmlDocument(filename, XML_GUI_CONFIG, doc); //throw (XmlError)

    FfsXmlParser parser(doc.RootElement());
    parser.readXmlGuiConfig(config); //read GUI layout configuration

    if (parser.errorsOccured())
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\"\n\n") +
                       parser.getErrorMessageFormatted(), XmlError::WARNING);
}


void xmlAccess::readBatchConfig(const wxString& filename, xmlAccess::XmlBatchConfig& config)
{
    //load XML
    if (!wxFileExists(filename))
        throw XmlError(wxString(_("File does not exist:")) + wxT("\n\"") + filename + wxT("\""));

    TiXmlDocument doc;
    loadXmlDocument(filename, XML_BATCH_CONFIG, doc); //throw (XmlError)

    FfsXmlParser parser(doc.RootElement());
    parser.readXmlBatchConfig(config); //read GUI layout configuration

    if (parser.errorsOccured())
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\"\n\n") +
                       parser.getErrorMessageFormatted(), XmlError::WARNING);
}


void xmlAccess::readGlobalSettings(xmlAccess::XmlGlobalSettings& config)
{
    //load XML
    if (!wxFileExists(getGlobalConfigFile()))
        throw XmlError(wxString(_("File does not exist:")) + wxT("\n\"") + getGlobalConfigFile() + wxT("\""));

    TiXmlDocument doc;
    loadXmlDocument(getGlobalConfigFile(), XML_GLOBAL_SETTINGS, doc); //throw (XmlError)

    FfsXmlParser parser(doc.RootElement());
    parser.readXmlGlobalSettings(config); //read GUI layout configuration

    if (parser.errorsOccured())
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + getGlobalConfigFile() + wxT("\"\n\n") +
                       parser.getErrorMessageFormatted(), XmlError::WARNING);
}


void xmlAccess::writeGuiConfig(const XmlGuiConfig& outputCfg, const wxString& filename)
{
    TiXmlDocument doc;
    getDefaultXmlDocument(XML_GUI_CONFIG, doc);

    //populate and write XML tree
    if (!writeXmlGuiConfig(outputCfg, doc)) //add GUI layout configuration settings
        throw XmlError(wxString(_("Error writing file:")) + wxT("\n\"") + filename + wxT("\""));

    saveXmlDocument(filename, doc); //throw (XmlError)
}


void xmlAccess::writeBatchConfig(const XmlBatchConfig& outputCfg, const wxString& filename)
{
    TiXmlDocument doc;
    getDefaultXmlDocument(XML_BATCH_CONFIG, doc);

    //populate and write XML tree
    if (!writeXmlBatchConfig(outputCfg, doc)) //add batch configuration settings
        throw XmlError(wxString(_("Error writing file:")) + wxT("\n\"") + filename + wxT("\""));

    saveXmlDocument(filename, doc); //throw (XmlError)
}


void xmlAccess::writeGlobalSettings(const XmlGlobalSettings& outputCfg)
{
    TiXmlDocument doc;
    getDefaultXmlDocument(XML_GLOBAL_SETTINGS, doc);

    //populate and write XML tree
    if (!writeXmlGlobalSettings(outputCfg, doc)) //add GUI layout configuration settings
        throw XmlError(wxString(_("Error writing file:")) + wxT("\n\"") + getGlobalConfigFile() + wxT("\""));

    saveXmlDocument(getGlobalConfigFile(), doc); //throw (XmlError)
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


bool readXmlElement(const std::string& name, const TiXmlElement* parent, SyncDirection& output)
{
    std::string dummy;
    if (xmlAccess::readXmlElement(name, parent, dummy))
    {
        if (dummy == "left")
            output = SYNC_DIR_LEFT;
        else if (dummy == "right")
            output = SYNC_DIR_RIGHT;
        else //treat all other input as "none"
            output = SYNC_DIR_NONE;

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


bool readXmlElement(const std::string& name, const TiXmlElement* parent , FreeFileSync::SymLinkHandling& output)
{
    std::string dummy;
    if (xmlAccess::readXmlElement(name, parent, dummy))
    {
        if (dummy == "Ignore")
            output = FreeFileSync::SYMLINK_IGNORE;
        else if (dummy == "UseDirectly")
            output = FreeFileSync::SYMLINK_USE_DIRECTLY;
        else if (dummy == "FollowLink")
            output = FreeFileSync::SYMLINK_FOLLOW_LINK;
        else
            return false;

        return true;
    }
    return false;
}


bool readXmlElement(const std::string& name, const TiXmlElement* parent, Zstring& output)
{
    wxString dummy;
    if (!xmlAccess::readXmlElement(name, parent, dummy))
        return false;

    output = wxToZ(dummy);
    return true;
}


bool readXmlAttribute(const std::string& name, const TiXmlElement* node, xmlAccess::ColumnTypes& output)
{
    int dummy = 0;
    if (xmlAccess::readXmlAttribute(name, node, dummy))
    {
        output = static_cast<xmlAccess::ColumnTypes>(dummy);
        return true;
    }
    else
        return false;
}


//################################################################################################################
void FfsXmlParser::readXmlLocalConfig(const TiXmlElement& folderPair, FolderPairEnh& enhPair)
{
    //read folder pairs
    readXmlElementLogging("Left",  &folderPair, enhPair.leftDirectory);
    readXmlElementLogging("Right", &folderPair, enhPair.rightDirectory);


//###########################################################
    //alternate sync configuration
    const TiXmlElement* altSyncConfig = TiXmlHandleConst(&folderPair).FirstChild("AlternateSyncConfig").ToElement();
    if (altSyncConfig)
    {
        AlternateSyncConfig* altSyncCfg = new AlternateSyncConfig;
        enhPair.altSyncConfig.reset(altSyncCfg);

        const TiXmlElement* syncCfg = TiXmlHandleConst(altSyncConfig).FirstChild("Synchronization").ToElement();
        const TiXmlElement* syncDirections = TiXmlHandleConst(syncCfg).FirstChild("Directions").ToElement();

        //read sync configuration
        readXmlElementLogging("Automatic",  syncCfg,        altSyncCfg->syncConfiguration.automatic);
        readXmlElementLogging("LeftOnly",   syncDirections, altSyncCfg->syncConfiguration.exLeftSideOnly);
        readXmlElementLogging("RightOnly",  syncDirections, altSyncCfg->syncConfiguration.exRightSideOnly);
        readXmlElementLogging("LeftNewer",  syncDirections, altSyncCfg->syncConfiguration.leftNewer);
        readXmlElementLogging("RightNewer", syncDirections, altSyncCfg->syncConfiguration.rightNewer);
        readXmlElementLogging("Different",  syncDirections, altSyncCfg->syncConfiguration.different);
        readXmlElementLogging("Conflict",   syncDirections, altSyncCfg->syncConfiguration.conflict);

        const TiXmlElement* miscSettings = TiXmlHandleConst(&folderPair).FirstChild("AlternateSyncConfig").FirstChild("Miscellaneous").ToElement();
        readXmlElementLogging("DeletionPolicy", miscSettings, altSyncCfg->handleDeletion);
        readXmlElementLogging("CustomDeletionFolder", miscSettings, altSyncCfg->customDeletionDirectory);
    }

//###########################################################
    //alternate filter configuration
    const TiXmlElement* filterCfg = TiXmlHandleConst(&folderPair).FirstChild("LocalFilter").ToElement();
    if (filterCfg)
    {
        //read filter settings
        readXmlElementLogging("Include", filterCfg, enhPair.localFilter.includeFilter);
        readXmlElementLogging("Exclude", filterCfg, enhPair.localFilter.excludeFilter);
    }
}


void FfsXmlParser::readXmlMainConfig(MainConfiguration& mainCfg)
{
    TiXmlHandleConst hRoot(getRoot()); //custom const handle: TiXml API seems broken in this regard

//###########################################################
    const TiXmlElement* cmpSettings  = hRoot.FirstChild("MainConfig").FirstChild("Comparison").ToElement();

    //read compare variant
    readXmlElementLogging("Variant", cmpSettings, mainCfg.compareVar);

    //max. allowed file time deviation
    readXmlElementLogging("FileTimeTolerance", cmpSettings, mainCfg.hidden.fileTimeTolerance);

    //include symbolic links at all?
    readXmlElementLogging("HandleSymlinks", cmpSettings, mainCfg.handleSymlinks);

//###########################################################
    const TiXmlElement* syncCfg = hRoot.FirstChild("MainConfig").FirstChild("Synchronization").ToElement();
    const TiXmlElement* syncDirections = TiXmlHandleConst(syncCfg).FirstChild("Directions").ToElement();

    //read sync configuration
    readXmlElementLogging("Automatic",  syncCfg,        mainCfg.syncConfiguration.automatic);
    readXmlElementLogging("LeftOnly",   syncDirections, mainCfg.syncConfiguration.exLeftSideOnly);
    readXmlElementLogging("RightOnly",  syncDirections, mainCfg.syncConfiguration.exRightSideOnly);
    readXmlElementLogging("LeftNewer",  syncDirections, mainCfg.syncConfiguration.leftNewer);
    readXmlElementLogging("RightNewer", syncDirections, mainCfg.syncConfiguration.rightNewer);
    readXmlElementLogging("Different",  syncDirections, mainCfg.syncConfiguration.different);
    readXmlElementLogging("Conflict",   syncDirections, mainCfg.syncConfiguration.conflict);

//###########################################################
    const TiXmlElement* syncConfig = hRoot.FirstChild("MainConfig").FirstChild("Synchronization").ToElement();

    //verify file copying
    readXmlElementLogging("VerifyCopiedFiles", syncConfig, mainCfg.hidden.verifyFileCopy);

//###########################################################
    const TiXmlElement* miscSettings = hRoot.FirstChild("MainConfig").FirstChild("Miscellaneous").ToElement();

    //misc
    readXmlElementLogging("DeletionPolicy", miscSettings, mainCfg.handleDeletion);
    readXmlElementLogging("CustomDeletionFolder", miscSettings, mainCfg.customDeletionDirectory);
//###########################################################
    const TiXmlElement* filter = TiXmlHandleConst(miscSettings).FirstChild("Filter").ToElement();

    //read filter settings
    Zstring includeFilter;
    Zstring excludeFilter;
    readXmlElementLogging("Include", filter, includeFilter);
    readXmlElementLogging("Exclude", filter, excludeFilter);

    mainCfg.globalFilter = FilterConfig(includeFilter, excludeFilter);

//###########################################################
    const TiXmlElement* pairs = hRoot.FirstChild("MainConfig").FirstChild("FolderPairs").FirstChild("Pair").ToElement();

    //read all folder pairs
    mainCfg.additionalPairs.clear();
    bool firstLoop = true;
    while (pairs)
    {
        FolderPairEnh newPair;
        readXmlLocalConfig(*pairs, newPair);

        if (firstLoop)   //read first folder pair
        {
            firstLoop = false;
            mainCfg.firstPair = newPair;
        }
        else  //read additional folder pairs
            mainCfg.additionalPairs.push_back(newPair);

        pairs = pairs->NextSiblingElement();
    }
}


void FfsXmlParser::readXmlGuiConfig(xmlAccess::XmlGuiConfig& outputCfg)
{
    //read main config
    readXmlMainConfig(outputCfg.mainCfg);

    //read GUI specific config data
    const TiXmlElement* guiConfig = TiXmlHandleConst(getRoot()).FirstChild("GuiConfig").ToElement();

    readXmlElementLogging("HideFiltered", guiConfig, outputCfg.hideFilteredElements);

    xmlAccess::OnError errorHand = ON_ERROR_POPUP;
    readXmlElementLogging("HandleError", guiConfig, errorHand);
    outputCfg.ignoreErrors = errorHand == xmlAccess::ON_ERROR_IGNORE;

    readXmlElementLogging("SyncPreviewActive", guiConfig, outputCfg.syncPreviewEnabled);
}


void FfsXmlParser::readXmlBatchConfig(xmlAccess::XmlBatchConfig& outputCfg)
{
    //read main config
    readXmlMainConfig(outputCfg.mainCfg);

    //read batch specific config
    const TiXmlElement* batchConfig = TiXmlHandleConst(getRoot()).FirstChild("BatchConfig").ToElement();

    readXmlElementLogging("Silent", batchConfig, outputCfg.silent);
    readXmlElementLogging("LogfileDirectory", batchConfig, outputCfg.logFileDirectory);
    readXmlElementLogging("HandleError", batchConfig, outputCfg.handleError);
}


void FfsXmlParser::readXmlGlobalSettings(xmlAccess::XmlGlobalSettings& outputCfg)
{
    //read global settings
    const TiXmlElement* global = TiXmlHandleConst(getRoot()).FirstChild("Shared").ToElement();

    //try to read program language setting
    readXmlElementLogging("Language", global, outputCfg.programLanguage);

    //ignore +/- 1 hour due to DST change
    readXmlElementLogging("IgnoreOneHourDifference", global, outputCfg.ignoreOneHourDiff);

    //copy locked files using VSS
    readXmlElementLogging("CopyLockedFiles", global, outputCfg.copyLockedFiles);


    const TiXmlElement* optionalDialogs = TiXmlHandleConst(getRoot()).FirstChild("Shared").FirstChild("ShowOptionalDialogs").ToElement();

    //folder dependency check
    readXmlElementLogging("CheckForDependentFolders", optionalDialogs, outputCfg.optDialogs.warningDependentFolders);
    //significant difference check
    readXmlElementLogging("CheckForSignificantDifference", optionalDialogs, outputCfg.optDialogs.warningSignificantDifference);
    //check free disk space
    readXmlElementLogging("CheckForFreeDiskSpace", optionalDialogs, outputCfg.optDialogs.warningNotEnoughDiskSpace);
    //check for unresolved conflicts
    readXmlElementLogging("CheckForUnresolvedConflicts", optionalDialogs, outputCfg.optDialogs.warningUnresolvedConflicts);

    readXmlElementLogging("NotifyDatabaseError", optionalDialogs, outputCfg.optDialogs.warningSyncDatabase);

    readXmlElementLogging("PopupOnConfigChange",       optionalDialogs, outputCfg.optDialogs.popupOnConfigChange);

    readXmlElementLogging("SummaryBeforeSync",         optionalDialogs, outputCfg.optDialogs.showSummaryBeforeSync);


    //gui specific global settings (optional)
    const TiXmlElement* gui = TiXmlHandleConst(getRoot()).FirstChild("Gui").ToElement();
    const TiXmlElement* mainWindow = TiXmlHandleConst(gui).FirstChild("Windows").FirstChild("Main").ToElement();

    //read application window size and position
    readXmlElementLogging("Width",     mainWindow, outputCfg.gui.widthNotMaximized);
    readXmlElementLogging("Height",    mainWindow, outputCfg.gui.heightNotMaximized);
    readXmlElementLogging("PosX",      mainWindow, outputCfg.gui.posXNotMaximized);
    readXmlElementLogging("PosY",      mainWindow, outputCfg.gui.posYNotMaximized);
    readXmlElementLogging("Maximized", mainWindow, outputCfg.gui.isMaximized);

    readXmlElementLogging("ManualDeletionOnBothSides", mainWindow, outputCfg.gui.deleteOnBothSides);
    readXmlElementLogging("ManualDeletionUseRecycler", mainWindow, outputCfg.gui.useRecyclerForManualDeletion);

    readXmlElementLogging("RespectCaseOnSearch", mainWindow, outputCfg.gui.textSearchRespectCase);

//###########################################################
    //read column attributes
    readXmlAttributeLogging("AutoAdjust", TiXmlHandleConst(mainWindow).FirstChild("LeftColumns").ToElement(), outputCfg.gui.autoAdjustColumnsLeft);
    readXmlAttributeLogging("ShowFileIcons", TiXmlHandleConst(mainWindow).FirstChild("LeftColumns").ToElement(), outputCfg.gui.showFileIconsLeft);

    const TiXmlElement* leftColumn = TiXmlHandleConst(mainWindow).FirstChild("LeftColumns").FirstChild("Column").ToElement();
    size_t colPos = 0;
    while (leftColumn)
    {
        ColumnAttrib newAttrib;
        newAttrib.position = colPos++;
        readXmlAttributeLogging("Type",    leftColumn, newAttrib.type);
        readXmlAttributeLogging("Visible", leftColumn, newAttrib.visible);
        readXmlAttributeLogging("Width",   leftColumn, newAttrib.width);
        outputCfg.gui.columnAttribLeft.push_back(newAttrib);

        leftColumn = leftColumn->NextSiblingElement();
    }

    readXmlAttributeLogging("AutoAdjust", TiXmlHandleConst(mainWindow).FirstChild("RightColumns").ToElement(), outputCfg.gui.autoAdjustColumnsRight);
    readXmlAttributeLogging("ShowFileIcons", TiXmlHandleConst(mainWindow).FirstChild("RightColumns").ToElement(), outputCfg.gui.showFileIconsRight);

    const TiXmlElement* rightColumn = TiXmlHandleConst(mainWindow).FirstChild("RightColumns").FirstChild("Column").ToElement();
    colPos = 0;
    while (rightColumn)
    {
        ColumnAttrib newAttrib;
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


    //external applications
    const TiXmlElement* extApps = TiXmlHandleConst(gui).FirstChild("ExternalApplications").FirstChild("Commandline").ToElement();
    if (extApps)
    {
        outputCfg.gui.externelApplications.clear();
        while (extApps)
        {
            wxString description;
            wxString cmdLine;

            readXmlAttributeLogging("Description", extApps, description);
            const char* text = extApps->GetText();
            if (text) cmdLine = wxString::FromUTF8(text); //read commandline
            outputCfg.gui.externelApplications.push_back(std::make_pair(description, cmdLine));

            extApps = extApps->NextSiblingElement();
        }
    }
    //load config file history
    const TiXmlElement* cfgHistory = TiXmlHandleConst(gui).FirstChild("ConfigHistory").ToElement();

    //load max. history size
    readXmlAttributeLogging("MaximumSize", cfgHistory, outputCfg.gui.cfgHistoryMax);

    //load config history elements
    readXmlElementLogging("File", cfgHistory, outputCfg.gui.cfgFileHistory);

    //last update check
    readXmlElementLogging("LastUpdateCheck", gui, outputCfg.gui.lastUpdateCheck);


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


void addXmlElement(const std::string& name, const SyncDirection value, TiXmlElement* parent)
{
    switch (value)
    {
    case SYNC_DIR_LEFT:
        xmlAccess::addXmlElement(name, std::string("left"), parent);
        break;
    case SYNC_DIR_RIGHT:
        xmlAccess::addXmlElement(name, std::string("right"), parent);
        break;
    case SYNC_DIR_NONE:
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


void addXmlElement(const std::string& name, const FreeFileSync::SymLinkHandling value, TiXmlElement* parent)
{
    switch (value)
    {
    case FreeFileSync::SYMLINK_IGNORE:
        xmlAccess::addXmlElement(name, std::string("Ignore"), parent);
        break;
    case FreeFileSync::SYMLINK_USE_DIRECTLY:
        xmlAccess::addXmlElement(name, std::string("UseDirectly"), parent);
        break;
    case FreeFileSync::SYMLINK_FOLLOW_LINK:
        xmlAccess::addXmlElement(name, std::string("FollowLink"), parent);
        break;
    }
}


void addXmlElement(const std::string& name, const Zstring& value, TiXmlElement* parent)
{
    xmlAccess::addXmlElement(name, wxString(zToWx(value)), parent);
}


void addXmlAttribute(const std::string& name, const xmlAccess::ColumnTypes value, TiXmlElement* node)
{
    xmlAccess::addXmlAttribute(name, static_cast<int>(value), node);
}


void writeXmlLocalConfig(const FolderPairEnh& enhPair, TiXmlElement& parent)
{
    //write folder pairs
    TiXmlElement* newfolderPair = new TiXmlElement("Pair");
    parent.LinkEndChild(newfolderPair);

    addXmlElement("Left",  enhPair.leftDirectory,  newfolderPair);
    addXmlElement("Right", enhPair.rightDirectory, newfolderPair);


    //alternate sync configuration
    const AlternateSyncConfig* altSyncConfig = enhPair.altSyncConfig.get();
    if (altSyncConfig)
    {
        TiXmlElement* syncCfg = new TiXmlElement("AlternateSyncConfig");
        newfolderPair->LinkEndChild(syncCfg);

        TiXmlElement* syncSettings = new TiXmlElement("Synchronization");
        syncCfg->LinkEndChild(syncSettings);

        //write sync configuration
        addXmlElement("Automatic",   altSyncConfig->syncConfiguration.automatic, syncSettings);

        TiXmlElement* syncDirections = new TiXmlElement("Directions");
        syncSettings->LinkEndChild(syncDirections);

        addXmlElement("LeftOnly",   altSyncConfig->syncConfiguration.exLeftSideOnly,  syncDirections);
        addXmlElement("RightOnly",  altSyncConfig->syncConfiguration.exRightSideOnly, syncDirections);
        addXmlElement("LeftNewer",  altSyncConfig->syncConfiguration.leftNewer,       syncDirections);
        addXmlElement("RightNewer", altSyncConfig->syncConfiguration.rightNewer,      syncDirections);
        addXmlElement("Different",  altSyncConfig->syncConfiguration.different,       syncDirections);
        addXmlElement("Conflict",   altSyncConfig->syncConfiguration.conflict,        syncDirections);


        TiXmlElement* miscSettings = new TiXmlElement("Miscellaneous");
        syncCfg->LinkEndChild(miscSettings);

        //misc
        addXmlElement("DeletionPolicy", altSyncConfig->handleDeletion, miscSettings);
        addXmlElement("CustomDeletionFolder", altSyncConfig->customDeletionDirectory, miscSettings);
    }

//###########################################################
    //alternate filter configuration
    TiXmlElement* filterCfg = new TiXmlElement("LocalFilter");
    newfolderPair->LinkEndChild(filterCfg);

    //write filter settings
    addXmlElement("Include", enhPair.localFilter.includeFilter, filterCfg);
    addXmlElement("Exclude", enhPair.localFilter.excludeFilter, filterCfg);
}


bool writeXmlMainConfig(const MainConfiguration& mainCfg, TiXmlDocument& doc)
{
    //reset hidden settings: we don't want implicit behaviour! Hidden settings are loaded but changes are not saved!
    MainConfiguration mainCfgLocal = mainCfg;
    mainCfgLocal.hidden = HiddenSettings();


    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlElement* settings = new TiXmlElement("MainConfig");
    root->LinkEndChild(settings);

//###########################################################
    TiXmlElement* cmpSettings = new TiXmlElement("Comparison");
    settings->LinkEndChild(cmpSettings);

    //write compare algorithm
    addXmlElement("Variant", mainCfgLocal.compareVar, cmpSettings);

    //max. allowed file time deviation
    addXmlElement("FileTimeTolerance", mainCfgLocal.hidden.fileTimeTolerance, cmpSettings);

    //include symbolic links at all?
    addXmlElement("HandleSymlinks", mainCfgLocal.handleSymlinks, cmpSettings);

//###########################################################
    TiXmlElement* syncSettings = new TiXmlElement("Synchronization");
    settings->LinkEndChild(syncSettings);

    //write sync configuration
    addXmlElement("Automatic", mainCfgLocal.syncConfiguration.automatic, syncSettings);

    TiXmlElement* syncDirections = new TiXmlElement("Directions");
    syncSettings->LinkEndChild(syncDirections);

    addXmlElement("LeftOnly",   mainCfgLocal.syncConfiguration.exLeftSideOnly,  syncDirections);
    addXmlElement("RightOnly",  mainCfgLocal.syncConfiguration.exRightSideOnly, syncDirections);
    addXmlElement("LeftNewer",  mainCfgLocal.syncConfiguration.leftNewer,       syncDirections);
    addXmlElement("RightNewer", mainCfgLocal.syncConfiguration.rightNewer,      syncDirections);
    addXmlElement("Different",  mainCfgLocal.syncConfiguration.different,       syncDirections);
    addXmlElement("Conflict",   mainCfgLocal.syncConfiguration.conflict,        syncDirections);

//###########################################################
    //verify file copying
    addXmlElement("VerifyCopiedFiles", mainCfgLocal.hidden.verifyFileCopy, syncSettings);

//###########################################################
    TiXmlElement* miscSettings = new TiXmlElement("Miscellaneous");
    settings->LinkEndChild(miscSettings);

    //write filter settings
    TiXmlElement* filter = new TiXmlElement("Filter");
    miscSettings->LinkEndChild(filter);

    addXmlElement("Include", mainCfgLocal.globalFilter.includeFilter, filter);
    addXmlElement("Exclude", mainCfgLocal.globalFilter.excludeFilter, filter);

    //other
    addXmlElement("DeletionPolicy", mainCfgLocal.handleDeletion, miscSettings);
    addXmlElement("CustomDeletionFolder", mainCfgLocal.customDeletionDirectory, miscSettings);

//###########################################################
    TiXmlElement* pairs = new TiXmlElement("FolderPairs");
    settings->LinkEndChild(pairs);

    //write first folder pair
    writeXmlLocalConfig(mainCfgLocal.firstPair, *pairs);

    //write additional folder pairs
    for (std::vector<FolderPairEnh>::const_iterator i = mainCfgLocal.additionalPairs.begin(); i != mainCfgLocal.additionalPairs.end(); ++i)
        writeXmlLocalConfig(*i, *pairs);

    return true;
}


bool writeXmlGuiConfig(const xmlAccess::XmlGuiConfig& inputCfg, TiXmlDocument& doc)
{
    //write main config
    if (!writeXmlMainConfig(inputCfg.mainCfg, doc))
        return false;

    //write GUI specific config
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlElement* guiConfig = new TiXmlElement("GuiConfig");
    root->LinkEndChild(guiConfig);

    addXmlElement("HideFiltered",      inputCfg.hideFilteredElements, guiConfig);

    addXmlElement("HandleError", inputCfg.ignoreErrors ? xmlAccess::ON_ERROR_IGNORE : xmlAccess::ON_ERROR_POPUP, guiConfig);

    addXmlElement("SyncPreviewActive", inputCfg.syncPreviewEnabled,   guiConfig);

    return true;
}


bool writeXmlBatchConfig(const xmlAccess::XmlBatchConfig& inputCfg, TiXmlDocument& doc)
{
    //write main config
    if (!writeXmlMainConfig(inputCfg.mainCfg, doc))
        return false;

    //write GUI specific config
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlElement* batchConfig = new TiXmlElement("BatchConfig");
    root->LinkEndChild(batchConfig);

    addXmlElement("Silent", inputCfg.silent, batchConfig);
    addXmlElement("LogfileDirectory", inputCfg.logFileDirectory, batchConfig);
    addXmlElement("HandleError", inputCfg.handleError, batchConfig);

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
    addXmlElement("Language", inputCfg.programLanguage, global);

    //ignore +/- 1 hour due to DST change
    addXmlElement("IgnoreOneHourDifference", inputCfg.ignoreOneHourDiff, global);

    //copy locked files using VSS
    addXmlElement("CopyLockedFiles", inputCfg.copyLockedFiles, global);


    //optional dialogs
    TiXmlElement* optionalDialogs = new TiXmlElement("ShowOptionalDialogs");
    global->LinkEndChild(optionalDialogs);

    //warning: dependent folders
    addXmlElement("CheckForDependentFolders", inputCfg.optDialogs.warningDependentFolders, optionalDialogs);

    //significant difference check
    addXmlElement("CheckForSignificantDifference", inputCfg.optDialogs.warningSignificantDifference, optionalDialogs);

    //check free disk space
    addXmlElement("CheckForFreeDiskSpace", inputCfg.optDialogs.warningNotEnoughDiskSpace, optionalDialogs);

    //check for unresolved conflicts
    addXmlElement("CheckForUnresolvedConflicts", inputCfg.optDialogs.warningUnresolvedConflicts, optionalDialogs);

    addXmlElement("NotifyDatabaseError", inputCfg.optDialogs.warningSyncDatabase, optionalDialogs);

    addXmlElement("PopupOnConfigChange", inputCfg.optDialogs.popupOnConfigChange, optionalDialogs);

    addXmlElement("SummaryBeforeSync", inputCfg.optDialogs.showSummaryBeforeSync, optionalDialogs);


    //###################################################################
    //write global gui settings
    TiXmlElement* gui = new TiXmlElement("Gui");
    root->LinkEndChild(gui);

    TiXmlElement* windows = new TiXmlElement("Windows");
    gui->LinkEndChild(windows);

    TiXmlElement* mainWindow = new TiXmlElement("Main");
    windows->LinkEndChild(mainWindow);

    //window size
    addXmlElement("Width",  inputCfg.gui.widthNotMaximized,  mainWindow);
    addXmlElement("Height", inputCfg.gui.heightNotMaximized, mainWindow);

    //window position
    addXmlElement("PosX",      inputCfg.gui.posXNotMaximized, mainWindow);
    addXmlElement("PosY",      inputCfg.gui.posYNotMaximized, mainWindow);
    addXmlElement("Maximized", inputCfg.gui.isMaximized,      mainWindow);

    addXmlElement("ManualDeletionOnBothSides", inputCfg.gui.deleteOnBothSides,            mainWindow);
    addXmlElement("ManualDeletionUseRecycler", inputCfg.gui.useRecyclerForManualDeletion, mainWindow);

    addXmlElement("RespectCaseOnSearch", inputCfg.gui.textSearchRespectCase, mainWindow);

    //write column attributes
    TiXmlElement* leftColumn = new TiXmlElement("LeftColumns");
    mainWindow->LinkEndChild(leftColumn);

    addXmlAttribute("AutoAdjust", inputCfg.gui.autoAdjustColumnsLeft, leftColumn);
    addXmlAttribute("ShowFileIcons", inputCfg.gui.showFileIconsLeft, leftColumn);

    ColumnAttributes columnAtrribLeftCopy = inputCfg.gui.columnAttribLeft; //can't change const vector
    sort(columnAtrribLeftCopy.begin(), columnAtrribLeftCopy.end(), xmlAccess::sortByPositionOnly);
    for (size_t i = 0; i < columnAtrribLeftCopy.size(); ++i)
    {
        TiXmlElement* subElement = new TiXmlElement("Column");
        leftColumn->LinkEndChild(subElement);

        const ColumnAttrib& colAttrib = columnAtrribLeftCopy[i];
        addXmlAttribute("Type",    colAttrib.type,    subElement);
        addXmlAttribute("Visible", colAttrib.visible, subElement);
        addXmlAttribute("Width",   colAttrib.width,   subElement);
    }

    TiXmlElement* rightColumn = new TiXmlElement("RightColumns");
    mainWindow->LinkEndChild(rightColumn);

    addXmlAttribute("AutoAdjust", inputCfg.gui.autoAdjustColumnsRight, rightColumn);
    addXmlAttribute("ShowFileIcons", inputCfg.gui.showFileIconsRight, rightColumn);

    ColumnAttributes columnAtrribRightCopy = inputCfg.gui.columnAttribRight;
    sort(columnAtrribRightCopy.begin(), columnAtrribRightCopy.end(), xmlAccess::sortByPositionOnly);
    for (size_t i = 0; i < columnAtrribRightCopy.size(); ++i)
    {
        TiXmlElement* subElement = new TiXmlElement("Column");
        rightColumn->LinkEndChild(subElement);

        const ColumnAttrib& colAttrib = columnAtrribRightCopy[i];
        addXmlAttribute("Type",    colAttrib.type,    subElement);
        addXmlAttribute("Visible", colAttrib.visible, subElement);
        addXmlAttribute("Width",   colAttrib.width,   subElement);
    }

    //write folder history elements
    TiXmlElement* historyLeft = new TiXmlElement("FolderHistoryLeft");
    mainWindow->LinkEndChild(historyLeft);
    TiXmlElement* historyRight = new TiXmlElement("FolderHistoryRight");
    mainWindow->LinkEndChild(historyRight);

    addXmlAttribute("MaximumSize", inputCfg.gui.folderHistLeftMax,  historyLeft);
    addXmlAttribute("MaximumSize", inputCfg.gui.folderHistRightMax, historyRight);

    addXmlElement("Folder", inputCfg.gui.folderHistoryLeft, historyLeft);
    addXmlElement("Folder", inputCfg.gui.folderHistoryRight, historyRight);

    addXmlElement("SelectedTabBottomLeft", inputCfg.gui.selectedTabBottomLeft, mainWindow);

    //external applications
    TiXmlElement* extApp = new TiXmlElement("ExternalApplications");
    gui->LinkEndChild(extApp);

    for (ExternalApps::const_iterator i = inputCfg.gui.externelApplications.begin(); i != inputCfg.gui.externelApplications.end(); ++i)
    {
        TiXmlElement* newEntry = new TiXmlElement("Commandline");
        extApp->LinkEndChild(newEntry);

        addXmlAttribute("Description", i->first,  newEntry);
        newEntry->LinkEndChild(new TiXmlText(i->second.ToUTF8())); //commandline
    }

    //write config file history
    TiXmlElement* cfgHistory = new TiXmlElement("ConfigHistory");
    gui->LinkEndChild(cfgHistory);

    addXmlAttribute("MaximumSize", inputCfg.gui.cfgHistoryMax, cfgHistory);
    addXmlElement("File", inputCfg.gui.cfgFileHistory, cfgHistory);


    //last update check
    addXmlElement("LastUpdateCheck", inputCfg.gui.lastUpdateCheck, gui);

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


void xmlAccess::OptionalDialogs::resetDialogs()
{
    warningDependentFolders        = true;
    warningSignificantDifference   = true;
    warningNotEnoughDiskSpace      = true;
    warningUnresolvedConflicts     = true;
    warningSyncDatabase            = true;
    popupOnConfigChange            = true;
    showSummaryBeforeSync          = true;
}


xmlAccess::XmlGuiConfig xmlAccess::convertBatchToGui(const xmlAccess::XmlBatchConfig& batchCfg)
{
    xmlAccess::XmlGuiConfig output;
    output.mainCfg = batchCfg.mainCfg;
    return output;
}


void xmlAccess::readGuiOrBatchConfig(const wxString& filename, XmlGuiConfig& config)  //throw (xmlAccess::XmlError);
{
    if (xmlAccess::getXmlType(filename) != xmlAccess::XML_BATCH_CONFIG)
    {
        xmlAccess::readGuiConfig(filename, config);
        return;
    }

    //convert batch config to gui config
    xmlAccess::XmlBatchConfig batchCfg;
    try
    {
        xmlAccess::readBatchConfig(filename, batchCfg); //throw (xmlAccess::XmlError);
    }
    catch (const xmlAccess::XmlError& e)
    {
        if (e.getSeverity() != xmlAccess::XmlError::WARNING)
            throw;

        config = convertBatchToGui(batchCfg); //do work despite parsing errors, then re-throw
        throw;                                //
    }

    config = convertBatchToGui(batchCfg);
}


wxString xmlAccess::getGlobalConfigFile()
{
    return FreeFileSync::getConfigDir() + wxT("GlobalSettings.xml");
}
