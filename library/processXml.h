#ifndef PROCESSXML_H_INCLUDED
#define PROCESSXML_H_INCLUDED

#include "../FreeFileSync.h"
#include "tinyxml/tinyxml.h"
#include <wx/intl.h>

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
        vector<FolderPair> directoryPairs;

        bool hideFilteredElements;
    };


    struct XmlBatchConfig
    {
        XmlBatchConfig() :  silent(false) {}

        MainConfiguration mainCfg;
        vector<FolderPair> directoryPairs;

        bool silent;
    };


    struct XmlGlobalSettings
    {
        struct _Global
        {
            _Global() :
                    programLanguage(wxLocale::GetSystemLanguage()),
#ifdef FFS_WIN
                    dstCheckActive(true),
#endif
                    folderDependCheckActive(true) {}

            int programLanguage;
#ifdef FFS_WIN
            bool dstCheckActive;
#endif
            bool folderDependCheckActive;
        } global;

        struct _Gui
        {
            _Gui() :
                    widthNotMaximized(wxDefaultCoord),
                    heightNotMaximized(wxDefaultCoord),
                    posXNotMaximized(wxDefaultCoord),
                    posYNotMaximized(wxDefaultCoord),
                    isMaximized(false) {}

            int widthNotMaximized;
            int heightNotMaximized;
            int posXNotMaximized;
            int posYNotMaximized;
            bool isMaximized;
            vector<int> columnWidthLeft;
            vector<int> columnWidthRight;
        } gui;

        //struct _Batch
    };


    XmlGuiConfig readGuiConfig(const wxString& filename);
    XmlBatchConfig readBatchConfig(const wxString& filename);
    XmlGlobalSettings readGlobalSettings();   //used for both GUI and batch mode, independent from configuration instance

    void writeGuiConfig(const wxString& filename, const XmlGuiConfig& inputCfg);
    void writeBatchConfig(const wxString& filename, const XmlBatchConfig& inputCfg);
    void writeGlobalSettings(const XmlGlobalSettings& inputCfg);

}


#endif // PROCESSXML_H_INCLUDED
