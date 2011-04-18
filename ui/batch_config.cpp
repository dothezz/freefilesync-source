// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "batch_config.h"
#include "../shared/xml_base.h"
#include "../shared/dir_picker_i18n.h"
#include "folder_pair.h"
#include <iterator>
#include <wx/wupdlock.h>
#include "../shared/help_provider.h"
#include "../shared/file_handling.h"
#include "msg_popup.h"
#include <wx/dnd.h>
#include <wx/msgdlg.h>
#include "../shared/mouse_move_dlg.h"

using namespace ffs3;


class BatchFileDropEvent : public wxFileDropTarget
{
public:
    BatchFileDropEvent(BatchDialog& dlg) :
        batchDlg(dlg) {}

    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& fileArray)
    {
        if (fileArray.IsEmpty())
            return false;

        std::vector<wxString> filenames;
        for (size_t i = 0; i < fileArray.GetCount(); ++i)
            filenames.push_back(fileArray[i]);


        switch (xmlAccess::getMergeType(filenames))   //throw ()
        {
            case xmlAccess::MERGE_BATCH:
            case xmlAccess::MERGE_GUI:
            case xmlAccess::MERGE_GUI_BATCH:
                if (filenames.size() == 1)
                {
                    batchDlg.loadBatchFile(filenames[0]);
                    return false;
                }
                else
                {
                    xmlAccess::XmlBatchConfig batchCfg;
                    try
                    {
                        convertConfig(filenames, batchCfg); //throw (xmlAccess::XmlError)
                    }
                    catch (const xmlAccess::XmlError& error)
                    {
                        if (error.getSeverity() == xmlAccess::XmlError::WARNING)
                            wxMessageBox(error.msg(), _("Warning"), wxOK | wxICON_WARNING);
                        else
                        {
                            wxMessageBox(error.msg(), _("Error"), wxOK | wxICON_ERROR);
                            return false;
                        }
                    }
                    batchDlg.loadBatchCfg(batchCfg);
                }
                break;

            case xmlAccess::MERGE_OTHER:
                wxMessageBox(_("Invalid FreeFileSync config file!"), _("Error"), wxOK | wxICON_ERROR);
                break;
        }

        return false;
    }

private:
    BatchDialog& batchDlg;
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
        return batchDlg.getCurrentConfiguration().mainCfg;
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

    void setValues(const Zstring& leftDir, const Zstring& rightDir, AltSyncCfgPtr syncCfg, const FilterConfig& filter)
    {
        setConfig(syncCfg, filter);
        dirNameLeft.setName(leftDir);
        dirNameRight.setName(rightDir);
    }
    Zstring getLeftDir() const
    {
        return dirNameLeft.getName();
    }
    Zstring getRightDir() const
    {
        return dirNameRight.getName();
    }

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

    void setValues(const Zstring& leftDir, const Zstring& rightDir, AltSyncCfgPtr syncCfg, const FilterConfig& filter)
    {
        setConfig(syncCfg, filter);
        dirNameLeft.setName(leftDir);
        dirNameRight.setName(rightDir);
    }
    Zstring getLeftDir() const
    {
        return dirNameLeft.getName();
    }
    Zstring getRightDir() const
    {
        return dirNameRight.getName();
    }

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
    loadBatchCfg(batchCfg);
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
    new MouseMoveWindow(*this, //allow moving main dialog by clicking (nearly) anywhere...
                        this,
                        m_panelOverview,
                        m_panelLogging,
                        m_staticText56,
                        m_staticText44,
                        m_bitmap27); //ownership passed to "this"
#endif

    wxWindowUpdateLocker dummy(this); //avoid display distortion

    m_bpButtonCmpConfig->SetBitmapLabel(GlobalResources::instance().getImage(wxT("cmpConfig")));
    m_bpButtonSyncConfig->SetBitmapLabel(GlobalResources::instance().getImage(wxT("syncConfig")));

    m_bpButtonHelp->SetBitmapLabel(GlobalResources::instance().getImage(wxT("help")));

    //init handling of first folder pair
    firstFolderPair.reset(new DirectoryPairBatchFirst(*this));

    m_bpButtonFilter->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(BatchDialog::OnGlobalFilterOpenContext), NULL, this);

    //prepare drag & drop for loading of *.ffs_batch files
    SetDropTarget(new BatchFileDropEvent(*this));
    logfileDir.reset(new DirectoryName(*m_panelLogging, *m_dirPickerLogfileDir, *m_textCtrlLogfileDir, sbSizerLogfileDir));

    //set icons for this dialog
    m_bpButtonAddPair->SetBitmapLabel(GlobalResources::instance().getImage(wxT("addFolderPair")));
    m_bitmap27->SetBitmap(GlobalResources::instance().getImage(wxT("batch")));

    m_buttonSave->SetFocus();
}

//------------------- error handling --------------------------

xmlAccess::OnError BatchDialog::getSelectionHandleError() const
{
    switch (m_choiceHandleError->GetSelection())
    {
        case 0:
            return xmlAccess::ON_ERROR_POPUP;
        case 1:
            return xmlAccess::ON_ERROR_IGNORE;
        case 2:
            return xmlAccess::ON_ERROR_EXIT;
        default:
            assert(false);
            return xmlAccess::ON_ERROR_POPUP;
    }
}


void BatchDialog::updateToolTipErrorHandling(const xmlAccess::OnError value)
{
    switch (value)
    {
        case xmlAccess::ON_ERROR_POPUP:
            m_choiceHandleError->SetToolTip(_("Show popup on errors or warnings"));
            break;
        case xmlAccess::ON_ERROR_IGNORE:
            m_choiceHandleError->SetToolTip(_("Hide all error and warning messages"));
            break;
        case xmlAccess::ON_ERROR_EXIT:
            m_choiceHandleError->SetToolTip(_("Abort synchronization immediately"));
            break;
    }
}


void BatchDialog::setSelectionHandleError(const xmlAccess::OnError value)
{
    m_choiceHandleError->Clear();
    m_choiceHandleError->Append(_("Show popup"));
    m_choiceHandleError->Append(_("Ignore errors"));
    if (m_checkBoxSilent->GetValue()) //this option shall be available for silent mode only!
        m_choiceHandleError->Append(_("Exit instantly"));

    //default
    m_choiceHandleError->SetSelection(0);

    switch (value)
    {
        case xmlAccess::ON_ERROR_POPUP:
            m_choiceHandleError->SetSelection(0);
            break;
        case xmlAccess::ON_ERROR_IGNORE:
            m_choiceHandleError->SetSelection(1);
            break;
        case xmlAccess::ON_ERROR_EXIT:
            if (m_checkBoxSilent->GetValue()) //this option shall be available for silent mode only!
                m_choiceHandleError->SetSelection(2);
            break;
    }

    updateToolTipErrorHandling(getSelectionHandleError());
}


void BatchDialog::OnChangeErrorHandling(wxCommandEvent& event)
{
    updateToolTipErrorHandling(getSelectionHandleError());
}


void BatchDialog::OnCmpSettings(wxCommandEvent& event)
{

    //show window right next to the compare-config button
    wxPoint windowPos = m_bpButtonCmpConfig->GetScreenPosition();
    windowPos.x += m_bpButtonCmpConfig->GetSize().GetWidth() + 5;

    if (ffs3::showCompareCfgDialog(windowPos,
                                   localBatchCfg.mainCfg.compareVar,
                                   localBatchCfg.mainCfg.handleSymlinks) == DefaultReturnCode::BUTTON_OKAY)
    {
        updateGui();
    }
}


void BatchDialog::OnSyncSettings(wxCommandEvent& event)
{
    SyncCfgDialog syncDlg(this,
                          localBatchCfg.mainCfg.compareVar,
                          localBatchCfg.mainCfg.syncConfiguration,
                          localBatchCfg.mainCfg.handleDeletion,
                          localBatchCfg.mainCfg.customDeletionDirectory,
                          NULL);
    if (syncDlg.ShowModal() == SyncCfgDialog::BUTTON_APPLY)
    {
        updateGui();
    }
}


void BatchDialog::OnConfigureFilter(wxCommandEvent& event)
{
    if (showFilterDialog(true, //is main filter dialog
                         localBatchCfg.mainCfg.globalFilter.includeFilter,
                         localBatchCfg.mainCfg.globalFilter.excludeFilter) == DefaultReturnCode::BUTTON_OKAY)
    {
        updateGui();
    }
}


void BatchDialog::updateGui() //re-evaluate gui after config changes
{
    xmlAccess::XmlBatchConfig cfg = getCurrentConfiguration();

    showNotebookpage(m_panelLogging, _("Logging"), cfg.silent);

    m_textCtrlLogfileDir ->Enable(cfg.logFileCountMax > 0);
    m_dirPickerLogfileDir->Enable(cfg.logFileCountMax > 0);

    //update compare variant name
    m_staticTextCmpVariant->SetLabel(wxString(wxT("(")) + getVariantName(cfg.mainCfg.compareVar) + wxT(")"));

    //update sync variant name
    m_staticTextSyncVariant->SetLabel(wxString(wxT("(")) + cfg.mainCfg.getSyncVariantName() + wxT(")"));

    //set filter icon
    if (isNullFilter(cfg.mainCfg.globalFilter))
    {
        m_bpButtonFilter->SetBitmapLabel(GlobalResources::instance().getImage(wxT("filterOff")));
        m_bpButtonFilter->SetToolTip(_("No filter selected"));
    }
    else
    {
        m_bpButtonFilter->SetBitmapLabel(GlobalResources::instance().getImage(wxT("filterOn")));
        m_bpButtonFilter->SetToolTip(_("Filter is active"));
    }

    m_panelOverview->Layout(); //adjust stuff inside scrolled window
}


void BatchDialog::OnGlobalFilterOpenContext(wxCommandEvent& event)
{
    const int menuId = 1234;
    contextMenu.reset(new wxMenu); //re-create context menu
    contextMenu->Append(menuId, _("Clear filter settings"));
    contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(BatchDialog::OnGlobalFilterRemConfirm), NULL, this);

    if (isNullFilter(localBatchCfg.mainCfg.globalFilter))
        contextMenu->Enable(menuId, false); //disable menu item, if clicking wouldn't make sense anyway

    PopupMenu(contextMenu.get()); //show context menu
}


void BatchDialog::OnGlobalFilterRemConfirm(wxCommandEvent& event)
{
    localBatchCfg.mainCfg.globalFilter = FilterConfig();
    updateGui();
}


void BatchDialog::OnCheckSilent(wxCommandEvent& event)
{
    updateGui();

    //reset error handling depending on "m_checkBoxSilent"
    setSelectionHandleError(getSelectionHandleError());
}


void BatchDialog::OnChangeMaxLogCountTxt(wxCommandEvent& event)
{
    updateGui();
}


void BatchDialog::OnHelp(wxCommandEvent& event)
{
#ifdef FFS_WIN
    ffs3::displayHelpEntry(wxT("html\\advanced\\ScheduleBatch.html"));
#elif defined FFS_LINUX
    ffs3::displayHelpEntry(wxT("html/advanced/ScheduleBatch.html"));
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
    EndModal(0);
}


void BatchDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(0);
}


void BatchDialog::OnSaveBatchJob(wxCommandEvent& event)
{
    //get a filename
    wxString defaultFileName = proposedBatchFileName.empty() ? wxT("SyncJob.ffs_batch") : proposedBatchFileName;

    //attention: proposedBatchFileName may be an imported *.ffs_gui file! We don't want to overwrite it with a BATCH config!
    if (defaultFileName.EndsWith(wxT(".ffs_gui")))
        defaultFileName.Replace(wxT(".ffs_gui"), wxT(".ffs_batch"), false);


    wxFileDialog filePicker(this, wxEmptyString, wxEmptyString, defaultFileName, wxString(_("FreeFileSync batch file")) + wxT(" (*.ffs_batch)|*.ffs_batch"), wxFD_SAVE); //creating this on freestore leads to memleak!
    if (filePicker.ShowModal() == wxID_OK)
    {
        const wxString newFileName = filePicker.GetPath();
        if (ffs3::fileExists(wxToZ(newFileName)))
        {
            QuestionDlg messageDlg(this,
                                   QuestionDlg::BUTTON_YES | QuestionDlg::BUTTON_CANCEL,
                                   wxString(_("File already exists. Overwrite?")) + wxT(" \"") + newFileName + wxT("\""));

            if (messageDlg.ShowModal() != QuestionDlg::BUTTON_YES)
            {
                OnSaveBatchJob(event); //retry
                return;
            }
        }

        //create batch file
        if (saveBatchFile(newFileName))
            EndModal(BATCH_FILE_SAVED);
    }
}


void BatchDialog::OnLoadBatchJob(wxCommandEvent& event)
{
    wxFileDialog filePicker(this, wxEmptyString, wxEmptyString, wxEmptyString, wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs_batch;*.ffs_gui)|*.ffs_batch;*.ffs_gui"), wxFD_OPEN); //creating this on freestore leads to memleak!
    if (filePicker.ShowModal() == wxID_OK)
        loadBatchFile(filePicker.GetPath());
}



inline
FolderPairEnh getEnahncedPair(const DirectoryPairBatch* panel)
{
    return FolderPairEnh(panel->getLeftDir(),
                         panel->getRightDir(),
                         panel->getAltSyncConfig(),
                         panel->getAltFilterConfig());
}


xmlAccess::XmlBatchConfig BatchDialog::getCurrentConfiguration() const
{
    xmlAccess::XmlBatchConfig batchCfg = localBatchCfg;

    //load parameter with ownership within wxWidgets controls...

    //first folder pair
    batchCfg.mainCfg.firstPair = FolderPairEnh(firstFolderPair->getLeftDir(),
                                               firstFolderPair->getRightDir(),
                                               firstFolderPair->getAltSyncConfig(),
                                               firstFolderPair->getAltFilterConfig());

    //add additional pairs
    batchCfg.mainCfg.additionalPairs.clear();
    std::transform(additionalFolderPairs.begin(), additionalFolderPairs.end(),
                   std::back_inserter(batchCfg.mainCfg.additionalPairs), getEnahncedPair);


    //load structure with batch settings "batchCfg"
    batchCfg.silent           = m_checkBoxSilent->GetValue();
    batchCfg.logFileDirectory = zToWx(logfileDir->getName());
    batchCfg.logFileCountMax  = m_spinCtrlLogCountMax->GetValue();
    batchCfg.handleError      = getSelectionHandleError();

    return batchCfg;
}


bool BatchDialog::saveBatchFile(const wxString& filename)
{
    const xmlAccess::XmlBatchConfig batchCfg = getCurrentConfiguration();

    //write config to XML
    try
    {
        xmlAccess::writeConfig(batchCfg, filename);
    }
    catch (const xmlAccess::XmlError& error)
    {
        wxMessageBox(error.msg().c_str(), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    SetTitle(wxString(_("Create a batch job")) + wxT(" - ") + filename);
    proposedBatchFileName = filename; //may be used on next save

    return true;
}


void BatchDialog::loadBatchFile(const wxString& filename)
{
    //load XML settings
    xmlAccess::XmlBatchConfig batchCfg;  //structure to receive gui settings
    try
    {
        //open a *.ffs_gui or *.ffs_batch file!
        std::vector<wxString> filenames;
        filenames.push_back(filename);

        xmlAccess::convertConfig(filenames, batchCfg); //throw (xmlAccess::XmlError)

        //xmlAccess::readConfig(filename, batchCfg);
    }
    catch (const xmlAccess::XmlError& error)
    {
        if (error.getSeverity() == xmlAccess::XmlError::WARNING)
            wxMessageBox(error.msg(), _("Warning"), wxOK | wxICON_WARNING);
        else
        {
            wxMessageBox(error.msg(), _("Error"), wxOK | wxICON_ERROR);
            return;
        }
    }

    SetTitle(wxString(_("Create a batch job")) + wxT(" - ") + filename);
    proposedBatchFileName = filename; //may be used on next save
    this->loadBatchCfg(batchCfg);
}


void BatchDialog::loadBatchCfg(const xmlAccess::XmlBatchConfig& batchCfg)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    //make working copy
    localBatchCfg = batchCfg;

    m_checkBoxSilent->SetValue(batchCfg.silent);
    //error handling is dependent from m_checkBoxSilent! /|\   \|/
    setSelectionHandleError(batchCfg.handleError);

    logfileDir->setName(wxToZ(batchCfg.logFileDirectory));
    m_spinCtrlLogCountMax->SetValue(static_cast<int>(batchCfg.logFileCountMax)); //attention: this one emits a "change value" event!! => updateGui() called implicitly!

    //set first folder pair
    firstFolderPair->setValues(batchCfg.mainCfg.firstPair.leftDirectory,
                               batchCfg.mainCfg.firstPair.rightDirectory,
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


void BatchDialog::OnAddFolderPair(wxCommandEvent& event)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    std::vector<FolderPairEnh> newPairs;
    newPairs.push_back(getCurrentConfiguration().mainCfg.firstPair);

    addFolderPair(newPairs, true); //add pair in front of additonal pairs

    //clear first pair
    const FolderPairEnh cfgEmpty;
    firstFolderPair->setValues(cfgEmpty.leftDirectory,
                               cfgEmpty.rightDirectory,
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
        const FolderPairEnh cfgSecond = getEnahncedPair(additionalFolderPairs[0]);

        //reset first pair
        firstFolderPair->setValues(cfgSecond.leftDirectory,
                                   cfgSecond.rightDirectory,
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
        NameFilter(firstFolderPair->getAltFilterConfig().includeFilter,
                   firstFolderPair->getAltFilterConfig().excludeFilter).isNull())
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

    int pairHeight = sbSizerMainPair->GetSize().GetHeight(); //respect height of main pair
    if (additionalFolderPairs.size() > 0)
        pairHeight += std::min<double>(1.5, additionalFolderPairs.size()) * //have 0.5 * height indicate that more folders are there
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


void BatchDialog::addFolderPair(const std::vector<ffs3::FolderPairEnh>& newPairs, bool addFront)
{
    wxWindowUpdateLocker dummy(m_panelOverview); //avoid display distortion

    if (!newPairs.empty())
    {
        //add folder pairs
        for (std::vector<ffs3::FolderPairEnh>::const_iterator i = newPairs.begin(); i != newPairs.end(); ++i)
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
            newPair->setValues(i->leftDirectory,
                               i->rightDirectory,
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
