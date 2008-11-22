#ifndef PROCESSXML_H_INCLUDED
#define PROCESSXML_H_INCLUDED

#include "../FreeFileSync.h"
#include "tinyxml/tinyxml.h"

namespace xmlAccess
{
    enum XmlType
    {
        XML_GUI_CONFIG,
        XML_BATCH_CONFIG,
        XML_OTHER
    };


    struct XmlMainConfig
    {
        MainConfiguration cfg;
        vector<FolderPair> directoryPairs;
    };


    struct XmlGuiConfig
    {
        XmlGuiConfig() :
                hideFilteredElements(false), //initialize values
                widthNotMaximized(-1),
                heightNotMaximized(-1),
                posXNotMaximized(-1),
                posYNotMaximized(-1),
                isMaximized(false) {}
        bool hideFilteredElements;
        int widthNotMaximized;
        int heightNotMaximized;
        int posXNotMaximized;
        int posYNotMaximized;
        bool isMaximized;
        vector<int> columnWidthLeft;
        vector<int> columnWidthRight;
    };


    struct XmlBatchConfig
    {
        XmlBatchConfig() :  silent(false) {}

        bool silent;
    };


    XmlType getXmlType(const wxString& filename);

    class XmlInput
    {
    public:
        XmlInput(const wxString& fileName, const XmlType type);
        ~XmlInput() {}

        bool loadedSuccessfully()
        {
            return loadSuccess;
        }

        //read basic FreefileSync settings (used by commandline and GUI), return true if ALL values have been retrieved successfully
        bool readXmlMainConfig(XmlMainConfig& outputCfg);
        //read additional gui settings, all values retrieved are optional, so check for initial values! (== -1)
        bool readXmlGuiConfig(XmlGuiConfig& outputCfg);
        //read additional batch settings, all values retrieved are optional
        bool readXmlBatchConfig(XmlBatchConfig& outputCfg);

    private:
//read
        bool readXmlElementValue(string& output, const TiXmlElement* parent, const string& name);
        bool readXmlElementValue(int& output, const TiXmlElement* parent, const string& name);
        bool readXmlElementValue(CompareVariant& output, const TiXmlElement* parent, const string& name);
        bool readXmlElementValue(SyncDirection& output, const TiXmlElement* parent, const string& name);
        bool readXmlElementValue(bool& output, const TiXmlElement* parent, const string& name);

        TiXmlDocument doc;
        bool loadSuccess;
    };


    class XmlOutput
    {
    public:
        XmlOutput(const wxString& fileName, const XmlType type);
        ~XmlOutput() {}

        bool writeToFile();

        //write basic FreefileSync settings (used by commandline and GUI), return true if everything was written successfully
        bool writeXmlMainConfig(const XmlMainConfig& inputCfg);
        //write additional gui settings
        bool writeXmlGuiConfig(const XmlGuiConfig& inputCfg);
        //write additional batch settings
        bool writeXmlBatchConfig(const XmlBatchConfig& inputCfg);

    private:
//write
        void addXmlElement(TiXmlElement* parent, const string& name, const string& value);
        void addXmlElement(TiXmlElement* parent, const string& name, const int value);
        void addXmlElement(TiXmlElement* parent, const string& name, const SyncDirection value);
        void addXmlElement(TiXmlElement* parent, const string& name, const bool value);

        TiXmlDocument doc;
        const wxString& m_fileName;
    };

}


#endif // PROCESSXML_H_INCLUDED
