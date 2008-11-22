#include "processXml.h"
#include <wx/filefn.h>
#include <wx/ffile.h>
#include "globalFunctions.h"

using namespace globalFunctions;
using namespace xmlAccess;


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
    else
        return XML_OTHER;
}


XmlInput::XmlInput(const wxString& fileName, const XmlType type) :
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
                }
            }
        }
    }
}


bool XmlInput::readXmlElementValue(string& output, const TiXmlElement* parent, const string& name)
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


bool XmlInput::readXmlElementValue(int& output, const TiXmlElement* parent, const string& name)
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


bool XmlInput::readXmlElementValue(CompareVariant& output, const TiXmlElement* parent, const string& name)
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


bool XmlInput::readXmlElementValue(SyncDirection& output, const TiXmlElement* parent, const string& name)
{
    string dummy;
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


bool XmlInput::readXmlElementValue(bool& output, const TiXmlElement* parent, const string& name)
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


bool XmlInput::readXmlMainConfig(XmlMainConfig& outputCfg)
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
    if (!readXmlElementValue(outputCfg.cfg.compareVar, cmpSettings, "Variant")) return false;

    //read folder pair(s)
    TiXmlElement* folderPair = TiXmlHandle(cmpSettings).FirstChild("Folders").FirstChild("Pair").ToElement();

    //read folder pairs
    while (folderPair)
    {
        FolderPair newPair;

        if (!readXmlElementValue(tempString, folderPair, "Left")) return false;
        newPair.leftDirectory = wxString::FromUTF8(tempString.c_str());

        if (!readXmlElementValue(tempString, folderPair, "Right")) return false;
        newPair.rightDirectory = wxString::FromUTF8(tempString.c_str());

        outputCfg.directoryPairs.push_back(newPair);
        folderPair = folderPair->NextSiblingElement();
    }

//###########################################################
    //read sync configuration
    if (!readXmlElementValue(outputCfg.cfg.syncConfiguration.exLeftSideOnly, syncConfig, "LeftOnly"))   return false;
    if (!readXmlElementValue(outputCfg.cfg.syncConfiguration.exRightSideOnly, syncConfig, "RightOnly")) return false;
    if (!readXmlElementValue(outputCfg.cfg.syncConfiguration.leftNewer, syncConfig, "LeftNewer"))       return false;
    if (!readXmlElementValue(outputCfg.cfg.syncConfiguration.rightNewer, syncConfig, "RightNewer"))     return false;
    if (!readXmlElementValue(outputCfg.cfg.syncConfiguration.different, syncConfig, "Different"))       return false;
//###########################################################
    //read filter settings
    if (!readXmlElementValue(outputCfg.cfg.filterIsActive, filter, "Active")) return false;

    if (!readXmlElementValue(tempString, filter, "Include"))  return false;
    outputCfg.cfg.includeFilter = wxString::FromUTF8(tempString.c_str());

    if (!readXmlElementValue(tempString, filter, "Exclude"))  return false;
    outputCfg.cfg.excludeFilter = wxString::FromUTF8(tempString.c_str());
//###########################################################
    //other
    if (!readXmlElementValue(outputCfg.cfg.useRecycleBin, miscSettings, "Recycler"))   return false;
    if (!readXmlElementValue(outputCfg.cfg.continueOnError, miscSettings, "Continue")) return false;

    return true;
}


bool XmlInput::readXmlGuiConfig(XmlGuiConfig& outputCfg)
{
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlHandle hRoot(root);

    //read GUI layout
    TiXmlElement* mainWindow = hRoot.FirstChild("GuiConfig").FirstChild("Windows").FirstChild("Main").ToElement();
    if (mainWindow)
    {
        if (!readXmlElementValue(outputCfg.hideFilteredElements, mainWindow, "HideFiltered"))
            outputCfg.hideFilteredElements = false;

        //read application window size and position
        if (!readXmlElementValue(outputCfg.widthNotMaximized, mainWindow, "Width"))
            outputCfg.widthNotMaximized = -1;
        if (!readXmlElementValue(outputCfg.heightNotMaximized, mainWindow, "Height"))
            outputCfg.heightNotMaximized = -1;
        if (!readXmlElementValue(outputCfg.posXNotMaximized, mainWindow, "PosX"))
            outputCfg.posXNotMaximized = -1;
        if (!readXmlElementValue(outputCfg.posYNotMaximized, mainWindow, "PosY"))
            outputCfg.posYNotMaximized = -1;
        if (!readXmlElementValue(outputCfg.isMaximized, mainWindow, "Maximized"))
            outputCfg.isMaximized = false;

//###########################################################
        //read column widths
        TiXmlElement* leftColumn = TiXmlHandle(mainWindow).FirstChild("LeftColumns").FirstChild("Width").ToElement();
        while (leftColumn)
        {
            const char* width = leftColumn->GetText();
            if (width) //may be NULL!!
                outputCfg.columnWidthLeft.push_back(stringToInt(width));
            else
                break;

            leftColumn = leftColumn->NextSiblingElement();
        }

        TiXmlElement* rightColumn = TiXmlHandle(mainWindow).FirstChild("RightColumns").FirstChild("Width").ToElement();
        while (rightColumn)
        {
            const char* width = rightColumn->GetText();
            if (width) //may be NULL!!
                outputCfg.columnWidthRight.push_back(stringToInt(width));
            else
                break;

            rightColumn = rightColumn->NextSiblingElement();
        }
    }

    return true;
}


bool XmlInput::readXmlBatchConfig(XmlBatchConfig& outputCfg)
{
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlHandle hRoot(root);

    //read batch settings
    TiXmlElement* batchConfig = hRoot.FirstChild("BatchConfig").ToElement();
    if (batchConfig)
    {
        //read application window size and position
        if (!readXmlElementValue(outputCfg.silent, batchConfig, "Silent"))
            outputCfg.silent = false;
    }

    return true;
}


XmlOutput::XmlOutput(const wxString& fileName, const XmlType type) :
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
    else
        assert(false);
    doc.LinkEndChild(root);
}


bool XmlOutput::writeToFile()
{
    //workaround to get a FILE* from a unicode filename
    wxFFile dummyFile(m_fileName, wxT("wb"));
    if (!dummyFile.IsOpened())
        return false;

    FILE* outputFile = dummyFile.fp();

    return doc.SaveFile(outputFile); //save XML
}


void XmlOutput::addXmlElement(TiXmlElement* parent, const string& name, const string& value)
{
    if (parent)
    {
        TiXmlElement* subElement = new TiXmlElement(name);
        parent->LinkEndChild(subElement);
        subElement->LinkEndChild(new TiXmlText(value));
    }
}


void XmlOutput::addXmlElement(TiXmlElement* parent, const string& name, const int value)
{
    addXmlElement(parent, name, numberToString(value));
}


void XmlOutput::addXmlElement(TiXmlElement* parent, const string& name, const SyncDirection value)
{
    if (value == SYNC_DIR_LEFT)
        addXmlElement(parent, name, string("left"));
    else if (value == SYNC_DIR_RIGHT)
        addXmlElement(parent, name, string("right"));
    else if (value == SYNC_DIR_NONE)
        addXmlElement(parent, name, string("none"));
    else
        assert(false);
}


void XmlOutput::addXmlElement(TiXmlElement* parent, const string& name, const bool value)
{
    if (value)
        addXmlElement(parent, name, string("true"));
    else
        addXmlElement(parent, name, string("false"));
}


bool XmlOutput::writeXmlMainConfig(const XmlMainConfig& inputCfg)
{
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlElement* settings = new TiXmlElement("MainConfig");
    root->LinkEndChild(settings);

//###########################################################
    TiXmlElement* cmpSettings = new TiXmlElement("Comparison");
    settings->LinkEndChild(cmpSettings);

    //write compare algorithm
    addXmlElement(cmpSettings, "Variant", inputCfg.cfg.compareVar);

    //write folder pair(s)
    TiXmlElement* folders = new TiXmlElement("Folders");
    cmpSettings->LinkEndChild(folders);

    //write folder pairs
    for (vector<FolderPair>::const_iterator i = inputCfg.directoryPairs.begin(); i != inputCfg.directoryPairs.end(); ++i)
    {
        TiXmlElement* folderPair = new TiXmlElement("Pair");
        folders->LinkEndChild(folderPair);

        addXmlElement(folderPair, "Left", string((i->leftDirectory).ToUTF8()));
        addXmlElement(folderPair, "Right", string((i->rightDirectory).ToUTF8()));
    }

//###########################################################
    TiXmlElement* syncSettings = new TiXmlElement("Synchronization");
    settings->LinkEndChild(syncSettings);

    //write sync configuration
    TiXmlElement* syncConfig = new TiXmlElement("Directions");
    syncSettings->LinkEndChild(syncConfig);

    addXmlElement(syncConfig, "LeftOnly",   inputCfg.cfg.syncConfiguration.exLeftSideOnly);
    addXmlElement(syncConfig, "RightOnly",  inputCfg.cfg.syncConfiguration.exRightSideOnly);
    addXmlElement(syncConfig, "LeftNewer",  inputCfg.cfg.syncConfiguration.leftNewer);
    addXmlElement(syncConfig, "RightNewer", inputCfg.cfg.syncConfiguration.rightNewer);
    addXmlElement(syncConfig, "Different",  inputCfg.cfg.syncConfiguration.different);

//###########################################################
    TiXmlElement* miscSettings = new TiXmlElement("Miscellaneous");
    settings->LinkEndChild(miscSettings);

    //write filter settings
    TiXmlElement* filter = new TiXmlElement("Filter");
    miscSettings->LinkEndChild(filter);

    addXmlElement(filter, "Active", inputCfg.cfg.filterIsActive);
    addXmlElement(filter, "Include", string((inputCfg.cfg.includeFilter).ToUTF8()));
    addXmlElement(filter, "Exclude", string((inputCfg.cfg.excludeFilter).ToUTF8()));

    //other
    addXmlElement(miscSettings, "Recycler", inputCfg.cfg.useRecycleBin);
    addXmlElement(miscSettings, "Continue", inputCfg.cfg.continueOnError);
//###########################################################

    return true;
}


bool XmlOutput::writeXmlGuiConfig(const XmlGuiConfig& inputCfg)
{
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlElement* guiLayout = new TiXmlElement("GuiConfig");
    root->LinkEndChild(guiLayout);

    TiXmlElement* windows = new TiXmlElement("Windows");
    guiLayout->LinkEndChild(windows);

    TiXmlElement* mainWindow = new TiXmlElement("Main");
    windows->LinkEndChild(mainWindow);

    addXmlElement(mainWindow, "HideFiltered", inputCfg.hideFilteredElements);

    //window size
    addXmlElement(mainWindow, "Width", inputCfg.widthNotMaximized);
    addXmlElement(mainWindow, "Height", inputCfg.heightNotMaximized);

    //window position
    addXmlElement(mainWindow, "PosX", inputCfg.posXNotMaximized);
    addXmlElement(mainWindow, "PosY", inputCfg.posYNotMaximized);
    addXmlElement(mainWindow, "Maximized", inputCfg.isMaximized);

    //write column sizes
    TiXmlElement* leftColumn = new TiXmlElement("LeftColumns");
    mainWindow->LinkEndChild(leftColumn);

    for (unsigned int i = 0; i < inputCfg.columnWidthLeft.size(); ++i)
        addXmlElement(leftColumn, "Width", inputCfg.columnWidthLeft[i]);

    TiXmlElement* rightColumn = new TiXmlElement("RightColumns");
    mainWindow->LinkEndChild(rightColumn);

    for (unsigned int i = 0; i < inputCfg.columnWidthRight.size(); ++i)
        addXmlElement(rightColumn, "Width", inputCfg.columnWidthRight[i]);

    return true;
}


bool XmlOutput::writeXmlBatchConfig(const XmlBatchConfig& inputCfg)
{
    TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    TiXmlElement* batchConfig = new TiXmlElement("BatchConfig");
    root->LinkEndChild(batchConfig);

    addXmlElement(batchConfig, "Silent", inputCfg.silent);

    return true;
}
