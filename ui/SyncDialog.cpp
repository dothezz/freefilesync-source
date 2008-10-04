#include "syncDialog.h"
#include "../library/globalFunctions.h"
#include "../library/resources.h"
#include <wx/msgdlg.h>
#include <wx/file.h>
#include <wx/stdpaths.h>

SyncDialog::SyncDialog(wxWindow* window,
                       const FileCompareResult& gridDataRef,
                       Configuration& config,
                       bool synchronizationEnabled) :
        SyncDlgGenerated(window),
        gridData(gridDataRef),
        cfg(config)
{
    //make working copy of mainDialog.cfg.syncConfiguration and recycler setting
    localSyncConfiguration = config.syncConfiguration;
    m_checkBoxUseRecycler->SetValue(cfg.useRecycleBin);
    m_checkBoxContinueError->SetValue(cfg.continueOnError);

    //set sync config icons
    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    //update preview
    calculatePreview();

    //set icons for this dialog
    m_bpButton18->SetBitmapLabel(*GlobalResources::bitmapStartSync);
    m_bpButton18->SetBitmapDisabled(*GlobalResources::bitmapStartSyncDis);
    m_bitmap13->SetBitmap(*GlobalResources::bitmapLeftOnlyDeact);
    m_bitmap14->SetBitmap(*GlobalResources::bitmapRightOnlyDeact);
    m_bitmap15->SetBitmap(*GlobalResources::bitmapLeftNewerDeact);
    m_bitmap16->SetBitmap(*GlobalResources::bitmapRightNewerDeact);
    m_bitmap17->SetBitmap(*GlobalResources::bitmapDifferentDeact);

    if (synchronizationEnabled)
        m_bpButton18->Enable();
    else
    {
        m_bpButton18->Disable();
        m_button6->SetFocus();
    }

    //set radiobutton
    if (localSyncConfiguration.exLeftSideOnly  == syncDirRight &&
            localSyncConfiguration.exRightSideOnly == syncDirRight &&
            localSyncConfiguration.leftNewer       == syncDirRight &&
            localSyncConfiguration.rightNewer      == syncDirRight &&
            localSyncConfiguration.different       == syncDirRight)
        m_radioBtn1->SetValue(true);    //one way ->

    else if (localSyncConfiguration.exLeftSideOnly  == syncDirRight &&
             localSyncConfiguration.exRightSideOnly == syncDirLeft &&
             localSyncConfiguration.leftNewer       == syncDirRight &&
             localSyncConfiguration.rightNewer      == syncDirLeft &&
             localSyncConfiguration.different       == syncDirNone)
        m_radioBtn2->SetValue(true);    //two way <->

    else
        m_radioBtn3->SetValue(true);    //other

    m_bpButton18->SetLabel(_("&Start"));
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
    if (syncConfig.exLeftSideOnly == syncDirRight)
    {
        button1->SetBitmapLabel(*GlobalResources::bitmapRightArrow);
        button1->SetToolTip(_("Copy from left to right"));
    }
    else if (syncConfig.exLeftSideOnly == syncDirLeft)
    {
        button1->SetBitmapLabel(*GlobalResources::bitmapDelete);
        button1->SetToolTip(_("Delete files/folders existing on left side only"));
    }
    else if (syncConfig.exLeftSideOnly == syncDirNone)
    {
        button1->SetBitmapLabel(*GlobalResources::bitmapNoArrow);
        button1->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.exRightSideOnly == syncDirRight)
    {
        button2->SetBitmapLabel(*GlobalResources::bitmapDelete);
        button2->SetToolTip(_("Delete files/folders existing on right side only"));
    }
    else if (syncConfig.exRightSideOnly == syncDirLeft)
    {
        button2->SetBitmapLabel(*GlobalResources::bitmapLeftArrow);
        button2->SetToolTip(_("Copy from right to left"));
    }
    else if (syncConfig.exRightSideOnly == syncDirNone)
    {
        button2->SetBitmapLabel(*GlobalResources::bitmapNoArrow);
        button2->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.leftNewer == syncDirRight)
    {
        button3->SetBitmapLabel(*GlobalResources::bitmapRightArrow);
        button3->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (syncConfig.leftNewer == syncDirLeft)
    {
        button3->SetBitmapLabel(*GlobalResources::bitmapLeftArrow);
        button3->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (syncConfig.leftNewer == syncDirNone)
    {
        button3->SetBitmapLabel(*GlobalResources::bitmapNoArrow);
        button3->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.rightNewer == syncDirRight)
    {
        button4->SetBitmapLabel(*GlobalResources::bitmapRightArrow);
        button4->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (syncConfig.rightNewer == syncDirLeft)
    {
        button4->SetBitmapLabel(*GlobalResources::bitmapLeftArrow);
        button4->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (syncConfig.rightNewer == syncDirNone)
    {
        button4->SetBitmapLabel(*GlobalResources::bitmapNoArrow);
        button4->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.different == syncDirRight)
    {
        button5->SetBitmapLabel(*GlobalResources::bitmapRightArrow);
        button5->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (syncConfig.different == syncDirLeft)
    {
        button5->SetBitmapLabel(*GlobalResources::bitmapLeftArrow);
        button5->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (syncConfig.different == syncDirNone)
    {
        button5->SetBitmapLabel(*GlobalResources::bitmapNoArrow);
        button5->SetToolTip(_("Do nothing"));
    }
}


void SyncDialog::calculatePreview()
{
    //update preview of bytes to be transferred:
    int objectsToCreate    = 0;
    int objectsToOverwrite = 0;
    int objectsToDelete    = 0;
    double dataToProcess   = 0;
    FreeFileSync::calcTotalBytesToSync(objectsToCreate,
                                       objectsToOverwrite,
                                       objectsToDelete,
                                       dataToProcess,
                                       gridData,
                                       localSyncConfiguration);

    wxString toCreate = globalFunctions::numberToWxString(objectsToCreate);
    wxString toUpdate = globalFunctions::numberToWxString(objectsToOverwrite);
    wxString toDelete = globalFunctions::numberToWxString(objectsToDelete);
    wxString data     = FreeFileSync::formatFilesizeToShortString(dataToProcess);

    globalFunctions::includeNumberSeparator(toCreate);
    globalFunctions::includeNumberSeparator(toUpdate);
    globalFunctions::includeNumberSeparator(toDelete);

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
    cfg.continueOnError   = m_checkBoxContinueError->GetValue();

    EndModal(0);
}

void SyncDialog::OnStartSync(wxCommandEvent& event)
{
    //write configuration to main dialog
    cfg.syncConfiguration = localSyncConfiguration;
    cfg.useRecycleBin     = m_checkBoxUseRecycler->GetValue();
    cfg.continueOnError   = m_checkBoxContinueError->GetValue();

    EndModal(StartSynchronizationProcess);
}


void SyncDialog::OnSelectRecycleBin(wxCommandEvent& event)
{
    if (event.IsChecked())
    {
        if (!FreeFileSync::recycleBinExists())
        {
            wxMessageBox(_("It was not possible to gain access to Recycle Bin!\n\nIt's likely that you are not using Windows XP. (Probably Vista)\nIf you want this feature included, please contact the author. :)"), _("Error") , wxOK | wxICON_ERROR);
            m_checkBoxUseRecycler->SetValue(false);
        }
    }
}


void SyncDialog::OnSyncLeftToRight(wxCommandEvent& event)
{
    localSyncConfiguration.exLeftSideOnly  = syncDirRight;
    localSyncConfiguration.exRightSideOnly = syncDirRight;
    localSyncConfiguration.leftNewer       = syncDirRight;
    localSyncConfiguration.rightNewer      = syncDirRight;
    localSyncConfiguration.different       = syncDirRight;

    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();

    //if event is triggered by button
    m_radioBtn1->SetValue(true);
}


void SyncDialog::OnSyncBothSides(wxCommandEvent& event)
{
    localSyncConfiguration.exLeftSideOnly  = syncDirRight;
    localSyncConfiguration.exRightSideOnly = syncDirLeft;
    localSyncConfiguration.leftNewer       = syncDirRight;
    localSyncConfiguration.rightNewer      = syncDirLeft;
    localSyncConfiguration.different       = syncDirNone;

    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();

    //if event is triggered by button
    m_radioBtn2->SetValue(true);
}


void toggleSyncDirection(SyncDirection& current)
{
    if (current == syncDirRight)
        current = syncDirLeft;
    else if (current == syncDirLeft)
        current = syncDirNone;
    else if (current== syncDirNone)
        current = syncDirRight;
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


BatchDialog::BatchDialog(wxWindow* window,
                         const Configuration& config,
                         const wxString& leftDir,
                         const wxString& rightDir) :
        BatchDlgGenerated(window)
{
    //make working copy of mainDialog.cfg.syncConfiguration and recycler setting
    localSyncConfiguration = config.syncConfiguration;
    SyncDialog::updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);

    m_checkBoxUseRecycler->SetValue(config.useRecycleBin);
    m_checkBoxContinueError->SetValue(config.continueOnError);

    switch (config.compareVar)
    {
    case compareByTimeAndSize:
        m_radioBtnSizeDate->SetValue(true);
        break;
    case compareByContent:
        m_radioBtnContent->SetValue(true);
        break;
    default:
        assert (false);
    }

    filterIsActive = config.filterIsActive;
    updateFilterButton();

    m_textCtrlInclude->SetValue(config.includeFilter);
    m_textCtrlExclude->SetValue(config.excludeFilter);

    m_directoryPanel1->SetValue(leftDir);
    m_directoryPanel2->SetValue(rightDir);

    //set icons for this dialog
    m_bitmap13->SetBitmap(*GlobalResources::bitmapLeftOnlyDeact);
    m_bitmap14->SetBitmap(*GlobalResources::bitmapRightOnlyDeact);
    m_bitmap15->SetBitmap(*GlobalResources::bitmapLeftNewerDeact);
    m_bitmap16->SetBitmap(*GlobalResources::bitmapRightNewerDeact);
    m_bitmap17->SetBitmap(*GlobalResources::bitmapDifferentDeact);
    m_bitmap8->SetBitmap(*GlobalResources::bitmapInclude);
    m_bitmap9->SetBitmap(*GlobalResources::bitmapExclude);

    m_buttonCreate->SetFocus();
}

BatchDialog::~BatchDialog()
{}


void BatchDialog::updateFilterButton()
{
    if (filterIsActive)
    {
        m_bpButtonFilter->SetBitmapLabel(*GlobalResources::bitmapFilterOn);
        m_bpButtonFilter->SetToolTip(_("Filter active: Press again to deactivate"));

        m_textCtrlInclude->Enable();
        m_textCtrlExclude->Enable();
    }
    else
    {
        m_bpButtonFilter->SetBitmapLabel(*GlobalResources::bitmapFilterOff);
        m_bpButtonFilter->SetToolTip(_("Press button to activate filter"));

        m_textCtrlInclude->Disable();
        m_textCtrlExclude->Disable();
    }
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
    if (event.IsChecked())
    {
        if (!FreeFileSync::recycleBinExists())
        {
            wxMessageBox(_("It was not possible to gain access to Recycle Bin!\n\nIt's likely that you are not using Windows XP. (Probably Vista)\nIf you want this feature included, please contact the author. :)"), _("Error") , wxOK | wxICON_ERROR);
            m_checkBoxUseRecycler->SetValue(false);
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


void BatchDialog::OnCreateJob(wxCommandEvent& event)
{
    //get a filename
#ifdef FFS_WIN
    wxString fileName = "SyncJob.cmd"; //proposal
    wxFileDialog* filePicker = new wxFileDialog(this, "", "", fileName, wxString(_("Command file")) + " (*.cmd)|*.cmd", wxFD_SAVE);
#elif defined FFS_LINUX
    wxString fileName = "SyncJob.sh";  //proposal
    wxFileDialog* filePicker = new wxFileDialog(this, "", "", fileName, wxString(_("Shell script")) + " (*.sh)|*.sh", wxFD_SAVE);
#else
    assert(false);
#endif

    if (filePicker->ShowModal() == wxID_OK)
    {
        fileName = filePicker->GetPath();
        if (wxFileExists(fileName))
        {
            wxMessageDialog* messageDlg = new wxMessageDialog(this, wxString("\"") + fileName + "\"" + _(" already exists. Overwrite?"), _("Warning") , wxOK | wxCANCEL);

            if (messageDlg->ShowModal() != wxID_OK)
            {
                event.Skip();
                return;
            }
        }

        //assemble command line parameters
        wxString outputString = parseConfiguration();

        //write export file
        wxFile output(fileName, wxFile::write);
        if (output.IsOpened())
        {
            output.Write(outputString);
            EndModal(batchFileCreated);
        }
        else
            wxMessageBox(wxString(_("Could not write to ")) + "\"" + fileName + "\"", _("An exception occured!"), wxOK | wxICON_ERROR);
    }

#ifdef FFS_LINUX
    //for linux the batch file needs the executable flag
    wxExecute(wxString("chmod +x ") + fileName);
#endif  // FFS_LINUX

    event.Skip();
}


wxString getFormattedSyncDirection(const SyncDirection direction)
{
    if (direction == syncDirRight)
        return 'R';
    else if (direction == syncDirLeft)
        return 'L';
    else if (direction == syncDirNone)
        return 'N';
    else
    {
        assert (false);
        return wxEmptyString;
    }
}


wxString BatchDialog::parseConfiguration()
{
    wxString output;

#ifdef FFS_LINUX
    //shell script identifier
    output+= "#!/bin/bash\n";
#endif

    output+= "\"" + wxStandardPaths::Get().GetExecutablePath() + "\"";

    output+= wxString(" -") + GlobalResources::paramCompare + " ";
    if (m_radioBtnSizeDate->GetValue())
        output+= GlobalResources::valueSizeDate;
    else if (m_radioBtnContent->GetValue())
        output+= GlobalResources::valueContent;
    else
        assert(false);

    output+= wxString(" -") + GlobalResources::paramCfg + " " +
             getFormattedSyncDirection(localSyncConfiguration.exLeftSideOnly) +
             getFormattedSyncDirection(localSyncConfiguration.exRightSideOnly) +
             getFormattedSyncDirection(localSyncConfiguration.leftNewer) +
             getFormattedSyncDirection(localSyncConfiguration.rightNewer) +
             getFormattedSyncDirection(localSyncConfiguration.different);

    if (filterIsActive)
    {
        output+= wxString(" -") + GlobalResources::paramInclude + " " +
                 "\"" + m_textCtrlInclude->GetValue() + "\"";

        output+= wxString(" -") + GlobalResources::paramExclude + " " +
                 "\"" + m_textCtrlExclude->GetValue() + "\"";
    }

    if (m_checkBoxUseRecycler->GetValue())
        output+= wxString(" -") + GlobalResources::paramRecycler;

    if (m_checkBoxContinueError->GetValue())
        output+= wxString(" -") + GlobalResources::paramContinueError;

    if (m_checkBoxSilent->GetValue())
        output+= wxString(" -") + GlobalResources::paramSilent;

    output+= wxString(" ") + "\"" + m_directoryPanel1->GetValue() + "\"";
    output+= wxString(" ") + "\"" + m_directoryPanel2->GetValue() + "\"";

    output+= "\n";

    return output;
}

