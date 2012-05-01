// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef PROCESSXML_H_INCLUDED
#define PROCESSXML_H_INCLUDED

#include <wx/gdicmn.h>
#include "../structures.h"
#include "xml_base.h"
#include "localization.h"
#include "../ui/column_attr.h"

namespace xmlAccess
{
enum XmlType
{
    XML_TYPE_GUI,
    XML_TYPE_BATCH,
    XML_TYPE_GLOBAL,
    XML_TYPE_OTHER
};

XmlType getXmlType(const Zstring& filename); //throw()


enum OnError
{
    ON_ERROR_POPUP,
    ON_ERROR_IGNORE,
    ON_ERROR_EXIT
};

enum OnGuiError
{
    ON_GUIERROR_POPUP,
    ON_GUIERROR_IGNORE
};

typedef wxString Description;
typedef wxString Commandline;
typedef std::vector<std::pair<Description, Commandline> > ExternalApps;

//---------------------------------------------------------------------
struct XmlGuiConfig
{
    XmlGuiConfig() :
        hideFilteredElements(false),
        handleError(ON_GUIERROR_POPUP),
        syncPreviewEnabled(true) {} //initialize values

    zen::MainConfiguration mainCfg;

    bool hideFilteredElements;
    OnGuiError handleError; //reaction on error situation during synchronization
    bool syncPreviewEnabled;

    bool operator==(const XmlGuiConfig& other) const
    {
        return mainCfg              == other.mainCfg               &&
               hideFilteredElements == other.hideFilteredElements  &&
               handleError          == other.handleError           &&
               syncPreviewEnabled   == other.syncPreviewEnabled;
    }

    bool operator!=(const XmlGuiConfig& other) const { return !(*this == other); }
};


struct XmlBatchConfig
{
    XmlBatchConfig() :
        showProgress(true),
        logFileCountMax(200),
        handleError(ON_ERROR_POPUP) {}

    zen::MainConfiguration mainCfg;

    bool showProgress;
    wxString logFileDirectory;
    size_t logFileCountMax;
    OnError handleError;       //reaction on error situation during synchronization
};


struct OptionalDialogs
{
    OptionalDialogs() { resetDialogs();}

    void resetDialogs();

    bool warningDependentFolders;
    bool warningMultiFolderWriteAccess;
    bool warningSignificantDifference;
    bool warningNotEnoughDiskSpace;
    bool warningUnresolvedConflicts;
    bool warningSyncDatabase;
    bool warningRecyclerMissing;
    bool popupOnConfigChange;
    bool showSummaryBeforeSync;
};


enum FileIconSize
{
    ICON_SIZE_SMALL,
    ICON_SIZE_MEDIUM,
    ICON_SIZE_LARGE
};


wxString getGlobalConfigFile();

struct XmlGlobalSettings
{
    //---------------------------------------------------------------------
    //Shared (GUI/BATCH) settings
    XmlGlobalSettings() :
        programLanguage(zen::retrieveSystemLanguage()),
        copyLockedFiles(true),
        copyFilePermissions(false),
        runWithBackgroundPriority(false),
        fileTimeTolerance(2),  //default 2s: FAT vs NTFS
        verifyFileCopy(false),
        transactionalFileCopy(true),
        createLockFile(true) {}

    int programLanguage;
    bool copyLockedFiles;          //VSS usage
    bool copyFilePermissions;

    bool runWithBackgroundPriority;

    size_t fileTimeTolerance; //max. allowed file time deviation
    bool verifyFileCopy;   //verify copied files
    bool transactionalFileCopy;
    bool createLockFile;

    OptionalDialogs optDialogs;

    //---------------------------------------------------------------------
    struct Gui
    {
        Gui() :
            dlgPos(wxDefaultCoord, wxDefaultCoord),
            dlgSize(wxDefaultCoord, wxDefaultCoord),
            isMaximized(false),
            maxFolderPairsVisible(6),
            columnAttribNavi (zen::getDefaultColumnAttributesNavi()),
            columnAttribLeft (zen::getDefaultColumnAttributesLeft()),
            columnAttribRight(zen::getDefaultColumnAttributesRight()),
            naviLastSortColumn(zen::defaultValueLastSortColumn),
            naviLastSortAscending(zen::defaultValueLastSortAscending),
            showPercentBar(zen::defaultValueShowPercentage),
            folderHistMax(15),
            onCompletionHistoryMax(8),
            deleteOnBothSides(false),
            useRecyclerForManualDeletion(true), //enable if OS supports it; else user will have to activate first and then get an error message
#ifdef FFS_WIN
            textSearchRespectCase(false),
#elif defined FFS_LINUX
            textSearchRespectCase(true),
#endif
            showIcons(true),
            iconSize(ICON_SIZE_SMALL),
            lastUpdateCheck(0)
        {
            //default external apps will be translated "on the fly"!!! First entry will be used for [Enter] or mouse double-click!
#ifdef FFS_WIN
            externelApplications.push_back(std::make_pair(L"Show in Explorer", //mark for extraction: _("Show in Explorer")
                                                          L"explorer /select, \"%name\""));
            externelApplications.push_back(std::make_pair(L"Open with default application", //mark for extraction: _("Open with default application")
                                                          L"\"%name\""));
#elif defined FFS_LINUX
            externelApplications.push_back(std::make_pair(L"Browse directory", //mark for extraction: _("Browse directory")
                                                          L"xdg-open \"%dir\""));
            externelApplications.push_back(std::make_pair(L"Open with default application", //mark for extraction: _("Open with default application")
                                                          L"xdg-open \"%name\""));
#endif
        }

        wxPoint dlgPos;
        wxSize dlgSize;
        bool isMaximized;

        int maxFolderPairsVisible;

        std::vector<zen::ColumnAttributeNavi> columnAttribNavi; //compressed view/navigation
        std::vector<zen::ColumnAttributeRim>  columnAttribLeft;
        std::vector<zen::ColumnAttributeRim>  columnAttribRight;

        zen::ColumnTypeNavi naviLastSortColumn; //remember sort on navigation panel
        bool naviLastSortAscending; //

        bool showPercentBar; //in navigation panel

        ExternalApps externelApplications;

        std::vector<wxString> cfgFileHistory;
        std::vector<wxString> lastUsedConfigFiles;

        std::vector<Zstring> folderHistoryLeft;
        std::vector<Zstring> folderHistoryRight;
        unsigned int folderHistMax;

        std::vector<std::wstring> onCompletionHistory;
        size_t onCompletionHistoryMax;

        bool deleteOnBothSides;
        bool useRecyclerForManualDeletion;
        bool textSearchRespectCase;

        bool showIcons;
        FileIconSize iconSize;

        long lastUpdateCheck;          //time of last update check

        wxString guiPerspectiveLast; //used by wxAuiManager
    } gui;

    //---------------------------------------------------------------------
    //struct Batch
};

void readConfig(const Zstring& filename, XmlGuiConfig&      config); //throw FfsXmlError
void readConfig(const Zstring& filename, XmlBatchConfig&    config); //throw FfsXmlError
void readConfig(                         XmlGlobalSettings& config); //throw FfsXmlError

void writeConfig(const XmlGuiConfig&      config, const Zstring& filename); //throw FfsXmlError
void writeConfig(const XmlBatchConfig&    config, const Zstring& filename); //throw FfsXmlError
void writeConfig(const XmlGlobalSettings& config);                          //throw FfsXmlError

//config conversion utilities
XmlGuiConfig   convertBatchToGui(const XmlBatchConfig& batchCfg);
XmlBatchConfig convertGuiToBatch(const XmlGuiConfig& guiCfg, const Zstring& referenceFile);


//convert (multiple) *.ffs_gui, *.ffs_batch files or combinations of both into target config structure:
enum MergeType
{
    MERGE_GUI,       //pure gui config files
    MERGE_BATCH,     //  " batch   "
    MERGE_GUI_BATCH, //gui and batch files
    MERGE_OTHER
};
MergeType getMergeType(const std::vector<Zstring>& filenames);   //throw ()

void convertConfig(const std::vector<Zstring>& filenames, XmlGuiConfig&   config); //throw xmlAccess::FfsXmlError
void convertConfig(const std::vector<Zstring>& filenames, XmlBatchConfig& config); //throw xmlAccess::FfsXmlError

wxString extractJobName(const wxString& configFilename);
}


#endif // PROCESSXML_H_INCLUDED
