#ifndef PROCESSXML_H_INCLUDED
#define PROCESSXML_H_INCLUDED

#include "../FreeFileSync.h"
#include "tinyxml/tinyxml.h"

using namespace FreeFileSync;


namespace xmlAccess
{
    enum XmlType
    {
        XML_GUI_CONFIG,
        XML_BATCH_CONFIG,
        XML_GLOBAL_SETTINGS,
        XML_OTHER
    };

    XmlType getXmlType(const wxString& filename);


    struct XmlGuiConfig
    {
        XmlGuiConfig() : hideFilteredElements(false) {} //initialize values

        MainConfiguration mainCfg;
        std::vector<FolderPair> directoryPairs;

        bool hideFilteredElements;
    };


    struct XmlBatchConfig
    {
        XmlBatchConfig() :  silent(false) {}

        MainConfiguration mainCfg;
        std::vector<FolderPair> directoryPairs;

        bool silent;
    };

    int retrieveSystemLanguage();


    struct XmlGlobalSettings
    {
//---------------------------------------------------------------------
//internal structures:
        enum ColumnTypes
        {
            FILENAME = 0,
            REL_PATH,
            SIZE,
            DATE
        };

        struct ColumnAttrib
        {
            ColumnTypes type;
            bool        visible;
            unsigned    position;
            int         width;
        };
        typedef std::vector<ColumnAttrib> ColumnAttributes;

//---------------------------------------------------------------------
        struct _Global
        {
            _Global() :
                    programLanguage(retrieveSystemLanguage()),
#ifdef FFS_WIN
                    handleDstOnFat32(true),
#endif
                    folderDependCheckActive(true) {}

            int programLanguage;
#ifdef FFS_WIN
            bool handleDstOnFat32;
#endif
            bool folderDependCheckActive;
        } global;

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
                    commandLineFileManager(wxT("explorer /select, %x"))
#elif defined FFS_LINUX
                    commandLineFileManager(wxT("konqueror \"%path\""))
#endif
            {}

            int widthNotMaximized;
            int heightNotMaximized;
            int posXNotMaximized;
            int posYNotMaximized;
            bool isMaximized;

            ColumnAttributes columnAttribLeft;
            ColumnAttributes columnAttribRight;
            std::vector<wxString> cfgFileHistory;
            wxString commandLineFileManager;
        } gui;

//---------------------------------------------------------------------
        //struct _Batch
    };


    inline
    bool sortByType(const XmlGlobalSettings::ColumnAttrib& a, const XmlGlobalSettings::ColumnAttrib& b)
    {
        return a.type < b.type;
    }


    inline
    bool sortByPositionOnly(const XmlGlobalSettings::ColumnAttrib& a, const XmlGlobalSettings::ColumnAttrib& b)
    {
        return a.position < b.position;
    }


    inline
    bool sortByPositionAndVisibility(const XmlGlobalSettings::ColumnAttrib& a, const XmlGlobalSettings::ColumnAttrib& b)
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
