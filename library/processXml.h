#ifndef PROCESSXML_H_INCLUDED
#define PROCESSXML_H_INCLUDED

#include "../structures.h"
#include "fileHandling.h"

namespace xmlAccess
{
    extern const wxString LAST_CONFIG_FILE;
    extern const wxString GLOBAL_CONFIG_FILE;

    enum OnError
    {
        ON_ERROR_POPUP,
        ON_ERROR_IGNORE,
        ON_ERROR_EXIT
    };

    enum XmlType
    {
        XML_GUI_CONFIG,
        XML_BATCH_CONFIG,
        XML_GLOBAL_SETTINGS,
        XML_OTHER
    };

    enum ColumnTypes
    {
        FILENAME = 0,
        REL_PATH,
        SIZE,
        DATE,
        FULL_NAME,
        DIRECTORY
    };
    const unsigned COLUMN_TYPE_COUNT = 6;

    struct ColumnAttrib
    {
        ColumnTypes type;
        bool        visible;
        unsigned    position;
        int         width;
    };
    typedef std::vector<ColumnAttrib> ColumnAttributes;

    XmlType getXmlType(const wxString& filename);
//---------------------------------------------------------------------

    struct XmlGuiConfig
    {
        XmlGuiConfig() :
                hideFilteredElements(false),
                ignoreErrors(false) {} //initialize values

        FreeFileSync::MainConfiguration mainCfg;
        std::vector<FreeFileSync::FolderPair> directoryPairs;

        bool hideFilteredElements;
        bool ignoreErrors; //reaction on error situation during synchronization

        bool operator==(const XmlGuiConfig& other) const
        {
            return mainCfg              == other.mainCfg &&
                   directoryPairs       == other.directoryPairs &&
                   hideFilteredElements == other.hideFilteredElements &&
                   ignoreErrors         == other.ignoreErrors;
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
    bool supportForSymbolicLinks();


    struct XmlGlobalSettings
    {
//---------------------------------------------------------------------
        struct _Shared
        {
            _Shared() :
                    programLanguage(retrieveSystemLanguage()),
                    fileTimeTolerance(2),  //default 2s: FAT vs NTFS
                    traverseDirectorySymlinks(false),
                    copyFileSymlinks(supportForSymbolicLinks()),
                    lastUpdateCheck(0)
            {
                resetWarnings();
            }

            int programLanguage;
            unsigned fileTimeTolerance; //max. allowed file time deviation
            bool traverseDirectorySymlinks;
            bool copyFileSymlinks; //copy symbolic link instead of target file
            long lastUpdateCheck; //time of last update check

            //warnings
            void resetWarnings();

            bool warningDependentFolders;
            bool warningSignificantDifference;
            bool warningNotEnoughDiskSpace;
        } shared;

//---------------------------------------------------------------------
        struct _Gui
        {
            _Gui() :
                    widthNotMaximized(wxDefaultCoord),
                    heightNotMaximized(wxDefaultCoord),
                    posXNotMaximized(wxDefaultCoord),
                    posYNotMaximized(wxDefaultCoord),
                    isMaximized(false),
#ifdef FFS_WIN
                    commandLineFileManager(wxT("explorer /select, %name")),
#elif defined FFS_LINUX
                    commandLineFileManager(wxT("konqueror \"%path\"")),
#endif
                    cfgHistoryMax(10),
                    folderHistLeftMax(12),
                    folderHistRightMax(12),
                    deleteOnBothSides(false),
                    useRecyclerForManualDeletion(FreeFileSync::recycleBinExists()), //enable if OS supports it; else user will have to activate first and then get an error message
                    showFileIcons(true),
                    popupOnConfigChange(true) {}

            int widthNotMaximized;
            int heightNotMaximized;
            int posXNotMaximized;
            int posYNotMaximized;
            bool isMaximized;

            ColumnAttributes columnAttribLeft;
            ColumnAttributes columnAttribRight;

            wxString commandLineFileManager;

            std::vector<wxString> cfgFileHistory;
            unsigned int cfgHistoryMax;

            std::vector<wxString> folderHistoryLeft;
            unsigned int folderHistLeftMax;

            std::vector<wxString> folderHistoryRight;
            unsigned int folderHistRightMax;

            bool deleteOnBothSides;
            bool useRecyclerForManualDeletion;
            bool showFileIcons;
            bool popupOnConfigChange;
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


    XmlGuiConfig readGuiConfig(const wxString& filename);
    XmlBatchConfig readBatchConfig(const wxString& filename);
    XmlGlobalSettings readGlobalSettings();   //used for both GUI and batch mode, independent from configuration instance

    void writeGuiConfig(const wxString& filename, const XmlGuiConfig& outputCfg);
    void writeBatchConfig(const wxString& filename, const XmlBatchConfig& outputCfg);
    void writeGlobalSettings(const XmlGlobalSettings& outputCfg);

}


#endif // PROCESSXML_H_INCLUDED
