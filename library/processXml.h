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

        FreeFileSync::MainConfiguration mainCfg;

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
        XmlBatchConfig() : silent(false), handleError(ON_ERROR_POPUP) {}

        FreeFileSync::MainConfiguration mainCfg;

        bool silent;
        OnError handleError;       //reaction on error situation during synchronization
        wxString logFileDirectory; //
    };

    int retrieveSystemLanguage();
    bool recycleBinAvailable();


    struct OptionalDialogs
    {
        OptionalDialogs()
        {
            resetDialogs();
        }

        void resetDialogs();

        bool warningDependentFolders;
        bool warningSignificantDifference;
        bool warningNotEnoughDiskSpace;
        bool warningUnresolvedConflicts;
        bool popupOnConfigChange;
        bool showSummaryBeforeSync;
    };


    struct XmlGlobalSettings
    {
//---------------------------------------------------------------------
        //Shared (GUI/BATCH) settings
        XmlGlobalSettings() :
                programLanguage(retrieveSystemLanguage()),
                ignoreOneHourDiff(false),
                lastUpdateCheck(0) {}

        int programLanguage;
        bool ignoreOneHourDiff; //ignore +/- 1 hour due to DST change
        long lastUpdateCheck;   //time of last update check

        OptionalDialogs optDialogs;

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
                    cfgHistoryMax(10),
                    folderHistLeftMax(12),
                    folderHistRightMax(12),
                    selectedTabBottomLeft(0),
                    deleteOnBothSides(false),
                    useRecyclerForManualDeletion(recycleBinAvailable()), //enable if OS supports it; else user will have to activate first and then get an error message
                    showFileIconsLeft(true),
                    showFileIconsRight(true)
            {
#ifdef FFS_WIN
                externelApplications.push_back(std::make_pair(wxT("Open with Explorer"), wxT("explorer /select, %name")));
#elif defined FFS_LINUX
                externelApplications.push_back(std::make_pair(wxT("Open with Konqueror"), wxT("konqueror \"%dir\"")));
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

    void readGuiOrBatchConfig(const wxString& filename, XmlGuiConfig& config);  //throw (xmlAccess::XmlError);

    void writeGuiConfig(     const XmlGuiConfig&      outputCfg, const wxString& filename); //throw (xmlAccess::XmlError);
    void writeBatchConfig(   const XmlBatchConfig&    outputCfg, const wxString& filename); //throw (xmlAccess::XmlError);
    void writeGlobalSettings(const XmlGlobalSettings& outputCfg);                           //throw (xmlAccess::XmlError);
}


#endif // PROCESSXML_H_INCLUDED
