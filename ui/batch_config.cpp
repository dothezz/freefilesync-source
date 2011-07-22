// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "batch_config.h"
#include "../shared/dir_picker_i18n.h"
#include "folder_pair.h"
#include <iterator>
#include <wx/wupdlock.h>
#include "../shared/help_provider.h"
#include "../shared/file_handling.h"
#include "msg_popup.h"
#include "gui_generated.h"
#include <wx/dnd.h>
#include <wx/msgdlg.h>
#include "../shared/custom_button.h"
#include "../shared/wx_choice_enum.h"
#include "../shared/mouse_move_dlg.h"

using namespace zen;

class DirectoryPairBatchFirst;
class DirectoryPairBatch;


class BatchDialog: public BatchDlgGenerated
{
    friend class BatchFileDropEvent;
    template <class GuiPanel>
    friend class FolderPairCallback;

public:
    BatchDialog(wxWindow* window, const xmlAccess::XmlBatchConfig& batchCfg);
    BatchDialog(wxWindow* window, const wxString& filename);
    ~BatchDialog();

private:
    void init();

    virtual void OnCmpSettings(        wxCommandEvent& event);
    virtual void OnSyncSettings(       wxCommandEvent& event);
    virtual void OnConfigureFilter(    wxCommandEvent& event);

    virtual void OnHelp(               wxCommandEvent& event);

    void OnGlobalFilterOpenContext(wxCommandEvent& event);
    void OnGlobalFilterRemConfirm(wxCommandEvent& event);
    virtual void OnCheckSaveLog(        wxCommandEvent& event);
    virtual void OnChangeMaxLogCountTxt(wxCommandEvent& event);
    virtual void OnClose(              wxCloseEvent&   event);
    virtual void OnCancel(             wxCommandEvent& event);
    virtual void OnSaveBatchJob(       wxCommandEvent& event);
    virtual void OnLoadBatchJob(       wxCommandEvent& event);
    virtual void OnAddFolderPair(      wxCommandEvent& event);
    virtual void OnRemoveFolderPair(   wxCommandEvent& event);
    virtual void OnRemoveTopFolderPair(wxCommandEvent& event);
    void OnFilesDropped(FFSFileDropEvent& event);

    void addFolderPair(const std::vector<zen::FolderPairEnh>& newPairs, bool addFront = false);
    void removeAddFolderPair(const int pos);
    void clearAddFolderPairs();

    void updateGuiForFolderPair();

    void updateGui(); //re-evaluate gui after config changes

    void showNotebookpage(wxWindow* page, const wxString& pageName, bool show);

    //error handling
    //xmlAccess::OnError getSelectionHandleError() const; -> obsolete, use getEnumVal()
    void setSelectionHandleError(xmlAccess::OnError value);
    void OnChangeErrorHandling(wxCommandEvent& event);

    bool saveBatchFile(const wxString& filename);
    void loadBatchFile(const wxString& filename);
    void loadBatchFile(const std::vector<wxString>& filenames);

    void setConfig(const xmlAccess::XmlBatchConfig& batchCfg);
    xmlAccess::XmlBatchConfig getConfig() const;

    std::shared_ptr<DirectoryPairBatchFirst> firstFolderPair; //always bound!!!
    std::vector<DirectoryPairBatch*> additionalFolderPairs;

    //used when saving batch file
    wxString proposedBatchFileName;

    xmlAccess::XmlBatchConfig localBatchCfg;

    std::unique_ptr<wxMenu> contextMenu;

    std::unique_ptr<zen::DirectoryName> logfileDir;

    zen::EnumDescrList<xmlAccess::OnError> enumDescrMap;
};

//###################################################################################################################################

//------------------------------------------------------------------
/*    class hierarchy:

            template<>
            FolderPairPanelBasic
                    /|\
                     |
             template<>
             FolderPairCallback      BatchFolderPairGenerated
                    /|\                        /|\
            _________|______________    ________|
           |                        |  |
  DirectoryPairBatchFirst    DirectoryPairBatch
*/

template <class GuiPanel>
class FolderPairCallback : public FolderPairPanelBasic<GuiPanel> //implements callback functionality to BatchDialog as imposed by FolderPairPanelBasic
{
public:
    FolderPairCallback(GuiPanel& basicPanel, BatchDialog& batchDialog) :
        FolderPairPanelBasic<GuiPanel>(basicPanel), //pass FolderPairGenerated part...
        batchDlg(batchDialog) {}

private:
    virtual wxWindow* getParentWindow()
    {
        return &batchDlg;
    }

    virtual MainConfiguration getMainConfig() const
    {
        return batchDlg.getConfig().mainCfg;
    }

    virtual void OnAltSyncCfgChange()
    {
        batchDlg.updateGui();
    }

    virtual void OnAltSyncCfgRemoveConfirm(wxCommandEvent& event)
    {
        FolderPairPanelBasic<GuiPanel>::OnAltSyncCfgRemoveConfirm(event);
        batchDlg.updateGui();
    }

    virtual void OnLocalFilterCfgChange() {}


    BatchDialog& batchDlg;
};


class DirectoryPairBatch:
    public BatchFolderPairGenerated, //DirectoryPairBatch "owns" BatchFolderPairGenerated!
    public FolderPairCallback<BatchFolderPairGenerated>
{
public:
    DirectoryPairBatch(wxWindow* parent, BatchDialog& batchDialog) :
        BatchFolderPairGenerated(parent),
        FolderPairCallback<BatchFolderPairGenerated>(static_cast<BatchFolderPairGenerated&>(*this), batchDialog), //pass BatchFolderPairGenerated part...
        dirNameLeft (*m_panelLeft,  *m_dirPickerLeft,  *m_directoryLeft),
        dirNameRight(*m_panelRight, *m_dirPickerRight, *m_directoryRight) {}

    void setValues(const wxString& leftDir, const wxString& rightDir, AltSyncCfgPtr syncCfg, const FilterConfig& filter)
    {
        setConfig(syncCfg, filter);
        dirNameLeft.setName(leftDir);
        dirNameRight.setName(rightDir);
    }
    wxString getLeftDir () const { return dirNameLeft .getName(); }
    wxString getRightDir() const { return dirNameRight.getName(); }

private:
    //support for drag and drop
    DirectoryName dirNameLeft;
    DirectoryName dirNameRight;
};


class DirectoryPairBatchFirst : public FolderPairCallback<BatchDlgGenerated>
{
public:
    DirectoryPairBatchFirst(BatchDialog& batchDialog) :
        FolderPairCallback<BatchDlgGenerated>(batchDialog, batchDialog),

        //prepare drag & drop
        dirNameLeft(*batchDialog.m_panelLeft,
                    *batchDialog.m_dirPickerLeft,
                    *batchDialog.m_directoryLeft),
        dirNameRight(*batchDialog.m_panelRight,
                     *batchDialog.m_dirPickerRight,
                     *batchDialog.m_directoryRight) {}

    void setValues(const wxString& leftDir, const wxString& rightDir, AltSyncCfgPtr syncCfg, const FilterConfig& filter)
    {
        setConfig(syncCfg, filter);
        dirNameLeft.setName(leftDir);
        dirNameRight.setName(rightDir);
    }
    wxString getLeftDir () const { return dirNameLeft .getName(); }
    wxString getRightDir() const { return dirNameRight.getName(); }

private:
    //support for drag and drop
    DirectoryName dirNameLeft;
    DirectoryName dirNameRight;
};


//###################################################################################################################################
BatchDialog::BatchDialog(wxWindow* window, const xmlAccess::XmlBatchConfig& batchCfg) :
    BatchDlgGenerated(window)
{
    init();
    setConfig(batchCfg);
}


BatchDialog::BatchDialog(wxWindow* window, const wxString& filename) :
    BatchDlgGenerated(window)
{
    init();
    loadBatchFile(filename);
}


BatchDialog::~BatchDialog() {} //non-inline destructor for std::auto_ptr to work with forward declaration


void BatchDialog::init()
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    wxWindowUpdateLocker dummy(this); //avoid display distortion

    enumDescrMap.
    add(xmlAccess::ON_ERROR_POPUP , _("Show popup")    , _("Show popup on errors or warnings")).
    add(xmlAccess::ON_ERROR_IGNORE, _("Ignore errors") , _("Hide all error and warning messages")).
    add(xmlAccess::ON_ERROR_EXIT  , _("Exit instantly"), _("Abort synchronization immediately"));


    m_bpButtonCmpConfig->SetBitmapLabel(GlobalResources::instance().getImage(wxT("cmpConfig")));
    m_bpButtonSyncConfig->SetBitmapLabel(GlobalResources::instance().getImage(wxT("syncConfig")));

    m_bpButtonHelp->SetBitmapLabel(GlobalResources::instance().getImage(wxT("help")));

    //init handling of first folder pair
    firstFolderPair.reset(new DirectoryPairBatchFirst(*this));

    m_bpButtonFilter->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(BatchDialog::OnGlobalFilterOpenContext), NULL, this);

    //prepare drag & drop for loading of *.ffs_batch files
    setupFileDrop(*this);
    Connect(FFS_DROP_FILE_EVENT, FFSFileDropEventHandler(BatchDialog::OnFilesDropped), NULL, this);

    logfileDir.reset(new DirectoryName(*m_panelBatchSettings, *m_dirPickerLogfileDir, *m_textCtrlLogfileDir));

    //set icons for this dialog
    m_bpButtonAddPair->SetBitmapLabel(GlobalResources::instance().getImage(wxT("addFolderPair")));
    m_bitmap27->SetBitmap(GlobalResources::instance().getImage(wxT("batch")));

    m_buttonSave->SetFocus();
}

//------------------- error handling --------------------------

void BatchDialog::setSelectionHandleError(xmlAccess::OnError value)
{
    //    if (m_checkBoxSilent->GetValue())
    setEnumVal(enumDescrMap, *m_choiceHandleError, value);
    /*  else
      {
          EnumDescrList<xmlAccess::OnError> tmp(enumDescrMap);
          tmp.descrList.pop_back(); //remove "Exit instantly" -> this option shall be available for silent mode only!
          setEnumVal(tmp, *m_choiceHandleError, value);
      }
      */
}

void BatchDialog::OnChangeErrorHandling(wxCommandEvent& event)
{
    updateTooltipEnumVal(enumDescrMap, *m_choiceHandleError);
}


void BatchDialog::OnCmpSettings(wxCommandEvent& event)
{
    //show window right next to the compare-config button
    //wxPoint windowPos = m_bpButtonCmpConfig->GetScreenPosition();
    //windowPos.x += m_bpButtonCmpConfig->GetSize().GetWidth() + 5;

    if (zen::showCompareCfgDialog(localBatchCfg.mainCfg.compareVar,
                                  localBatchCfg.mainCfg.handleSymlinks) == ReturnSmallDlg::BUTTON_OKAY)
    {
        updateGui();
    }
}


void BatchDialog::OnSyncSettings(wxCommandEvent& event)
{
    if (showSyncConfigDlg(localBatchCfg.mainCfg.compareVar,
                          localBatchCfg.mainCfg.syncConfiguration,
                          localBatchCfg.mainCfg.handleDeletion,
                          localBatchCfg.mainCfg.customDeletionDirectory,
                          NULL) == ReturnSyncConfig::BUTTON_OKAY) //optional input parameter
    {
        updateGui();
    }
}


void BatchDialog::OnConfigureFilter(wxCommandEvent& event)
{
    if (showFilterDialog(true, //is main filter dialog
                         localBatchCfg.mainCfg.globalFilter) == ReturnSmallDlg::BUTTON_OKAY)
    {
        updateGui();
    }
}


void BatchDialog::updateGui() //re-evaluate gui after config changes
{
    xmlAccess::XmlBatchConfig cfg = getConfig();

    //showNotebookpage(m_panelLogging, _("Logging"), cfg.silent);

    m_panelLogfile->Enable(cfg.logFileCountMax > 0);

    //update compare variant name
    m_staticTextCmpVariant->SetLabel(wxString(wxT("(")) + getVariantName(cfg.mainCfg.compareVar) + wxT(")"));

    //update sync variant name
    m_staticTextSyncVariant->SetLabel(wxString(wxT("(")) + cfg.mainCfg.getSyncVariantName() + wxT(")"));

    //set filter icon
    if (isNullFilter(cfg.mainCfg.globalFilter))
    {
        setBitmapLabel(*m_bpButtonFilter, GlobalResources::instance().getImage(wxT("filterOff")));
        m_bpButtonFilter->SetToolTip(_("No filter selected"));
    }
    else
    {
        setBitmapLabel(*m_bpButtonFilter, GlobalResources::instance().getImage(wxT("filterOn")));
        m_bpButtonFilter->SetToolTip(_("Filter is active"));
    }

    m_panelOverview->Layout(); //adjust stuff inside scrolled window
}


void BatchDialog::OnGlobalFilterOpenContext(wxCommandEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu

    wxMenuItem* itemClear = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Clear filter settings"));
    contextMenu->Append(itemClear);
    contextMenu->Connect(itemClear->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(BatchDialog::OnGlobalFilterRemConfirm), NULL, this);

    if (isNullFilter(localBatchCfg.mainCfg.globalFilter))
        contextMenu->Enable(itemClear->GetId(), false); //disable menu item, if clicking wouldn't make sense anyway

    PopupMenu(contextMenu.get()); //show context menu
}


void BatchDialog::OnGlobalFilterRemConfirm(wxCommandEvent& event)
{
    localBatchCfg.mainCfg.globalFilter = FilterConfig();
    updateGui();
}


void BatchDialog::OnCheckSaveLog(wxCommandEvent& event)
{
    updateGui();

    //reset error handling depending on "m_checkBoxSilent"
    //setSelectionHandleError(getEnumVal(enumDescrMap, *m_choiceHandleError));
}


void BatchDialog::OnChangeMaxLogCountTxt(wxCommandEvent& event)
{
    updateGui();
}


void BatchDialog::OnFilesDropped(FFSFileDropEvent& event)
{
    if (event.getFiles().empty())
        return;

    const std::vector<wxString>& fileList = event.getFiles();

    switch (xmlAccess::getMergeType(fileList))   //throw ()
    {
        case xmlAccess::MERGE_BATCH:
        case xmlAccess::MERGE_GUI:
        case xmlAccess::MERGE_GUI_BATCH:
            loadBatchFile(fileList);
            break;

        case xmlAccess::MERGE_OTHER:
            wxMessageBox(_("Invalid FreeFileSync config file!"), _("Error"), wxOK | wxICON_ERROR);
            break;
    }
}


void BatchDialog::OnHelp(wxCommandEvent& event)
{
#ifdef FFS_WIN
    zen::displayHelpEntry(wxT("html\\advanced\\ScheduleBatch.html"));
#elif defined FFS_LINUX
    zen::displayHelpEntry(wxT("html/advanced/ScheduleBatch.html"));
#endif
}


void BatchDialog::showNotebookpage(wxWindow* page, const wxString& pageName, bool show)
{
    int windowPosition = -1;
    for (size_t i = 0; i < m_notebookSettings->GetPageCount(); ++i)
        if (m_notebookSettings->GetPage(i) == page)
        {
            windowPosition = static_cast<int>(i);
            break;
        }

    if (show)
    {
        if (windowPosition == -1)
            m_notebookSettings->AddPage(page, pageName, false);
    }
    else
    {
        if (windowPosition != -1)
        {
            //do not delete currently selected tab!!
            if (m_notebookSettings->GetCurrentPage() == m_notebookSettings->GetPage(windowPosition))
                m_notebookSettings->ChangeSelection(0);

            m_notebookSettings->RemovePage(windowPosition);
        }
    }
}


void BatchDialog::OnClose(wxCloseEvent&   event)
{
    EndModal(ReturnBatchConfig::BUTTON_CANCEL);
}


void BatchDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(ReturnBatchConfig::BUTTON_CANCEL);
}


void BatchDialog::OnSaveBatchJob(wxCommandEvent& event)
{
    //get a filename
    wxString defaultFileName = proposedBatchFileName.empty() ? wxT("SyncJob.ffs_batch") : proposedBatchFileName;

    //attention: proposedBatchFileName may be an imported *.ffs_gui file! We don't want to overwrite it with a BATCH config!
    if (defaultFileName.EndsWith(wxT(".ffs_gui")))
        defaultFileName.Replace(wxT(".ffs_gui"), wxT(".ffs_batch"), false);


    wxFileDialog filePicker(this,
                            wxEmptyString,
                            wxEmptyString,
                            defaultFileName,
                            wxString(_("FreeFileSync batch file")) + wxT(" (*.ffs_batch)|*.ffs_batch"),
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT); //creating this on freestore leads to memleak!
    if (filePicker.ShowModal() == wxID_OK)
    {
        const wxString newFileName = filePicker.GetPath();

        //create batch file
        if (saveBatchFile(newFileName))
            EndModal(ReturnBatchConfig::BATCH_FILE_SAVED);
    }
}


void BatchDialog::OnLoadBatchJob(wxCommandEvent& event)
{
    wxFileDialog filePicker(this,
                            wxEmptyString,
                            beforeLast(proposedBatchFileName, FILE_NAME_SEPARATOR), //set default dir: empty string if "currentConfigFileName" is empty or has no path separator
                            wxEmptyString,
                            wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs_batch;*.ffs_gui)|*.ffs_batch;*.ffs_gui"),
                            wxFD_OPEN | wxFD_MULTIPLE); //creating this on freestore leads to memleak!
    if (filePicker.ShowModal() == wxID_OK)
    {
        wxArrayString tmp;
        filePicker.GetPaths(tmp);
        std::vector<wxString> fileNames(tmp.begin(), tmp.end());

        loadBatchFile(fileNames);
    }
}



inline
FolderPairEnh getEnhancedPair(const DirectoryPairBatch* panel)
{
    return FolderPairEnh(toZ(panel->getLeftDir()),
                         toZ(panel->getRightDir()),
                         panel->getAltSyncConfig(),
                         panel->getAltFilterConfig());
}


bool BatchDialog::saveBatchFile(const wxString& filename)
{
    const xmlAccess::XmlBatchConfig batchCfg = getConfig();

    //write config to XML
    try
    {
        xmlAccess::writeConfig(batchCfg, filename);
    }
    catch (const xmlAccess::FfsXmlError& error)
    {
        wxMessageBox(error.msg().c_str(), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    SetTitle(filename);
    proposedBatchFileName = filename; //may be used on next save

    return true;
}


void BatchDialog::loadBatchFile(const wxString& filename)
{
    std::vector<wxString> filenames;
    filenames.push_back(filename);
    loadBatchFile(filenames);
}


void BatchDialog::loadBatchFile(const std::vector<wxString>& filenames)
{
    if (filenames.empty())
        return;

    //load XML settings
    xmlAccess::XmlBatchConfig batchCfg;  //structure to receive gui settings
    try
    {
        //open a *.ffs_gui or *.ffs_batch file!
        xmlAccess::convertConfig(filenames, batchCfg); //throw (xmlAccess::FfsXmlError)

        //xmlAccess::readConfig(filename, batchCfg);
    }
    catch (const xmlAccess::FfsXmlError& error)
    {
        if (error.getSeverity() == xmlAccess::FfsXmlError::WARNING)
            wxMessageBox(error.msg(), _("Warning"), wxOK | wxICON_WARNING);
        else
        {
            wxMessageBox(error.msg(), _("Error"), wxOK | wxICON_ERROR);
            return;
        }
    }

    const wxString activeFile = filenames.size() == 1 ? filenames[0] : wxString();

    if (activeFile.empty())
        SetTitle(_("Create a batch job"));
    else
        SetTitle(activeFile);

    proposedBatchFileName = activeFile; //may be used on next save

    setConfig(batchCfg);
}


void BatchDialog::setConfig(const xmlAccess::XmlBatchConfig& batchCfg)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    //make working copy
    localBatchCfg = batchCfg;

    m_checkBoxSilent->SetValue(batchCfg.silent);

    //error handling is dependent from m_checkBoxSilent! /|\   \|/
    setSelectionHandleError(batchCfg.handleError);

    logfileDir->setName(batchCfg.logFileDirectory);
    m_spinCtrlLogCountMax->SetValue(static_cast<int>(batchCfg.logFileCountMax)); //attention: this one emits a "change value" event!! => updateGui() called implicitly!

    //set first folder pair
    firstFolderPair->setValues(toWx(batchCfg.mainCfg.firstPair.leftDirectory),
                               toWx(batchCfg.mainCfg.firstPair.rightDirectory),
                               batchCfg.mainCfg.firstPair.altSyncConfig,
                               batchCfg.mainCfg.firstPair.localFilter);

    //remove existing additional folder pairs
    clearAddFolderPairs();

    //set additional pairs
    addFolderPair(batchCfg.mainCfg.additionalPairs);

    updateGui(); //re-evaluate gui after config changes

    Fit(); //needed
    Refresh(); //needed
    Centre();
}


xmlAccess::XmlBatchConfig BatchDialog::getConfig() const
{
    xmlAccess::XmlBatchConfig batchCfg = localBatchCfg;

    //load parameter with ownership within wxWidgets controls...

    //first folder pair
    batchCfg.mainCfg.firstPair = FolderPairEnh(toZ(firstFolderPair->getLeftDir()),
                                               toZ(firstFolderPair->getRightDir()),
                                               firstFolderPair->getAltSyncConfig(),
                                               firstFolderPair->getAltFilterConfig());

    //add additional pairs
    batchCfg.mainCfg.additionalPairs.clear();
    std::transform(additionalFolderPairs.begin(), additionalFolderPairs.end(),
                   std::back_inserter(batchCfg.mainCfg.additionalPairs), getEnhancedPair);


    //load structure with batch settings "batchCfg"
    batchCfg.silent           = m_checkBoxSilent->GetValue();
    batchCfg.logFileDirectory = logfileDir->getName();
    batchCfg.logFileCountMax  = m_spinCtrlLogCountMax->GetValue();
    batchCfg.handleError      = getEnumVal(enumDescrMap, *m_choiceHandleError);

    return batchCfg;
}


void BatchDialog::OnAddFolderPair(wxCommandEvent& event)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    std::vector<FolderPairEnh> newPairs;
    newPairs.push_back(getConfig().mainCfg.firstPair);

    addFolderPair(newPairs, true); //add pair in front of additonal pairs

    //clear first pair
    const FolderPairEnh cfgEmpty;
    firstFolderPair->setValues(toWx(cfgEmpty.leftDirectory),
                               toWx(cfgEmpty.rightDirectory),
                               cfgEmpty.altSyncConfig,
                               cfgEmpty.localFilter);
}


void BatchDialog::OnRemoveFolderPair(wxCommandEvent& event)
{
    //find folder pair originating the event
    const wxObject* const eventObj = event.GetEventObject();
    for (std::vector<DirectoryPairBatch*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
    {
        if (eventObj == static_cast<wxObject*>((*i)->m_bpButtonRemovePair))
        {
            removeAddFolderPair(i - additionalFolderPairs.begin());
            return;
        }
    }
}


void BatchDialog::OnRemoveTopFolderPair(wxCommandEvent& event)
{
    if (additionalFolderPairs.size() > 0)
    {
        //get settings from second folder pair
        const FolderPairEnh cfgSecond = getEnhancedPair(additionalFolderPairs[0]);

        //reset first pair
        firstFolderPair->setValues(toWx(cfgSecond.leftDirectory),
                                   toWx(cfgSecond.rightDirectory),
                                   cfgSecond.altSyncConfig,
                                   cfgSecond.localFilter);

        removeAddFolderPair(0); //remove second folder pair (first of additional folder pairs)
    }
}


void BatchDialog::updateGuiForFolderPair()
{
    //adapt delete top folder pair button
    if (additionalFolderPairs.size() == 0)
        m_bpButtonRemovePair->Hide();
    else
        m_bpButtonRemovePair->Show();

    //adapt local filter and sync cfg for first folder pair
    if (additionalFolderPairs.size() == 0 &&
        firstFolderPair->getAltSyncConfig().get() == NULL &&
        isNullFilter(firstFolderPair->getAltFilterConfig()))
    {
        m_bpButtonLocalFilter->Hide();
        m_bpButtonAltSyncCfg->Hide();
    }
    else
    {
        m_bpButtonLocalFilter->Show();
        m_bpButtonAltSyncCfg->Show();
    }

    //update controls
    const int maxAddFolderPairsVisible = 2;

    int pairHeight = sbSizerMainPair->GetSize().GetHeight(); //respect height of main pair
    if (additionalFolderPairs.size() > 0)
        pairHeight += std::min<double>(maxAddFolderPairsVisible + 0.5, additionalFolderPairs.size()) * //have 0.5 * height indicate that more folders are there
                      additionalFolderPairs[0]->GetSize().GetHeight();

    m_scrolledWindow6->SetMinSize(wxSize( -1, pairHeight));


    m_scrolledWindow6->Fit();     //adjust scrolled window size
    m_scrolledWindow6->Layout();  //adjust scrolled window size

    //bSizerAddFolderPairs->FitInside(m_scrolledWindow6);  //adjust scrolled window size

    m_panelOverview->Layout(); //adjust stuff inside scrolled window
    //m_panelOverview->InvalidateBestSize(); //needed for Fit() to work correctly!

    if (m_scrolledWindow6->GetSize().GetHeight() < pairHeight)
        Fit();                     //adapt dialog size
}


void BatchDialog::addFolderPair(const std::vector<zen::FolderPairEnh>& newPairs, bool addFront)
{
    wxWindowUpdateLocker dummy(m_panelOverview); //avoid display distortion

    if (!newPairs.empty())
    {
        //add folder pairs
        for (std::vector<zen::FolderPairEnh>::const_iterator i = newPairs.begin(); i != newPairs.end(); ++i)
        {
            DirectoryPairBatch* newPair = new DirectoryPairBatch(m_scrolledWindow6, *this);

            if (addFront)
            {
                bSizerAddFolderPairs->Insert(0, newPair, 0, wxEXPAND, 5);
                additionalFolderPairs.insert(additionalFolderPairs.begin(), newPair);
            }
            else
            {
                bSizerAddFolderPairs->Add(newPair, 0, wxEXPAND, 5);
                additionalFolderPairs.push_back(newPair);
            }

            //register events
            newPair->m_bpButtonRemovePair->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(BatchDialog::OnRemoveFolderPair), NULL, this );

            //set alternate configuration
            newPair->setValues(toWx(i->leftDirectory),
                               toWx(i->rightDirectory),
                               i->altSyncConfig,
                               i->localFilter);
        }

        //after changing folder pairs window focus is lost: results in scrolled window scrolling to top each time window is shown: we don't want this
        m_bpButtonAddPair->SetFocus();
    }

    updateGuiForFolderPair();
}


void BatchDialog::removeAddFolderPair(const int pos)
{
    wxWindowUpdateLocker dummy(m_panelOverview); //avoid display distortion

    if (0 <= pos && pos < static_cast<int>(additionalFolderPairs.size()))
    {
        //remove folder pairs from window
        DirectoryPairBatch* pairToDelete = additionalFolderPairs[pos];

        bSizerAddFolderPairs->Detach(pairToDelete); //Remove() does not work on Window*, so do it manually
        pairToDelete->Destroy();                 //
        additionalFolderPairs.erase(additionalFolderPairs.begin() + pos); //remove last element in vector

        //after changing folder pairs window focus is lost: results in scrolled window scrolling to top each time window is shown: we don't want this
        m_bpButtonCmpConfig->SetFocus();
    }

    updateGuiForFolderPair();
}


void BatchDialog::clearAddFolderPairs()
{
    wxWindowUpdateLocker dummy(m_panelOverview); //avoid display distortion

    additionalFolderPairs.clear();
    bSizerAddFolderPairs->Clear(true);

    updateGuiForFolderPair();
}


/*
#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include <shlobj.h>
#endif  // FFS_WIN

template <typename T>
struct CleanUp
{
    CleanUp(T* element) : m_element(element) {}

    ~CleanUp()
    {
        m_element->Release();
    }

    T* m_element;
};


bool BatchDialog::createBatchFile(const wxString& filename)
{
    //create shell link (instead of batch file) for full Unicode support
    HRESULT hResult = E_FAIL;
    IShellLink* pShellLink = NULL;

    if (FAILED(CoCreateInstance(CLSID_ShellLink,       //class identifier
                                NULL,                  //object isn't part of an aggregate
                                CLSCTX_INPROC_SERVER,  //context for running executable code
                                IID_IShellLink,        //interface identifier
                                (void**)&pShellLink))) //pointer to storage of interface pointer
        return false;
    CleanUp<IShellLink> cleanOnExit(pShellLink);

    wxString freeFileSyncExe = wxStandardPaths::Get().GetExecutablePath();
    if (FAILED(pShellLink->SetPath(freeFileSyncExe.c_str())))
        return false;

    if (FAILED(pShellLink->SetArguments(getCommandlineArguments().c_str())))
        return false;

    if (FAILED(pShellLink->SetIconLocation(freeFileSyncExe.c_str(), 1))) //second icon from executable file is used
        return false;

    if (FAILED(pShellLink->SetDescription(_("FreeFileSync Batch Job"))))
        return false;

    IPersistFile*  pPersistFile = NULL;
    if (FAILED(pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile)))
        return false;
    CleanUp<IPersistFile> cleanOnExit2(pPersistFile);

    //pPersistFile->Save accepts unicode input only
#ifdef _UNICODE
    hResult = pPersistFile->Save(filename.c_str(), TRUE);
#else
    WCHAR wszTemp [MAX_PATH];
    if (MultiByteToWideChar(CP_ACP, 0, filename.c_str(), -1, wszTemp, MAX_PATH) == 0)
        return false;

    hResult = pPersistFile->Save(wszTemp, TRUE);
#endif
    if (FAILED(hResult))
        return false;

    return true;
}
*/



ReturnBatchConfig::ButtonPressed zen::showSyncBatchDlg(const wxString& filename)
{
    BatchDialog batchDlg(NULL, filename);
    return static_cast<ReturnBatchConfig::ButtonPressed>(batchDlg.ShowModal());
}


ReturnBatchConfig::ButtonPressed zen::showSyncBatchDlg(const xmlAccess::XmlBatchConfig& batchCfg)
{
    BatchDialog batchDlg(NULL, batchCfg);
    return static_cast<ReturnBatchConfig::ButtonPressed>(batchDlg.ShowModal());
}
