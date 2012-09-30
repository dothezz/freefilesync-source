// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "process_xml.h"
#include <utility>
#include <zenxml/xml.h>
#include "ffs_paths.h"
#include <zen/file_handling.h>
#include <zen/file_io.h>
#include "xml_base.h"

using namespace zen;
using namespace xmlAccess; //functionally needed for correct overload resolution!!!

using namespace std::rel_ops;


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


XmlType xmlAccess::getXmlType(const Zstring& filename) //throw()
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
    return toWx(zen::getConfigDir()) + wxT("GlobalSettings.xml");
}


void xmlAccess::OptionalDialogs::resetDialogs()
{
    warningDependentFolders        = true;
    warningFolderPairRaceCondition = true;
    warningSignificantDifference   = true;
    warningNotEnoughDiskSpace      = true;
    warningUnresolvedConflicts     = true;
    warningDatabaseError           = true;
    warningRecyclerMissing         = true;
    popupOnConfigChange            = true;
    confirmSyncStart               = true;
}


xmlAccess::XmlGuiConfig xmlAccess::convertBatchToGui(const xmlAccess::XmlBatchConfig& batchCfg)
{
    XmlGuiConfig output;
    output.mainCfg = batchCfg.mainCfg;

    switch (batchCfg.handleError)
    {
        case ON_ERROR_EXIT:
        case ON_ERROR_POPUP:
            output.handleError = ON_GUIERROR_POPUP;
            break;
        case ON_ERROR_IGNORE:
            output.handleError = ON_GUIERROR_IGNORE;
            break;
    }
    return output;
}


xmlAccess::XmlBatchConfig xmlAccess::convertGuiToBatch(const xmlAccess::XmlGuiConfig& guiCfg, const Zstring& referenceFile)
{
    XmlBatchConfig output; //use default batch-settings

    switch (guiCfg.handleError)
    {
        case ON_GUIERROR_POPUP:
            output.handleError = ON_ERROR_POPUP;
            break;
        case ON_GUIERROR_IGNORE:
            output.handleError = ON_ERROR_IGNORE;
            break;
    }

    //try to take over batch-specific settings from reference
    if (!referenceFile.empty() && getXmlType(referenceFile) == XML_TYPE_BATCH)
        try
        {
            std::vector<Zstring> filenames;
            filenames.push_back(referenceFile);
            mergeConfigs(filenames, output); //throw xmlAccess::FfsXmlError
        }
        catch (xmlAccess::FfsXmlError&) {}

    output.mainCfg = guiCfg.mainCfg;
    return output;
}


xmlAccess::MergeType xmlAccess::getMergeType(const std::vector<Zstring>& filenames)  //throw()
{
    bool guiCfgExists   = false;
    bool batchCfgExists = false;

    for (auto iter = filenames.begin(); iter != filenames.end(); ++iter)
    {
        switch (xmlAccess::getXmlType(*iter)) //throw()
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
XmlCfg loadCfgImpl(const Zstring& filename, std::unique_ptr<xmlAccess::FfsXmlError>& warning) //throw xmlAccess::FfsXmlError
{
    XmlCfg cfg;
    try
    {
        xmlAccess::readConfig(filename, cfg); //throw xmlAccess::FfsXmlError
    }
    catch (const xmlAccess::FfsXmlError& e)
    {
        if (e.getSeverity() == xmlAccess::FfsXmlError::FATAL)
            throw;
        else
            warning.reset(new xmlAccess::FfsXmlError(e));
    }
    return cfg;
}


template <class XmlCfg>
void mergeConfigFilesImpl(const std::vector<Zstring>& filenames, XmlCfg& config) //throw xmlAccess::FfsXmlError
{
    assert(!filenames.empty());
    if (filenames.empty())
        return;

    std::vector<zen::MainConfiguration> mainCfgs;
    std::unique_ptr<FfsXmlError> savedWarning;

    std::for_each(filenames.begin(), filenames.end(),
                  [&](const Zstring& filename)
    {
        switch (getXmlType(filename))
        {
            case XML_TYPE_GUI:
                mainCfgs.push_back(loadCfgImpl<XmlGuiConfig>(filename, savedWarning).mainCfg); //throw xmlAccess::FfsXmlError
                break;

            case XML_TYPE_BATCH:
                mainCfgs.push_back(loadCfgImpl<XmlBatchConfig>(filename, savedWarning).mainCfg); //throw xmlAccess::FfsXmlError
                break;

            case XML_TYPE_GLOBAL:
            case XML_TYPE_OTHER:
                if (!fileExists(filename))
                    throw FfsXmlError(replaceCpy(_("Cannot find file %x."), L"%x", fmtFileName(filename)));
                else
                    throw FfsXmlError(replaceCpy(_("File %x does not contain a valid configuration."), L"%x", fmtFileName(filename)));
        }
    });

    try //...to init all non-"mainCfg" settings with first config file
    {
        xmlAccess::readConfig(filenames[0], config); //throw xmlAccess::FfsXmlError
    }
    catch (xmlAccess::FfsXmlError&) {}

    config.mainCfg = merge(mainCfgs);

    if (savedWarning.get()) //"re-throw" exception
        throw* savedWarning;
}
}


void xmlAccess::mergeConfigs(const std::vector<Zstring>& filenames, XmlGuiConfig& config) //throw FfsXmlError
{
    mergeConfigFilesImpl(filenames, config); //throw FfsXmlError
}


void xmlAccess::mergeConfigs(const std::vector<Zstring>& filenames, XmlBatchConfig& config) //throw FfsXmlError
{
    mergeConfigFilesImpl(filenames, config);   //throw FfsXmlError
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
void writeText(const FileIconSize& value, std::string& output)
{
    switch (value)
    {
        case ICON_SIZE_SMALL:
            output = "Small";
            break;
        case ICON_SIZE_MEDIUM:
            output = "Medium";
            break;
        case ICON_SIZE_LARGE:
            output = "Large";
            break;
    }
}


template <> inline
bool readText(const std::string& input, FileIconSize& value)
{
    std::string tmp = input;
    zen::trim(tmp);
    if (tmp == "Small")
        value = ICON_SIZE_SMALL;
    else if (tmp == "Medium")
        value = ICON_SIZE_MEDIUM;
    else if (tmp == "Large")
        value = ICON_SIZE_LARGE;
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
            output = "Permanent";
            break;
        case DELETE_TO_RECYCLER:
            output = "RecycleBin";
            break;
        case DELETE_TO_VERSIONING:
            output = "Versioning";
            break;
    }
}

template <> inline
bool readText(const std::string& input, DeletionPolicy& value)
{
    std::string tmp = input;
    zen::trim(tmp);
    //------------------
    warn_static("remove after migration?")
    if (tmp == "DeletePermanently")//obsolete name
        value = DELETE_PERMANENTLY;
    else if (tmp == "MoveToRecycleBin")//obsolete name
        value = DELETE_TO_RECYCLER;
    else if (tmp == "MoveToCustomDirectory")//obsolete name
        value = DELETE_TO_VERSIONING;
    else
        //------------------
        if (tmp == "Permanent")
            value = DELETE_PERMANENTLY;
        else if (tmp == "RecycleBin")
            value = DELETE_TO_RECYCLER;
        else if (tmp == "Versioning")
            value = DELETE_TO_VERSIONING;
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
        case UTIME_TODAY:
            output = "Today";
            break;
        case UTIME_THIS_MONTH:
            output = "Month";
            break;
        case UTIME_THIS_YEAR:
            output = "Year";
            break;
        case UTIME_LAST_X_DAYS:
            output = "x-days";
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
    else if (tmp == "Today")
        value = UTIME_TODAY;
    else if (tmp == "Month")
        value = UTIME_THIS_MONTH;
    else if (tmp == "Year")
        value = UTIME_THIS_YEAR;
    else if (tmp == "x-days")
        value = UTIME_LAST_X_DAYS;
    else
        return false;
    return true;
}


template <> inline
void writeText(const ColumnTypeRim& value, std::string& output)
{
    switch (value)
    {
        case COL_TYPE_DIRECTORY:
            output = "Base";
            break;
        case COL_TYPE_FULL_PATH:
            output = "Full";
            break;
        case COL_TYPE_REL_PATH:
            output = "Rel";
            break;
        case COL_TYPE_FILENAME:
            output = "Name";
            break;
        case COL_TYPE_SIZE:
            output = "Size";
            break;
        case COL_TYPE_DATE:
            output = "Date";
            break;
        case COL_TYPE_EXTENSION:
            output = "Ext";
            break;
    }
}

template <> inline
bool readText(const std::string& input, ColumnTypeRim& value)
{
    std::string tmp = input;
    zen::trim(tmp);
    if (tmp == "Base")
        value = COL_TYPE_DIRECTORY;
    else if (tmp == "Full")
        value = COL_TYPE_FULL_PATH;
    else if (tmp == "Rel")
        value = COL_TYPE_REL_PATH;
    else if (tmp == "Name")
        value = COL_TYPE_FILENAME;
    else if (tmp == "Size")
        value = COL_TYPE_SIZE;
    else if (tmp == "Date")
        value = COL_TYPE_DATE;
    else if (tmp == "Ext")
        value = COL_TYPE_EXTENSION;
    else
        return false;
    return true;
}


template <> inline
void writeText(const ColumnTypeNavi& value, std::string& output)
{
    switch (value)
    {
        case COL_TYPE_NAVI_BYTES:
            output = "Bytes";
            break;
        case COL_TYPE_NAVI_DIRECTORY:
            output = "Tree";
            break;
    }
}

template <> inline
bool readText(const std::string& input, ColumnTypeNavi& value)
{
    std::string tmp = input;
    zen::trim(tmp);
    if (tmp == "Bytes")
        value = COL_TYPE_NAVI_BYTES;
    else if (tmp == "Tree")
        value = COL_TYPE_NAVI_DIRECTORY;
    else
        return false;
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
void writeText(const DirectionConfig::Variant& value, std::string& output)
{
    switch (value)
    {
        case DirectionConfig::AUTOMATIC:
            output = "Automatic";
            break;
        case DirectionConfig::MIRROR:
            output = "Mirror";
            break;
        case DirectionConfig::UPDATE:
            output = "Update";
            break;
        case DirectionConfig::CUSTOM:
            output = "Custom";
            break;
    }
}

template <> inline
bool readText(const std::string& input, DirectionConfig::Variant& value)
{
    std::string tmp = input;
    zen::trim(tmp);
    if (tmp == "Automatic")
        value = DirectionConfig::AUTOMATIC;
    else if (tmp == "Mirror")
        value = DirectionConfig::MIRROR;
    else if (tmp == "Update")
        value = DirectionConfig::UPDATE;
    else if (tmp == "Custom")
        value = DirectionConfig::CUSTOM;
    else
        return false;
    return true;
}


template <> inline
bool readStruc(const XmlElement& input, ColumnAttributeRim& value)
{
    XmlIn in(input);
    bool rv1 = in.attribute("Type",    value.type_);
    bool rv2 = in.attribute("Visible", value.visible_);
    bool rv3 = in.attribute("Width",   value.offset_); //offset == width if stretch is 0
    bool rv4 = in.attribute("Stretch", value.stretch_);
    return rv1 && rv2 && rv3 && rv4;
}

template <> inline
void writeStruc(const ColumnAttributeRim& value, XmlElement& output)
{
    XmlOut out(output);
    out.attribute("Type",     value.type_);
    out.attribute("Visible",  value.visible_);
    out.attribute("Width",    value.offset_);
    out.attribute("Stretch",  value.stretch_);
}


template <> inline
bool readStruc(const XmlElement& input, ColumnAttributeNavi& value)
{
    XmlIn in(input);
    bool rv1 = in.attribute("Type",    value.type_);
    bool rv2 = in.attribute("Visible", value.visible_);
    bool rv3 = in.attribute("Width",   value.offset_); //offset == width if stretch is 0
    bool rv4 = in.attribute("Stretch", value.stretch_);
    return rv1 && rv2 && rv3 && rv4;
}

template <> inline
void writeStruc(const ColumnAttributeNavi& value, XmlElement& output)
{
    XmlOut out(output);
    out.attribute("Type",    value.type_);
    out.attribute("Visible", value.visible_);
    out.attribute("Width",   value.offset_);
    out.attribute("Stretch", value.stretch_);
}
}


namespace
{
void readConfig(const XmlIn& in, CompConfig& cmpConfig)
{
    in["Variant"       ](cmpConfig.compareVar);
    in["HandleSymlinks"](cmpConfig.handleSymlinks);
}


void readConfig(const XmlIn& in, DirectionConfig& directCfg)
{
    in["Variant"](directCfg.var);

    XmlIn inCustDir = in["CustomDirections"];
    inCustDir["LeftOnly"  ](directCfg.custom.exLeftSideOnly);
    inCustDir["RightOnly" ](directCfg.custom.exRightSideOnly);
    inCustDir["LeftNewer" ](directCfg.custom.leftNewer);
    inCustDir["RightNewer"](directCfg.custom.rightNewer);
    inCustDir["Different" ](directCfg.custom.different);
    inCustDir["Conflict"  ](directCfg.custom.conflict);
}


void readConfig(const XmlIn& in, SyncConfig& syncCfg)
{
    readConfig(in, syncCfg.directionCfg);

    in["DeletionPolicy"](syncCfg.handleDeletion);

    warn_static("remove after migration?")
    if (in["CustomDeletionFolder"])
    {
        in["CustomDeletionFolder"](syncCfg.versioningDirectory);//obsolete name
        syncCfg.versionCountLimit = -1; //new parameter
    }
    else
    {
        in["VersioningFolder"](syncCfg.versioningDirectory);
        in["VersioningFolder"].attribute("Limit", syncCfg.versionCountLimit);
    }
}


void readConfig(const XmlIn& in, FilterConfig& filter)
{
    in["Include"](filter.includeFilter);
    in["Exclude"](filter.excludeFilter);

    in["TimeSpan"](filter.timeSpan);
    warn_static("remove after migration?")
    if (in["UnitTimeSpan"]) in["UnitTimeSpan"](filter.unitTimeSpan);//obsolete name
    else
        in["TimeSpan"].attribute("Type", filter.unitTimeSpan);

    in["SizeMin"](filter.sizeMin);
    if (in["UnitSizeMin"]) in["UnitSizeMin"](filter.unitSizeMin);//obsolete name
    else
        in["SizeMin"].attribute("Unit", filter.unitSizeMin);

    in["SizeMax"](filter.sizeMax);
    if (in["UnitSizeMax"]) in["UnitSizeMax"](filter.unitSizeMax);//obsolete name
    else
        in["SizeMax"].attribute("Unit", filter.unitSizeMax);
}


void readConfig(const XmlIn& in, FolderPairEnh& enhPair)
{
    //read folder pairs
    in["Left" ](enhPair.leftDirectory);
    in["Right"](enhPair.rightDirectory);

    //###########################################################
    //alternate comp configuration (optional)
    if (XmlIn inAltCmp = in["CompareConfig"])
    {
        CompConfig altCmpCfg;
        readConfig(inAltCmp, altCmpCfg);

        enhPair.altCmpConfig = std::make_shared<CompConfig>(altCmpCfg);
    }
    //###########################################################
    //alternate sync configuration (optional)
    if (XmlIn inAltSync = in["SyncConfig"])
    {
        SyncConfig altSyncCfg;
        readConfig(inAltSync, altSyncCfg);

        enhPair.altSyncConfig = std::make_shared<SyncConfig>(altSyncCfg);
    }

    //###########################################################
    //alternate filter configuration
    if (XmlIn inLocFilter = in["LocalFilter"])
        readConfig(inLocFilter, enhPair.localFilter);
}


void readConfig(const XmlIn& in, MainConfiguration& mainCfg)
{
    //read compare settings
    XmlIn inMain = in["MainConfig"];

    readConfig(inMain["Comparison"], mainCfg.cmpConfig);
    //###########################################################

    //read sync configuration
    readConfig(inMain["SyncConfig"], mainCfg.syncCfg);
    //###########################################################

    //read filter settings
    readConfig(inMain["GlobalFilter"], mainCfg.globalFilter);

    //###########################################################
    //read all folder pairs
    mainCfg.additionalPairs.clear();

    bool firstIter = true;
    for (XmlIn inPair = inMain["FolderPairs"]["Pair"]; inPair; inPair.next())
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

    warn_static("remove after migration?")
    if (inMain["ExecuteWhenFinished"]) inMain["ExecuteWhenFinished"](mainCfg.onCompletion); //obsolete name
    else
        inMain["OnCompletion"](mainCfg.onCompletion);
}


void readConfig(const XmlIn& in, xmlAccess::XmlGuiConfig& config)
{
    ::readConfig(in, config.mainCfg); //read main config

    //read GUI specific config data
    XmlIn inGuiCfg = in["GuiConfig"];

    warn_static("remove after migration?")
    if (inGuiCfg["HideFiltered"     ]) //obsolete name
    {
        inGuiCfg["HideFiltered"     ](config.showFilteredElements);
        config.showFilteredElements = !config.showFilteredElements;
    }
    else
        inGuiCfg["ShowFiltered"](config.showFilteredElements);

    inGuiCfg["HandleError"      ](config.handleError);
    inGuiCfg["SyncPreviewActive"](config.showSyncAction);
}


void readConfig(const XmlIn& in, xmlAccess::XmlBatchConfig& config)
{
    ::readConfig(in, config.mainCfg); //read main config

    //read GUI specific config data
    XmlIn inBatchCfg = in["BatchConfig"];

    inBatchCfg["HandleError"     ](config.handleError);
    inBatchCfg["ShowProgress"    ](config.showProgress);

    warn_static("remove after migration?")
    if (inBatchCfg["LogfileDirectory"]) inBatchCfg["LogfileDirectory"](config.logFileDirectory); //obsolete name
    else
        inBatchCfg["LogfileFolder"](config.logFileDirectory);

    if (inBatchCfg["LogfileCountMax" ]) inBatchCfg["LogfileCountMax"](config.logfilesCountLimit); //obsolete name
    else
        inBatchCfg["LogfileFolder"].attribute("Limit", config.logfilesCountLimit);
}


void readConfig(const XmlIn& in, XmlGlobalSettings& config)
{
    XmlIn inShared = in["Shared"];

    //try to read program language setting
    inShared["Language"](config.programLanguage);

    inShared["CopyLockedFiles"      ](config.copyLockedFiles);
    inShared["CopyFilePermissions"  ](config.copyFilePermissions);
    inShared["TransactionalFileCopy"](config.transactionalFileCopy);
    inShared["LockDirectoriesDuringSync"](config.createLockFile);
    inShared["VerifyCopiedFiles"        ](config.verifyFileCopy);
    inShared["RunWithBackgroundPriority"](config.runWithBackgroundPriority);

    //max. allowed file time deviation
    inShared["FileTimeTolerance"](config.fileTimeTolerance);

    XmlIn inOpt = inShared["OptionalDialogs"];
    inOpt["WarnUnresolvedConflicts"    ](config.optDialogs.warningUnresolvedConflicts);
    inOpt["WarnNotEnoughDiskSpace"     ](config.optDialogs.warningNotEnoughDiskSpace);
    inOpt["WarnSignificantDifference"  ](config.optDialogs.warningSignificantDifference);
    inOpt["WarnRecycleBinNotAvailable" ](config.optDialogs.warningRecyclerMissing);
    inOpt["WarnDatabaseError"          ](config.optDialogs.warningDatabaseError);
    inOpt["WarnDependentFolders"       ](config.optDialogs.warningDependentFolders);
    inOpt["WarnFolderPairRaceCondition"](config.optDialogs.warningFolderPairRaceCondition);
    inOpt["PromptSaveConfig"           ](config.optDialogs.popupOnConfigChange);
    inOpt["ConfirmSyncStart"           ](config.optDialogs.confirmSyncStart);

    warn_static("remove after migration?")
    if (!inOpt)
    {
        inOpt = inShared["ShowOptionalDialogs"];
        inOpt["CheckForDependentFolders"     ](config.optDialogs.warningDependentFolders);
        inOpt["CheckForMultipleWriteAccess"  ](config.optDialogs.warningFolderPairRaceCondition);
        inOpt["CheckForSignificantDifference"](config.optDialogs.warningSignificantDifference);
        inOpt["CheckForFreeDiskSpace"](config.optDialogs.warningNotEnoughDiskSpace);
        inOpt["CheckForUnresolvedConflicts"](config.optDialogs.warningUnresolvedConflicts);
        inOpt["NotifyDatabaseError"   ](config.optDialogs.warningDatabaseError);
        inOpt["CheckMissingRecycleBin"](config.optDialogs.warningRecyclerMissing);
        inOpt["PopupOnConfigChange"   ](config.optDialogs.popupOnConfigChange);
        inOpt["SummaryBeforeSync"     ](config.optDialogs.confirmSyncStart);
    }

    //gui specific global settings (optional)
    XmlIn inGui = in["Gui"];
    XmlIn inWnd = inGui["MainDialog"];

    //read application window size and position
    inWnd.attribute("Width",     config.gui.dlgSize.x);
    inWnd.attribute("Height",    config.gui.dlgSize.y);
    inWnd.attribute("PosX",      config.gui.dlgPos.x);
    inWnd.attribute("PosY",      config.gui.dlgPos.y);
    inWnd.attribute("Maximized", config.gui.isMaximized);

    XmlIn inManualDel = inWnd["ManualDeletion"];
    inManualDel.attribute("DeleteOnBothSides", config.gui.deleteOnBothSides);
    inManualDel.attribute("UseRecycler"      , config.gui.useRecyclerForManualDeletion);

    inWnd["CaseSensitiveSearch"    ](config.gui.textSearchRespectCase);
    inWnd["MaxFolderPairsVisible"](config.gui.maxFolderPairsVisible);

    //###########################################################
    //read column attributes
    XmlIn inColNavi = inWnd["OverviewColumns"];
    inColNavi(config.gui.columnAttribNavi);

    inColNavi.attribute("ShowPercentage", config.gui.showPercentBar);
    inColNavi.attribute("SortByColumn",   config.gui.naviLastSortColumn);
    inColNavi.attribute("SortAscending",  config.gui.naviLastSortAscending);

    XmlIn inMainGrid = inWnd["MainGrid"];
    inMainGrid.attribute("ShowIcons",  config.gui.showIcons);
    inMainGrid.attribute("IconSize",   config.gui.iconSize);
    inMainGrid.attribute("SashOffset", config.gui.sashOffset);


    XmlIn inColLeft = inMainGrid["ColumnsLeft"];
    inColLeft(config.gui.columnAttribLeft);

    XmlIn inColRight = inMainGrid["ColumnsRight"];
    inColRight(config.gui.columnAttribRight);
    //###########################################################

    inWnd["Layout"](config.gui.guiPerspectiveLast);

    //load config file history
    warn_static("remove after migration?")
    if (inGui["LastConfigActive"]) inGui["LastConfigActive"](config.gui.lastUsedConfigFiles);//obsolete name
    else
        inGui["LastUsedConfig"](config.gui.lastUsedConfigFiles);

    inGui["ConfigHistory"](config.gui.cfgFileHistory);
    inGui["ConfigHistory"].attribute("MaxSize", config.gui.cfgFileHistMax);

    inGui["FolderHistoryLeft" ](config.gui.folderHistoryLeft);
    inGui["FolderHistoryRight"](config.gui.folderHistoryRight);
    inGui["FolderHistoryLeft"].attribute("MaxSize", config.gui.folderHistMax);

    inGui["OnCompletionHistory"](config.gui.onCompletionHistory);
    inGui["OnCompletionHistory"].attribute("MaxSize", config.gui.onCompletionHistoryMax);

    //external applications
    inGui["ExternalApplications"](config.gui.externelApplications);


    warn_static("remove after migration?")
    //convert new internal macro naming convention: %name -> %item_path%; %dir -> %item_folder%
    for (auto iter = config.gui.externelApplications.begin(); iter != config.gui.externelApplications.end(); ++iter)
    {
        replace(iter->second, L"%nameCo", L"%item2_path%"); //unambiguous "Co" names first
        replace(iter->second, L"%dirCo" , L"%item2_folder%");
        replace(iter->second, L"%name"  , L"%item_path%");
        replace(iter->second, L"%dir"   , L"%item_folder%");
    }


    //last update check
    inGui["LastUpdateCheck"](config.gui.lastUpdateCheck);

    //batch specific global settings
    //XmlIn inBatch = in["Batch"];
}


template <class ConfigType>
void readConfig(const Zstring& filename, XmlType type, ConfigType& config)
{
    XmlDoc doc;
    loadXmlDocument(filename, doc);  //throw FfsXmlError

    if (getXmlType(doc) != type) //throw()
        throw FfsXmlError(replaceCpy(_("File %x does not contain a valid configuration."), L"%x", fmtFileName(filename)));

    XmlIn in(doc);
    ::readConfig(in, config);

    if (in.errorsOccured())
        throw FfsXmlError(replaceCpy(_("Configuration file %x loaded partially only."), L"%x", fmtFileName(filename)) + L"\n\n" +
                          getErrorMessageFormatted(in), FfsXmlError::WARNING);
}
}


void xmlAccess::readConfig(const Zstring& filename, xmlAccess::XmlGuiConfig& config)
{
    ::readConfig(filename, XML_TYPE_GUI, config);
}


void xmlAccess::readConfig(const Zstring& filename, xmlAccess::XmlBatchConfig& config)
{
    ::readConfig(filename, XML_TYPE_BATCH, config);
}


void xmlAccess::readConfig(xmlAccess::XmlGlobalSettings& config)
{
    ::readConfig(toZ(getGlobalConfigFile()), XML_TYPE_GLOBAL, config);
}


//################################################################################################
namespace
{
void writeConfig(const CompConfig& cmpConfig, XmlOut& out)
{
    out["Variant"       ](cmpConfig.compareVar);
    out["HandleSymlinks"](cmpConfig.handleSymlinks);
}


void writeConfig(const DirectionConfig& directCfg, XmlOut& out)
{
    out["Variant"](directCfg.var);

    XmlOut outCustDir = out["CustomDirections"];
    outCustDir["LeftOnly"  ](directCfg.custom.exLeftSideOnly);
    outCustDir["RightOnly" ](directCfg.custom.exRightSideOnly);
    outCustDir["LeftNewer" ](directCfg.custom.leftNewer);
    outCustDir["RightNewer"](directCfg.custom.rightNewer);
    outCustDir["Different" ](directCfg.custom.different);
    outCustDir["Conflict"  ](directCfg.custom.conflict);
}


void writeConfig(const SyncConfig& syncCfg, XmlOut& out)
{
    writeConfig(syncCfg.directionCfg, out);

    out["DeletionPolicy"  ](syncCfg.handleDeletion);
    out["VersioningFolder"](syncCfg.versioningDirectory);
    out["VersioningFolder"].attribute("Limit", syncCfg.versionCountLimit);
}


void writeConfig(const FilterConfig& filter, XmlOut& out)
{
    out["Include"](filter.includeFilter);
    out["Exclude"](filter.excludeFilter);

    out["TimeSpan"](filter.timeSpan);
    out["TimeSpan"].attribute("Type", filter.unitTimeSpan);

    out["SizeMin"](filter.sizeMin);
    out["SizeMin"].attribute("Unit", filter.unitSizeMin);

    out["SizeMax"](filter.sizeMax);
    out["SizeMax"].attribute("Unit", filter.unitSizeMax);
}


void writeConfigFolderPair(const FolderPairEnh& enhPair, XmlOut& out)
{
    XmlOut outPair = out.ref().addChild("Pair");

    //read folder pairs
    outPair["Left" ](enhPair.leftDirectory);
    outPair["Right"](enhPair.rightDirectory);

    //###########################################################
    //alternate comp configuration (optional)
    if (enhPair.altCmpConfig.get())
    {
        XmlOut outAlt = outPair["CompareConfig"];
        writeConfig(*enhPair.altCmpConfig, outAlt);
    }
    //###########################################################
    //alternate sync configuration (optional)
    if (enhPair.altSyncConfig.get())
    {
        XmlOut outAltSync = outPair["SyncConfig"];
        writeConfig(*enhPair.altSyncConfig, outAltSync);
    }

    //###########################################################
    //alternate filter configuration
    if (enhPair.localFilter != FilterConfig()) //don't spam .ffs_gui file with default filter entries
    {
        XmlOut outFilter = outPair["LocalFilter"];
        writeConfig(enhPair.localFilter, outFilter);
    }
}


void writeConfig(const MainConfiguration& mainCfg, XmlOut& out)
{
    XmlOut outMain = out["MainConfig"];

    XmlOut outCmp = outMain["Comparison"];

    writeConfig(mainCfg.cmpConfig, outCmp);
    //###########################################################

    XmlOut outSync = outMain["SyncConfig"];

    writeConfig(mainCfg.syncCfg, outSync);
    //###########################################################

    XmlOut outFilter = outMain["GlobalFilter"];
    //write filter settings
    writeConfig(mainCfg.globalFilter, outFilter);

    //###########################################################
    //write all folder pairs

    XmlOut outFp = outMain["FolderPairs"];

    //write first folder pair
    writeConfigFolderPair(mainCfg.firstPair, outFp);

    //write additional folder pairs
    std::for_each(mainCfg.additionalPairs.begin(), mainCfg.additionalPairs.end(),
    [&](const FolderPairEnh& fp) { writeConfigFolderPair(fp, outFp); });

    outMain["OnCompletion"](mainCfg.onCompletion);
}


void writeConfig(const XmlGuiConfig& config, XmlOut& out)
{
    writeConfig(config.mainCfg, out); //write main config

    //write GUI specific config data
    XmlOut outGuiCfg = out["GuiConfig"];

    outGuiCfg["ShowFiltered"     ](config.showFilteredElements);
    outGuiCfg["HandleError"      ](config.handleError);
    outGuiCfg["SyncPreviewActive"](config.showSyncAction);
}

void writeConfig(const XmlBatchConfig& config, XmlOut& out)
{

    writeConfig(config.mainCfg, out); //write main config

    //write GUI specific config data
    XmlOut outBatchCfg = out["BatchConfig"];

    outBatchCfg["HandleError"    ](config.handleError);
    outBatchCfg["ShowProgress"   ](config.showProgress);
    outBatchCfg["LogfileFolder"  ](config.logFileDirectory);
    outBatchCfg["LogfileFolder"].attribute("Limit", config.logfilesCountLimit);
}


void writeConfig(const XmlGlobalSettings& config, XmlOut& out)
{
    XmlOut outShared = out["Shared"];

    //write program language setting
    outShared["Language"](config.programLanguage);

    outShared["CopyLockedFiles"      ](config.copyLockedFiles);
    outShared["CopyFilePermissions"  ](config.copyFilePermissions);
    outShared["TransactionalFileCopy"](config.transactionalFileCopy);
    outShared["LockDirectoriesDuringSync"](config.createLockFile);
    outShared["VerifyCopiedFiles"        ](config.verifyFileCopy);
    outShared["RunWithBackgroundPriority"](config.runWithBackgroundPriority);

    //max. allowed file time deviation
    outShared["FileTimeTolerance"](config.fileTimeTolerance);

    XmlOut outOpt = outShared["OptionalDialogs"];
    outOpt["WarnUnresolvedConflicts"    ](config.optDialogs.warningUnresolvedConflicts);
    outOpt["WarnNotEnoughDiskSpace"     ](config.optDialogs.warningNotEnoughDiskSpace);
    outOpt["WarnSignificantDifference"  ](config.optDialogs.warningSignificantDifference);
    outOpt["WarnRecycleBinNotAvailable" ](config.optDialogs.warningRecyclerMissing);
    outOpt["WarnDatabaseError"          ](config.optDialogs.warningDatabaseError);
    outOpt["WarnDependentFolders"       ](config.optDialogs.warningDependentFolders);
    outOpt["WarnFolderPairRaceCondition"](config.optDialogs.warningFolderPairRaceCondition);
    outOpt["PromptSaveConfig"           ](config.optDialogs.popupOnConfigChange);
    outOpt["ConfirmSyncStart"           ](config.optDialogs.confirmSyncStart);

    //gui specific global settings (optional)
    XmlOut outGui = out["Gui"];
    XmlOut outWnd = outGui["MainDialog"];

    //write application window size and position
    outWnd.attribute("Width",     config.gui.dlgSize.x);
    outWnd.attribute("Height",    config.gui.dlgSize.y);
    outWnd.attribute("PosX",      config.gui.dlgPos.x);
    outWnd.attribute("PosY",      config.gui.dlgPos.y);
    outWnd.attribute("Maximized", config.gui.isMaximized);

    XmlOut outManualDel = outWnd["ManualDeletion"];
    outManualDel.attribute("DeleteOnBothSides", config.gui.deleteOnBothSides);
    outManualDel.attribute("UseRecycler"      , config.gui.useRecyclerForManualDeletion);

    outWnd["CaseSensitiveSearch"     ](config.gui.textSearchRespectCase);
    outWnd["MaxFolderPairsVisible"](config.gui.maxFolderPairsVisible);

    //###########################################################
    //write column attributes
    XmlOut outColNavi = outWnd["OverviewColumns"];
    outColNavi(config.gui.columnAttribNavi);

    outColNavi.attribute("ShowPercentage", config.gui.showPercentBar);
    outColNavi.attribute("SortByColumn",   config.gui.naviLastSortColumn);
    outColNavi.attribute("SortAscending",  config.gui.naviLastSortAscending);

    XmlOut outMainGrid = outWnd["MainGrid"];
    outMainGrid.attribute("ShowIcons",  config.gui.showIcons);
    outMainGrid.attribute("IconSize",   config.gui.iconSize);
    outMainGrid.attribute("SashOffset", config.gui.sashOffset);

    XmlOut outColLeft = outMainGrid["ColumnsLeft"];
    outColLeft(config.gui.columnAttribLeft);

    XmlOut outColRight = outMainGrid["ColumnsRight"];
    outColRight(config.gui.columnAttribRight);
    //###########################################################

    outWnd["Layout"](config.gui.guiPerspectiveLast);

    //load config file history
    outGui["LastUsedConfig"](config.gui.lastUsedConfigFiles);
    outGui["ConfigHistory" ](config.gui.cfgFileHistory);
    outGui["ConfigHistory"].attribute("MaxSize", config.gui.cfgFileHistMax);

    outGui["FolderHistoryLeft" ](config.gui.folderHistoryLeft);
    outGui["FolderHistoryRight"](config.gui.folderHistoryRight);
    outGui["FolderHistoryLeft" ].attribute("MaxSize", config.gui.folderHistMax);

    outGui["OnCompletionHistory"](config.gui.onCompletionHistory);
    outGui["OnCompletionHistory"].attribute("MaxSize", config.gui.onCompletionHistoryMax);

    //external applications
    outGui["ExternalApplications"](config.gui.externelApplications);

    //last update check
    outGui["LastUpdateCheck"](config.gui.lastUpdateCheck);

    //batch specific global settings
    //XmlOut outBatch = out["Batch"];
}


template <class ConfigType>
void writeConfig(const ConfigType& config, XmlType type, const Zstring& filename)
{
    XmlDoc doc("FreeFileSync");
    setXmlType(doc, type); //throw()

    XmlOut out(doc);
    writeConfig(config, out);

    saveXmlDocument(doc, filename); //throw FfsXmlError
}
}

void xmlAccess::writeConfig(const XmlGuiConfig& config, const Zstring& filename)
{
    ::writeConfig(config, XML_TYPE_GUI, filename); //throw FfsXmlError
}


void xmlAccess::writeConfig(const XmlBatchConfig& config, const Zstring& filename)
{
    ::writeConfig(config, XML_TYPE_BATCH, filename); //throw FfsXmlError
}


void xmlAccess::writeConfig(const XmlGlobalSettings& config)
{
    ::writeConfig(config, XML_TYPE_GLOBAL, toZ(getGlobalConfigFile())); //throw FfsXmlError
}


std::wstring xmlAccess::extractJobName(const Zstring& configFilename)
{
    const Zstring shortName = afterLast(configFilename, FILE_NAME_SEPARATOR); //returns the whole string if separator not found
    const Zstring jobName = beforeLast(shortName, Zstr('.')); //returns empty string if seperator not found
    return utfCvrtTo<std::wstring>(jobName.empty() ? shortName : jobName);
}
