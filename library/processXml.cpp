#include "processXml.h"
#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/intl.h>
#include "globalFunctions.h"
#include "tinyxml/tinyxml.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#endif

using namespace FreeFileSync;

const wxString xmlAccess::LAST_CONFIG_FILE   = wxT("LastRun.ffs_gui");
const wxString xmlAccess::GLOBAL_CONFIG_FILE = wxT("GlobalSettings.xml");


//small helper functions
bool readXmlElementValue(std::string& output,                  const TiXmlElement* parent, const std::string& name);
bool readXmlElementValue(int& output,                          const TiXmlElement* parent, const std::string& name);
bool readXmlElementValue(CompareVariant& output,               const TiXmlElement* parent, const std::string& name);
bool readXmlElementValue(SyncDirection& output, const TiXmlElement* parent, const std::string& name);
bool readXmlElementValue(bool& output,                         const TiXmlElement* parent, const std::string& name);

void addXmlElement(TiXmlElement* parent, const std::string& name, const std::string& value);
void addXmlElement(TiXmlElement* parent, const std::string& name, const int value);
void addXmlElement(TiXmlElement* parent, const std::string& name, const SyncDirection value);
void addXmlElement(TiXmlElement* parent, const std::string& name, const bool value);


class XmlConfigInput
{
public:
    XmlConfigInput(const wxString& fileName, const xmlAccess::XmlType type);
    ~XmlConfigInput() {}

    bool loadedSuccessfully()
    {
        return loadSuccess;
    }

    //read gui settings, all values retrieved are optional, so check for initial values! (== -1)
    bool readXmlGuiConfig(xmlAccess::XmlGuiConfig& outputCfg);
    //read batch settings, all values retrieved are optional
    bool readXmlBatchConfig(xmlAccess::XmlBatchConfig& outputCfg);
    //read global settings, valid for both GUI and batch mode, independent from configuration
    bool readXmlGlobalSettings(xmlAccess::XmlGlobalSettings& outputCfg);

private:
    //read basic FreefileSync settings (used by commandline and GUI), return true if ALL values have been retrieved successfully
    bool readXmlMainConfig(MainConfiguration& mainCfg, std::vector<FolderPair>& directoryPairs);

    TiXmlDocument doc;
    bool loadSuccess;
};


class XmlConfigOutput
{
public:
    XmlConfigOutput(const wxString& fileName, const xmlAccess::XmlType type);
    ~XmlConfigOutput() {}

    bool writeToFile();

    //write gui settings
    bool writeXmlGuiConfig(const xmlAccess::XmlGuiConfig& outputCfg);
    //write batch settings
    bool writeXmlBatchConfig(const xmlAccess::XmlBatchConfig& outputCfg);
    //write global settings
    bool writeXmlGlobalSettings(const xmlAccess::XmlGlobalSettings& outputCfg);

private:
    //write basic FreefileSync settings (used by commandline and GUI), return true if everything was written successfully
    bool writeXmlMainConfig(const MainConfiguration& mainCfg, const std::vector<FolderPair>& directoryPairs);

    TiXmlDocument doc;
    const wxString& m_fileName;
};


xmlAccess::XmlType xmlAccess::getXmlType(const wxString& filename)
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

    if (!root || (root->ValueStr() != std::string("FreeFileSync"))) //check for FFS configuration xml
        return XML_OTHER;

    const char* cfgType = root->Attribute("XmlType");
    if (!cfgType)
        return XML_OTHER;

    if (std::string(cfgType) == "BATCH")
        return XML_BATCH_CONFIG;
    else if (std::string(cfgType) == "GUI")
        return XML_GUI_CONFIG;
    else if (std::string(cfgType) == "GLOBAL")
        return XML_GLOBAL_SETTINGS;
    else
        return XML_OTHER;
}


xmlAccess::XmlGuiConfig xmlAccess::readGuiConfig(const wxString& filename)
{
    //load XML
    if (!wxFileExists(filename))
        throw FileError(Zstring(_("File does not exist:")) + wxT(" \"") + filename.c_str() + wxT("\""));

    XmlConfigInput inputFile(filename, XML_GUI_CONFIG);

    XmlGuiConfig outputCfg;

    if (!inputFile.loadedSuccessfully())
        throw FileError(Zstring(_("Error reading file:")) + wxT(" \"") + filename.c_str() + wxT("\""));

    if (!inputFile.readXmlGuiConfig(outputCfg)) //read GUI layout configuration
        throw FileError(Zstring(_("Error parsing configuration file:")) + wxT(" \"") + filename.c_str() + wxT("\""));

    return outputCfg;
}


xmlAccess::XmlBatchConfig xmlAccess::readBatchConfig(const wxString& filename)
{
    //load XML
    if (!wxFileExists(filename))
        throw FileError(Zstring(_("File does not exist:")) + wxT(" \"") + filename.c_str() + wxT("\""));

    XmlConfigInput inputFile(filename, XML_BATCH_CONFIG);

    XmlBatchConfig outputCfg;

    if (!inputFile.loadedSuccessfully())
        throw FileError(Zstring(_("Error reading file:")) + wxT(" \"") + filename.c_str() + wxT("\""));

    if (!inputFile.readXmlBatchConfig(outputCfg))
        throw FileError(Zstring(_("Error parsing configuration file:")) + wxT(" \"") + filename.c_str() + wxT("\""));

    return outputCfg;
}


xmlAccess::XmlGlobalSettings xmlAccess::readGlobalSettings()
{
    //load XML
    if (!wxFileExists(xmlAccess::GLOBAL_CONFIG_FILE))
        throw FileError(Zstring(_("File does not exist:")) + wxT(" \"") + xmlAccess::GLOBAL_CONFIG_FILE.c_str() + wxT("\""));

    XmlConfigInput inputFile(xmlAccess::GLOBAL_CONFIG_FILE, XML_GLOBAL_SETTINGS);

    XmlGlobalSettings outputCfg;

    if (!inputFile.loadedSuccessfully())
        throw FileError(Zstring(_("Error reading file:")) + wxT(" \"") + xmlAccess::GLOBAL_CONFIG_FILE.c_str() + wxT("\""));

    if (!inputFile.readXmlGlobalSettings(outputCfg))
        throw FileError(Zstring(_("Error parsing configuration file:")) + wxT(" \"") + xmlAccess::GLOBAL_CONFIG_FILE.c_str() + wxT("\""));

    return outputCfg;
}


void xmlAccess::writeGuiConfig(const wxString& filename, const XmlGuiConfig& outputCfg)
{
    XmlConfigOutput outputFile(filename, XML_GUI_CONFIG);

    //populate and write XML tree
    if (    !outputFile.writeXmlGuiConfig(outputCfg) || //add GUI layout configuration settings
            !outputFile.writeToFile()) //save XML
        throw FileError(Zstring(_("Error writing file:")) + wxT(" \"") + filename.c_str() + wxT("\""));
    return;
}


void xmlAccess::writeBatchConfig(const wxString& filename, const XmlBatchConfig& outputCfg)
{
    XmlConfigOutput outputFile(filename, XML_BATCH_CONFIG);

    //populate and write XML tree
    if (    !outputFile.writeXmlBatchConfig(outputCfg) || //add batch configuration settings
            !outputFile.writeToFile()) //save XML
        throw FileError(Zstring(_("Error writing file:")) + wxT(" \"") + filename.c_str() + wxT("\""));
    return;
}


void xmlAccess::writeGlobalSettings(const XmlGlobalSettings& outputCfg)
{
    XmlConfigOutput outputFile(xmlAccess::GLOBAL_CONFIG_FILE, XML_GLOBAL_SETTINGS);

    //populate and write XML tree
    if (    !outputFile.writeXmlGlobalSettings(outputCfg) || //add GUI layout configuration settings
            !outputFile.writeToFile()) //save XML
        throw FileError(Zstring(_("Error writing file:")) + wxT(" \"") + xmlAccess::GLOBAL_CONFIG_FILE.c_str() + wxT("\""));
    return;
}


XmlConfigInput::XmlConfigInput(const wxString& fileName, const xmlAccess::XmlType type) :
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

            if (root && (root->ValueStr() == std::string("FreeFileSync"))) //check for FFS configuration xml
            {
                const char* cfgType = root->Attribute("XmlType");
                if (cfgType)
                {
                    if (type == xmlAccess::XML_GUI_CONFIG)
                        loadSuccess = std::string(cfgType) == "GUI";
                    else if (type == xmlAccess::XML_BATCH_CONFIG)
                        loadSuccess = std::string(cfgType) == "BATCH";
                    else if (type == xmlAccess::XML_GLOBAL_SETTINGS)
                        loadSuccess = std::string(cfgType) == "GLOBAL";
                }
            }
        }
    }
}


bool readXmlElementValue(std::string& output, const TiXmlElement* parent, const std::string& name)
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


bool readXmlElementValue(int& output, const TiXmlElement* parent, const std::string& name)
{
    std::string temp;
    if (readXmlElementValue(temp, parent, name))
    {
        output = globalFunctions::stringToInt(temp);
        return true;
    }
    else
        return false;
}


bool readXmlElementValue(long& output, const TiXmlElement* parent, const std::string& name)
{
    std::string temp;
    if (readXmlElementValue(temp, parent, name))
    {
        output = globalFunctions::stringToLong(temp);
        return true;
    }
    else
        return false;
}


bool readXmlElementValue(CompareVariant& output, const TiXmlElement* parent, const std::string& name)
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


bool readXmlElementValue(SyncDirection& output, const TiXmlElement* parent, const std::string& name)
{
    std::string dummy;
    if (readXmlElementValue(dummy, parent, name))
    {
        if (dummy == "left")
            output = SYNC_DIR_LEFT;
        else if (dummy == "right")
            output = SYNC_DIR_RIGHT;
        else //treat all other input as "none"
            output = SYNC_DIR_NONE;

        return true;
    }
    else
        return false;
}


bool readXmlElementValue(bool& output, const TiXmlElement* parent, const std::string& name)
{
    std::string dummy;
    if (readXmlElementValue(dummy, parent, name))
    {
        output = (dummy == "true");
        return true;
    }
    else
        return false;
}


bool readXmlElementValue(xmlAccess::OnError& output, const TiXmlElement* parent, const std::string& name)
{
    std::string dummy;
    if (readXmlElementValue(dummy, parent, name))
    {
        if (dummy == "Ignore")
            output = xmlAccess::ON_ERROR_IGNORE;
        else if (dummy == "Exit")
            output = xmlAccess::ON_ERROR_EXIT;
        else //treat all other input as popup
            output = xmlAccess::ON_ERROR_POPUP;

        return true;
    }
    else
        return false;
}


void readXmlElementTable(std::vector<wxString>& output, const TiXmlElement* parent, const std::string& name)
{
    output.clear();

    if (parent)
    {
        //load elements
        const TiXmlElement* element = parent->FirstChildElement(name);
        while (element)
        {
            const char* text = element->GetText();
            if (text) //may be NULL!!
                output.push_back(wxString::FromUTF8(text));
            else
                break;
            element = element->NextSiblingElement();
        }
    }
}

//################################################################################################################


bool XmlConfigInput::readXmlMainConfig(MainConfiguration& mainCfg, std::vector<FolderPair>& directoryPairs)
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

    std::string tempString;
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

    return true;
}


bool XmlConfigInput::readXmlGuiConfig(xmlAccess::XmlGuiConfig& outputCfg)
{
    //read main config
    if (!readXmlMainConfig(outputCfg.mainCfg, outputCfg.directoryPairs))
        return false;

    //read GUI specific config data
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlHandle hRoot(root);

    TiXmlElement* guiConfig = hRoot.FirstChild("GuiConfig").ToElement();
    if (guiConfig)
    {
        readXmlElementValue(outputCfg.hideFilteredElements, guiConfig, "HideFiltered");
        readXmlElementValue(outputCfg.ignoreErrors, guiConfig, "IgnoreErrors");
        readXmlElementValue(outputCfg.syncPreviewEnabled, guiConfig, "SyncPreviewActive");
    }

    return true;
}


bool XmlConfigInput::readXmlBatchConfig(xmlAccess::XmlBatchConfig& outputCfg)
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
        readXmlElementValue(outputCfg.silent, batchConfig, "Silent");

        std::string tempString;
        if (readXmlElementValue(tempString, batchConfig, "LogfileDirectory"))
            outputCfg.logFileDirectory = wxString::FromUTF8(tempString.c_str());

        readXmlElementValue(outputCfg.handleError, batchConfig, "HandleError");
    }

    return true;
}


bool XmlConfigInput::readXmlGlobalSettings(xmlAccess::XmlGlobalSettings& outputCfg)
{
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlHandle hRoot(root);

    //read global settings
    TiXmlElement* global = hRoot.FirstChild("Shared").ToElement();
    if (global)
    {
        //try to read program language setting
        readXmlElementValue(outputCfg.shared.programLanguage, global, "Language");

        //max. allowed file time deviation
        int dummy = 0;
        if (readXmlElementValue(dummy, global, "FileTimeTolerance"))
            outputCfg.shared.fileTimeTolerance = dummy;

        //ignore +/- 1 hour due to DST change
        readXmlElementValue(outputCfg.shared.ignoreOneHourDiff, global, "IgnoreOneHourDifference");

        //traverse into symbolic links (to folders)
        readXmlElementValue(outputCfg.shared.traverseDirectorySymlinks, global, "TraverseDirectorySymlinks");

        //copy symbolic links to files
        readXmlElementValue(outputCfg.shared.copyFileSymlinks, global, "CopyFileSymlinks");

        //last update check
        readXmlElementValue(outputCfg.shared.lastUpdateCheck, global, "LastCheckForUpdates");
    }

    TiXmlElement* warnings = hRoot.FirstChild("Shared").FirstChild("Warnings").ToElement();
    if (warnings)
    {
        //folder dependency check
        readXmlElementValue(outputCfg.shared.warningDependentFolders, warnings, "CheckForDependentFolders");

        //significant difference check
        readXmlElementValue(outputCfg.shared.warningSignificantDifference, warnings, "CheckForSignificantDifference");

        //check free disk space
        readXmlElementValue(outputCfg.shared.warningNotEnoughDiskSpace, warnings, "CheckForFreeDiskSpace");

        //check for unresolved conflicts
        readXmlElementValue(outputCfg.shared.warningUnresolvedConflicts, warnings, "CheckForUnresolvedConflicts");

        //small reminder that synchronization will be starting immediately
        readXmlElementValue(outputCfg.shared.warningSynchronizationStarting, warnings, "SynchronizationStarting");
    }

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

        readXmlElementValue(outputCfg.gui.deleteOnBothSides, mainWindow, "ManualDeletionOnBothSides");
        readXmlElementValue(outputCfg.gui.useRecyclerForManualDeletion, mainWindow, "ManualDeletionUseRecycler");
        readXmlElementValue(outputCfg.gui.showFileIcons, mainWindow, "ShowFileIcons");
        readXmlElementValue(outputCfg.gui.popupOnConfigChange, mainWindow, "PopupOnConfigChange");

//###########################################################
        //read column attributes
        TiXmlElement* leftColumn = TiXmlHandle(mainWindow).FirstChild("LeftColumns").FirstChild("Column").ToElement();
        unsigned int colPos = 0;
        while (leftColumn)
        {
            const char* type    = leftColumn->Attribute("Type");
            const char* visible = leftColumn->Attribute("Visible");
            const char* width   = leftColumn->Attribute("Width");

            if (type && visible && width) //may be NULL!!
            {
                xmlAccess::ColumnAttrib newAttrib;
                newAttrib.type     = xmlAccess::ColumnTypes(globalFunctions::stringToInt(type));
                newAttrib.visible  = std::string(visible) != std::string("false");
                newAttrib.position = colPos;
                newAttrib.width    = globalFunctions::stringToInt(width);
                outputCfg.gui.columnAttribLeft.push_back(newAttrib);
            }
            else
                break;

            leftColumn = leftColumn->NextSiblingElement();
            ++colPos;
        }

        TiXmlElement* rightColumn = TiXmlHandle(mainWindow).FirstChild("RightColumns").FirstChild("Column").ToElement();
        colPos = 0;
        while (rightColumn)
        {
            const char* type    = rightColumn->Attribute("Type");
            const char* visible = rightColumn->Attribute("Visible");
            const char* width   = rightColumn->Attribute("Width");

            if (type && visible && width) //may be NULL!!
            {
                xmlAccess::ColumnAttrib newAttrib;
                newAttrib.type     = xmlAccess::ColumnTypes(globalFunctions::stringToInt(type));
                newAttrib.visible  = std::string(visible) != std::string("false");
                newAttrib.position = colPos;
                newAttrib.width    = globalFunctions::stringToInt(width);
                outputCfg.gui.columnAttribRight.push_back(newAttrib);
            }
            else
                break;

            rightColumn = rightColumn->NextSiblingElement();
            ++colPos;
        }

        //load folder history elements
        const TiXmlElement* historyLeft = TiXmlHandle(mainWindow).FirstChild("FolderHistoryLeft").ToElement();
        if (historyLeft)
        {
            //load max. history size
            const char* histSizeMax = historyLeft->Attribute("MaximumSize");
            if (histSizeMax) //may be NULL!
                outputCfg.gui.folderHistLeftMax = globalFunctions::stringToInt(histSizeMax);

            //load config history elements
            readXmlElementTable(outputCfg.gui.folderHistoryLeft, historyLeft, "Folder");
        }

        const TiXmlElement* historyRight = TiXmlHandle(mainWindow).FirstChild("FolderHistoryRight").ToElement();
        if (historyRight)
        {
            //load max. history size
            const char* histSizeMax = historyRight->Attribute("MaximumSize");
            if (histSizeMax) //may be NULL!
                outputCfg.gui.folderHistRightMax = globalFunctions::stringToInt(histSizeMax);

            //load config history elements
            readXmlElementTable(outputCfg.gui.folderHistoryRight, historyRight, "Folder");
        }
    }

    TiXmlElement* gui = hRoot.FirstChild("Gui").ToElement();
    if (gui)
    {
        //commandline for file manager integration
        std::string tempString;
        if (readXmlElementValue(tempString, gui, "FileManager"))
            outputCfg.gui.commandLineFileManager = wxString::FromUTF8(tempString.c_str());
    }

    //load config file history
    TiXmlElement* cfgHistory = hRoot.FirstChild("Gui").FirstChild("ConfigHistory").ToElement();
    if (cfgHistory)
    {
        //load max. history size
        const char* histSizeMax = cfgHistory->Attribute("MaximumSize");
        if (histSizeMax) //may be NULL!
            outputCfg.gui.cfgHistoryMax = globalFunctions::stringToInt(histSizeMax);

        //load config history elements
        readXmlElementTable(outputCfg.gui.cfgFileHistory, cfgHistory, "File");
    }


//batch specific global settings
    TiXmlElement* batch = hRoot.FirstChild("Batch").ToElement();
    if (!batch) return false;


//    if (!readXmlElementValue(outputCfg.dummy, global, "Language")) return false;

    return true;
}


XmlConfigOutput::XmlConfigOutput(const wxString& fileName, const xmlAccess::XmlType type) :
        m_fileName(fileName)
{
    TiXmlBase::SetCondenseWhiteSpace(false); //do not condense whitespace characters

    TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "UTF-8", ""); //delete won't be necessary later; ownership passed to TiXmlDocument!
    doc.LinkEndChild(decl);

    TiXmlElement* root = new TiXmlElement("FreeFileSync");
    if (type == xmlAccess::XML_GUI_CONFIG)
        root->SetAttribute("XmlType", "GUI"); //xml configuration type
    else if (type == xmlAccess::XML_BATCH_CONFIG)
        root->SetAttribute("XmlType", "BATCH");
    else if (type == xmlAccess::XML_GLOBAL_SETTINGS)
        root->SetAttribute("XmlType", "GLOBAL");
    else
        assert(false);
    doc.LinkEndChild(root);
}


bool XmlConfigOutput::writeToFile()
{
    //workaround to get a FILE* from a unicode filename
    wxFFile dummyFile(m_fileName, wxT("w")); //no need for "binary" mode here
    if (!dummyFile.IsOpened())
        return false;

    FILE* outputFile = dummyFile.fp();

    return doc.SaveFile(outputFile); //save XML
}


void addXmlElement(TiXmlElement* parent, const std::string& name, const std::string& value)
{
    if (parent)
    {
        TiXmlElement* subElement = new TiXmlElement(name);
        parent->LinkEndChild(subElement);
        subElement->LinkEndChild(new TiXmlText(value));
    }
}


void addXmlElement(TiXmlElement* parent, const std::string& name, const int value)
{
    addXmlElement(parent, name, globalFunctions::numberToString(value));
}


void addXmlElement(TiXmlElement* parent, const std::string& name, const long value)
{
    addXmlElement(parent, name, globalFunctions::numberToString(value));
}


void addXmlElement(TiXmlElement* parent, const std::string& name, const SyncDirection value)
{
    if (value == SYNC_DIR_LEFT)
        addXmlElement(parent, name, std::string("left"));
    else if (value == SYNC_DIR_RIGHT)
        addXmlElement(parent, name, std::string("right"));
    else if (value == SYNC_DIR_NONE)
        addXmlElement(parent, name, std::string("none"));
    else
        assert(false);
}


void addXmlElement(TiXmlElement* parent, const std::string& name, const bool value)
{
    if (value)
        addXmlElement(parent, name, std::string("true"));
    else
        addXmlElement(parent, name, std::string("false"));
}


void addXmlElement(TiXmlElement* parent, const std::string& name, const xmlAccess::OnError value)
{
    if (value == xmlAccess::ON_ERROR_IGNORE)
        addXmlElement(parent, name, std::string("Ignore"));
    else if (value == xmlAccess::ON_ERROR_EXIT)
        addXmlElement(parent, name, std::string("Exit"));
    else if (value == xmlAccess::ON_ERROR_POPUP)
        addXmlElement(parent, name, std::string("Popup"));
    else
        assert(false);
}


void addXmlElementTable(TiXmlElement* parent, const std::string& name, const std::vector<wxString>& input)
{
    for (std::vector<wxString>::const_iterator i = input.begin(); i != input.end(); ++i)
        addXmlElement(parent, name, std::string(i->ToUTF8()));
}


bool XmlConfigOutput::writeXmlMainConfig(const MainConfiguration& mainCfg, const std::vector<FolderPair>& directoryPairs)
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
    for (std::vector<FolderPair>::const_iterator i = directoryPairs.begin(); i != directoryPairs.end(); ++i)
    {
        TiXmlElement* folderPair = new TiXmlElement("Pair");
        folders->LinkEndChild(folderPair);

        addXmlElement(folderPair, "Left", std::string(wxString(i->leftDirectory.c_str()).ToUTF8()));
        addXmlElement(folderPair, "Right", std::string(wxString(i->rightDirectory.c_str()).ToUTF8()));
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
    addXmlElement(filter, "Include", std::string((mainCfg.includeFilter).ToUTF8()));
    addXmlElement(filter, "Exclude", std::string((mainCfg.excludeFilter).ToUTF8()));

    //other
    addXmlElement(miscSettings, "UseRecycler", mainCfg.useRecycleBin);
//###########################################################

    return true;
}


bool XmlConfigOutput::writeXmlGuiConfig(const xmlAccess::XmlGuiConfig& inputCfg)
{
    //write main config
    if (!writeXmlMainConfig(inputCfg.mainCfg, inputCfg.directoryPairs))
        return false;

    //write GUI specific config
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlElement* guiConfig = new TiXmlElement("GuiConfig");
    root->LinkEndChild(guiConfig);

    addXmlElement(guiConfig, "HideFiltered", inputCfg.hideFilteredElements);
    addXmlElement(guiConfig, "IgnoreErrors", inputCfg.ignoreErrors);
    addXmlElement(guiConfig, "SyncPreviewActive", inputCfg.syncPreviewEnabled);

    return true;
}


bool XmlConfigOutput::writeXmlBatchConfig(const xmlAccess::XmlBatchConfig& inputCfg)
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
    addXmlElement(batchConfig, "LogfileDirectory", std::string(inputCfg.logFileDirectory.ToUTF8()));
    addXmlElement(batchConfig, "HandleError", inputCfg.handleError);

    return true;
}


bool XmlConfigOutput::writeXmlGlobalSettings(const xmlAccess::XmlGlobalSettings& inputCfg)
{
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    //###################################################################
    //write global settings
    TiXmlElement* global = new TiXmlElement("Shared");
    root->LinkEndChild(global);

    //program language
    addXmlElement(global, "Language", inputCfg.shared.programLanguage);

    //max. allowed file time deviation
    addXmlElement(global, "FileTimeTolerance", int(inputCfg.shared.fileTimeTolerance));

    //ignore +/- 1 hour due to DST change
    addXmlElement(global, "IgnoreOneHourDifference", inputCfg.shared.ignoreOneHourDiff);

    //traverse into symbolic links (to folders)
    addXmlElement(global, "TraverseDirectorySymlinks", inputCfg.shared.traverseDirectorySymlinks);

    //copy symbolic links to files
    addXmlElement(global, "CopyFileSymlinks", inputCfg.shared.copyFileSymlinks);

    //last update check
    addXmlElement(global, "LastCheckForUpdates", inputCfg.shared.lastUpdateCheck);

    //warnings
    TiXmlElement* warnings = new TiXmlElement("Warnings");
    global->LinkEndChild(warnings);

    //warning: dependent folders
    addXmlElement(warnings, "CheckForDependentFolders", inputCfg.shared.warningDependentFolders);

    //significant difference check
    addXmlElement(warnings, "CheckForSignificantDifference", inputCfg.shared.warningSignificantDifference);

    //check free disk space
    addXmlElement(warnings, "CheckForFreeDiskSpace", inputCfg.shared.warningNotEnoughDiskSpace);

    //check for unresolved conflicts
    addXmlElement(warnings, "CheckForUnresolvedConflicts", inputCfg.shared.warningUnresolvedConflicts);

    //small reminder that synchronization will be starting immediately
    addXmlElement(warnings, "SynchronizationStarting", inputCfg.shared.warningSynchronizationStarting);


    //###################################################################
    //write global gui settings
    TiXmlElement* gui = new TiXmlElement("Gui");
    root->LinkEndChild(gui);

    TiXmlElement* windows = new TiXmlElement("Windows");
    gui->LinkEndChild(windows);

    TiXmlElement* mainWindow = new TiXmlElement("Main");
    windows->LinkEndChild(mainWindow);

    //window size
    addXmlElement(mainWindow, "Width", inputCfg.gui.widthNotMaximized);
    addXmlElement(mainWindow, "Height", inputCfg.gui.heightNotMaximized);

    //window position
    addXmlElement(mainWindow, "PosX", inputCfg.gui.posXNotMaximized);
    addXmlElement(mainWindow, "PosY", inputCfg.gui.posYNotMaximized);
    addXmlElement(mainWindow, "Maximized", inputCfg.gui.isMaximized);

    addXmlElement(mainWindow, "ManualDeletionOnBothSides", inputCfg.gui.deleteOnBothSides);
    addXmlElement(mainWindow, "ManualDeletionUseRecycler", inputCfg.gui.useRecyclerForManualDeletion);
    addXmlElement(mainWindow, "ShowFileIcons"            , inputCfg.gui.showFileIcons);
    addXmlElement(mainWindow, "PopupOnConfigChange"      , inputCfg.gui.popupOnConfigChange);

    //write column attributes
    TiXmlElement* leftColumn = new TiXmlElement("LeftColumns");
    mainWindow->LinkEndChild(leftColumn);
    xmlAccess::ColumnAttributes columnAtrribLeftCopy = inputCfg.gui.columnAttribLeft; //can't change const vector
    sort(columnAtrribLeftCopy.begin(), columnAtrribLeftCopy.end(), xmlAccess::sortByPositionOnly);
    for (unsigned int i = 0; i < columnAtrribLeftCopy.size(); ++i)
    {
        TiXmlElement* subElement = new TiXmlElement("Column");
        leftColumn->LinkEndChild(subElement);

        const xmlAccess::ColumnAttrib& colAttrib = columnAtrribLeftCopy[i];
        subElement->SetAttribute("Type", colAttrib.type);
        if (colAttrib.visible) subElement->SetAttribute("Visible", "true");
        else subElement->SetAttribute("Visible", "false");
        subElement->SetAttribute("Width", colAttrib.width);
    }

    TiXmlElement* rightColumn = new TiXmlElement("RightColumns");
    mainWindow->LinkEndChild(rightColumn);
    xmlAccess::ColumnAttributes columnAtrribRightCopy = inputCfg.gui.columnAttribRight;
    sort(columnAtrribRightCopy.begin(), columnAtrribRightCopy.end(), xmlAccess::sortByPositionOnly);
    for (unsigned int i = 0; i < columnAtrribRightCopy.size(); ++i)
    {
        TiXmlElement* subElement = new TiXmlElement("Column");
        rightColumn->LinkEndChild(subElement);

        const xmlAccess::ColumnAttrib& colAttrib = columnAtrribRightCopy[i];
        subElement->SetAttribute("Type", colAttrib.type);
        if (colAttrib.visible) subElement->SetAttribute("Visible", "true");
        else subElement->SetAttribute("Visible", "false");
        subElement->SetAttribute("Width", colAttrib.width);
    }

    //write folder history elements
    TiXmlElement* historyLeft = new TiXmlElement("FolderHistoryLeft");
    mainWindow->LinkEndChild(historyLeft);
    TiXmlElement* historyRight = new TiXmlElement("FolderHistoryRight");
    mainWindow->LinkEndChild(historyRight);

    historyLeft->SetAttribute("MaximumSize", globalFunctions::numberToString(inputCfg.gui.folderHistLeftMax));
    addXmlElementTable(historyLeft, "Folder", inputCfg.gui.folderHistoryLeft);

    historyRight->SetAttribute("MaximumSize", globalFunctions::numberToString(inputCfg.gui.folderHistRightMax));
    addXmlElementTable(historyRight, "Folder", inputCfg.gui.folderHistoryRight);


    //commandline for file manager integration
    addXmlElement(gui, "FileManager", std::string((inputCfg.gui.commandLineFileManager).ToUTF8()));

    //write config file history
    TiXmlElement* cfgHistory = new TiXmlElement("ConfigHistory");
    gui->LinkEndChild(cfgHistory);

    cfgHistory->SetAttribute("MaximumSize", globalFunctions::numberToString(inputCfg.gui.cfgHistoryMax));
    addXmlElementTable(cfgHistory, "File", inputCfg.gui.cfgFileHistory);

    //###################################################################
    //write global batch settings

    TiXmlElement* batch = new TiXmlElement("Batch");
    root->LinkEndChild(batch);

    return true;
}


int xmlAccess::retrieveSystemLanguage()
{
    const int lang = wxLocale::GetSystemLanguage();

    switch (lang) //map language dialects
    {
        //variants of wxLANGUAGE_GERMAN
    case wxLANGUAGE_GERMAN_AUSTRIAN:
    case wxLANGUAGE_GERMAN_BELGIUM:
    case wxLANGUAGE_GERMAN_LIECHTENSTEIN:
    case wxLANGUAGE_GERMAN_LUXEMBOURG:
    case wxLANGUAGE_GERMAN_SWISS:
        return wxLANGUAGE_GERMAN;

        //variants of wxLANGUAGE_FRENCH
    case wxLANGUAGE_FRENCH_BELGIAN:
    case wxLANGUAGE_FRENCH_CANADIAN:
    case wxLANGUAGE_FRENCH_LUXEMBOURG:
    case wxLANGUAGE_FRENCH_MONACO:
    case wxLANGUAGE_FRENCH_SWISS:
        return wxLANGUAGE_FRENCH;

        //variants of wxLANGUAGE_DUTCH
    case wxLANGUAGE_DUTCH_BELGIAN:
        return wxLANGUAGE_DUTCH;

        //variants of wxLANGUAGE_ITALIAN
    case wxLANGUAGE_ITALIAN_SWISS:
        return wxLANGUAGE_ITALIAN;

        //variants of wxLANGUAGE_CHINESE_SIMPLIFIED
    case wxLANGUAGE_CHINESE:
    case wxLANGUAGE_CHINESE_TRADITIONAL:
    case wxLANGUAGE_CHINESE_HONGKONG:
    case wxLANGUAGE_CHINESE_MACAU:
    case wxLANGUAGE_CHINESE_SINGAPORE:
    case wxLANGUAGE_CHINESE_TAIWAN:
        return wxLANGUAGE_CHINESE_SIMPLIFIED;

        //variants of wxLANGUAGE_SPANISH
    case wxLANGUAGE_SPANISH_ARGENTINA:
    case wxLANGUAGE_SPANISH_BOLIVIA:
    case wxLANGUAGE_SPANISH_CHILE:
    case wxLANGUAGE_SPANISH_COLOMBIA:
    case wxLANGUAGE_SPANISH_COSTA_RICA:
    case wxLANGUAGE_SPANISH_DOMINICAN_REPUBLIC:
    case wxLANGUAGE_SPANISH_ECUADOR:
    case wxLANGUAGE_SPANISH_EL_SALVADOR:
    case wxLANGUAGE_SPANISH_GUATEMALA:
    case wxLANGUAGE_SPANISH_HONDURAS:
    case wxLANGUAGE_SPANISH_MEXICAN:
    case wxLANGUAGE_SPANISH_MODERN:
    case wxLANGUAGE_SPANISH_NICARAGUA:
    case wxLANGUAGE_SPANISH_PANAMA:
    case wxLANGUAGE_SPANISH_PARAGUAY:
    case wxLANGUAGE_SPANISH_PERU:
    case wxLANGUAGE_SPANISH_PUERTO_RICO:
    case wxLANGUAGE_SPANISH_URUGUAY:
    case wxLANGUAGE_SPANISH_US:
    case wxLANGUAGE_SPANISH_VENEZUELA:
        return wxLANGUAGE_SPANISH;

        //case wxLANGUAGE_JAPANESE:
        //case wxLANGUAGE_POLISH:
        //case wxLANGUAGE_SLOVENIAN:
        //case wxLANGUAGE_HUNGARIAN:
        //case wxLANGUAGE_PORTUGUESE:
        //case wxLANGUAGE_PORTUGUESE_BRAZILIAN:

    default:
        return lang;
    }
}


bool xmlAccess::supportForSymbolicLinks()
{
#ifdef FFS_WIN
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    //symbolic links are supported starting with Vista
    if (GetVersionEx(&osvi))
        return osvi.dwMajorVersion > 5; //XP has majorVersion == 5, minorVersion == 1, Vista majorVersion == 6

    return false;
#elif defined FFS_LINUX
    return true;
#endif
}


void xmlAccess::XmlGlobalSettings::_Shared::resetWarnings()
{
    warningDependentFolders        = true;
    warningSignificantDifference   = true;
    warningNotEnoughDiskSpace      = true;
    warningUnresolvedConflicts     = true;
    warningSynchronizationStarting = true;
}
