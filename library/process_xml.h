// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef PROCESSXML_H_INCLUDED
#define PROCESSXML_H_INCLUDED

#include <wx/gdicmn.h>
#include "../structures.h"
#include "../shared/xml_error.h"
#include "../shared/localization.h"

namespace xmlAccess
{
enum XmlType
{
    XML_TYPE_GUI,
    XML_TYPE_BATCH,
    XML_TYPE_GLOBAL,
    XML_TYPE_OTHER
};

XmlType getXmlType(const wxString& filename); //throw()


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

enum ColumnTypes
{
    DIRECTORY, //this needs to begin with 0 and be continuous (some code relies on it)
    FULL_PATH,
    REL_PATH,
    FILENAME,
    SIZE,
    DATE,
    EXTENSION
};
const size_t COLUMN_TYPE_COUNT = 7;

struct ColumnAttrib
{
    ColumnTypes type;
    bool         visible;
    size_t       position;
    int          width;
};
typedef std::vector<ColumnAttrib> ColumnAttributes;


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
        return mainCfg                == other.mainCfg               &&
               hideFilteredElements   == other.hideFilteredElements  &&
               handleError            == other.handleError           &&
               syncPreviewEnabled     == other.syncPreviewEnabled;
    }

    bool operator!=(const XmlGuiConfig& other) const
    {
        return !(*this == other);
    }
};


struct XmlBatchConfig
{
    XmlBatchConfig() :
        silent(false),
        logFileCountMax(200),
        handleError(ON_ERROR_POPUP) {}

    zen::MainConfiguration mainCfg;

    bool silent;
    wxString logFileDirectory;
    size_t logFileCountMax;
    OnError handleError;       //reaction on error situation during synchronization
};


struct OptionalDialogs
{
    OptionalDialogs()
    {
        resetDialogs();
    }

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


wxString getGlobalConfigFile();

struct XmlGlobalSettings
{
    //---------------------------------------------------------------------
    //Shared (GUI/BATCH) settings
    XmlGlobalSettings() :
        programLanguage(zen::retrieveSystemLanguage()),
        copyLockedFiles(true),
        copyFilePermissions(false),
        fileTimeTolerance(2),  //default 2s: FAT vs NTFS
        verifyFileCopy(false),
		transactionalFileCopy(true) {}

    int programLanguage;
    bool copyLockedFiles;          //VSS usage
    bool copyFilePermissions;

    size_t fileTimeTolerance; //max. allowed file time deviation
    bool verifyFileCopy;   //verify copied files
	bool transactionalFileCopy;

    OptionalDialogs optDialogs;

    //---------------------------------------------------------------------
    struct _Gui
    {
        _Gui() :
            dlgPos(wxDefaultCoord, wxDefaultCoord),
            dlgSize(wxDefaultCoord, wxDefaultCoord),
            isMaximized(false),
            maxFolderPairsVisible(6),
            autoAdjustColumnsLeft(false),
            autoAdjustColumnsRight(false),
            folderHistMax(12),
            deleteOnBothSides(false),
            useRecyclerForManualDeletion(true), //enable if OS supports it; else user will have to activate first and then get an error message
#ifdef FFS_WIN
            textSearchRespectCase(false),
#elif defined FFS_LINUX
            textSearchRespectCase(true),
#endif
            showFileIconsLeft(true),
            showFileIconsRight(true),
            lastUpdateCheck(0)
        {
            //default external apps will be translated "on the fly"!!!
#ifdef FFS_WIN
            externelApplications.push_back(std::make_pair(wxT("Show in Explorer"), //mark for extraction: _("Show in Explorer")
                                                          wxT("explorer /select, \"%name\"")));
            externelApplications.push_back(std::make_pair(wxT("Open with default application"), //mark for extraction: _("Open with default application")
                                                          wxT("\"%name\"")));
#elif defined FFS_LINUX
            externelApplications.push_back(std::make_pair(wxT("Browse directory"), //mark for extraction: _("Browse directory")
                                                          wxT("xdg-open \"%dir\"")));
            externelApplications.push_back(std::make_pair(wxT("Open with default application"), //mark for extraction: _("Open with default application")
                                                          wxT("xdg-open \"%name\"")));
#endif
        }

        wxPoint dlgPos;
        wxSize dlgSize;
        bool isMaximized;

        int maxFolderPairsVisible;

        ColumnAttributes columnAttribLeft;
        ColumnAttributes columnAttribRight;

        bool autoAdjustColumnsLeft;
        bool autoAdjustColumnsRight;

        ExternalApps externelApplications;

        std::vector<wxString> cfgFileHistory;
        std::vector<wxString> lastUsedConfigFiles;

        std::vector<wxString> folderHistoryLeft;
        std::vector<wxString> folderHistoryRight;
        unsigned int folderHistMax;

        bool deleteOnBothSides;
        bool useRecyclerForManualDeletion;
        bool textSearchRespectCase;
        bool showFileIconsLeft;
        bool showFileIconsRight;

        long lastUpdateCheck;          //time of last update check

        wxString guiPerspectiveLast; //used by wxAuiManager
    } gui;

    //---------------------------------------------------------------------
    //struct _Batch
};


inline
bool sortByType(const ColumnAttrib& a, const ColumnAttrib& b)
{
    return a.type < b.type;
}


inline
bool sortByPositionOnly(const ColumnAttrib& a, const ColumnAttrib& b)
{
    return a.position < b.position;
}


inline
bool sortByPositionAndVisibility(const ColumnAttrib& a, const ColumnAttrib& b)
{
    if (a.visible == false) //hidden elements shall appear at end of vector
        return false;
    if (b.visible == false)
        return true;
    return a.position < b.position;
}

void readConfig(const wxString& filename, XmlGuiConfig&      config); //throw (xmlAccess::FfsXmlError)
void readConfig(const wxString& filename, XmlBatchConfig&    config); //throw (xmlAccess::FfsXmlError)
void readConfig(                          XmlGlobalSettings& config); //throw (xmlAccess::FfsXmlError)

void writeConfig(const XmlGuiConfig&      config, const wxString& filename); //throw (xmlAccess::FfsXmlError)
void writeConfig(const XmlBatchConfig&    config, const wxString& filename); //throw (xmlAccess::FfsXmlError)
void writeConfig(const XmlGlobalSettings& config);                           //throw (xmlAccess::FfsXmlError)

//config conversion utilities
XmlGuiConfig   convertBatchToGui(const XmlBatchConfig& batchCfg);
XmlBatchConfig convertGuiToBatch(const XmlGuiConfig&   guiCfg);


//convert (multiple) *.ffs_gui, *.ffs_batch files or combinations of both into target config structure:
enum MergeType
{
    MERGE_GUI,       //pure gui config files
    MERGE_BATCH,     //  " batch   "
    MERGE_GUI_BATCH, //gui and batch files
    MERGE_OTHER
};
MergeType getMergeType(const std::vector<wxString>& filenames);   //throw ()

void convertConfig(const std::vector<wxString>& filenames, XmlGuiConfig&   config); //throw (xmlAccess::FfsXmlError)
void convertConfig(const std::vector<wxString>& filenames, XmlBatchConfig& config); //throw (xmlAccess::FfsXmlError)
}


#endif // PROCESSXML_H_INCLUDED
