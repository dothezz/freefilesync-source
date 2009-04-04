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


SyncDialog::SyncDialog(wxWindow* window,
                       const FileCompareResult& gridDataRef,
                       MainConfiguration& config,
                       bool& ignoreErrors,
                       bool synchronizationEnabled) :
        SyncDlgGenerated(window),
        gridData(gridDataRef),
        cfg(config),
        m_ignoreErrors(ignoreErrors)
{
    //make working copy of mainDialog.cfg.syncConfiguration and recycler setting
    localSyncConfiguration = config.syncConfiguration;
    m_checkBoxUseRecycler->SetValue(cfg.useRecycleBin);
    m_checkBoxIgnoreErrors->SetValue(m_ignoreErrors);

    //set sync config icons
    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    //update preview
    calculatePreview();

    //set icons for this dialog
    m_bitmap13->SetBitmap(*globalResource.bitmapLeftOnly);
    m_bitmap14->SetBitmap(*globalResource.bitmapRightOnly);
    m_bitmap15->SetBitmap(*globalResource.bitmapLeftNewer);
    m_bitmap16->SetBitmap(*globalResource.bitmapRightNewer);
    m_bitmap17->SetBitmap(*globalResource.bitmapDifferent);

    if (synchronizationEnabled)
    {
        m_button18->SetForegroundColour(*wxBLACK);
        m_button18->setBitmapFront(*globalResource.bitmapStartSync);
        m_button18->Enable();
    }
    else
    {
        m_button18->SetForegroundColour(wxColor(94, 94, 94)); //grey
        m_button18->setBitmapFront(*globalResource.bitmapStartSyncDis);
        m_button18->Disable();
        m_button6->SetFocus();
    }

    bSizer201->Layout(); //wxButtonWithImage size might have changed


    //set radiobutton
    if (    localSyncConfiguration.exLeftSideOnly  == SyncConfiguration::SYNC_DIR_RIGHT &&
            localSyncConfiguration.exRightSideOnly == SyncConfiguration::SYNC_DIR_RIGHT &&
            localSyncConfiguration.leftNewer       == SyncConfiguration::SYNC_DIR_RIGHT &&
            localSyncConfiguration.rightNewer      == SyncConfiguration::SYNC_DIR_RIGHT &&
            localSyncConfiguration.different       == SyncConfiguration::SYNC_DIR_RIGHT)
        m_radioBtn1->SetValue(true);    //one way ->

    else if (localSyncConfiguration.exLeftSideOnly  == SyncConfiguration::SYNC_DIR_RIGHT &&
             localSyncConfiguration.exRightSideOnly == SyncConfiguration::SYNC_DIR_NONE  &&
             localSyncConfiguration.leftNewer       == SyncConfiguration::SYNC_DIR_RIGHT &&
             localSyncConfiguration.rightNewer      == SyncConfiguration::SYNC_DIR_NONE  &&
             localSyncConfiguration.different       == SyncConfiguration::SYNC_DIR_NONE)
        m_radioBtnUpdate->SetValue(true);    //Update ->

    else if (localSyncConfiguration.exLeftSideOnly  == SyncConfiguration::SYNC_DIR_RIGHT &&
             localSyncConfiguration.exRightSideOnly == SyncConfiguration::SYNC_DIR_LEFT  &&
             localSyncConfiguration.leftNewer       == SyncConfiguration::SYNC_DIR_RIGHT &&
             localSyncConfiguration.rightNewer      == SyncConfiguration::SYNC_DIR_LEFT  &&
             localSyncConfiguration.different       == SyncConfiguration::SYNC_DIR_NONE)
        m_radioBtn2->SetValue(true);    //two way <->

    else
        m_radioBtn3->SetValue(true);    //other

    //set tooltip for ambivalent category "different"
    adjustToolTips(m_bitmap17, config.compareVar);
}

//#################################################################################################################

SyncDialog::~SyncDialog() {}


void SyncDialog::updateConfigIcons(wxBitmapButton* button1,
                                   wxBitmapButton* button2,
                                   wxBitmapButton* button3,
                                   wxBitmapButton* button4,
                                   wxBitmapButton* button5,
                                   const SyncConfiguration& syncConfig)
{
    if (syncConfig.exLeftSideOnly == SyncConfiguration::SYNC_DIR_RIGHT)
    {
        button1->SetBitmapLabel(*globalResource.bitmapArrowRightCr);
        button1->SetToolTip(_("Copy from left to right"));
    }
    else if (syncConfig.exLeftSideOnly == SyncConfiguration::SYNC_DIR_LEFT)
    {
        button1->SetBitmapLabel(*globalResource.bitmapDeleteLeft);
        button1->SetToolTip(_("Delete files/folders existing on left side only"));
    }
    else if (syncConfig.exLeftSideOnly == SyncConfiguration::SYNC_DIR_NONE)
    {
        button1->SetBitmapLabel(*globalResource.bitmapArrowNone);
        button1->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.exRightSideOnly == SyncConfiguration::SYNC_DIR_RIGHT)
    {
        button2->SetBitmapLabel(*globalResource.bitmapDeleteRight);
        button2->SetToolTip(_("Delete files/folders existing on right side only"));
    }
    else if (syncConfig.exRightSideOnly == SyncConfiguration::SYNC_DIR_LEFT)
    {
        button2->SetBitmapLabel(*globalResource.bitmapArrowLeftCr);
        button2->SetToolTip(_("Copy from right to left"));
    }
    else if (syncConfig.exRightSideOnly == SyncConfiguration::SYNC_DIR_NONE)
    {
        button2->SetBitmapLabel(*globalResource.bitmapArrowNone);
        button2->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.leftNewer == SyncConfiguration::SYNC_DIR_RIGHT)
    {
        button3->SetBitmapLabel(*globalResource.bitmapArrowRight);
        button3->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (syncConfig.leftNewer == SyncConfiguration::SYNC_DIR_LEFT)
    {
        button3->SetBitmapLabel(*globalResource.bitmapArrowLeft);
        button3->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (syncConfig.leftNewer == SyncConfiguration::SYNC_DIR_NONE)
    {
        button3->SetBitmapLabel(*globalResource.bitmapArrowNone);
        button3->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.rightNewer == SyncConfiguration::SYNC_DIR_RIGHT)
    {
        button4->SetBitmapLabel(*globalResource.bitmapArrowRight);
        button4->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (syncConfig.rightNewer == SyncConfiguration::SYNC_DIR_LEFT)
    {
        button4->SetBitmapLabel(*globalResource.bitmapArrowLeft);
        button4->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (syncConfig.rightNewer == SyncConfiguration::SYNC_DIR_NONE)
    {
        button4->SetBitmapLabel(*globalResource.bitmapArrowNone);
        button4->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.different == SyncConfiguration::SYNC_DIR_RIGHT)
    {
        button5->SetBitmapLabel(*globalResource.bitmapArrowRight);
        button5->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (syncConfig.different == SyncConfiguration::SYNC_DIR_LEFT)
    {
        button5->SetBitmapLabel(*globalResource.bitmapArrowLeft);
        button5->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (syncConfig.different == SyncConfiguration::SYNC_DIR_NONE)
    {
        button5->SetBitmapLabel(*globalResource.bitmapArrowNone);
        button5->SetToolTip(_("Do nothing"));
    }
}


void SyncDialog::adjustToolTips(wxStaticBitmap* bitmap, const CompareVariant var)
{
    //set tooltip for ambivalent category "different"
    if (var == CMP_BY_TIME_SIZE)
    {
        bitmap->SetToolTip(_("Files that exist on both sides, have same date but different filesizes"));
    }
    else if (var == CMP_BY_CONTENT)
    {
        bitmap->SetToolTip(_("Files that exist on both sides and have different content"));
    }
    else
        assert(false);
}


void SyncDialog::calculatePreview()
{
    //update preview of bytes to be transferred:
    int objectsToCreate    = 0;
    int objectsToOverwrite = 0;
    int objectsToDelete    = 0;
    double dataToProcess   = 0;
    FreeFileSync::calcTotalBytesToSync(gridData,
                                       localSyncConfiguration,
                                       objectsToCreate,
                                       objectsToOverwrite,
                                       objectsToDelete,
                                       dataToProcess);

    wxString toCreate = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(objectsToCreate));
    wxString toUpdate = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(objectsToOverwrite));
    wxString toDelete = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(objectsToDelete));
    wxString data     = FreeFileSync::formatFilesizeToShortString(dataToProcess);

    m_textCtrlCreate->SetValue(toCreate);
    m_textCtrlUpdate->SetValue(toUpdate);
    m_textCtrlDelete->SetValue(toDelete);
    m_textCtrlData->SetValue(data);
}


void SyncDialog::OnClose(wxCloseEvent& event)
{
    EndModal(0);
}


void SyncDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(0);
}


void SyncDialog::OnBack(wxCommandEvent& event)
{
    //write configuration to main dialog
    cfg.syncConfiguration = localSyncConfiguration;
    cfg.useRecycleBin     = m_checkBoxUseRecycler->GetValue();
    m_ignoreErrors        = m_checkBoxIgnoreErrors->GetValue();

    EndModal(0);
}

void SyncDialog::OnStartSync(wxCommandEvent& event)
{
    //write configuration to main dialog
    cfg.syncConfiguration = localSyncConfiguration;
    cfg.useRecycleBin     = m_checkBoxUseRecycler->GetValue();
    m_ignoreErrors        = m_checkBoxIgnoreErrors->GetValue();

    EndModal(BUTTON_START);
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
    localSyncConfiguration.exLeftSideOnly  = SyncConfiguration::SYNC_DIR_RIGHT;
    localSyncConfiguration.exRightSideOnly = SyncConfiguration::SYNC_DIR_RIGHT;
    localSyncConfiguration.leftNewer       = SyncConfiguration::SYNC_DIR_RIGHT;
    localSyncConfiguration.rightNewer      = SyncConfiguration::SYNC_DIR_RIGHT;
    localSyncConfiguration.different       = SyncConfiguration::SYNC_DIR_RIGHT;

    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();

    //if event is triggered by button
    m_radioBtn1->SetValue(true);
}


void SyncDialog::OnSyncUpdate(wxCommandEvent& event)
{
    localSyncConfiguration.exLeftSideOnly  = SyncConfiguration::SYNC_DIR_RIGHT;
    localSyncConfiguration.exRightSideOnly = SyncConfiguration::SYNC_DIR_NONE;
    localSyncConfiguration.leftNewer       = SyncConfiguration::SYNC_DIR_RIGHT;
    localSyncConfiguration.rightNewer      = SyncConfiguration::SYNC_DIR_NONE;
    localSyncConfiguration.different       = SyncConfiguration::SYNC_DIR_NONE;

    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();

    //if event is triggered by button
    m_radioBtnUpdate->SetValue(true);
}


void SyncDialog::OnSyncBothSides(wxCommandEvent& event)
{
    localSyncConfiguration.exLeftSideOnly  = SyncConfiguration::SYNC_DIR_RIGHT;
    localSyncConfiguration.exRightSideOnly = SyncConfiguration::SYNC_DIR_LEFT;
    localSyncConfiguration.leftNewer       = SyncConfiguration::SYNC_DIR_RIGHT;
    localSyncConfiguration.rightNewer      = SyncConfiguration::SYNC_DIR_LEFT;
    localSyncConfiguration.different       = SyncConfiguration::SYNC_DIR_NONE;

    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();

    //if event is triggered by button
    m_radioBtn2->SetValue(true);
}


void toggleSyncDirection(SyncConfiguration::Direction& current)
{
    if (current == SyncConfiguration::SYNC_DIR_RIGHT)
        current = SyncConfiguration::SYNC_DIR_LEFT;
    else if (current == SyncConfiguration::SYNC_DIR_LEFT)
        current = SyncConfiguration::SYNC_DIR_NONE;
    else if (current== SyncConfiguration::SYNC_DIR_NONE)
        current = SyncConfiguration::SYNC_DIR_RIGHT;
    else
        assert (false);
}


void SyncDialog::OnExLeftSideOnly( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.exLeftSideOnly);
    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();
    //set custom config button
    m_radioBtn3->SetValue(true);
}


void SyncDialog::OnExRightSideOnly( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.exRightSideOnly);
    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();
    //set custom config button
    m_radioBtn3->SetValue(true);
}


void SyncDialog::OnLeftNewer( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.leftNewer);
    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();
    //set custom config button
    m_radioBtn3->SetValue(true);
}


void SyncDialog::OnRightNewer( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.rightNewer);
    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();
    //set custom config button
    m_radioBtn3->SetValue(true);
}


void SyncDialog::OnDifferent( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.different);
    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();
    //set custom config button
    m_radioBtn3->SetValue(true);
}
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
    //prepare drag & drop
    SetDropTarget(new BatchFileDropEvent(this));

    //set icons for this dialog
    m_bitmap13->SetBitmap(*globalResource.bitmapLeftOnly);
    m_bitmap14->SetBitmap(*globalResource.bitmapRightOnly);
    m_bitmap15->SetBitmap(*globalResource.bitmapLeftNewer);
    m_bitmap16->SetBitmap(*globalResource.bitmapRightNewer);
    m_bitmap17->SetBitmap(*globalResource.bitmapDifferent);
    m_bitmap8->SetBitmap(*globalResource.bitmapInclude);
    m_bitmap9->SetBitmap(*globalResource.bitmapExclude);
    m_bitmap27->SetBitmap(*globalResource.bitmapBatch);
}


void BatchDialog::updateFilterButton()
{
    if (filterIsActive)
    {
        m_bpButtonFilter->SetBitmapLabel(*globalResource.bitmapFilterOn);
        m_bpButtonFilter->SetToolTip(_("Filter active: Press again to deactivate"));

        m_textCtrlInclude->Enable();
        m_textCtrlExclude->Enable();
    }
    else
    {
        m_bpButtonFilter->SetBitmapLabel(*globalResource.bitmapFilterOff);
        m_bpButtonFilter->SetToolTip(_("Press button to activate filter"));

        m_textCtrlInclude->Disable();
        m_textCtrlExclude->Disable();
    }
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
    SyncDialog::updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
}


void BatchDialog::OnExRightSideOnly(wxCommandEvent& event)
{
    toggleSyncDirection(localSyncConfiguration.exRightSideOnly);
    SyncDialog::updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
}


void BatchDialog::OnLeftNewer(wxCommandEvent& event)
{
    toggleSyncDirection(localSyncConfiguration.leftNewer);
    SyncDialog::updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
}


void BatchDialog::OnRightNewer(wxCommandEvent& event)
{
    toggleSyncDirection(localSyncConfiguration.rightNewer);
    SyncDialog::updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
}


void BatchDialog::OnDifferent(wxCommandEvent& event)
{
    toggleSyncDirection(localSyncConfiguration.different);
    SyncDialog::updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
}


void BatchDialog::OnFilterButton(wxCommandEvent& event)
{
    filterIsActive = !filterIsActive;
    updateFilterButton();
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


void BatchDialog::OnChangeCompareVar(wxCommandEvent& event)
{
    CompareVariant var;
    if (m_radioBtnSizeDate->GetValue())
        var = CMP_BY_TIME_SIZE;
    else if (m_radioBtnContent->GetValue())
        var = CMP_BY_CONTENT;
    else
    {
        assert(false);
        var = CMP_BY_TIME_SIZE;
    }

    //set tooltip for ambivalent category "different"
    SyncDialog::adjustToolTips(m_bitmap17, var);
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
    wxString fileName = wxT("SyncJob.ffs_batch"); //proposal

    if (!proposedBatchFileName.empty())
        fileName = proposedBatchFileName;

    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, fileName, wxString(_("FreeFileSync batch file")) + wxT(" (*.ffs_batch)|*.ffs_batch"), wxFD_SAVE);

    if (filePicker->ShowModal() == wxID_OK)
    {
        fileName = filePicker->GetPath();
        if (FreeFileSync::fileExists(fileName.c_str()))
        {
            wxMessageDialog* messageDlg = new wxMessageDialog(this, wxString(_("File already exists. Overwrite?")) + wxT(" \"") + fileName + wxT("\""), _("Warning") , wxOK | wxCANCEL);

            if (messageDlg->ShowModal() != wxID_OK)
                return;
        }

        //create batch file
        if (saveBatchFile(fileName))
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
    if (m_radioBtnSizeDate->GetValue())
        batchCfg.mainCfg.compareVar = CMP_BY_TIME_SIZE;
    else if (m_radioBtnContent->GetValue())
        batchCfg.mainCfg.compareVar = CMP_BY_CONTENT;
    else
        return false;

    batchCfg.mainCfg.syncConfiguration = localSyncConfiguration;
    batchCfg.mainCfg.filterIsActive    = filterIsActive;
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
    SyncDialog::updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);

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
    default:
        assert (false);
    }
    //adjust toolTip
    SyncDialog::adjustToolTips(m_bitmap17, batchCfg.mainCfg.compareVar);

    filterIsActive = batchCfg.mainCfg.filterIsActive;
    updateFilterButton();

    m_textCtrlInclude->SetValue(batchCfg.mainCfg.includeFilter);
    m_textCtrlExclude->SetValue(batchCfg.mainCfg.excludeFilter);

    m_checkBoxSilent->SetValue(batchCfg.silent);

    //remove existing folder pairs
    localFolderPairs.clear();
    bSizerFolderPairs->Clear(true);

    //add folder pairs
    int scrWindowHeight = 0;
    for (std::vector<FolderPair>::const_iterator i = batchCfg.directoryPairs.begin(); i != batchCfg.directoryPairs.end(); ++i)
    {
        BatchFolderPairGenerated* newPair = new BatchFolderPairGenerated(m_scrolledWindow6);
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

    m_scrolledWindow6->Layout();
    //m_scrolledWindow6->Fit();

    Fit();
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
