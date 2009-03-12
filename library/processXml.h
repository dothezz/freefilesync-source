#ifndef PROCESSXML_H_INCLUDED
#define PROCESSXML_H_INCLUDED

#include "../FreeFileSync.h"
#include "tinyxml/tinyxml.h"

using namespace FreeFileSync;


namespace xmlAccess
{
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
        FULL_NAME
    };
    const unsigned COLUMN_TYPE_COUNT = 5;

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
        XmlGuiConfig() : hideFilteredElements(false), ignoreErrors(false) {} //initialize values

        MainConfiguration mainCfg;
        std::vector<FolderPair> directoryPairs;

        bool hideFilteredElements;
        bool ignoreErrors; //reaction on error situation during synchronization
    };


    struct XmlBatchConfig
    {
        XmlBatchConfig() : silent(false), handleError(ON_ERROR_POPUP) {}

        MainConfiguration mainCfg;
        std::vector<FolderPair> directoryPairs;

        bool silent;
        OnError handleError; //reaction on error situation during synchronization
    };

    int retrieveSystemLanguage();


    struct XmlGlobalSettings
    {
//---------------------------------------------------------------------
        struct _Shared
        {
            _Shared() :
                    programLanguage(retrieveSystemLanguage()),
#ifdef FFS_WIN
                    handleDstOnFat32(true),
#endif
                    traverseSymbolicLinks(false)
            {
                resetWarnings();
            }

            int programLanguage;
#ifdef FFS_WIN
            bool handleDstOnFat32;
#endif
            bool traverseSymbolicLinks;

            //warnings
            void resetWarnings();

            bool warningDependentFolders;
            bool warningSignificantDifference;
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
                    cfgHistoryMaxItems(10),
                    deleteOnBothSides(false),
                    useRecyclerForManualDeletion(FreeFileSync::recycleBinExists()) //enable if OS supports it; else user will have to activate first and then get an error message
            {}

            int widthNotMaximized;
            int heightNotMaximized;
            int posXNotMaximized;
            int posYNotMaximized;
            bool isMaximized;

            ColumnAttributes columnAttribLeft;
            ColumnAttributes columnAttribRight;
            wxString commandLineFileManager;
            std::vector<wxString> cfgFileHistory;
            unsigned cfgHistoryMaxItems;
            bool deleteOnBothSides;
            bool useRecyclerForManualDeletion;
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
