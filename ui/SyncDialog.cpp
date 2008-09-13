#include "syncDialog.h"
#include "../library/globalFunctions.h"

SyncDialog::SyncDialog(MainDialog* window)
        : SyncDialogGenerated(window), mainDialog(window)
{
    //make working copy of mainDialog->syncConfiguration and recycler setting
    localSyncConfiguration = mainDialog->syncConfiguration;

    m_checkBoxUseRecycler->SetValue(mainDialog->useRecycleBin);
    m_checkBoxHideErrors->SetValue(mainDialog->hideErrorMessages);

    //set icons for this dialog
    m_bpButton18->SetBitmapLabel(*GlobalResources::bitmapStartSync);
    m_bitmap13->SetBitmap(*GlobalResources::bitmapLeftOnlyDeact);
    m_bitmap14->SetBitmap(*GlobalResources::bitmapRightOnlyDeact);
    m_bitmap15->SetBitmap(*GlobalResources::bitmapLeftNewerDeact);
    m_bitmap16->SetBitmap(*GlobalResources::bitmapRightNewerDeact);
    m_bitmap17->SetBitmap(*GlobalResources::bitmapDifferentDeact);

    //set sync config icons
    updateConfigIcons();

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
}

//#################################################################################################################

SyncDialog::~SyncDialog() {}


void SyncDialog::updateConfigIcons()
{
    if (localSyncConfiguration.exLeftSideOnly == syncDirRight)
    {
        m_bpButton5->SetBitmapLabel(*GlobalResources::bitmapRightArrow);
        m_bpButton5->SetToolTip(_("Copy from left to right"));
    }
    else if (localSyncConfiguration.exLeftSideOnly == syncDirLeft)
    {
        m_bpButton5->SetBitmapLabel(*GlobalResources::bitmapDelete);
        m_bpButton5->SetToolTip(_("Delete files existing on left side only"));
    }
    else if (localSyncConfiguration.exLeftSideOnly == syncDirNone)
    {
        m_bpButton5->SetBitmapLabel(wxNullBitmap);
        m_bpButton5->SetToolTip(_("Do nothing"));
    }

    if (localSyncConfiguration.exRightSideOnly == syncDirRight)
    {
        m_bpButton6->SetBitmapLabel(*GlobalResources::bitmapDelete);
        m_bpButton6->SetToolTip(_("Delete files existing on right side only"));
    }
    else if (localSyncConfiguration.exRightSideOnly == syncDirLeft)
    {
        m_bpButton6->SetBitmapLabel(*GlobalResources::bitmapLeftArrow);
        m_bpButton6->SetToolTip(_("Copy from right to left"));
    }
    else if (localSyncConfiguration.exRightSideOnly == syncDirNone)
    {
        m_bpButton6->SetBitmapLabel(wxNullBitmap);
        m_bpButton6->SetToolTip(_("Do nothing"));
    }

    if (localSyncConfiguration.leftNewer == syncDirRight)
    {
        m_bpButton7->SetBitmapLabel(*GlobalResources::bitmapRightArrow);
        m_bpButton7->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (localSyncConfiguration.leftNewer == syncDirLeft)
    {
        m_bpButton7->SetBitmapLabel(*GlobalResources::bitmapLeftArrow);
        m_bpButton7->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (localSyncConfiguration.leftNewer == syncDirNone)
    {
        m_bpButton7->SetBitmapLabel(wxNullBitmap);
        m_bpButton7->SetToolTip(_("Do nothing"));
    }

    if (localSyncConfiguration.rightNewer == syncDirRight)
    {
        m_bpButton8->SetBitmapLabel(*GlobalResources::bitmapRightArrow);
        m_bpButton8->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (localSyncConfiguration.rightNewer == syncDirLeft)
    {
        m_bpButton8->SetBitmapLabel(*GlobalResources::bitmapLeftArrow);
        m_bpButton8->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (localSyncConfiguration.rightNewer == syncDirNone)
    {
        m_bpButton8->SetBitmapLabel(wxNullBitmap);
        m_bpButton8->SetToolTip(_("Do nothing"));
    }

    if (localSyncConfiguration.different == syncDirRight)
    {
        m_bpButton9->SetBitmapLabel(*GlobalResources::bitmapRightArrow);
        m_bpButton9->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (localSyncConfiguration.different == syncDirLeft)
    {
        m_bpButton9->SetBitmapLabel(*GlobalResources::bitmapLeftArrow);
        m_bpButton9->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (localSyncConfiguration.different == syncDirNone)
    {
        m_bpButton9->SetBitmapLabel(wxNullBitmap);
        m_bpButton9->SetToolTip(_("Do nothing"));
    }

    //update preview of bytes to be transferred:
    int objectsTotal = 0;
    double dataTotal = 0;
    FreeFileSync::calcTotalBytesToSync(objectsTotal, dataTotal, mainDialog->currentGridData, localSyncConfiguration);

    wxString objects = globalFunctions::numberToWxString(objectsTotal);
    globalFunctions::includeNumberSeparator(objects);
    wxString data  = FreeFileSync::formatFilesizeToShortString(dataTotal);

    m_textCtrl12->SetValue(objects);
    m_textCtrl5->SetValue(data);
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
    mainDialog->syncConfiguration = localSyncConfiguration;
    mainDialog->useRecycleBin     = m_checkBoxUseRecycler->GetValue();
    mainDialog->hideErrorMessages = m_checkBoxHideErrors->GetValue();

    EndModal(0);
}

void SyncDialog::OnStartSync(wxCommandEvent& event)
{
    //write configuration to main dialog
    mainDialog->syncConfiguration = localSyncConfiguration;
    mainDialog->useRecycleBin     = m_checkBoxUseRecycler->GetValue();
    mainDialog->hideErrorMessages = m_checkBoxHideErrors->GetValue();

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


void SyncDialog::OnSyncLeftToRight( wxCommandEvent& event )
{
    localSyncConfiguration.exLeftSideOnly  = syncDirRight;
    localSyncConfiguration.exRightSideOnly = syncDirRight;
    localSyncConfiguration.leftNewer       = syncDirRight;
    localSyncConfiguration.rightNewer      = syncDirRight;
    localSyncConfiguration.different       = syncDirRight;

    updateConfigIcons();

    //if event is triggered by button
    m_radioBtn1->SetValue(true);
}


void SyncDialog::OnSyncBothSides( wxCommandEvent& event )
{
    localSyncConfiguration.exLeftSideOnly  = syncDirRight;
    localSyncConfiguration.exRightSideOnly = syncDirLeft;
    localSyncConfiguration.leftNewer       = syncDirRight;
    localSyncConfiguration.rightNewer      = syncDirLeft;
    localSyncConfiguration.different       = syncDirNone;

    updateConfigIcons();
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
    updateConfigIcons();
    //set custom config button
    m_radioBtn3->SetValue(true);
}

void SyncDialog::OnExRightSideOnly( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.exRightSideOnly);
    updateConfigIcons();
    //set custom config button
    m_radioBtn3->SetValue(true);
}

void SyncDialog::OnLeftNewer( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.leftNewer);
    updateConfigIcons();
    //set custom config button
    m_radioBtn3->SetValue(true);
}

void SyncDialog::OnRightNewer( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.rightNewer);
    updateConfigIcons();
    //set custom config button
    m_radioBtn3->SetValue(true);
}

void SyncDialog::OnDifferent( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.different);
    updateConfigIcons();
    //set custom config button
    m_radioBtn3->SetValue(true);
}
