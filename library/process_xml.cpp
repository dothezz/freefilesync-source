// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "process_xml.h"
#include "../shared/i18n.h"
#include "../shared/global_func.h"
#include "../shared/standard_paths.h"
#include "../shared/string_conv.h"
#include "../shared/file_handling.h"

using namespace zen;
using namespace xmlAccess; //functionally needed!!!


XmlType xmlAccess::getXmlType(const wxString& filename) //throw()
{
    try
    {
        TiXmlDocument doc;
        loadXmlDocument(filename, doc); //throw (XmlError)
        return getXmlType(doc);
    }
    catch (const XmlError&) {}

    return XML_TYPE_OTHER;
}


XmlType xmlAccess::getXmlType(const TiXmlDocument& doc) //throw()
{
    const TiXmlElement* root = doc.RootElement();
    if (root && root->ValueStr() == std::string("FreeFileSync"))
    {
        const char* cfgType = root->Attribute("XmlType");
        if (cfgType)
        {
            const std::string type(cfgType);

            if (type == "GUI")
                return XML_TYPE_GUI;
            else if (type == "BATCH")
                return XML_TYPE_BATCH;
            else if (type == "GLOBAL")
                return XML_TYPE_GLOBAL;
        }
    }
    return XML_TYPE_OTHER;
}


void xmlAccess::initXmlDocument(TiXmlDocument& doc, XmlType type) //throw()
{
    TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "UTF-8", ""); //delete won't be necessary later; ownership passed to TiXmlDocument!
    doc.LinkEndChild(decl);

    TiXmlElement* root = new TiXmlElement("FreeFileSync");
    doc.LinkEndChild(root);

    switch (type)
    {
        case XML_TYPE_GUI:
            addXmlAttribute("XmlType", "GUI", doc.RootElement());
            break;
        case XML_TYPE_BATCH:
            addXmlAttribute("XmlType", "BATCH", doc.RootElement());
            break;
        case XML_TYPE_GLOBAL:
            addXmlAttribute("XmlType", "GLOBAL", doc.RootElement());
            break;
        case XML_TYPE_OTHER:
            break;
    }
}


class FfsXmlErrorLogger : public xmlAccess::XmlErrorLogger
{
public:
    //read gui settings, all values retrieved are optional, so check for initial values! (== -1)
    void readConfig(const TiXmlElement* root, xmlAccess::XmlGuiConfig& outputCfg);
    //read batch settings, all values retrieved are optional
    void readConfig(const TiXmlElement* root, xmlAccess::XmlBatchConfig& outputCfg);
    //read global settings, valid for both GUI and batch mode, independent from configuration
    void readConfig(const TiXmlElement* root, xmlAccess::XmlGlobalSettings& outputCfg);

private:
    //read alternate configuration (optional) => might point to NULL
    void readConfig(const TiXmlElement& folderPair, FolderPairEnh& enhPair);
    void readFilter(const TiXmlElement& parent, FilterConfig& output);

    //read basic FreefileSync settings (used by commandline and GUI)
    void readConfig(const TiXmlElement* root, MainConfiguration& mainCfg);
};



bool readXmlElement(const std::string& name, const TiXmlElement* parent, CompareVariant& output)
{
    std::string dummy;
    if (xmlAccess::readXmlElement(name, parent, dummy))
    {
        if (dummy == "ByTimeAndSize")
            output = zen::CMP_BY_TIME_SIZE;
        else if (dummy == "ByContent")
            output = zen::CMP_BY_CONTENT;
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


bool readXmlElement(const std::string& name, const TiXmlElement* parent , OnGuiError& output)
{
    std::string dummy;
    if (xmlAccess::readXmlElement(name, parent, dummy))
    {
        if (dummy == "Popup")
            output = ON_GUIERROR_POPUP;
        else if (dummy == "Ignore")
            output = ON_GUIERROR_IGNORE;
        else
            return false;

        return true;
    }
    return false;
}


bool readXmlElement(const std::string& name, const TiXmlElement* parent , zen::DeletionPolicy& output)
{
    std::string dummy;
    if (xmlAccess::readXmlElement(name, parent, dummy))
    {
        if (dummy == "DeletePermanently")
            output = zen::DELETE_PERMANENTLY;
        else if (dummy == "MoveToRecycleBin")
            output = zen::MOVE_TO_RECYCLE_BIN;
        else if (dummy == "MoveToCustomDirectory")
            output = zen::MOVE_TO_CUSTOM_DIRECTORY;
        else
            return false;

        return true;
    }
    return false;
}


bool readXmlElement(const std::string& name, const TiXmlElement* parent , zen::SymLinkHandling& output)
{
    std::string dummy;
    if (xmlAccess::readXmlElement(name, parent, dummy))
    {
        if (dummy == "Ignore")
            output = zen::SYMLINK_IGNORE;
        else if (dummy == "UseDirectly")
            output = zen::SYMLINK_USE_DIRECTLY;
        else if (dummy == "FollowLink")
            output = zen::SYMLINK_FOLLOW_LINK;
        else
            return false;

        return true;
    }
    return false;
}


bool readXmlElement(const std::string& name, const TiXmlElement* parent , zen::UnitTime& output)
{
    std::string dummy;
    if (xmlAccess::readXmlElement(name, parent, dummy))
    {
        if (dummy == "Inactive")
            output = zen::UTIME_NONE;
        else if (dummy == "Second")
            output = zen::UTIME_SEC;
        else if (dummy == "Minute")
            output = zen::UTIME_MIN;
        else if (dummy == "Hour")
            output = zen::UTIME_HOUR;
        else if (dummy == "Day")
            output = zen::UTIME_DAY;
        else
            return false;

        return true;
    }
    return false;
}


bool readXmlElement(const std::string& name, const TiXmlElement* parent , zen::UnitSize& output)
{
    std::string dummy;
    if (xmlAccess::readXmlElement(name, parent, dummy))
    {
        if (dummy == "Inactive")
            output = zen::USIZE_NONE;
        else if (dummy == "Byte")
            output = zen::USIZE_BYTE;
        else if (dummy == "KB")
            output = zen::USIZE_KB;
        else if (dummy == "MB")
            output = zen::USIZE_MB;
        else
            return false;

        return true;
    }
    return false;
}


bool readXmlElement(const std::string& name, const TiXmlElement* parent , zen::SyncConfig::Variant& output)
{
    std::string dummy;
    if (!xmlAccess::readXmlElement(name, parent, dummy))
        return false;

    if (dummy == "Automatic")
        output = SyncConfig::AUTOMATIC;
    else if (dummy == "Mirror")
        output = SyncConfig::MIRROR;
    else if (dummy == "Update")
        output = SyncConfig::UPDATE;
    else if (dummy == "Custom")
        output = SyncConfig::CUSTOM;
    else
        return false;

    return true;
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


void FfsXmlErrorLogger::readFilter(const TiXmlElement& parent, FilterConfig& output)
{
    //read filter settings
    readXmlElementLogging("Include", &parent, output.includeFilter);
    readXmlElementLogging("Exclude", &parent, output.excludeFilter);

    //migration "strategy": no error checking on these... :P

    readXmlElementLogging("TimeSpan",     &parent, output.timeSpan);
    readXmlElementLogging("UnitTimeSpan", &parent, output.unitTimeSpan);

    readXmlElementLogging("SizeMin",     &parent, output.sizeMin);
    readXmlElementLogging("UnitSizeMin", &parent, output.unitSizeMin);

    readXmlElementLogging("SizeMax",     &parent, output.sizeMax);
    readXmlElementLogging("UnitSizeMax", &parent, output.unitSizeMax);
}


//################################################################################################################
void FfsXmlErrorLogger::readConfig(const TiXmlElement& folderPair, FolderPairEnh& enhPair)
{
    //read folder pairs
    readXmlElementLogging("Left",  &folderPair, enhPair.leftDirectory);
    readXmlElementLogging("Right", &folderPair, enhPair.rightDirectory);


    //###########################################################
    //alternate sync configuration
    const TiXmlElement* xmlAltSyncCfg = TiXmlHandleConst(&folderPair).FirstChild("AlternateSyncConfig").ToElement();
    if (xmlAltSyncCfg)
    {
        AlternateSyncConfig* altSyncCfg = new AlternateSyncConfig;
        enhPair.altSyncConfig.reset(altSyncCfg);

        const TiXmlElement* xmlSyncDirections = TiXmlHandleConst(xmlAltSyncCfg).FirstChild("CustomDirections").ToElement();

        //read sync configuration
        readXmlElementLogging("Variant",  xmlAltSyncCfg, altSyncCfg->syncConfiguration.var);

        readXmlElementLogging("LeftOnly",   xmlSyncDirections, altSyncCfg->syncConfiguration.custom.exLeftSideOnly);
        readXmlElementLogging("RightOnly",  xmlSyncDirections, altSyncCfg->syncConfiguration.custom.exRightSideOnly);
        readXmlElementLogging("LeftNewer",  xmlSyncDirections, altSyncCfg->syncConfiguration.custom.leftNewer);
        readXmlElementLogging("RightNewer", xmlSyncDirections, altSyncCfg->syncConfiguration.custom.rightNewer);
        readXmlElementLogging("Different",  xmlSyncDirections, altSyncCfg->syncConfiguration.custom.different);
        readXmlElementLogging("Conflict",   xmlSyncDirections, altSyncCfg->syncConfiguration.custom.conflict);

        readXmlElementLogging("DeletionPolicy",       xmlAltSyncCfg, altSyncCfg->handleDeletion);
        readXmlElementLogging("CustomDeletionFolder", xmlAltSyncCfg, altSyncCfg->customDeletionDirectory);
    }

    //###########################################################
    //alternate filter configuration
    const TiXmlElement* filterCfg = TiXmlHandleConst(&folderPair).FirstChild("LocalFilter").ToElement();
    if (filterCfg)
        readFilter(*filterCfg, enhPair.localFilter);
}


void FfsXmlErrorLogger::readConfig(const TiXmlElement* root, MainConfiguration& mainCfg)
{
    TiXmlHandleConst hRoot(root); //custom const handle: TiXml API seems broken in this regard

    //###########################################################
    const TiXmlElement* xmlMainConfig = hRoot.FirstChild("MainConfig").ToElement();

    const TiXmlElement* cmpSettings  = hRoot.FirstChild("MainConfig").FirstChild("Comparison").ToElement();

    //read compare variant
    readXmlElementLogging("Variant", cmpSettings, mainCfg.compareVar);

    //include symbolic links at all?
    readXmlElementLogging("HandleSymlinks", cmpSettings, mainCfg.handleSymlinks);

    //###########################################################
    const TiXmlElement* xmlSyncCfg = hRoot.FirstChild("MainConfig").FirstChild("SyncConfig").ToElement();
    const TiXmlElement* syncDirections = TiXmlHandleConst(xmlSyncCfg).FirstChild("CustomDirections").ToElement();

    //read sync configuration
    readXmlElementLogging("Variant",    xmlSyncCfg,        mainCfg.syncConfiguration.var);

    readXmlElementLogging("LeftOnly",   syncDirections, mainCfg.syncConfiguration.custom.exLeftSideOnly);
    readXmlElementLogging("RightOnly",  syncDirections, mainCfg.syncConfiguration.custom.exRightSideOnly);
    readXmlElementLogging("LeftNewer",  syncDirections, mainCfg.syncConfiguration.custom.leftNewer);
    readXmlElementLogging("RightNewer", syncDirections, mainCfg.syncConfiguration.custom.rightNewer);
    readXmlElementLogging("Different",  syncDirections, mainCfg.syncConfiguration.custom.different);
    readXmlElementLogging("Conflict",   syncDirections, mainCfg.syncConfiguration.custom.conflict);

    //###########################################################
    //misc
    readXmlElementLogging("DeletionPolicy",       xmlSyncCfg, mainCfg.handleDeletion);
    readXmlElementLogging("CustomDeletionFolder", xmlSyncCfg, mainCfg.customDeletionDirectory);
    //###########################################################
    const TiXmlElement* filter = TiXmlHandleConst(xmlMainConfig).FirstChild("GlobalFilter").ToElement();

    //read filter settings
    if (filter)
        readFilter(*filter, mainCfg.globalFilter);
    else
        logError("GlobalFilter");

    //###########################################################
    const TiXmlElement* pairs = hRoot.FirstChild("MainConfig").FirstChild("FolderPairs").FirstChild("Pair").ToElement();

    //read all folder pairs
    mainCfg.additionalPairs.clear();
    bool firstLoop = true;
    while (pairs)
    {
        FolderPairEnh newPair;
        readConfig(*pairs, newPair);

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


void FfsXmlErrorLogger::readConfig(const TiXmlElement* root, xmlAccess::XmlGuiConfig& outputCfg)
{
    //read main config
    readConfig(root, outputCfg.mainCfg);

    //read GUI specific config data
    const TiXmlElement* guiConfig = TiXmlHandleConst(root).FirstChild("GuiConfig").ToElement();

    readXmlElementLogging("HideFiltered",      guiConfig, outputCfg.hideFilteredElements);
    readXmlElementLogging("HandleError",       guiConfig, outputCfg.handleError);
    readXmlElementLogging("SyncPreviewActive", guiConfig, outputCfg.syncPreviewEnabled);
}


void FfsXmlErrorLogger::readConfig(const TiXmlElement* root, xmlAccess::XmlBatchConfig& outputCfg)
{
    //read main config
    readConfig(root, outputCfg.mainCfg);

    //read batch specific config
    const TiXmlElement* batchConfig = TiXmlHandleConst(root).FirstChild("BatchConfig").ToElement();

    readXmlElementLogging("Silent", batchConfig, outputCfg.silent);
    readXmlElementLogging("LogfileDirectory", batchConfig, outputCfg.logFileDirectory);
    readXmlElementLogging("LogfileCountMax", batchConfig, outputCfg.logFileCountMax);
    readXmlElementLogging("HandleError", batchConfig, outputCfg.handleError);
}


void FfsXmlErrorLogger::readConfig(const TiXmlElement* root, xmlAccess::XmlGlobalSettings& outputCfg)
{
    //read global settings
    const TiXmlElement* global = TiXmlHandleConst(root).FirstChild("Shared").ToElement();

    //try to read program language setting
    readXmlElementLogging("Language", global, outputCfg.programLanguage);

    //copy locked files using VSS
    readXmlElementLogging("CopyLockedFiles", global, outputCfg.copyLockedFiles);

    //file permissions
    readXmlElementLogging("CopyFilePermissions", global, outputCfg.copyFilePermissions);

    //verify file copying
    readXmlElementLogging("VerifyCopiedFiles", global, outputCfg.verifyFileCopy);

    //max. allowed file time deviation
    readXmlElementLogging("FileTimeTolerance", global, outputCfg.fileTimeTolerance);


    const TiXmlElement* optionalDialogs = TiXmlHandleConst(root).FirstChild("Shared").FirstChild("ShowOptionalDialogs").ToElement();

    //folder dependency check
    readXmlElementLogging("CheckForDependentFolders", optionalDialogs, outputCfg.optDialogs.warningDependentFolders);

    //check for multi write access for directory as part of multiple pairs
    readXmlElementLogging("CheckForMultipleWriteAccess", optionalDialogs, outputCfg.optDialogs.warningMultiFolderWriteAccess);

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
    const TiXmlElement* gui = TiXmlHandleConst(root).FirstChild("Gui").ToElement();
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

    size_t folderPairMax = 0;
    readXmlElementLogging("FolderPairsMax", mainWindow, folderPairMax);
    if (folderPairMax != 0) //if reading fails, leave at default
        outputCfg.gui.addFolderPairCountMax = std::max(static_cast<size_t>(2), folderPairMax) - 1; //map folderPairMax to additionalFolderPairMax

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

    readXmlElementLogging("FolderHistoryLeft",  mainWindow, outputCfg.gui.folderHistoryLeft);
    readXmlElementLogging("FolderHistoryRight", mainWindow, outputCfg.gui.folderHistoryRight);
    readXmlElementLogging("MaximumHistorySize", mainWindow, outputCfg.gui.folderHistMax);

    readXmlElementLogging("Perspective", mainWindow, outputCfg.gui.guiPerspectiveLast);

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

    //load config history elements
    readXmlAttributeLogging("LastUsed", cfgHistory, outputCfg.gui.lastUsedConfigFile);
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
        case zen::CMP_BY_TIME_SIZE:
            xmlAccess::addXmlElement(name, std::string("ByTimeAndSize"), parent);
            break;
        case zen::CMP_BY_CONTENT:
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


void addXmlElement(const std::string& name, const OnGuiError value, TiXmlElement* parent)
{
    switch (value)
    {
        case ON_GUIERROR_IGNORE:
            xmlAccess::addXmlElement(name, std::string("Ignore"), parent);
            break;
        case ON_GUIERROR_POPUP:
            xmlAccess::addXmlElement(name, std::string("Popup"), parent);
            break;
    }
}


void addXmlElement(const std::string& name, const zen::DeletionPolicy value, TiXmlElement* parent)
{
    switch (value)
    {
        case zen::DELETE_PERMANENTLY:
            xmlAccess::addXmlElement(name, std::string("DeletePermanently"), parent);
            break;
        case zen::MOVE_TO_RECYCLE_BIN:
            xmlAccess::addXmlElement(name, std::string("MoveToRecycleBin"), parent);
            break;
        case zen::MOVE_TO_CUSTOM_DIRECTORY:
            xmlAccess::addXmlElement(name, std::string("MoveToCustomDirectory"), parent);
            break;
    }
}


void addXmlElement(const std::string& name, const zen::SymLinkHandling value, TiXmlElement* parent)
{
    switch (value)
    {
        case zen::SYMLINK_IGNORE:
            xmlAccess::addXmlElement(name, std::string("Ignore"), parent);
            break;
        case zen::SYMLINK_USE_DIRECTLY:
            xmlAccess::addXmlElement(name, std::string("UseDirectly"), parent);
            break;
        case zen::SYMLINK_FOLLOW_LINK:
            xmlAccess::addXmlElement(name, std::string("FollowLink"), parent);
            break;
    }
}


void addXmlElement(const std::string& name, const zen::UnitTime value, TiXmlElement* parent)
{
    switch (value)
    {
        case zen::UTIME_NONE:
            xmlAccess::addXmlElement(name, std::string("Inactive"), parent);
            break;
        case zen::UTIME_SEC:
            xmlAccess::addXmlElement(name, std::string("Second"), parent);
            break;
        case zen::UTIME_MIN:
            xmlAccess::addXmlElement(name, std::string("Minute"), parent);
            break;
        case zen::UTIME_HOUR:
            xmlAccess::addXmlElement(name, std::string("Hour"), parent);
            break;
        case zen::UTIME_DAY:
            xmlAccess::addXmlElement(name, std::string("Day"), parent);
            break;
    }
}


void addXmlElement(const std::string& name, zen::UnitSize value, TiXmlElement* parent)
{
    switch (value)
    {
        case zen::USIZE_NONE:
            xmlAccess::addXmlElement(name, std::string("Inactive"), parent);
            break;
        case zen::USIZE_BYTE:
            xmlAccess::addXmlElement(name, std::string("Byte"), parent);
            break;
        case zen::USIZE_KB:
            xmlAccess::addXmlElement(name, std::string("KB"), parent);
            break;
        case zen::USIZE_MB:
            xmlAccess::addXmlElement(name, std::string("MB"), parent);
            break;
    }
}


void addXmlElement(const std::string& name, SyncConfig::Variant value, TiXmlElement* parent)
{
    switch (value)
    {
        case SyncConfig::AUTOMATIC:
            xmlAccess::addXmlElement(name, std::string("Automatic"), parent);
            break;
        case SyncConfig::MIRROR:
            xmlAccess::addXmlElement(name, std::string("Mirror"), parent);
            break;
        case SyncConfig::UPDATE:
            xmlAccess::addXmlElement(name, std::string("Update"), parent);
            break;
        case SyncConfig::CUSTOM:
            xmlAccess::addXmlElement(name, std::string("Custom"), parent);
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


void writeFilter(const FilterConfig& input, TiXmlElement& parent)
{
    addXmlElement("Include", input.includeFilter, &parent);
    addXmlElement("Exclude", input.excludeFilter, &parent);

    addXmlElement("TimeSpan",     input.timeSpan,     &parent);
    addXmlElement("UnitTimeSpan", input.unitTimeSpan, &parent);

    addXmlElement("SizeMin",      input.sizeMin,      &parent);
    addXmlElement("UnitSizeMin",  input.unitSizeMin,  &parent);

    addXmlElement("SizeMax",      input.sizeMax,      &parent);
    addXmlElement("UnitSizeMax",  input.unitSizeMax,  &parent);
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
        TiXmlElement* xmlAltSyncCfg = new TiXmlElement("AlternateSyncConfig");
        newfolderPair->LinkEndChild(xmlAltSyncCfg);

        //write sync configuration
        addXmlElement("Variant",   altSyncConfig->syncConfiguration.var, xmlAltSyncCfg);

        TiXmlElement* syncDirections = new TiXmlElement("CustomDirections");
        xmlAltSyncCfg->LinkEndChild(syncDirections);

        addXmlElement("LeftOnly",   altSyncConfig->syncConfiguration.custom.exLeftSideOnly,  syncDirections);
        addXmlElement("RightOnly",  altSyncConfig->syncConfiguration.custom.exRightSideOnly, syncDirections);
        addXmlElement("LeftNewer",  altSyncConfig->syncConfiguration.custom.leftNewer,       syncDirections);
        addXmlElement("RightNewer", altSyncConfig->syncConfiguration.custom.rightNewer,      syncDirections);
        addXmlElement("Different",  altSyncConfig->syncConfiguration.custom.different,       syncDirections);
        addXmlElement("Conflict",   altSyncConfig->syncConfiguration.custom.conflict,        syncDirections);

        //misc
        addXmlElement("DeletionPolicy",       altSyncConfig->handleDeletion,          xmlAltSyncCfg);
        addXmlElement("CustomDeletionFolder", altSyncConfig->customDeletionDirectory, xmlAltSyncCfg);
    }

    //###########################################################
    //alternate filter configuration
    TiXmlElement* filterCfg = new TiXmlElement("LocalFilter");
    newfolderPair->LinkEndChild(filterCfg);

    //write filter settings
    writeFilter(enhPair.localFilter, *filterCfg);
}


void writeConfig(const MainConfiguration& mainCfg, TiXmlElement& root)
{
    TiXmlElement* xmlMainCfg = new TiXmlElement("MainConfig");
    root.LinkEndChild(xmlMainCfg);

    //###########################################################
    TiXmlElement* cmpSettings = new TiXmlElement("Comparison");
    xmlMainCfg->LinkEndChild(cmpSettings);

    //write compare algorithm
    addXmlElement("Variant", mainCfg.compareVar, cmpSettings);

    //include symbolic links at all?
    addXmlElement("HandleSymlinks", mainCfg.handleSymlinks, cmpSettings);

    //###########################################################
    TiXmlElement* xmlSyncCfg = new TiXmlElement("SyncConfig");
    xmlMainCfg->LinkEndChild(xmlSyncCfg);

    //write sync configuration
    addXmlElement("Variant", mainCfg.syncConfiguration.var, xmlSyncCfg);

    TiXmlElement* syncDirections = new TiXmlElement("CustomDirections");
    xmlSyncCfg->LinkEndChild(syncDirections);

    addXmlElement("LeftOnly",   mainCfg.syncConfiguration.custom.exLeftSideOnly,  syncDirections);
    addXmlElement("RightOnly",  mainCfg.syncConfiguration.custom.exRightSideOnly, syncDirections);
    addXmlElement("LeftNewer",  mainCfg.syncConfiguration.custom.leftNewer,       syncDirections);
    addXmlElement("RightNewer", mainCfg.syncConfiguration.custom.rightNewer,      syncDirections);
    addXmlElement("Different",  mainCfg.syncConfiguration.custom.different,       syncDirections);
    addXmlElement("Conflict",   mainCfg.syncConfiguration.custom.conflict,        syncDirections);

    //###########################################################
    //write filter settings
    TiXmlElement* filter = new TiXmlElement("GlobalFilter");
    xmlMainCfg->LinkEndChild(filter);

    writeFilter(mainCfg.globalFilter, *filter);

    //other
    addXmlElement("DeletionPolicy",       mainCfg.handleDeletion,          xmlSyncCfg);
    addXmlElement("CustomDeletionFolder", mainCfg.customDeletionDirectory, xmlSyncCfg);

    //###########################################################
    TiXmlElement* pairs = new TiXmlElement("FolderPairs");
    xmlMainCfg->LinkEndChild(pairs);

    //write first folder pair
    writeXmlLocalConfig(mainCfg.firstPair, *pairs);

    //write additional folder pairs
    for (std::vector<FolderPairEnh>::const_iterator i = mainCfg.additionalPairs.begin(); i != mainCfg.additionalPairs.end(); ++i)
        writeXmlLocalConfig(*i, *pairs);
}


void writeConfig(const xmlAccess::XmlGuiConfig& inputCfg, TiXmlElement& root)
{
    //write main config
    writeConfig(inputCfg.mainCfg, root);

    //write GUI specific config
    TiXmlElement* guiConfig = new TiXmlElement("GuiConfig");
    root.LinkEndChild(guiConfig);

    addXmlElement("HideFiltered",      inputCfg.hideFilteredElements, guiConfig);
    addXmlElement("HandleError",       inputCfg.handleError,          guiConfig);
    addXmlElement("SyncPreviewActive", inputCfg.syncPreviewEnabled,   guiConfig);
}


void writeConfig(const xmlAccess::XmlBatchConfig& inputCfg, TiXmlElement& root)
{
    //write main config
    writeConfig(inputCfg.mainCfg, root);

    //write GUI specific config
    TiXmlElement* batchConfig = new TiXmlElement("BatchConfig");
    root.LinkEndChild(batchConfig);

    addXmlElement("Silent", inputCfg.silent, batchConfig);
    addXmlElement("LogfileDirectory", inputCfg.logFileDirectory, batchConfig);
    addXmlElement("LogfileCountMax", inputCfg.logFileCountMax, batchConfig);
    addXmlElement("HandleError", inputCfg.handleError, batchConfig);
}


void writeConfig(const xmlAccess::XmlGlobalSettings& inputCfg, TiXmlElement& root)
{
    //write global settings
    TiXmlElement* global = new TiXmlElement("Shared");
    root.LinkEndChild(global);

    //program language
    addXmlElement("Language", inputCfg.programLanguage, global);

    //copy locked files using VSS
    addXmlElement("CopyLockedFiles", inputCfg.copyLockedFiles, global);

    //file permissions
    addXmlElement("CopyFilePermissions", inputCfg.copyFilePermissions, global);

    //verify file copying
    addXmlElement("VerifyCopiedFiles", inputCfg.verifyFileCopy, global);

    //max. allowed file time deviation
    addXmlElement("FileTimeTolerance", inputCfg.fileTimeTolerance, global);


    //optional dialogs
    TiXmlElement* optionalDialogs = new TiXmlElement("ShowOptionalDialogs");
    global->LinkEndChild(optionalDialogs);

    //warning: dependent folders
    addXmlElement("CheckForDependentFolders", inputCfg.optDialogs.warningDependentFolders, optionalDialogs);

    //check for multi write access for directory as part of multiple pairs
    addXmlElement("CheckForMultipleWriteAccess", inputCfg.optDialogs.warningMultiFolderWriteAccess, optionalDialogs);

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
    root.LinkEndChild(gui);

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

    addXmlElement("FolderPairsMax", inputCfg.gui.addFolderPairCountMax + 1 /*add main pair*/, mainWindow);


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

    addXmlElement("FolderHistoryLeft",  inputCfg.gui.folderHistoryLeft , mainWindow);
    addXmlElement("FolderHistoryRight", inputCfg.gui.folderHistoryRight, mainWindow);
    addXmlElement("MaximumHistorySize", inputCfg.gui.folderHistMax     , mainWindow);

    addXmlElement("Perspective", inputCfg.gui.guiPerspectiveLast, mainWindow);

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

    addXmlAttribute("LastUsed", inputCfg.gui.lastUsedConfigFile, cfgHistory);
    addXmlElement("File", inputCfg.gui.cfgFileHistory, cfgHistory);

    //last update check
    addXmlElement("LastUpdateCheck", inputCfg.gui.lastUpdateCheck, gui);

    //###################################################################
    //write global batch settings

    TiXmlElement* batch = new TiXmlElement("Batch");
    root.LinkEndChild(batch);
}


wxString xmlAccess::getGlobalConfigFile()
{
    return zen::getConfigDir() + wxT("GlobalSettings.xml");
}


void xmlAccess::OptionalDialogs::resetDialogs()
{
    warningDependentFolders        = true;
    warningMultiFolderWriteAccess  = true;
    warningSignificantDifference   = true;
    warningNotEnoughDiskSpace      = true;
    warningUnresolvedConflicts     = true;
    warningSyncDatabase            = true;
    popupOnConfigChange            = true;
    showSummaryBeforeSync          = true;
}


xmlAccess::XmlGuiConfig xmlAccess::convertBatchToGui(const xmlAccess::XmlBatchConfig& batchCfg)
{
    XmlGuiConfig output;
    output.mainCfg = batchCfg.mainCfg;
    return output;
}


xmlAccess::XmlBatchConfig xmlAccess::convertGuiToBatch(const xmlAccess::XmlGuiConfig& guiCfg)
{
    XmlBatchConfig output;
    output.mainCfg = guiCfg.mainCfg;

    switch (guiCfg.handleError)
    {
        case ON_GUIERROR_POPUP:
            output.handleError = xmlAccess::ON_ERROR_POPUP;
            break;
        case ON_GUIERROR_IGNORE:
            output.handleError = xmlAccess::ON_ERROR_IGNORE;
            break;
    }

    return output;
}


xmlAccess::MergeType xmlAccess::getMergeType(const std::vector<wxString>& filenames)   //throw ()
{
    bool guiCfgExists   = false;
    bool batchCfgExists = false;

    for (std::vector<wxString>::const_iterator i = filenames.begin(); i != filenames.end(); ++i)
    {
        switch (xmlAccess::getXmlType(*i)) //throw()
        {
            case XML_TYPE_GUI:
                guiCfgExists = true;
                break;

            case XML_TYPE_BATCH:
                batchCfgExists = true;
                break;

            case XML_TYPE_GLOBAL:
            case XML_TYPE_OTHER:
                return MERGE_OTHER;
        }
    }

    if (guiCfgExists && batchCfgExists)
        return MERGE_GUI_BATCH;
    else if (guiCfgExists && !batchCfgExists)
        return MERGE_GUI;
    else if (!guiCfgExists && batchCfgExists)
        return MERGE_BATCH;
    else
        return MERGE_OTHER;
}


namespace
{
template <class XmlCfg>
XmlCfg loadCfgImpl(const wxString& filename, std::auto_ptr<xmlAccess::XmlError>& exeption) //throw (xmlAccess::XmlError)
{
    XmlCfg cfg;
    try
    {
        xmlAccess::readConfig(filename, cfg); //throw (xmlAccess::XmlError);
    }
    catch (const xmlAccess::XmlError& e)
    {
        if (e.getSeverity() == xmlAccess::XmlError::FATAL)
            throw;
        else
            exeption.reset(new xmlAccess::XmlError(e));
    }
    return cfg;
}


template <class XmlCfg>
void mergeConfigFilesImpl(const std::vector<wxString>& filenames, XmlCfg& config)   //throw (xmlAccess::XmlError)
{
    using namespace xmlAccess;

    assert(!filenames.empty());
    if (filenames.empty())
        return;

    std::vector<zen::MainConfiguration> mainCfgs;
    std::auto_ptr<XmlError> savedException;

    for (std::vector<wxString>::const_iterator i = filenames.begin(); i != filenames.end(); ++i)
    {
        switch (getXmlType(*i))
        {
            case XML_TYPE_GUI:
                mainCfgs.push_back(loadCfgImpl<XmlGuiConfig>(*i, savedException).mainCfg); //throw (xmlAccess::XmlError)
                break;

            case XML_TYPE_BATCH:
                mainCfgs.push_back(loadCfgImpl<XmlBatchConfig>(*i, savedException).mainCfg); //throw (xmlAccess::XmlError)
                break;

            case XML_TYPE_GLOBAL:
            case XML_TYPE_OTHER:
                break;
        }
    }

    if (mainCfgs.empty())
        throw XmlError(_("Invalid FreeFileSync config file!"));

    try //...to init all non-"mainCfg" settings with first config file
    {
        xmlAccess::readConfig(filenames[0], config); //throw (xmlAccess::XmlError);
    }
    catch (...) {}

    config.mainCfg = merge(mainCfgs);

    if (savedException.get()) //"re-throw" exception
        throw* savedException;
}
}


void xmlAccess::convertConfig(const std::vector<wxString>& filenames, XmlGuiConfig& config)   //throw (xmlAccess::XmlError)
{
    mergeConfigFilesImpl(filenames, config);   //throw (xmlAccess::XmlError)
}


void xmlAccess::convertConfig(const std::vector<wxString>& filenames, XmlBatchConfig& config) //throw (xmlAccess::XmlError);
{
    mergeConfigFilesImpl(filenames, config);   //throw (xmlAccess::XmlError)
}







void xmlAccess::readConfig(const wxString& filename, xmlAccess::XmlGuiConfig& config)
{
    //load XML
    if (!fileExists(wxToZ(filename)))
        throw XmlError(wxString(_("File does not exist:")) + wxT("\n\"") + filename + wxT("\""));

    TiXmlDocument doc;
    loadXmlDocument(filename, doc); //throw (XmlError)

    if (getXmlType(doc) != XML_TYPE_GUI) //throw()
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\""));

    FfsXmlErrorLogger logger;
    logger.readConfig(doc.RootElement(), config); //read GUI layout configuration

    if (logger.errorsOccurred())
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\"\n\n") +
                       logger.getErrorMessageFormatted(), XmlError::WARNING);
}


void xmlAccess::readConfig(const wxString& filename, xmlAccess::XmlBatchConfig& config)
{
    //load XML
    if (!fileExists(wxToZ(filename)))
        throw XmlError(wxString(_("File does not exist:")) + wxT("\n\"") + filename + wxT("\""));

    TiXmlDocument doc;
    loadXmlDocument(filename, doc); //throw (XmlError)

    if (getXmlType(doc) != XML_TYPE_BATCH) //throw()
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\""));

    FfsXmlErrorLogger logger;
    logger.readConfig(doc.RootElement(), config); //read GUI layout configuration

    if (logger.errorsOccurred())
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\"\n\n") +
                       logger.getErrorMessageFormatted(), XmlError::WARNING);
}


void xmlAccess::readConfig(xmlAccess::XmlGlobalSettings& config)
{
    //load XML
    if (!fileExists(wxToZ(getGlobalConfigFile())))
        throw XmlError(wxString(_("File does not exist:")) + wxT("\n\"") + getGlobalConfigFile() + wxT("\""));

    TiXmlDocument doc;
    loadXmlDocument(getGlobalConfigFile(), doc); //throw (XmlError)

    if (getXmlType(doc) != XML_TYPE_GLOBAL) //throw()
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + getGlobalConfigFile() + wxT("\""));

    FfsXmlErrorLogger logger;
    logger.readConfig(doc.RootElement(), config); //read GUI layout configuration

    if (logger.errorsOccurred())
        throw XmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + getGlobalConfigFile() + wxT("\"\n\n") +
                       logger.getErrorMessageFormatted(), XmlError::WARNING);
}


void xmlAccess::writeConfig(const XmlGuiConfig& outputCfg, const wxString& filename)
{
    TiXmlDocument doc;
    initXmlDocument(doc, XML_TYPE_GUI); //throw()

    if (!doc.RootElement())
        throw XmlError(wxString(_("Error writing file:")) + wxT("\n\"") + filename + wxT("\""));

    writeConfig(outputCfg, *doc.RootElement()); //add GUI layout configuration settings

    saveXmlDocument(filename, doc); //throw (XmlError)
}


void xmlAccess::writeConfig(const XmlBatchConfig& outputCfg, const wxString& filename)
{
    TiXmlDocument doc;
    initXmlDocument(doc, XML_TYPE_BATCH); //throw()

    if (!doc.RootElement())
        throw XmlError(wxString(_("Error writing file:")) + wxT("\n\"") + filename + wxT("\""));

    writeConfig(outputCfg, *doc.RootElement()); //add GUI layout configuration settings

    saveXmlDocument(filename, doc); //throw (XmlError)
}


void xmlAccess::writeConfig(const XmlGlobalSettings& outputCfg)
{
    TiXmlDocument doc;
    initXmlDocument(doc, XML_TYPE_GLOBAL); //throw()

    if (!doc.RootElement())
        throw XmlError(wxString(_("Error writing file:")) + wxT("\n\"") + getGlobalConfigFile() + wxT("\""));

    writeConfig(outputCfg, *doc.RootElement()); //add GUI layout configuration settings

    saveXmlDocument(getGlobalConfigFile(), doc); //throw (XmlError)
}
