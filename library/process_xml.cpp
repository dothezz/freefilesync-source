// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "process_xml.h"
#include <zenXml/zenxml.h>
#include "../shared/i18n.h"
#include "../shared/global_func.h"
#include "../shared/standard_paths.h"
#include "../shared/string_conv.h"
#include "../shared/file_handling.h"
#include "../shared/file_io.h"
#include "../shared/xml_base.h"

using namespace zen;
using namespace xmlAccess; //functionally needed for correct overload resolution!!!


XmlType getXmlType(const zen::XmlDoc& doc) //throw()
{
    if (doc.root().getNameAs<std::string>() == "FreeFileSync")
    {
        std::string type;
        if (doc.root().getAttribute("XmlType", type))
        {
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


XmlType xmlAccess::getXmlType(const wxString& filename) //throw()
{
    XmlDoc doc;
    try
    {
        //do NOT use zen::loadStream as it will superfluously load even huge files!
        loadXmlDocument(filename, doc); //throw FfsXmlError, quick exit if file is not an FFS XML
    }
    catch (const FfsXmlError&)
    {
        return XML_TYPE_OTHER;
    }
    return ::getXmlType(doc);
}


void setXmlType(XmlDoc& doc, XmlType type) //throw()
{
    switch (type)
    {
        case XML_TYPE_GUI:
            doc.root().setAttribute("XmlType", "GUI");
            break;
        case XML_TYPE_BATCH:
            doc.root().setAttribute("XmlType", "BATCH");
            break;
        case XML_TYPE_GLOBAL:
            doc.root().setAttribute("XmlType", "GLOBAL");
            break;
        case XML_TYPE_OTHER:
            assert(false);
            break;
    }
}
//################################################################################################################


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
    warningRecyclerMissing         = true;
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
XmlCfg loadCfgImpl(const wxString& filename, std::unique_ptr<xmlAccess::FfsXmlError>& exeption) //throw (xmlAccess::FfsXmlError)
{
    XmlCfg cfg;
    try
    {
        xmlAccess::readConfig(filename, cfg); //throw (xmlAccess::FfsXmlError);
    }
    catch (const xmlAccess::FfsXmlError& e)
    {
        if (e.getSeverity() == xmlAccess::FfsXmlError::FATAL)
            throw;
        else
            exeption.reset(new xmlAccess::FfsXmlError(e));
    }
    return cfg;
}


template <class XmlCfg>
void mergeConfigFilesImpl(const std::vector<wxString>& filenames, XmlCfg& config)   //throw (xmlAccess::FfsXmlError)
{
    using namespace xmlAccess;

    assert(!filenames.empty());
    if (filenames.empty())
        return;

    std::vector<zen::MainConfiguration> mainCfgs;
    std::unique_ptr<FfsXmlError> savedException;

    for (std::vector<wxString>::const_iterator i = filenames.begin(); i != filenames.end(); ++i)
    {
        switch (getXmlType(*i))
        {
            case XML_TYPE_GUI:
                mainCfgs.push_back(loadCfgImpl<XmlGuiConfig>(*i, savedException).mainCfg); //throw (xmlAccess::FfsXmlError)
                break;

            case XML_TYPE_BATCH:
                mainCfgs.push_back(loadCfgImpl<XmlBatchConfig>(*i, savedException).mainCfg); //throw (xmlAccess::FfsXmlError)
                break;

            case XML_TYPE_GLOBAL:
            case XML_TYPE_OTHER:
                break;
        }
    }

    if (mainCfgs.empty())
        throw FfsXmlError(_("Invalid FreeFileSync config file!"));

    try //...to init all non-"mainCfg" settings with first config file
    {
        xmlAccess::readConfig(filenames[0], config); //throw (xmlAccess::FfsXmlError);
    }
    catch (...) {}

    config.mainCfg = merge(mainCfgs);

    if (savedException.get()) //"re-throw" exception
        throw* savedException;
}
}


void xmlAccess::convertConfig(const std::vector<wxString>& filenames, XmlGuiConfig& config)   //throw (xmlAccess::FfsXmlError)
{
    mergeConfigFilesImpl(filenames, config);   //throw (xmlAccess::FfsXmlError)
}


void xmlAccess::convertConfig(const std::vector<wxString>& filenames, XmlBatchConfig& config) //throw (xmlAccess::FfsXmlError);
{
    mergeConfigFilesImpl(filenames, config);   //throw (xmlAccess::FfsXmlError)
}


namespace zen
{
template <> inline
void writeText(const CompareVariant& value, std::string& output)
{
    switch (value)
    {
        case zen::CMP_BY_TIME_SIZE:
            output = "ByTimeAndSize";
            break;
        case zen::CMP_BY_CONTENT:
            output = "ByContent";
            break;
    }
}

template <> inline
bool readText(const std::string& input, CompareVariant& value)
{
    std::string tmp = input;
    zen::trim(tmp);
    if (tmp == "ByTimeAndSize")
        value = zen::CMP_BY_TIME_SIZE;
    else if (tmp == "ByContent")
        value = zen::CMP_BY_CONTENT;
    else
        return false;
    return true;
}


template <> inline
void writeText(const SyncDirection& value, std::string& output)
{
    switch (value)
    {
        case SYNC_DIR_LEFT:
            output = "left";
            break;
        case SYNC_DIR_RIGHT:
            output = "right";
            break;
        case SYNC_DIR_NONE:
            output = "none";
            break;
    }
}

template <> inline
bool readText(const std::string& input, SyncDirection& value)
{
    std::string tmp = input;
    zen::trim(tmp);
    if (tmp == "left")
        value = SYNC_DIR_LEFT;
    else if (tmp == "right")
        value = SYNC_DIR_RIGHT;
    else if (tmp == "none")
        value = SYNC_DIR_NONE;
    else
        return false;
    return true;
}


template <> inline
void writeText(const OnError& value, std::string& output)
{
    switch (value)
    {
        case ON_ERROR_IGNORE:
            output = "Ignore";
            break;
        case ON_ERROR_EXIT:
            output = "Exit";
            break;
        case ON_ERROR_POPUP:
            output = "Popup";
            break;
    }
}

template <> inline
bool readText(const std::string& input, OnError& value)
{
    std::string tmp = input;
    zen::trim(tmp);
    if (tmp == "Ignore")
        value = ON_ERROR_IGNORE;
    else if (tmp == "Exit")
        value = ON_ERROR_EXIT;
    else if (tmp == "Popup")
        value = ON_ERROR_POPUP;
    else
        return false;
    return true;
}


template <> inline
void writeText(const OnGuiError& value, std::string& output)
{
    switch (value)
    {
        case ON_GUIERROR_IGNORE:
            output = "Ignore";
            break;
        case ON_GUIERROR_POPUP:
            output = "Popup";
            break;
    }
}

template <> inline
bool readText(const std::string& input, OnGuiError& value)
{
    std::string tmp = input;
    zen::trim(tmp);
    if (tmp == "Ignore")
        value = ON_GUIERROR_IGNORE;
    else if (tmp == "Popup")
        value = ON_GUIERROR_POPUP;
    else
        return false;
    return true;
}


template <> inline
void writeText(const DeletionPolicy& value, std::string& output)
{
    switch (value)
    {
        case DELETE_PERMANENTLY:
            output = "DeletePermanently";
            break;
        case MOVE_TO_RECYCLE_BIN:
            output = "MoveToRecycleBin";
            break;
        case MOVE_TO_CUSTOM_DIRECTORY:
            output = "MoveToCustomDirectory";
            break;
    }
}

template <> inline
bool readText(const std::string& input, DeletionPolicy& value)
{
    std::string tmp = input;
    zen::trim(tmp);
    if (tmp == "DeletePermanently")
        value = DELETE_PERMANENTLY;
    else if (tmp == "MoveToRecycleBin")
        value = MOVE_TO_RECYCLE_BIN;
    else if (tmp == "MoveToCustomDirectory")
        value = MOVE_TO_CUSTOM_DIRECTORY;
    else
        return false;
    return true;
}


template <> inline
void writeText(const SymLinkHandling& value, std::string& output)
{
    switch (value)
    {
        case SYMLINK_IGNORE:
            output = "Ignore";
            break;
        case SYMLINK_USE_DIRECTLY:
            output = "UseDirectly";
            break;
        case SYMLINK_FOLLOW_LINK:
            output = "FollowLink";
            break;
    }
}

template <> inline
bool readText(const std::string& input, SymLinkHandling& value)
{
    std::string tmp = input;
    zen::trim(tmp);
    if (tmp == "Ignore")
        value = SYMLINK_IGNORE;
    else if (tmp == "UseDirectly")
        value = SYMLINK_USE_DIRECTLY;
    else if (tmp == "FollowLink")
        value = SYMLINK_FOLLOW_LINK;
    else
        return false;
    return true;
}


template <> inline
void writeText(const UnitTime& value, std::string& output)
{
    switch (value)
    {
        case UTIME_NONE:
            output = "Inactive";
            break;
        case UTIME_SEC:
            output = "Second";
            break;
        case UTIME_MIN:
            output = "Minute";
            break;
        case UTIME_HOUR:
            output = "Hour";
            break;
        case UTIME_DAY:
            output = "Day";
            break;
    }
}

template <> inline
bool readText(const std::string& input, UnitTime& value)
{
    std::string tmp = input;
    zen::trim(tmp);
    if (tmp == "Inactive")
        value = UTIME_NONE;
    else if (tmp == "Second")
        value = UTIME_SEC;
    else if (tmp == "Minute")
        value = UTIME_MIN;
    else if (tmp == "Hour")
        value = UTIME_HOUR;
    else if (tmp == "Day")
        value = UTIME_DAY;
    else
        return false;
    return true;
}


template <> inline
void writeText(const ColumnTypes& value, std::string& output)
{
    output = toString<std::string>(value);
}

template <> inline
bool readText(const std::string& input, ColumnTypes& value)
{
    value = static_cast<ColumnTypes>(toNumber<int>(input));
    return true;
}


template <> inline
void writeText(const UnitSize& value, std::string& output)
{
    switch (value)
    {
        case USIZE_NONE:
            output = "Inactive";
            break;
        case USIZE_BYTE:
            output = "Byte";
            break;
        case USIZE_KB:
            output = "KB";
            break;
        case USIZE_MB:
            output = "MB";
            break;
    }
}

template <> inline
bool readText(const std::string& input, UnitSize& value)
{
    std::string tmp = input;
    zen::trim(tmp);
    if (tmp == "Inactive")
        value = USIZE_NONE;
    else if (tmp == "Byte")
        value = USIZE_BYTE;
    else if (tmp == "KB")
        value = USIZE_KB;
    else if (tmp == "MB")
        value = USIZE_MB;
    else
        return false;
    return true;
}


template <> inline
void writeText(const SyncConfig::Variant& value, std::string& output)
{
    switch (value)
    {
        case SyncConfig::AUTOMATIC:
            output = "Automatic";
            break;
        case SyncConfig::MIRROR:
            output = "Mirror";
            break;
        case SyncConfig::UPDATE:
            output = "Update";
            break;
        case SyncConfig::CUSTOM:
            output = "Custom";
            break;
    }
}

template <> inline
bool readText(const std::string& input, SyncConfig::Variant& value)
{
    std::string tmp = input;
    zen::trim(tmp);
    if (tmp == "Automatic")
        value = SyncConfig::AUTOMATIC;
    else if (tmp == "Mirror")
        value = SyncConfig::MIRROR;
    else if (tmp == "Update")
        value = SyncConfig::UPDATE;
    else if (tmp == "Custom")
        value = SyncConfig::CUSTOM;
    else
        return false;
    return true;
}


template <> inline
bool readValue(const XmlElement& input, ColumnAttrib& value)
{
    XmlIn in(input);
    bool rv1 = in.attribute("Type",    value.type);
    bool rv2 = in.attribute("Visible", value.visible);
    bool rv3 = in.attribute("Width",   value.width);
    value.position = 0;
    return rv1 && rv2 && rv3;
}

template <> inline
void writeValue(const ColumnAttrib& value, XmlElement& output)
{
    XmlOut out(output);
    out.attribute("Type",    value.type);
    out.attribute("Visible", value.visible);
    out.attribute("Width",   value.width);
}
}


namespace
{
void readConfig(const XmlIn& in, SyncConfig& syncCfg)
{
    in["Variant"](syncCfg.var);

    XmlIn inCustDir = in["CustomDirections"];
    inCustDir["LeftOnly"  ](syncCfg.custom.exLeftSideOnly);
    inCustDir["RightOnly" ](syncCfg.custom.exRightSideOnly);
    inCustDir["LeftNewer" ](syncCfg.custom.leftNewer);
    inCustDir["RightNewer"](syncCfg.custom.rightNewer);
    inCustDir["Different" ](syncCfg.custom.different);
    inCustDir["Conflict"  ](syncCfg.custom.conflict);
}


void readConfig(const XmlIn& in, FilterConfig& filter)
{
    in["Include"](filter.includeFilter);
    in["Exclude"](filter.excludeFilter);

    in["TimeSpan"    ](filter.timeSpan);
    in["UnitTimeSpan"](filter.unitTimeSpan);

    in["SizeMin"    ](filter.sizeMin);
    in["UnitSizeMin"](filter.unitSizeMin);

    in["SizeMax"    ](filter.sizeMax);
    in["UnitSizeMax"](filter.unitSizeMax);
}


void readConfig(const XmlIn& in, FolderPairEnh& enhPair)
{
    //read folder pairs
    in["Left" ](enhPair.leftDirectory);
    in["Right"](enhPair.rightDirectory);

    //###########################################################
    //alternate sync configuration (optional)
    XmlIn inAlt = in["AlternateSyncConfig"];
    if (inAlt)
    {
        std::shared_ptr<AlternateSyncConfig> altSyncCfg = std::make_shared<AlternateSyncConfig>();
        enhPair.altSyncConfig = altSyncCfg;

        //read sync configuration
        readConfig(inAlt, altSyncCfg->syncConfiguration);

        inAlt["DeletionPolicy"      ](altSyncCfg->handleDeletion);
        inAlt["CustomDeletionFolder"](altSyncCfg->customDeletionDirectory);
    }

    //###########################################################
    //alternate filter configuration
    readConfig(in["LocalFilter"], enhPair.localFilter);
}


void readConfig(const XmlIn& in, MainConfiguration& mainCfg)
{
    XmlIn inCmp  = in["MainConfig"]["Comparison"];

    //read compare variant
    inCmp["Variant"](mainCfg.compareVar);

    //include symbolic links at all?
    inCmp["HandleSymlinks"](mainCfg.handleSymlinks);

    //###########################################################
    XmlIn inSync = in["MainConfig"]["SyncConfig"];

    //read sync configuration
    readConfig(inSync, mainCfg.syncConfiguration);
    //###########################################################

    //misc
    inSync["DeletionPolicy"      ](mainCfg.handleDeletion);
    inSync["CustomDeletionFolder"](mainCfg.customDeletionDirectory);
    //###########################################################

    XmlIn inFilter = in["MainConfig"]["GlobalFilter"];
    //read filter settings
    readConfig(inFilter, mainCfg.globalFilter);

    //###########################################################
    //read all folder pairs
    mainCfg.additionalPairs.clear();

    bool firstIter = true;
    for (XmlIn inPair = in["MainConfig"]["FolderPairs"]["Pair"]; inPair; inPair.next())
    {
        FolderPairEnh newPair;
        readConfig(inPair, newPair);

        if (firstIter)
        {
            firstIter = false;
            mainCfg.firstPair = newPair; //set first folder pair
        }
        else
            mainCfg.additionalPairs.push_back(newPair); //set additional folder pairs
    }
}


void readConfig(const XmlIn& in, xmlAccess::XmlGuiConfig& config)
{
    ::readConfig(in, config.mainCfg); //read main config

    //read GUI specific config data
    XmlIn inGuiCfg = in["GuiConfig"];

    inGuiCfg["HideFiltered"     ](config.hideFilteredElements);
    inGuiCfg["HandleError"      ](config.handleError);
    inGuiCfg["SyncPreviewActive"](config.syncPreviewEnabled);
}


void readConfig(const XmlIn& in, xmlAccess::XmlBatchConfig& config)
{
    ::readConfig(in, config.mainCfg); //read main config

    //read GUI specific config data
    XmlIn inBatchCfg = in["BatchConfig"];

    inBatchCfg["Silent"          ](config.silent);
    inBatchCfg["LogfileDirectory"](config.logFileDirectory);
    inBatchCfg["LogfileCountMax" ](config.logFileCountMax);
    inBatchCfg["HandleError"     ](config.handleError);
}


void readConfig(const XmlIn& in, XmlGlobalSettings& config)
{
    XmlIn inShared = in["Shared"];

    //try to read program language setting
    inShared["Language"](config.programLanguage);

    inShared["CopyLockedFiles"      ](config.copyLockedFiles);
    inShared["CopyFilePermissions"  ](config.copyFilePermissions);
	inShared["TransactionalFileCopy"](config.transactionalFileCopy);
    inShared["VerifyCopiedFiles"    ](config.verifyFileCopy);

    //max. allowed file time deviation
    inShared["FileTimeTolerance"](config.fileTimeTolerance);

    XmlIn inOpt = inShared["ShowOptionalDialogs"];
    inOpt["CheckForDependentFolders"     ](config.optDialogs.warningDependentFolders);
    inOpt["CheckForMultipleWriteAccess"  ](config.optDialogs.warningMultiFolderWriteAccess);
    inOpt["CheckForSignificantDifference"](config.optDialogs.warningSignificantDifference);
    inOpt["CheckForFreeDiskSpace"](config.optDialogs.warningNotEnoughDiskSpace);
    inOpt["CheckForUnresolvedConflicts"](config.optDialogs.warningUnresolvedConflicts);
    inOpt["NotifyDatabaseError"](config.optDialogs.warningSyncDatabase);
    inOpt["CheckMissingRecycleBin"](config.optDialogs.warningRecyclerMissing);
    inOpt["PopupOnConfigChange"](config.optDialogs.popupOnConfigChange);
    inOpt["SummaryBeforeSync"  ](config.optDialogs.showSummaryBeforeSync);

    //gui specific global settings (optional)
    XmlIn inGui = in["Gui"];
    XmlIn inWnd = inGui["Windows"]["Main"];

    //read application window size and position
    inWnd["Width"    ](config.gui.dlgSize.x);
    inWnd["Height"   ](config.gui.dlgSize.y);
    inWnd["PosX"     ](config.gui.dlgPos.x);
    inWnd["PosY"     ](config.gui.dlgPos.y);
    inWnd["Maximized"](config.gui.isMaximized);

    inWnd["MaxFolderPairsVisible"](config.gui.maxFolderPairsVisible);

    inWnd["ManualDeletionOnBothSides"](config.gui.deleteOnBothSides);
    inWnd["ManualDeletionUseRecycler"](config.gui.useRecyclerForManualDeletion);
    inWnd["RespectCaseOnSearch"      ](config.gui.textSearchRespectCase);

    //###########################################################

    //read column attributes
    XmlIn inColLeft = inWnd["LeftColumns"];

    inColLeft.attribute("AutoAdjust",    config.gui.autoAdjustColumnsLeft);
    inColLeft.attribute("ShowFileIcons", config.gui.showFileIconsLeft);

    inColLeft(config.gui.columnAttribLeft);
    for (size_t i = 0; i < config.gui.columnAttribLeft.size(); ++i)
        config.gui.columnAttribLeft[i].position = i;

    //###########################################################
    XmlIn inColRight = inWnd["RightColumns"];

    inColRight.attribute("AutoAdjust",    config.gui.autoAdjustColumnsRight);
    inColRight.attribute("ShowFileIcons", config.gui.showFileIconsRight);

    inColRight(config.gui.columnAttribRight);
    for (size_t i = 0; i < config.gui.columnAttribRight.size(); ++i)
        config.gui.columnAttribRight[i].position = i;

    inWnd["FolderHistoryLeft" ](config.gui.folderHistoryLeft);
    inWnd["FolderHistoryRight"](config.gui.folderHistoryRight);
    inWnd["MaximumHistorySize"](config.gui.folderHistMax);
    inWnd["Perspective"       ](config.gui.guiPerspectiveLast);

    //external applications
    inGui["ExternalApplications"](config.gui.externelApplications);

    //load config file history
    inGui["LastConfigActive"](config.gui.lastUsedConfigFiles);
    inGui["ConfigHistory"](config.gui.cfgFileHistory);

    //last update check
    inGui["LastUpdateCheck"](config.gui.lastUpdateCheck);

    //batch specific global settings
    //XmlIn inBatch = in["Batch"];
}


template <class ConfigType>
void readConfig(const wxString& filename, XmlType type, ConfigType& config)
{
    if (!fileExists(toZ(filename)))
        throw FfsXmlError(wxString(_("File does not exist:")) + wxT("\n\"") + filename + wxT("\""));

    XmlDoc doc;
    loadXmlDocument(filename, doc);  //throw (FfsXmlError)

    if (getXmlType(doc) != type) //throw()
        throw FfsXmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\""));

    XmlIn in(doc);
    ::readConfig(in, config);

    if (in.errorsOccured())
        throw FfsXmlError(wxString(_("Error parsing configuration file:")) + wxT("\n\"") + filename + wxT("\"\n\n") +
                          getErrorMessageFormatted(in), FfsXmlError::WARNING);
}
}


void xmlAccess::readConfig(const wxString& filename, xmlAccess::XmlGuiConfig& config)
{
    ::readConfig(filename, XML_TYPE_GUI, config);
}


void xmlAccess::readConfig(const wxString& filename, xmlAccess::XmlBatchConfig& config)
{
    ::readConfig(filename, XML_TYPE_BATCH, config);
}


void xmlAccess::readConfig(xmlAccess::XmlGlobalSettings& config)
{
    ::readConfig(getGlobalConfigFile(), XML_TYPE_GLOBAL, config);
}


//################################################################################################
namespace
{
void writeConfig(const SyncConfig& syncCfg, XmlOut& out)
{
    out["Variant"](syncCfg.var);

    XmlOut outCustDir = out["CustomDirections"];
    outCustDir["LeftOnly"  ](syncCfg.custom.exLeftSideOnly);
    outCustDir["RightOnly" ](syncCfg.custom.exRightSideOnly);
    outCustDir["LeftNewer" ](syncCfg.custom.leftNewer);
    outCustDir["RightNewer"](syncCfg.custom.rightNewer);
    outCustDir["Different" ](syncCfg.custom.different);
    outCustDir["Conflict"  ](syncCfg.custom.conflict);
}


void writeConfig(const FilterConfig& filter, XmlOut& out)
{
    out["Include"](filter.includeFilter);
    out["Exclude"](filter.excludeFilter);

    out["TimeSpan"    ](filter.timeSpan);
    out["UnitTimeSpan"](filter.unitTimeSpan);

    out["SizeMin"    ](filter.sizeMin);
    out["UnitSizeMin"](filter.unitSizeMin);

    out["SizeMax"    ](filter.sizeMax);
    out["UnitSizeMax"](filter.unitSizeMax);
}


void writeConfigFolderPair(const FolderPairEnh& enhPair, XmlOut& out)
{
    XmlOut outPair = out.ref().addChild("Pair");

    //read folder pairs
    outPair["Left" ](enhPair.leftDirectory);
    outPair["Right"](enhPair.rightDirectory);

    //###########################################################
    //alternate sync configuration (optional)
    if (enhPair.altSyncConfig.get())
    {
        XmlOut outAlt = outPair["AlternateSyncConfig"];

        //read sync configuration
        writeConfig(enhPair.altSyncConfig->syncConfiguration, outAlt);

        outAlt["DeletionPolicy"      ](enhPair.altSyncConfig->handleDeletion);
        outAlt["CustomDeletionFolder"](enhPair.altSyncConfig->customDeletionDirectory);
    }

    //###########################################################
    //alternate filter configuration
    XmlOut outFilter = outPair["LocalFilter"];
    writeConfig(enhPair.localFilter, outFilter);
}


void writeConfig(const MainConfiguration& mainCfg, XmlOut& out)
{
    XmlOut outCmp  = out["MainConfig"]["Comparison"];

    //write compare variant
    outCmp["Variant"](mainCfg.compareVar);

    //include symbolic links at all?
    outCmp["HandleSymlinks"](mainCfg.handleSymlinks);

    //###########################################################
    XmlOut outSync = out["MainConfig"]["SyncConfig"];

    //write sync configuration
    writeConfig(mainCfg.syncConfiguration, outSync);
    //###########################################################

    //misc
    outSync["DeletionPolicy"      ](mainCfg.handleDeletion);
    outSync["CustomDeletionFolder"](mainCfg.customDeletionDirectory);
    //###########################################################

    XmlOut outFilter = out["MainConfig"]["GlobalFilter"];
    //write filter settings
    writeConfig(mainCfg.globalFilter, outFilter);

    //###########################################################
    //write all folder pairs

    XmlOut outFp = out["MainConfig"]["FolderPairs"];

    //write first folder pair
    writeConfigFolderPair(mainCfg.firstPair, outFp);

    //write additional folder pairs
    std::for_each(mainCfg.additionalPairs.begin(), mainCfg.additionalPairs.end(),
    [&](const FolderPairEnh& fp) { writeConfigFolderPair(fp, outFp); });
}


void writeConfig(const XmlGuiConfig& config, XmlOut& out)
{
    writeConfig(config.mainCfg, out); //write main config

    //write GUI specific config data
    XmlOut outGuiCfg = out["GuiConfig"];

    outGuiCfg["HideFiltered"     ](config.hideFilteredElements);
    outGuiCfg["HandleError"      ](config.handleError);
    outGuiCfg["SyncPreviewActive"](config.syncPreviewEnabled);
}

void writeConfig(const XmlBatchConfig& config, XmlOut& out)
{

    writeConfig(config.mainCfg, out); //write main config

    //write GUI specific config data
    XmlOut outBatchCfg = out["BatchConfig"];

    outBatchCfg["Silent"          ](config.silent);
    outBatchCfg["LogfileDirectory"](config.logFileDirectory);
    outBatchCfg["LogfileCountMax" ](config.logFileCountMax);
    outBatchCfg["HandleError"     ](config.handleError);
}


void writeConfig(const XmlGlobalSettings& config, XmlOut& out)
{
    XmlOut outShared = out["Shared"];

    //write program language setting
    outShared["Language"](config.programLanguage);

    outShared["CopyLockedFiles"      ](config.copyLockedFiles);
    outShared["CopyFilePermissions"  ](config.copyFilePermissions);
    outShared["TransactionalFileCopy"](config.transactionalFileCopy);
    outShared["VerifyCopiedFiles"    ](config.verifyFileCopy);

    //max. allowed file time deviation
    outShared["FileTimeTolerance"](config.fileTimeTolerance);

    XmlOut outOpt = outShared["ShowOptionalDialogs"];
    outOpt["CheckForDependentFolders"     ](config.optDialogs.warningDependentFolders);
    outOpt["CheckForMultipleWriteAccess"  ](config.optDialogs.warningMultiFolderWriteAccess);
    outOpt["CheckForSignificantDifference"](config.optDialogs.warningSignificantDifference);
    outOpt["CheckForFreeDiskSpace"](config.optDialogs.warningNotEnoughDiskSpace);
    outOpt["CheckForUnresolvedConflicts"](config.optDialogs.warningUnresolvedConflicts);
    outOpt["NotifyDatabaseError"](config.optDialogs.warningSyncDatabase);
    outOpt["CheckMissingRecycleBin"](config.optDialogs.warningRecyclerMissing);
    outOpt["PopupOnConfigChange"](config.optDialogs.popupOnConfigChange);
    outOpt["SummaryBeforeSync"  ](config.optDialogs.showSummaryBeforeSync);

    //gui specific global settings (optional)
    XmlOut outGui = out["Gui"];
    XmlOut outWnd = outGui["Windows"]["Main"];

    //write application window size and position
    outWnd["Width"    ](config.gui.dlgSize.x);
    outWnd["Height"   ](config.gui.dlgSize.y);
    outWnd["PosX"     ](config.gui.dlgPos.x);
    outWnd["PosY"     ](config.gui.dlgPos.y);
    outWnd["Maximized"](config.gui.isMaximized);

    outWnd["MaxFolderPairsVisible"](config.gui.maxFolderPairsVisible);

    outWnd["ManualDeletionOnBothSides"](config.gui.deleteOnBothSides);
    outWnd["ManualDeletionUseRecycler"](config.gui.useRecyclerForManualDeletion);
    outWnd["RespectCaseOnSearch"      ](config.gui.textSearchRespectCase);

    //###########################################################

    //write column attributes
    XmlOut outColLeft = outWnd["LeftColumns"];

    outColLeft.attribute("AutoAdjust",    config.gui.autoAdjustColumnsLeft);
    outColLeft.attribute("ShowFileIcons", config.gui.showFileIconsLeft);

    outColLeft(config.gui.columnAttribLeft);

    //###########################################################
    XmlOut outColRight = outWnd["RightColumns"];

    outColRight.attribute("AutoAdjust",    config.gui.autoAdjustColumnsRight);
    outColRight.attribute("ShowFileIcons", config.gui.showFileIconsRight);

    outColRight(config.gui.columnAttribRight);

    outWnd["FolderHistoryLeft" ](config.gui.folderHistoryLeft);
    outWnd["FolderHistoryRight"](config.gui.folderHistoryRight);
    outWnd["MaximumHistorySize"](config.gui.folderHistMax);
    outWnd["Perspective"       ](config.gui.guiPerspectiveLast);

    //external applications
    outGui["ExternalApplications"](config.gui.externelApplications);

    //load config file history
    outGui["LastConfigActive"](config.gui.lastUsedConfigFiles);
    outGui["ConfigHistory"](config.gui.cfgFileHistory);

    //last update check
    outGui["LastUpdateCheck"](config.gui.lastUpdateCheck);

    //batch specific global settings
    //XmlOut outBatch = out["Batch"];
}


template <class ConfigType>
void writeConfig(const ConfigType& config, XmlType type, const wxString& filename)
{
    XmlDoc doc("FreeFileSync");
    setXmlType(doc, type); //throw()

    XmlOut out(doc);
    writeConfig(config, out);

    saveXmlDocument(doc, filename); //throw (FfsXmlError)
}
}

void xmlAccess::writeConfig(const XmlGuiConfig& config, const wxString& filename)
{
    ::writeConfig(config, XML_TYPE_GUI, filename); //throw (FfsXmlError)
}


void xmlAccess::writeConfig(const XmlBatchConfig& config, const wxString& filename)
{
    ::writeConfig(config, XML_TYPE_BATCH, filename); //throw (FfsXmlError)
}


void xmlAccess::writeConfig(const XmlGlobalSettings& config)
{
    ::writeConfig(config, XML_TYPE_GLOBAL, getGlobalConfigFile()); //throw (FfsXmlError)
}
