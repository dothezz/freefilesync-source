// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef PROCESSXML_H_INCLUDED
#define PROCESSXML_H_INCLUDED

#include "../structures.h"
#include "../shared/xml_error.h"
#include "../shared/i18n.h"


namespace xmlAccess
{
enum OnError
{
    ON_ERROR_POPUP,
    ON_ERROR_IGNORE,
    ON_ERROR_EXIT
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
        ignoreErrors(false),
        syncPreviewEnabled(true) {} //initialize values

    ffs3::MainConfiguration mainCfg;

    bool hideFilteredElements;
    bool ignoreErrors; //reaction on error situation during synchronization
    bool syncPreviewEnabled;

    bool operator==(const XmlGuiConfig& other) const
    {
        return mainCfg                == other.mainCfg               &&
               hideFilteredElements   == other.hideFilteredElements  &&
               ignoreErrors           == other.ignoreErrors          &&
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

    ffs3::MainConfiguration mainCfg;

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
    bool popupOnConfigChange;
    bool showSummaryBeforeSync;
};


wxString getGlobalConfigFile();

struct XmlGlobalSettings
{
    //---------------------------------------------------------------------
    //Shared (GUI/BATCH) settings
    XmlGlobalSettings() :
        programLanguage(ffs3::retrieveSystemLanguage()),
        copyLockedFiles(true),
        copyFilePermissions(false),
        fileTimeTolerance(2),  //default 2s: FAT vs NTFS
        verifyFileCopy(false) {}

    int programLanguage;
    bool copyLockedFiles;          //VSS usage
    bool copyFilePermissions;

    size_t fileTimeTolerance; //max. allowed file time deviation
    bool verifyFileCopy;   //verify copied files

    OptionalDialogs optDialogs;

    //---------------------------------------------------------------------
    struct _Gui
    {
        _Gui() :
            widthNotMaximized( wxDefaultCoord),
            heightNotMaximized(wxDefaultCoord),
            posXNotMaximized(  wxDefaultCoord),
            posYNotMaximized(  wxDefaultCoord),
            isMaximized(false),
            autoAdjustColumnsLeft(false),
            autoAdjustColumnsRight(false),
            folderHistLeftMax(12),
            folderHistRightMax(12),
            deleteOnBothSides(false),
            useRecyclerForManualDeletion(true), //enable if OS supports it; else user will have to activate first and then get an error message
#ifdef FFS_WIN
            textSearchRespectCase(false),
#elif defined FFS_LINUX
            textSearchRespectCase(true),
#endif
            showFileIconsLeft(true),
            showFileIconsRight(true),
            addFolderPairCountMax(5),
            lastUpdateCheck(0)
        {
            //default external apps will be translated "on the fly"!!!
#ifdef FFS_WIN
            externelApplications.push_back(std::make_pair(wxT("Open with Explorer"), //mark for extraction: _("Open with Explorer")
                                                          wxT("explorer /select, \"%name\"")));
            externelApplications.push_back(std::make_pair(wxT("Open with default application"), //mark for extraction: _("Open with default application")
                                                          wxT("cmd /c start \"\" \"%name\"")));
#elif defined FFS_LINUX
            externelApplications.push_back(std::make_pair(wxT("Browse directory"), //mark for extraction: _("Browse directory")
                                                          wxT("xdg-open \"%dir\"")));
            externelApplications.push_back(std::make_pair(wxT("Open with default application"), //mark for extraction: _("Open with default application")
                                                          wxT("xdg-open \"%name\"")));
#endif
        }

        int widthNotMaximized;
        int heightNotMaximized;
        int posXNotMaximized;
        int posYNotMaximized;
        bool isMaximized;

        ColumnAttributes columnAttribLeft;
        ColumnAttributes columnAttribRight;

        bool autoAdjustColumnsLeft;
        bool autoAdjustColumnsRight;

        ExternalApps externelApplications;

        std::vector<wxString> cfgFileHistory;
        wxString lastUsedConfigFile;

        std::vector<wxString> folderHistoryLeft;
        unsigned int folderHistLeftMax;

        std::vector<wxString> folderHistoryRight;
        unsigned int folderHistRightMax;

        bool deleteOnBothSides;
        bool useRecyclerForManualDeletion;
        bool textSearchRespectCase;
        bool showFileIconsLeft;
        bool showFileIconsRight;

        size_t addFolderPairCountMax;
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

void readConfig(const wxString& filename, XmlGuiConfig&      config); //throw (xmlAccess::XmlError)
void readConfig(const wxString& filename, XmlBatchConfig&    config); //throw (xmlAccess::XmlError)
void readConfig(                          XmlGlobalSettings& config); //throw (xmlAccess::XmlError)

void writeConfig(const XmlGuiConfig&      outputCfg, const wxString& filename); //throw (xmlAccess::XmlError)
void writeConfig(const XmlBatchConfig&    outputCfg, const wxString& filename); //throw (xmlAccess::XmlError)
void writeConfig(const XmlGlobalSettings& outputCfg);                           //throw (xmlAccess::XmlError)

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

void convertConfig(const std::vector<wxString>& filenames, XmlGuiConfig&   config); //throw (xmlAccess::XmlError)
void convertConfig(const std::vector<wxString>& filenames, XmlBatchConfig& config); //throw (xmlAccess::XmlError)
}


#endif // PROCESSXML_H_INCLUDED
