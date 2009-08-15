#ifndef PROCESSXML_H_INCLUDED
#define PROCESSXML_H_INCLUDED

#include "../structures.h"

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
        DIRECTORY,
        FULL_PATH,
        REL_PATH,
        FILENAME,
        SIZE,
        DATE
    };
    const unsigned int COLUMN_TYPE_COUNT = 6;

    struct ColumnAttrib
    {
        ColumnTypes type;
        bool         visible;
        unsigned int position;
        int          width;
    };
    typedef std::vector<ColumnAttrib> ColumnAttributes;

//---------------------------------------------------------------------

    struct XmlGuiConfig
    {
        XmlGuiConfig() :
                hideFilteredElements(false),
                ignoreErrors(false),
                syncPreviewEnabled(true) {} //initialize values

        FreeFileSync::MainConfiguration mainCfg;
        std::vector<FreeFileSync::FolderPair> directoryPairs;

        bool hideFilteredElements;
        bool ignoreErrors; //reaction on error situation during synchronization
        bool syncPreviewEnabled;

        bool operator==(const XmlGuiConfig& other) const
        {
            return mainCfg                == other.mainCfg               &&
                   directoryPairs         == other.directoryPairs        &&
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
        XmlBatchConfig() : silent(false), handleError(ON_ERROR_POPUP) {}

        FreeFileSync::MainConfiguration mainCfg;
        std::vector<FreeFileSync::FolderPair> directoryPairs;

        bool silent;
        OnError handleError;       //reaction on error situation during synchronization
        wxString logFileDirectory; //
    };

    int retrieveSystemLanguage();
    bool recycleBinAvailable();


    struct WarningMessages
    {
        WarningMessages()
        {
            resetWarnings();
        }

        void resetWarnings();

        bool warningDependentFolders;
        bool warningSignificantDifference;
        bool warningNotEnoughDiskSpace;
        bool warningUnresolvedConflicts;
    };


    struct XmlGlobalSettings
    {
//---------------------------------------------------------------------
        //Shared (GUI/BATCH) settings
        XmlGlobalSettings() :
                programLanguage(retrieveSystemLanguage()),
                fileTimeTolerance(2),  //default 2s: FAT vs NTFS
                ignoreOneHourDiff(true),
                traverseDirectorySymlinks(false),
                copyFileSymlinks(true),
                lastUpdateCheck(0) {}

        int programLanguage;
        unsigned int fileTimeTolerance; //max. allowed file time deviation
        bool ignoreOneHourDiff; //ignore +/- 1 hour due to DST change
        bool traverseDirectorySymlinks;
        bool copyFileSymlinks; //copy symbolic link instead of target file
        long lastUpdateCheck;  //time of last update check

        WarningMessages warnings;

//---------------------------------------------------------------------
        struct _Gui
        {
            _Gui() :
                    widthNotMaximized(wxDefaultCoord),
                    heightNotMaximized(wxDefaultCoord),
                    posXNotMaximized(wxDefaultCoord),
                    posYNotMaximized(wxDefaultCoord),
                    isMaximized(false),
                    autoAdjustColumnsLeft(false),
                    autoAdjustColumnsRight(false),
#ifdef FFS_WIN
                    commandLineFileManager(wxT("explorer /select, %name")),
#elif defined FFS_LINUX
                    commandLineFileManager(wxT("konqueror \"%dir\"")),
#endif
                    cfgHistoryMax(10),
                    folderHistLeftMax(12),
                    folderHistRightMax(12),
                    selectedTabBottomLeft(0),
                    deleteOnBothSides(false),
                    useRecyclerForManualDeletion(recycleBinAvailable()), //enable if OS supports it; else user will have to activate first and then get an error message
                    showFileIconsLeft(true),
                    showFileIconsRight(true),
                    popupOnConfigChange(true),
                    showSummaryBeforeSync(true) {}

            int widthNotMaximized;
            int heightNotMaximized;
            int posXNotMaximized;
            int posYNotMaximized;
            bool isMaximized;

            ColumnAttributes columnAttribLeft;
            ColumnAttributes columnAttribRight;

            bool autoAdjustColumnsLeft;
            bool autoAdjustColumnsRight;

            wxString commandLineFileManager;

            std::vector<wxString> cfgFileHistory;
            unsigned int cfgHistoryMax;

            std::vector<wxString> folderHistoryLeft;
            unsigned int folderHistLeftMax;

            std::vector<wxString> folderHistoryRight;
            unsigned int folderHistRightMax;

            int selectedTabBottomLeft;

            bool deleteOnBothSides;
            bool useRecyclerForManualDeletion;
            bool showFileIconsLeft;
            bool showFileIconsRight;
            bool popupOnConfigChange;
            bool showSummaryBeforeSync;
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

    void readGuiConfig(  const wxString& filename, XmlGuiConfig&      config); //throw (xmlAccess::XmlError);
    void readBatchConfig(const wxString& filename, XmlBatchConfig&    config); //throw (xmlAccess::XmlError);
    void readGlobalSettings(                       XmlGlobalSettings& config); //throw (xmlAccess::XmlError);

    void writeGuiConfig(     const XmlGuiConfig&      outputCfg, const wxString& filename); //throw (xmlAccess::XmlError);
    void writeBatchConfig(   const XmlBatchConfig&    outputCfg, const wxString& filename); //throw (xmlAccess::XmlError);
    void writeGlobalSettings(const XmlGlobalSettings& outputCfg);                           //throw (xmlAccess::XmlError);
}


#endif // PROCESSXML_H_INCLUDED
