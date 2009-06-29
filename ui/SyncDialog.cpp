#include "syncDialog.h"
#include "../library/globalFunctions.h"
#include "../library/resources.h"
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <wx/ffile.h>
#include "../library/customButton.h"
#include "../synchronization.h"
#include "../algorithm.h"
#include <wx/dnd.h>
#include "dragAndDrop.h"

using namespace FreeFileSync;


SyncDialog::SyncDialog(wxWindow* window,
                       const FolderComparison& folderCmpRef,
                       MainConfiguration& config,
                       bool& ignoreErrors) :
        SyncDlgGenerated(window),
        folderCmp(folderCmpRef),
        cfg(config),
        m_ignoreErrors(ignoreErrors)
{
    //make working copy of mainDialog.cfg.syncConfiguration and recycler setting
    localSyncConfiguration = config.syncConfiguration;
    m_checkBoxUseRecycler->SetValue(cfg.useRecycleBin);
    m_checkBoxIgnoreErrors->SetValue(m_ignoreErrors);

    //set sync config icons
    updateConfigIcons(cfg.compareVar, localSyncConfiguration);

    //set icons for this dialog
    m_bitmapLeftOnly->SetBitmap(*GlobalResources::getInstance().bitmapLeftOnly);
    m_bitmapRightOnly->SetBitmap(*GlobalResources::getInstance().bitmapRightOnly);
    m_bitmapLeftNewer->SetBitmap(*GlobalResources::getInstance().bitmapLeftNewer);
    m_bitmapRightNewer->SetBitmap(*GlobalResources::getInstance().bitmapRightNewer);
    m_bitmapDifferent->SetBitmap(*GlobalResources::getInstance().bitmapDifferent);

    bSizer201->Layout(); //wxButtonWithImage size might have changed

    //set radiobutton
    switch (localSyncConfiguration.getVariant())
    {
    case SyncConfiguration::MIRROR:
        m_radioBtn1->SetValue(true);    //one way ->
        break;
    case SyncConfiguration::UPDATE:
        m_radioBtnUpdate->SetValue(true);    //Update ->
        break;
    case SyncConfiguration::TWOWAY:
        m_radioBtn2->SetValue(true);    //two way <->
        break;
    case SyncConfiguration::CUSTOM:
        m_radioBtn3->SetValue(true);    //other
        break;
    }

    Fit();
}

//#################################################################################################################

SyncDialog::~SyncDialog() {}


void SyncDialog::updateConfigIcons(const FreeFileSync::CompareVariant cmpVar, const FreeFileSync::SyncConfiguration& syncConfig)
{
    updateConfigIcons(cmpVar,
                      syncConfig,
                      m_bpButtonLeftOnly,
                      m_bpButtonRightOnly,
                      m_bpButtonLeftNewer,
                      m_bpButtonRightNewer,
                      m_bpButtonDifferent,
                      m_bitmapLeftOnly,
                      m_bitmapRightOnly,
                      m_bitmapLeftNewer,
                      m_bitmapRightNewer,
                      m_bitmapDifferent);
}


void SyncDialog::updateConfigIcons(const CompareVariant compareVar,
                                   const SyncConfiguration& syncConfig,
                                   wxBitmapButton* buttonLeftOnly,
                                   wxBitmapButton* buttonRightOnly,
                                   wxBitmapButton* buttonLeftNewer,
                                   wxBitmapButton* buttonRightNewer,
                                   wxBitmapButton* buttonDifferent,
                                   wxStaticBitmap* bitmapLeftOnly,
                                   wxStaticBitmap* bitmapRightOnly,
                                   wxStaticBitmap* bitmapLeftNewer,
                                   wxStaticBitmap* bitmapRightNewer,
                                   wxStaticBitmap* bitmapDifferent)
{
    //display only relevant sync options
    switch (compareVar)
    {
    case CMP_BY_TIME_SIZE:
        buttonLeftOnly  ->Show();
        buttonRightOnly ->Show();
        buttonLeftNewer ->Show();
        buttonRightNewer->Show();
        buttonDifferent ->Hide();

        bitmapLeftOnly  ->Show();
        bitmapRightOnly ->Show();
        bitmapLeftNewer ->Show();
        bitmapRightNewer->Show();
        bitmapDifferent ->Hide();
        break;

    case CMP_BY_CONTENT:
        buttonLeftOnly  ->Show();
        buttonRightOnly ->Show();
        buttonLeftNewer ->Hide();
        buttonRightNewer->Hide();
        buttonDifferent ->Show();

        bitmapLeftOnly  ->Show();
        bitmapRightOnly ->Show();
        bitmapLeftNewer ->Hide();
        bitmapRightNewer->Hide();
        bitmapDifferent ->Show();
        break;
    }


    if (syncConfig.exLeftSideOnly == SYNC_DIR_RIGHT)
    {
        buttonLeftOnly->SetBitmapLabel(*GlobalResources::getInstance().bitmapArrowRightCr);
        buttonLeftOnly->SetToolTip(_("Copy from left to right"));
    }
    else if (syncConfig.exLeftSideOnly == SYNC_DIR_LEFT)
    {
        buttonLeftOnly->SetBitmapLabel(*GlobalResources::getInstance().bitmapDeleteLeft);
        buttonLeftOnly->SetToolTip(_("Delete files/folders existing on left side only"));
    }
    else if (syncConfig.exLeftSideOnly == SYNC_DIR_NONE)
    {
        buttonLeftOnly->SetBitmapLabel(*GlobalResources::getInstance().bitmapArrowNone);
        buttonLeftOnly->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.exRightSideOnly == SYNC_DIR_RIGHT)
    {
        buttonRightOnly->SetBitmapLabel(*GlobalResources::getInstance().bitmapDeleteRight);
        buttonRightOnly->SetToolTip(_("Delete files/folders existing on right side only"));
    }
    else if (syncConfig.exRightSideOnly == SYNC_DIR_LEFT)
    {
        buttonRightOnly->SetBitmapLabel(*GlobalResources::getInstance().bitmapArrowLeftCr);
        buttonRightOnly->SetToolTip(_("Copy from right to left"));
    }
    else if (syncConfig.exRightSideOnly == SYNC_DIR_NONE)
    {
        buttonRightOnly->SetBitmapLabel(*GlobalResources::getInstance().bitmapArrowNone);
        buttonRightOnly->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.leftNewer == SYNC_DIR_RIGHT)
    {
        buttonLeftNewer->SetBitmapLabel(*GlobalResources::getInstance().bitmapArrowRight);
        buttonLeftNewer->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (syncConfig.leftNewer == SYNC_DIR_LEFT)
    {
        buttonLeftNewer->SetBitmapLabel(*GlobalResources::getInstance().bitmapArrowLeft);
        buttonLeftNewer->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (syncConfig.leftNewer == SYNC_DIR_NONE)
    {
        buttonLeftNewer->SetBitmapLabel(*GlobalResources::getInstance().bitmapArrowNone);
        buttonLeftNewer->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.rightNewer == SYNC_DIR_RIGHT)
    {
        buttonRightNewer->SetBitmapLabel(*GlobalResources::getInstance().bitmapArrowRight);
        buttonRightNewer->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (syncConfig.rightNewer == SYNC_DIR_LEFT)
    {
        buttonRightNewer->SetBitmapLabel(*GlobalResources::getInstance().bitmapArrowLeft);
        buttonRightNewer->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (syncConfig.rightNewer == SYNC_DIR_NONE)
    {
        buttonRightNewer->SetBitmapLabel(*GlobalResources::getInstance().bitmapArrowNone);
        buttonRightNewer->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.different == SYNC_DIR_RIGHT)
    {
        buttonDifferent->SetBitmapLabel(*GlobalResources::getInstance().bitmapArrowRight);
        buttonDifferent->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (syncConfig.different == SYNC_DIR_LEFT)
    {
        buttonDifferent->SetBitmapLabel(*GlobalResources::getInstance().bitmapArrowLeft);
        buttonDifferent->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (syncConfig.different == SYNC_DIR_NONE)
    {
        buttonDifferent->SetBitmapLabel(*GlobalResources::getInstance().bitmapArrowNone);
        buttonDifferent->SetToolTip(_("Do nothing"));
    }
}


void SyncDialog::OnClose(wxCloseEvent& event)
{
    EndModal(0);
}


void SyncDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(0);
}


void SyncDialog::OnApply(wxCommandEvent& event)
{
    //write configuration to main dialog
    cfg.syncConfiguration = localSyncConfiguration;
    cfg.useRecycleBin     = m_checkBoxUseRecycler->GetValue();
    m_ignoreErrors        = m_checkBoxIgnoreErrors->GetValue();

    EndModal(BUTTON_OKAY);
}


void SyncDialog::OnSelectRecycleBin(wxCommandEvent& event)
{
    if (event.IsChecked())
    {
        if (!FreeFileSync::recycleBinExists())
        {
            wxMessageBox(_("It was not possible to initialize the Recycle Bin!\n\nIt's likely that you are not using Windows.\nIf you want this feature included, please contact the author. :)"), _("Error") , wxOK | wxICON_ERROR);
            m_checkBoxUseRecycler->SetValue(false);
        }
    }
}


void SyncDialog::OnSyncLeftToRight(wxCommandEvent& event)
{
    localSyncConfiguration.setVariant(SyncConfiguration::MIRROR);

    updateConfigIcons(cfg.compareVar, localSyncConfiguration);

    //if event is triggered by button
    m_radioBtn1->SetValue(true);
}


void SyncDialog::OnSyncUpdate(wxCommandEvent& event)
{
    localSyncConfiguration.setVariant(SyncConfiguration::UPDATE);

    updateConfigIcons(cfg.compareVar, localSyncConfiguration);

    //if event is triggered by button
    m_radioBtnUpdate->SetValue(true);
}


void SyncDialog::OnSyncBothSides(wxCommandEvent& event)
{
    localSyncConfiguration.setVariant(SyncConfiguration::TWOWAY);

    updateConfigIcons(cfg.compareVar, localSyncConfiguration);

    //if event is triggered by button
    m_radioBtn2->SetValue(true);
}


void toggleSyncDirection(SyncDirection& current)
{
    if (current == SYNC_DIR_RIGHT)
        current = SYNC_DIR_LEFT;
    else if (current == SYNC_DIR_LEFT)
        current = SYNC_DIR_NONE;
    else if (current== SYNC_DIR_NONE)
        current = SYNC_DIR_RIGHT;
    else
        assert (false);
}


void SyncDialog::OnExLeftSideOnly( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.exLeftSideOnly);
    updateConfigIcons(cfg.compareVar, localSyncConfiguration);
    //set custom config button
    m_radioBtn3->SetValue(true);
}


void SyncDialog::OnExRightSideOnly( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.exRightSideOnly);
    updateConfigIcons(cfg.compareVar, localSyncConfiguration);
    //set custom config button
    m_radioBtn3->SetValue(true);
}


void SyncDialog::OnLeftNewer( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.leftNewer);
    updateConfigIcons(cfg.compareVar, localSyncConfiguration);
    //set custom config button
    m_radioBtn3->SetValue(true);
}


void SyncDialog::OnRightNewer( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.rightNewer);
    updateConfigIcons(cfg.compareVar, localSyncConfiguration);
    //set custom config button
    m_radioBtn3->SetValue(true);
}


void SyncDialog::OnDifferent( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.different);
    updateConfigIcons(cfg.compareVar, localSyncConfiguration);
    //set custom config button
    m_radioBtn3->SetValue(true);
}
//###################################################################################################################################


class BatchFolderPairPanel : public BatchFolderPairGenerated
{
public:
    BatchFolderPairPanel(wxWindow* parent) :
            BatchFolderPairGenerated(parent),
            dragDropOnLeft(m_panelLeft, m_dirPickerLeft, m_directoryLeft),
            dragDropOnRight(m_panelRight, m_dirPickerRight, m_directoryRight) {}

private:
    //support for drag and drop
    DragDropOnDlg dragDropOnLeft;
    DragDropOnDlg dragDropOnRight;
};

//###################################################################################################################################


class BatchFileDropEvent : public wxFileDropTarget
{
public:
    BatchFileDropEvent(BatchDialog* dlg) :
            batchDlg(dlg) {}

    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
    {
        if (!filenames.IsEmpty())
        {
            const wxString droppedFileName = filenames[0];

            xmlAccess::XmlType fileType = xmlAccess::getXmlType(droppedFileName);

            //test if ffs batch file has been dropped
            if (fileType == xmlAccess::XML_BATCH_CONFIG)
                batchDlg->loadBatchFile(droppedFileName);
            else
            {
                wxString errorMessage = _("%x is not a valid FreeFileSync batch file!");
                errorMessage.Replace(wxT("%x"), wxString(wxT("\"")) + droppedFileName + wxT("\""), false);
                wxMessageBox(errorMessage, _("Error"), wxOK | wxICON_ERROR);
            }
        }
        return false;
    }

private:
    BatchDialog* batchDlg;
};


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


void BatchDialog::init()
{
    //prepare drag & drop for loading of *.ffs_batch files
    SetDropTarget(new BatchFileDropEvent(this));

    dragDropOnLogfileDir.reset(new DragDropOnDlg(m_panelLogging, m_dirPickerLogfileDir, m_textCtrlLogfileDir));

    //set icons for this dialog
    m_bitmapLeftOnly->SetBitmap(*GlobalResources::getInstance().bitmapLeftOnly);
    m_bitmapRightOnly->SetBitmap(*GlobalResources::getInstance().bitmapRightOnly);
    m_bitmapLeftNewer->SetBitmap(*GlobalResources::getInstance().bitmapLeftNewer);
    m_bitmapRightNewer->SetBitmap(*GlobalResources::getInstance().bitmapRightNewer);
    m_bitmapDifferent->SetBitmap(*GlobalResources::getInstance().bitmapDifferent);
    m_bitmap8->SetBitmap(*GlobalResources::getInstance().bitmapInclude);
    m_bitmap9->SetBitmap(*GlobalResources::getInstance().bitmapExclude);
    m_bitmap27->SetBitmap(*GlobalResources::getInstance().bitmapBatch);
}


xmlAccess::OnError BatchDialog::getSelectionHandleError()
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


void updateToolTip(wxChoice* choiceHandleError, const xmlAccess::OnError value)
{
    switch (value)
    {
    case xmlAccess::ON_ERROR_POPUP:
        choiceHandleError->SetToolTip(_("Show popup on errors or warnings"));
        break;
    case xmlAccess::ON_ERROR_IGNORE:
        choiceHandleError->SetToolTip(_("Hide all error and warning messages"));
        break;
    case xmlAccess::ON_ERROR_EXIT:
        choiceHandleError->SetToolTip(_("Exit immediately and set returncode < 0"));
        break;
    default:
        assert(false);
        choiceHandleError->SetToolTip(wxEmptyString);
    }
}


void BatchDialog::setSelectionHandleError(const xmlAccess::OnError value)
{
    m_choiceHandleError->Clear();
    m_choiceHandleError->Append(_("Show popup"));
    m_choiceHandleError->Append(_("Ignore errors"));
    m_choiceHandleError->Append(_("Exit with RC < 0"));

    switch (value)
    {
    case xmlAccess::ON_ERROR_POPUP:
        m_choiceHandleError->SetSelection(0);
        break;
    case xmlAccess::ON_ERROR_IGNORE:
        m_choiceHandleError->SetSelection(1);
        break;
    case xmlAccess::ON_ERROR_EXIT:
        m_choiceHandleError->SetSelection(2);
        break;
    default:
        assert(false);
        m_choiceHandleError->SetSelection(0);
    }

    updateToolTip(m_choiceHandleError, getSelectionHandleError());
}


void BatchDialog::OnChangeErrorHandling(wxCommandEvent& event)
{
    updateToolTip(m_choiceHandleError, getSelectionHandleError());
}


void BatchDialog::OnExLeftSideOnly(wxCommandEvent& event)
{
    toggleSyncDirection(localSyncConfiguration.exLeftSideOnly);
    updateConfigIcons(getCurrentCompareVar(), localSyncConfiguration);
}


void BatchDialog::OnExRightSideOnly(wxCommandEvent& event)
{
    toggleSyncDirection(localSyncConfiguration.exRightSideOnly);
    updateConfigIcons(getCurrentCompareVar(), localSyncConfiguration);
}


void BatchDialog::OnLeftNewer(wxCommandEvent& event)
{
    toggleSyncDirection(localSyncConfiguration.leftNewer);
    updateConfigIcons(getCurrentCompareVar(), localSyncConfiguration);
}


void BatchDialog::OnRightNewer(wxCommandEvent& event)
{
    toggleSyncDirection(localSyncConfiguration.rightNewer);
    updateConfigIcons(getCurrentCompareVar(), localSyncConfiguration);
}


void BatchDialog::OnDifferent(wxCommandEvent& event)
{
    toggleSyncDirection(localSyncConfiguration.different);
    updateConfigIcons(getCurrentCompareVar(), localSyncConfiguration);
}


void BatchDialog::OnCheckFilter(wxCommandEvent& event)
{
    updateVisibleTabs();
}


void BatchDialog::OnCheckLogging(wxCommandEvent& event)
{
    updateVisibleTabs();
}


void BatchDialog::updateVisibleTabs()
{
    showNotebookpage(m_panelFilter, _("Filter"), m_checkBoxFilter->GetValue());
    showNotebookpage(m_panelLogging, _("Logging"), m_checkBoxSilent->GetValue());
}


void BatchDialog::showNotebookpage(wxWindow* page, const wxString& pageName, bool show)
{
    int windowPosition = -1;
    for (size_t i = 0; i < m_notebookSettings->GetPageCount(); ++i)
        if (    static_cast<wxWindow*>(m_notebookSettings->GetPage(i)) ==
                static_cast<wxWindow*>(page))
        {
            windowPosition = i;
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


void BatchDialog::OnSelectRecycleBin(wxCommandEvent& event)
{
    if (m_checkBoxUseRecycler->GetValue())
    {
        if (!FreeFileSync::recycleBinExists())
        {
            wxMessageBox(_("It was not possible to initialize the Recycle Bin!\n\nIt's likely that you are not using Windows.\nIf you want this feature included, please contact the author. :)"), _("Error") , wxOK | wxICON_ERROR);
            m_checkBoxUseRecycler->SetValue(false);
        }
    }
}


CompareVariant BatchDialog::getCurrentCompareVar()
{
    if (m_radioBtnSizeDate->GetValue())
        return CMP_BY_TIME_SIZE;
    else if (m_radioBtnContent->GetValue())
        return CMP_BY_CONTENT;
    else
    {
        assert(false);
        return CMP_BY_TIME_SIZE;
    }
}


void BatchDialog::updateConfigIcons(const FreeFileSync::CompareVariant cmpVar, const FreeFileSync::SyncConfiguration& syncConfig)
{
    SyncDialog::updateConfigIcons(cmpVar,
                                  syncConfig,
                                  m_bpButtonLeftOnly,
                                  m_bpButtonRightOnly,
                                  m_bpButtonLeftNewer,
                                  m_bpButtonRightNewer,
                                  m_bpButtonDifferent,
                                  m_bitmapLeftOnly,
                                  m_bitmapRightOnly,
                                  m_bitmapLeftNewer,
                                  m_bitmapRightNewer,
                                  m_bitmapDifferent);
}


void BatchDialog::OnChangeCompareVar(wxCommandEvent& event)
{
    updateConfigIcons(getCurrentCompareVar(), localSyncConfiguration);
    Fit();
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
    const wxString defaultFileName = proposedBatchFileName.empty() ? wxT("SyncJob.ffs_batch") : proposedBatchFileName;

    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, defaultFileName, wxString(_("FreeFileSync batch file")) + wxT(" (*.ffs_batch)|*.ffs_batch"), wxFD_SAVE);
    if (filePicker->ShowModal() == wxID_OK)
    {
        const wxString newFileName = filePicker->GetPath();
        if (FreeFileSync::fileExists(newFileName.c_str()))
        {
            wxMessageDialog* messageDlg = new wxMessageDialog(this, wxString(_("File already exists. Overwrite?")) + wxT(" \"") + newFileName + wxT("\""), _("Warning") , wxOK | wxCANCEL);

            if (messageDlg->ShowModal() != wxID_OK)
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
    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, wxEmptyString, wxString(_("FreeFileSync batch file")) + wxT(" (*.ffs_batch)|*.ffs_batch"), wxFD_OPEN);;
    if (filePicker->ShowModal() == wxID_OK)
        loadBatchFile(filePicker->GetPath());
}


bool BatchDialog::saveBatchFile(const wxString& filename)
{
    xmlAccess::XmlBatchConfig batchCfg;

    //load structure with basic settings "mainCfg"
    batchCfg.mainCfg.compareVar        = getCurrentCompareVar();
    batchCfg.mainCfg.syncConfiguration = localSyncConfiguration;
    batchCfg.mainCfg.filterIsActive    = m_checkBoxFilter->GetValue();
    batchCfg.mainCfg.includeFilter     = m_textCtrlInclude->GetValue();
    batchCfg.mainCfg.excludeFilter     = m_textCtrlExclude->GetValue();
    batchCfg.mainCfg.useRecycleBin     = m_checkBoxUseRecycler->GetValue();
    batchCfg.handleError               = getSelectionHandleError();

    for (unsigned int i = 0; i < localFolderPairs.size(); ++i)
    {
        FolderPair newPair;
        newPair.leftDirectory  = localFolderPairs[i]->m_directoryLeft->GetValue().c_str();
        newPair.rightDirectory = localFolderPairs[i]->m_directoryRight->GetValue().c_str();

        batchCfg.directoryPairs.push_back(newPair);
    }

    //load structure with batch settings "batchCfg"
    batchCfg.silent = m_checkBoxSilent->GetValue();
    batchCfg.logFileDirectory = m_textCtrlLogfileDir->GetValue();

    //write config to XML
    try
    {
        xmlAccess::writeBatchConfig(filename, batchCfg);
    }
    catch (const FileError& error)
    {
        wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
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
        batchCfg = xmlAccess::readBatchConfig(filename);
    }
    catch (const FileError& error)
    {
        wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
        return;
    }

    SetTitle(wxString(_("Create a batch job")) + wxT(" - ") + filename);
    proposedBatchFileName = filename; //may be used on next save

    this->loadBatchCfg(batchCfg);
}


void BatchDialog::loadBatchCfg(const xmlAccess::XmlBatchConfig& batchCfg)
{
    //make working copy of mainDialog.cfg.syncConfiguration and recycler setting
    localSyncConfiguration = batchCfg.mainCfg.syncConfiguration;

    m_checkBoxUseRecycler->SetValue(batchCfg.mainCfg.useRecycleBin);
    setSelectionHandleError(batchCfg.handleError);

    switch (batchCfg.mainCfg.compareVar)
    {
    case CMP_BY_TIME_SIZE:
        m_radioBtnSizeDate->SetValue(true);
        break;
    case CMP_BY_CONTENT:
        m_radioBtnContent->SetValue(true);
        break;
    }

    updateConfigIcons(batchCfg.mainCfg.compareVar, batchCfg.mainCfg.syncConfiguration);

    m_checkBoxFilter->SetValue(batchCfg.mainCfg.filterIsActive);
    m_textCtrlInclude->SetValue(batchCfg.mainCfg.includeFilter);
    m_textCtrlExclude->SetValue(batchCfg.mainCfg.excludeFilter);

    m_checkBoxSilent->SetValue(batchCfg.silent);
    m_textCtrlLogfileDir->SetValue(batchCfg.logFileDirectory);

    //remove existing folder pairs
    localFolderPairs.clear();
    bSizerFolderPairs->Clear(true);

    //add folder pairs
    int scrWindowHeight = 0;
    for (std::vector<FolderPair>::const_iterator i = batchCfg.directoryPairs.begin(); i != batchCfg.directoryPairs.end(); ++i)
    {
        BatchFolderPairPanel* newPair = new BatchFolderPairPanel(m_scrolledWindow6);
        newPair->m_directoryLeft->SetValue(i->leftDirectory.c_str());
        newPair->m_directoryRight->SetValue(i->rightDirectory.c_str());

        bSizerFolderPairs->Add( newPair, 0, wxEXPAND, 5);
        localFolderPairs.push_back(newPair);

        if (i == batchCfg.directoryPairs.begin())
            scrWindowHeight = newPair->GetSize().GetHeight();
    }
    //set size of scrolled window
    int pairCount = std::min(localFolderPairs.size(), size_t(3)); //up to 3 additional pairs shall be shown
    m_scrolledWindow6->SetMinSize(wxSize( -1, scrWindowHeight * pairCount));

    updateVisibleTabs();

    m_scrolledWindow6->Layout(); //needed
    m_panelOverview->Layout(); //needed

    Fit(); //needed
    Centre();
    m_buttonSave->SetFocus();
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
