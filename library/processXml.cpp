#include "processXml.h"
#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/intl.h>
#include "globalFunctions.h"

using namespace globalFunctions;
using namespace xmlAccess;

//small helper functions
bool readXmlElementValue(string& output, const TiXmlElement* parent, const string& name);
bool readXmlElementValue(int& output, const TiXmlElement* parent, const string& name);
bool readXmlElementValue(CompareVariant& output, const TiXmlElement* parent, const string& name);
bool readXmlElementValue(SyncConfiguration::Direction& output, const TiXmlElement* parent, const string& name);
bool readXmlElementValue(bool& output, const TiXmlElement* parent, const string& name);

void addXmlElement(TiXmlElement* parent, const string& name, const string& value);
void addXmlElement(TiXmlElement* parent, const string& name, const int value);
void addXmlElement(TiXmlElement* parent, const string& name, const SyncConfiguration::Direction value);
void addXmlElement(TiXmlElement* parent, const string& name, const bool value);


class XmlConfigInput
{
public:
    XmlConfigInput(const wxString& fileName, const XmlType type);
    ~XmlConfigInput() {}

    bool loadedSuccessfully()
    {
        return loadSuccess;
    }

    //read gui settings, all values retrieved are optional, so check for initial values! (== -1)
    bool readXmlGuiConfig(XmlGuiConfig& outputCfg);
    //read batch settings, all values retrieved are optional
    bool readXmlBatchConfig(XmlBatchConfig& outputCfg);
    //read global settings, valid for both GUI and batch mode, independent from configuration
    bool readXmlGlobalSettings(XmlGlobalSettings& outputCfg);

private:
    //read basic FreefileSync settings (used by commandline and GUI), return true if ALL values have been retrieved successfully
    bool readXmlMainConfig(MainConfiguration& mainCfg, vector<FolderPair>& directoryPairs);

    TiXmlDocument doc;
    bool loadSuccess;
};


class XmlConfigOutput
{
public:
    XmlConfigOutput(const wxString& fileName, const XmlType type);
    ~XmlConfigOutput() {}

    bool writeToFile();

    //write gui settings
    bool writeXmlGuiConfig(const XmlGuiConfig& inputCfg);
    //write batch settings
    bool writeXmlBatchConfig(const XmlBatchConfig& inputCfg);
    //write global settings
    bool writeXmlGlobalSettings(const XmlGlobalSettings& inputCfg);

private:
    //write basic FreefileSync settings (used by commandline and GUI), return true if everything was written successfully
    bool writeXmlMainConfig(const MainConfiguration& mainCfg, const vector<FolderPair>& directoryPairs);

    TiXmlDocument doc;
    const wxString& m_fileName;
};


XmlType xmlAccess::getXmlType(const wxString& filename)
{
    if (!wxFileExists(filename))
        return XML_OTHER;

    //workaround to get a FILE* from a unicode filename
    wxFFile configFile(filename, wxT("rb"));
    if (!configFile.IsOpened())
        return XML_OTHER;

    FILE* inputFile = configFile.fp();

    TiXmlDocument doc;
    if (!doc.LoadFile(inputFile)) //fails if inputFile is no proper XML
        return XML_OTHER;

    TiXmlElement* root = doc.RootElement();

    if (!root || (root->ValueStr() != string("FreeFileSync"))) //check for FFS configuration xml
        return XML_OTHER;

    const char* cfgType = root->Attribute("XmlType");
    if (!cfgType)
        return XML_OTHER;

    if (string(cfgType) == "BATCH")
        return XML_BATCH_CONFIG;
    else if (string(cfgType) == "GUI")
        return XML_GUI_CONFIG;
    else if (string(cfgType) == "GLOBAL")
        return XML_GLOBAL_SETTINGS;
    else
        return XML_OTHER;
}


XmlGuiConfig xmlAccess::readGuiConfig(const wxString& filename)
{
    //load XML
    XmlConfigInput inputFile(filename, XML_GUI_CONFIG);

    XmlGuiConfig outputCfg;

    if (!inputFile.loadedSuccessfully())
        throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + filename + wxT("\""));

    if (!inputFile.readXmlGuiConfig(outputCfg)) //read GUI layout configuration
        throw FileError(wxString(_("Error parsing configuration file:")) + wxT(" \"") + filename + wxT("\""));

    return outputCfg;
}


XmlBatchConfig xmlAccess::readBatchConfig(const wxString& filename)
{
    //load XML
    XmlConfigInput inputFile(filename, XML_BATCH_CONFIG);

    XmlBatchConfig outputCfg;

    if (!inputFile.loadedSuccessfully())
        throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + filename + wxT("\""));

    if (!inputFile.readXmlBatchConfig(outputCfg))
        throw FileError(wxString(_("Error parsing configuration file:")) + wxT(" \"") + filename + wxT("\""));

    return outputCfg;
}


XmlGlobalSettings xmlAccess::readGlobalSettings()
{
    //load XML
    XmlConfigInput inputFile(FreeFileSync::GLOBAL_CONFIG_FILE, XML_GLOBAL_SETTINGS);

    XmlGlobalSettings outputCfg;

    if (!inputFile.loadedSuccessfully())
        throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + FreeFileSync::GLOBAL_CONFIG_FILE + wxT("\""));

    if (!inputFile.readXmlGlobalSettings(outputCfg))
        throw FileError(wxString(_("Error parsing configuration file:")) + wxT(" \"") + FreeFileSync::GLOBAL_CONFIG_FILE + wxT("\""));

    return outputCfg;
}


void xmlAccess::writeGuiConfig(const wxString& filename, const XmlGuiConfig& inputCfg)
{
    XmlConfigOutput outputFile(filename, XML_GUI_CONFIG);

    //populate and write XML tree
    if (    !outputFile.writeXmlGuiConfig(inputCfg) || //add GUI layout configuration settings
            !outputFile.writeToFile()) //save XML
        throw FileError(wxString(_("Error writing file:")) + wxT(" \"") + filename + wxT("\""));
    return;
}


void xmlAccess::writeBatchConfig(const wxString& filename, const XmlBatchConfig& inputCfg)
{
    XmlConfigOutput outputFile(filename, XML_BATCH_CONFIG);

    //populate and write XML tree
    if (    !outputFile.writeXmlBatchConfig(inputCfg) || //add GUI layout configuration settings
            !outputFile.writeToFile()) //save XML
        throw FileError(wxString(_("Error writing file:")) + wxT(" \"") + filename + wxT("\""));
    return;
}


void xmlAccess::writeGlobalSettings(const XmlGlobalSettings& inputCfg)
{
    XmlConfigOutput outputFile(FreeFileSync::GLOBAL_CONFIG_FILE, XML_GLOBAL_SETTINGS);

    //populate and write XML tree
    if (    !outputFile.writeXmlGlobalSettings(inputCfg) || //add GUI layout configuration settings
            !outputFile.writeToFile()) //save XML
        throw FileError(wxString(_("Error writing file:")) + wxT(" \"") + FreeFileSync::GLOBAL_CONFIG_FILE + wxT("\""));
    return;
}


XmlConfigInput::XmlConfigInput(const wxString& fileName, const XmlType type) :
        loadSuccess(false)
{
    if (!wxFileExists(fileName)) //avoid wxWidgets error message when wxFFile receives not existing file
        return;

    //workaround to get a FILE* from a unicode filename
    wxFFile dummyFile(fileName, wxT("rb"));
    if (dummyFile.IsOpened())
    {
        FILE* inputFile = dummyFile.fp();

        TiXmlBase::SetCondenseWhiteSpace(false); //do not condense whitespace characters

        if (doc.LoadFile(inputFile)) //load XML; fails if inputFile is no proper XML
        {
            TiXmlElement* root = doc.RootElement();

            if (root && (root->ValueStr() == string("FreeFileSync"))) //check for FFS configuration xml
            {
                const char* cfgType = root->Attribute("XmlType");
                if (cfgType)
                {
                    if (type == XML_GUI_CONFIG)
                        loadSuccess = string(cfgType) == "GUI";
                    else if (type == XML_BATCH_CONFIG)
                        loadSuccess = string(cfgType) == "BATCH";
                    else if (type == XML_GLOBAL_SETTINGS)
                        loadSuccess = string(cfgType) == "GLOBAL";
                }
            }
        }
    }
}


bool readXmlElementValue(string& output, const TiXmlElement* parent, const string& name)
{
    if (parent)
    {
        const TiXmlElement* child = parent->FirstChildElement(name);
        if (child)
        {
            const char* text = child->GetText();
            if (text) //may be NULL!!
                output = text;
            else
                output.clear();
            return true;
        }
    }

    return false;
}


bool readXmlElementValue(int& output, const TiXmlElement* parent, const string& name)
{
    string temp;
    if (readXmlElementValue(temp, parent, name))
    {
        output = stringToInt(temp);
        return true;
    }
    else
        return false;
}


bool readXmlElementValue(CompareVariant& output, const TiXmlElement* parent, const string& name)
{
    int dummy = 0;
    if (readXmlElementValue(dummy, parent, name))
    {
        output = CompareVariant(dummy);
        return true;
    }
    else
        return false;
}


bool readXmlElementValue(SyncConfiguration::Direction& output, const TiXmlElement* parent, const string& name)
{
    string dummy;
    if (readXmlElementValue(dummy, parent, name))
    {
        if (dummy == "left")
            output = SyncConfiguration::SYNC_DIR_LEFT;
        else if (dummy == "right")
            output = SyncConfiguration::SYNC_DIR_RIGHT;
        else //treat all other input as "none"
            output = SyncConfiguration::SYNC_DIR_NONE;

        return true;
    }
    else
        return false;
}


bool readXmlElementValue(bool& output, const TiXmlElement* parent, const string& name)
{
    string dummy;
    if (readXmlElementValue(dummy, parent, name))
    {
        output = (dummy == "true");
        return true;
    }
    else
        return false;
}


bool XmlConfigInput::readXmlMainConfig(MainConfiguration& mainCfg, vector<FolderPair>& directoryPairs)
{
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlHandle hRoot(root);

    TiXmlElement* cmpSettings  = hRoot.FirstChild("MainConfig").FirstChild("Comparison").ToElement();
    TiXmlElement* syncConfig   = hRoot.FirstChild("MainConfig").FirstChild("Synchronization").FirstChild("Directions").ToElement();
    TiXmlElement* miscSettings = hRoot.FirstChild("MainConfig").FirstChild("Miscellaneous").ToElement();
    TiXmlElement* filter       = TiXmlHandle(miscSettings).FirstChild("Filter").ToElement();

    if (!cmpSettings || !syncConfig || !miscSettings || !filter)
        return false;

    string tempString;
//###########################################################
    //read compare variant
    if (!readXmlElementValue(mainCfg.compareVar, cmpSettings, "Variant")) return false;

    //read folder pair(s)
    TiXmlElement* folderPair = TiXmlHandle(cmpSettings).FirstChild("Folders").FirstChild("Pair").ToElement();

    //read folder pairs
    directoryPairs.clear();
    while (folderPair)
    {
        FolderPair newPair;

        if (!readXmlElementValue(tempString, folderPair, "Left")) return false;
        newPair.leftDirectory = wxString::FromUTF8(tempString.c_str()).c_str();

        if (!readXmlElementValue(tempString, folderPair, "Right")) return false;
        newPair.rightDirectory = wxString::FromUTF8(tempString.c_str()).c_str();

        directoryPairs.push_back(newPair);
        folderPair = folderPair->NextSiblingElement();
    }

//###########################################################
    //read sync configuration
    if (!readXmlElementValue(mainCfg.syncConfiguration.exLeftSideOnly, syncConfig, "LeftOnly"))   return false;
    if (!readXmlElementValue(mainCfg.syncConfiguration.exRightSideOnly, syncConfig, "RightOnly")) return false;
    if (!readXmlElementValue(mainCfg.syncConfiguration.leftNewer, syncConfig, "LeftNewer"))       return false;
    if (!readXmlElementValue(mainCfg.syncConfiguration.rightNewer, syncConfig, "RightNewer"))     return false;
    if (!readXmlElementValue(mainCfg.syncConfiguration.different, syncConfig, "Different"))       return false;
//###########################################################
    //read filter settings
    if (!readXmlElementValue(mainCfg.filterIsActive, filter, "Active")) return false;

    if (!readXmlElementValue(tempString, filter, "Include"))  return false;
    mainCfg.includeFilter = wxString::FromUTF8(tempString.c_str());

    if (!readXmlElementValue(tempString, filter, "Exclude"))  return false;
    mainCfg.excludeFilter = wxString::FromUTF8(tempString.c_str());
//###########################################################
    //other
    readXmlElementValue(mainCfg.useRecycleBin, miscSettings, "UseRecycler");
    readXmlElementValue(mainCfg.ignoreErrors, miscSettings, "IgnoreErrors");

    return true;
}


bool XmlConfigInput::readXmlGuiConfig(XmlGuiConfig& outputCfg)
{
    //read main config
    if (!readXmlMainConfig(outputCfg.mainCfg, outputCfg.directoryPairs))
        return false;

    //read GUI specific config data
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlHandle hRoot(root);

    //read GUI layout
    TiXmlElement* mainWindow = hRoot.FirstChild("GuiConfig").FirstChild("Windows").FirstChild("Main").ToElement();
    if (mainWindow)
    {
        readXmlElementValue(outputCfg.hideFilteredElements, mainWindow, "HideFiltered");
    }

    return true;
}


bool XmlConfigInput::readXmlBatchConfig(XmlBatchConfig& outputCfg)
{
    //read main config
    if (!readXmlMainConfig(outputCfg.mainCfg, outputCfg.directoryPairs))
        return false;

    //read batch specific config
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlHandle hRoot(root);

    //read batch settings
    TiXmlElement* batchConfig = hRoot.FirstChild("BatchConfig").ToElement();
    if (batchConfig)
    {
        //read application window size and position
        readXmlElementValue(outputCfg.silent, batchConfig, "Silent");
    }

    return true;
}


bool XmlConfigInput::readXmlGlobalSettings(XmlGlobalSettings& outputCfg)
{
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlHandle hRoot(root);

    //read global settings
    TiXmlElement* global = hRoot.FirstChild("Global").ToElement();
    if (!global) return false;

    //program language
    readXmlElementValue(outputCfg.global.programLanguage, global, "Language");

#ifdef FFS_WIN
    //daylight saving time check
    readXmlElementValue(outputCfg.global.dstCheckActive, global, "DaylightSavingTimeCheckActive");
#endif

    //folder dependency check
    readXmlElementValue(outputCfg.global.folderDependCheckActive, global, "FolderDependencyCheckActive");

    //gui specific global settings (optional)
    TiXmlElement* mainWindow = hRoot.FirstChild("Gui").FirstChild("Windows").FirstChild("Main").ToElement();
    if (mainWindow)
    {
        //read application window size and position
        readXmlElementValue(outputCfg.gui.widthNotMaximized, mainWindow, "Width");
        readXmlElementValue(outputCfg.gui.heightNotMaximized, mainWindow, "Height");
        readXmlElementValue(outputCfg.gui.posXNotMaximized, mainWindow, "PosX");
        readXmlElementValue(outputCfg.gui.posYNotMaximized, mainWindow, "PosY");
        readXmlElementValue(outputCfg.gui.isMaximized, mainWindow, "Maximized");

//###########################################################
        //read column widths
        TiXmlElement* leftColumn = TiXmlHandle(mainWindow).FirstChild("LeftColumns").FirstChild("Width").ToElement();
        while (leftColumn)
        {
            const char* width = leftColumn->GetText();
            if (width) //may be NULL!!
                outputCfg.gui.columnWidthLeft.push_back(stringToInt(width));
            else
                break;
            leftColumn = leftColumn->NextSiblingElement();
        }

        TiXmlElement* rightColumn = TiXmlHandle(mainWindow).FirstChild("RightColumns").FirstChild("Width").ToElement();
        while (rightColumn)
        {
            const char* width = rightColumn->GetText();
            if (width) //may be NULL!!
                outputCfg.gui.columnWidthRight.push_back(stringToInt(width));
            else
                break;
            rightColumn = rightColumn->NextSiblingElement();
        }

        //read column positions
        TiXmlElement* leftColumnPos = TiXmlHandle(mainWindow).FirstChild("LeftColumnPositions").FirstChild("Position").ToElement();
        while (leftColumnPos)
        {
            const char* width = leftColumnPos->GetText();
            if (width) //may be NULL!!
                outputCfg.gui.columnPositionsLeft.push_back(stringToInt(width));
            else
                break;
            leftColumnPos = leftColumnPos->NextSiblingElement();
        }

        TiXmlElement* rightColumnPos = TiXmlHandle(mainWindow).FirstChild("RightColumnPositions").FirstChild("Position").ToElement();
        while (rightColumnPos)
        {
            const char* width = rightColumnPos->GetText();
            if (width) //may be NULL!!
                outputCfg.gui.columnPositionsRight.push_back(stringToInt(width));
            else
                break;
            rightColumnPos = rightColumnPos->NextSiblingElement();
        }
    }


//batch specific global settings
    TiXmlElement* batch = hRoot.FirstChild("Batch").ToElement();
    if (!batch) return false;


//    if (!readXmlElementValue(outputCfg.dummy, global, "Language")) return false;

    return true;
}


XmlConfigOutput::XmlConfigOutput(const wxString& fileName, const XmlType type) :
        m_fileName(fileName)
{
    TiXmlBase::SetCondenseWhiteSpace(false); //do not condense whitespace characters

    TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "UTF-8", ""); //delete won't be necessary later; ownership passed to TiXmlDocument!
    doc.LinkEndChild(decl);

    TiXmlElement* root = new TiXmlElement("FreeFileSync");
    if (type == XML_GUI_CONFIG)
        root->SetAttribute("XmlType", "GUI"); //xml configuration type
    else if (type == XML_BATCH_CONFIG)
        root->SetAttribute("XmlType", "BATCH");
    else if (type == XML_GLOBAL_SETTINGS)
        root->SetAttribute("XmlType", "GLOBAL");
    else
        assert(false);
    doc.LinkEndChild(root);
}


bool XmlConfigOutput::writeToFile()
{
    //workaround to get a FILE* from a unicode filename
    wxFFile dummyFile(m_fileName, wxT("wb"));
    if (!dummyFile.IsOpened())
        return false;

    FILE* outputFile = dummyFile.fp();

    return doc.SaveFile(outputFile); //save XML
}


void addXmlElement(TiXmlElement* parent, const string& name, const string& value)
{
    if (parent)
    {
        TiXmlElement* subElement = new TiXmlElement(name);
        parent->LinkEndChild(subElement);
        subElement->LinkEndChild(new TiXmlText(value));
    }
}


void addXmlElement(TiXmlElement* parent, const string& name, const int value)
{
    addXmlElement(parent, name, numberToString(value));
}


void addXmlElement(TiXmlElement* parent, const string& name, const SyncConfiguration::Direction value)
{
    if (value == SyncConfiguration::SYNC_DIR_LEFT)
        addXmlElement(parent, name, string("left"));
    else if (value == SyncConfiguration::SYNC_DIR_RIGHT)
        addXmlElement(parent, name, string("right"));
    else if (value == SyncConfiguration::SYNC_DIR_NONE)
        addXmlElement(parent, name, string("none"));
    else
        assert(false);
}


void addXmlElement(TiXmlElement* parent, const string& name, const bool value)
{
    if (value)
        addXmlElement(parent, name, string("true"));
    else
        addXmlElement(parent, name, string("false"));
}


bool XmlConfigOutput::writeXmlMainConfig(const MainConfiguration& mainCfg, const vector<FolderPair>& directoryPairs)
{
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlElement* settings = new TiXmlElement("MainConfig");
    root->LinkEndChild(settings);

//###########################################################
    TiXmlElement* cmpSettings = new TiXmlElement("Comparison");
    settings->LinkEndChild(cmpSettings);

    //write compare algorithm
    addXmlElement(cmpSettings, "Variant", mainCfg.compareVar);

    //write folder pair(s)
    TiXmlElement* folders = new TiXmlElement("Folders");
    cmpSettings->LinkEndChild(folders);

    //write folder pairs
    for (vector<FolderPair>::const_iterator i = directoryPairs.begin(); i != directoryPairs.end(); ++i)
    {
        TiXmlElement* folderPair = new TiXmlElement("Pair");
        folders->LinkEndChild(folderPair);

        addXmlElement(folderPair, "Left", string(wxString(i->leftDirectory.c_str()).ToUTF8()));
        addXmlElement(folderPair, "Right", string(wxString(i->rightDirectory.c_str()).ToUTF8()));
    }

//###########################################################
    TiXmlElement* syncSettings = new TiXmlElement("Synchronization");
    settings->LinkEndChild(syncSettings);

    //write sync configuration
    TiXmlElement* syncConfig = new TiXmlElement("Directions");
    syncSettings->LinkEndChild(syncConfig);

    addXmlElement(syncConfig, "LeftOnly",   mainCfg.syncConfiguration.exLeftSideOnly);
    addXmlElement(syncConfig, "RightOnly",  mainCfg.syncConfiguration.exRightSideOnly);
    addXmlElement(syncConfig, "LeftNewer",  mainCfg.syncConfiguration.leftNewer);
    addXmlElement(syncConfig, "RightNewer", mainCfg.syncConfiguration.rightNewer);
    addXmlElement(syncConfig, "Different",  mainCfg.syncConfiguration.different);

//###########################################################
    TiXmlElement* miscSettings = new TiXmlElement("Miscellaneous");
    settings->LinkEndChild(miscSettings);

    //write filter settings
    TiXmlElement* filter = new TiXmlElement("Filter");
    miscSettings->LinkEndChild(filter);

    addXmlElement(filter, "Active", mainCfg.filterIsActive);
    addXmlElement(filter, "Include", string((mainCfg.includeFilter).ToUTF8()));
    addXmlElement(filter, "Exclude", string((mainCfg.excludeFilter).ToUTF8()));

    //other
    addXmlElement(miscSettings, "UseRecycler", mainCfg.useRecycleBin);
    addXmlElement(miscSettings, "IgnoreErrors", mainCfg.ignoreErrors);
//###########################################################

    return true;
}


bool XmlConfigOutput::writeXmlGuiConfig(const XmlGuiConfig& inputCfg)
{
    //write main config
    if (!writeXmlMainConfig(inputCfg.mainCfg, inputCfg.directoryPairs))
        return false;

    //write GUI specific config
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlElement* guiLayout = new TiXmlElement("GuiConfig");
    root->LinkEndChild(guiLayout);

    TiXmlElement* windows = new TiXmlElement("Windows");
    guiLayout->LinkEndChild(windows);

    TiXmlElement* mainWindow = new TiXmlElement("Main");
    windows->LinkEndChild(mainWindow);

    addXmlElement(mainWindow, "HideFiltered", inputCfg.hideFilteredElements);

    return true;
}


bool XmlConfigOutput::writeXmlBatchConfig(const XmlBatchConfig& inputCfg)
{
    //write main config
    if (!writeXmlMainConfig(inputCfg.mainCfg, inputCfg.directoryPairs))
        return false;

    //write GUI specific config
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlElement* batchConfig = new TiXmlElement("BatchConfig");
    root->LinkEndChild(batchConfig);

    addXmlElement(batchConfig, "Silent", inputCfg.silent);

    return true;
}


bool XmlConfigOutput::writeXmlGlobalSettings(const XmlGlobalSettings& inputCfg)
{
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    //###################################################################
    //write global settings
    TiXmlElement* global = new TiXmlElement("Global");
    root->LinkEndChild(global);

    //program language
    addXmlElement(global, "Language", inputCfg.global.programLanguage);

#ifdef FFS_WIN
    //daylight saving time check
    addXmlElement(global, "DaylightSavingTimeCheckActive", inputCfg.global.dstCheckActive);
#endif

    //folder dependency check
    addXmlElement(global, "FolderDependencyCheckActive", inputCfg.global.folderDependCheckActive);

    //###################################################################
    //write gui settings
    TiXmlElement* guiLayout = new TiXmlElement("Gui");
    root->LinkEndChild(guiLayout);

    TiXmlElement* windows = new TiXmlElement("Windows");
    guiLayout->LinkEndChild(windows);

    TiXmlElement* mainWindow = new TiXmlElement("Main");
    windows->LinkEndChild(mainWindow);

    //window size
    addXmlElement(mainWindow, "Width", inputCfg.gui.widthNotMaximized);
    addXmlElement(mainWindow, "Height", inputCfg.gui.heightNotMaximized);

    //window position
    addXmlElement(mainWindow, "PosX", inputCfg.gui.posXNotMaximized);
    addXmlElement(mainWindow, "PosY", inputCfg.gui.posYNotMaximized);
    addXmlElement(mainWindow, "Maximized", inputCfg.gui.isMaximized);

    //write column sizes
    TiXmlElement* leftColumn = new TiXmlElement("LeftColumns");
    mainWindow->LinkEndChild(leftColumn);

    for (unsigned int i = 0; i < inputCfg.gui.columnWidthLeft.size(); ++i)
        addXmlElement(leftColumn, "Width", inputCfg.gui.columnWidthLeft[i]);

    TiXmlElement* rightColumn = new TiXmlElement("RightColumns");
    mainWindow->LinkEndChild(rightColumn);

    for (unsigned int i = 0; i < inputCfg.gui.columnWidthRight.size(); ++i)
        addXmlElement(rightColumn, "Width", inputCfg.gui.columnWidthRight[i]);

    //write column positions
    TiXmlElement* leftColumnPos = new TiXmlElement("LeftColumnPositions");
    mainWindow->LinkEndChild(leftColumnPos);

    for (unsigned int i = 0; i < inputCfg.gui.columnPositionsLeft.size(); ++i)
        addXmlElement(leftColumnPos, "Position", inputCfg.gui.columnPositionsLeft[i]);

    TiXmlElement* rightColumnPos = new TiXmlElement("RightColumnPositions");
    mainWindow->LinkEndChild(rightColumnPos);

    for (unsigned int i = 0; i < inputCfg.gui.columnPositionsRight.size(); ++i)
        addXmlElement(rightColumnPos, "Position", inputCfg.gui.columnPositionsRight[i]);

    //###################################################################
    //write batch settings

    TiXmlElement* batch = new TiXmlElement("Batch");
    root->LinkEndChild(batch);

    return true;
}


int xmlAccess::retrieveSystemLanguage() //map language dialects
{
    const int lang = wxLocale::GetSystemLanguage();

    switch (lang)
    {
    case wxLANGUAGE_GERMAN_AUSTRIAN:
    case wxLANGUAGE_GERMAN_BELGIUM:
    case wxLANGUAGE_GERMAN_LIECHTENSTEIN:
    case wxLANGUAGE_GERMAN_LUXEMBOURG:
    case wxLANGUAGE_GERMAN_SWISS:
        return wxLANGUAGE_GERMAN;

    case wxLANGUAGE_FRENCH_BELGIAN:
    case wxLANGUAGE_FRENCH_CANADIAN:
    case wxLANGUAGE_FRENCH_LUXEMBOURG:
    case wxLANGUAGE_FRENCH_MONACO:
    case wxLANGUAGE_FRENCH_SWISS:
        return wxLANGUAGE_FRENCH;

        //case wxLANGUAGE_JAPANESE:

    case wxLANGUAGE_DUTCH_BELGIAN:
        return wxLANGUAGE_DUTCH;

    case wxLANGUAGE_CHINESE:
    case wxLANGUAGE_CHINESE_TRADITIONAL:
    case wxLANGUAGE_CHINESE_HONGKONG:
    case wxLANGUAGE_CHINESE_MACAU:
    case wxLANGUAGE_CHINESE_SINGAPORE:
    case wxLANGUAGE_CHINESE_TAIWAN:
        return wxLANGUAGE_CHINESE_SIMPLIFIED;

    default:
        return lang;
    }
}
