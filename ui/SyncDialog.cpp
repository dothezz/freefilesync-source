#include "syncDialog.h"
#include "../library/globalFunctions.h"

SyncDialog::SyncDialog(MainDialog* window)
        : SyncDialogGenerated(window), mainDialog(window)
{
    //make working copy of mainDialog->syncConfiguration and recycler setting
    localSyncConfiguration = mainDialog->syncConfiguration;

    m_checkBoxUseRecycler->SetValue(mainDialog->useRecycleBin);

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
    if (localSyncConfiguration.exLeftSideOnly  == SyncDirRight &&
            localSyncConfiguration.exRightSideOnly == SyncDirRight &&
            localSyncConfiguration.leftNewer       == SyncDirRight &&
            localSyncConfiguration.rightNewer      == SyncDirRight &&
            localSyncConfiguration.different       == SyncDirRight)
        m_radioBtn1->SetValue(true);    //one way ->

    else if (localSyncConfiguration.exLeftSideOnly  == SyncDirRight &&
             localSyncConfiguration.exRightSideOnly == SyncDirLeft &&
             localSyncConfiguration.leftNewer       == SyncDirRight &&
             localSyncConfiguration.rightNewer      == SyncDirLeft &&
             localSyncConfiguration.different       == SyncDirNone)
        m_radioBtn2->SetValue(true);    //two way <->

    else
        m_radioBtn3->SetValue(true);    //other
}

//#################################################################################################################

SyncDialog::~SyncDialog() {}


void SyncDialog::updateConfigIcons()
{
    if (localSyncConfiguration.exLeftSideOnly == SyncDirRight)
    {
        m_bpButton5->SetBitmapLabel(*GlobalResources::bitmapRightArrow);
        m_bpButton5->SetToolTip(_("Copy from left to right"));
    }
    else if (localSyncConfiguration.exLeftSideOnly == SyncDirLeft)
    {
        m_bpButton5->SetBitmapLabel(*GlobalResources::bitmapDelete);
        m_bpButton5->SetToolTip(_("Delete files existing on left side only"));
    }
    else if (localSyncConfiguration.exLeftSideOnly == SyncDirNone)
    {
        m_bpButton5->SetBitmapLabel(wxNullBitmap);
        m_bpButton5->SetToolTip(_("Do nothing"));
    }

    if (localSyncConfiguration.exRightSideOnly == SyncDirRight)
    {
        m_bpButton6->SetBitmapLabel(*GlobalResources::bitmapDelete);
        m_bpButton6->SetToolTip(_("Delete files existing on right side only"));
    }
    else if (localSyncConfiguration.exRightSideOnly == SyncDirLeft)
    {
        m_bpButton6->SetBitmapLabel(*GlobalResources::bitmapLeftArrow);
        m_bpButton6->SetToolTip(_("Copy from right to left"));
    }
    else if (localSyncConfiguration.exRightSideOnly == SyncDirNone)
    {
        m_bpButton6->SetBitmapLabel(wxNullBitmap);
        m_bpButton6->SetToolTip(_("Do nothing"));
    }

    if (localSyncConfiguration.leftNewer == SyncDirRight)
    {
        m_bpButton7->SetBitmapLabel(*GlobalResources::bitmapRightArrow);
        m_bpButton7->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (localSyncConfiguration.leftNewer == SyncDirLeft)
    {
        m_bpButton7->SetBitmapLabel(*GlobalResources::bitmapLeftArrow);
        m_bpButton7->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (localSyncConfiguration.leftNewer == SyncDirNone)
    {
        m_bpButton7->SetBitmapLabel(wxNullBitmap);
        m_bpButton7->SetToolTip(_("Do nothing"));
    }

    if (localSyncConfiguration.rightNewer == SyncDirRight)
    {
        m_bpButton8->SetBitmapLabel(*GlobalResources::bitmapRightArrow);
        m_bpButton8->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (localSyncConfiguration.rightNewer == SyncDirLeft)
    {
        m_bpButton8->SetBitmapLabel(*GlobalResources::bitmapLeftArrow);
        m_bpButton8->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (localSyncConfiguration.rightNewer == SyncDirNone)
    {
        m_bpButton8->SetBitmapLabel(wxNullBitmap);
        m_bpButton8->SetToolTip(_("Do nothing"));
    }

    if (localSyncConfiguration.different == SyncDirRight)
    {
        m_bpButton9->SetBitmapLabel(*GlobalResources::bitmapRightArrow);
        m_bpButton9->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (localSyncConfiguration.different == SyncDirLeft)
    {
        m_bpButton9->SetBitmapLabel(*GlobalResources::bitmapLeftArrow);
        m_bpButton9->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (localSyncConfiguration.different == SyncDirNone)
    {
        m_bpButton9->SetBitmapLabel(wxNullBitmap);
        m_bpButton9->SetToolTip(_("Do nothing"));
    }

    //update preview of bytes to be transferred:
    m_textCtrl5->SetValue(FreeFileSync::formatFilesizeToShortString(FreeFileSync::calcTotalBytesToTransfer(mainDialog->currentGridData, localSyncConfiguration)));
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

    EndModal(0);
}

void SyncDialog::OnStartSync(wxCommandEvent& event)
{
    //write configuration to main dialog
    mainDialog->syncConfiguration = localSyncConfiguration;
    mainDialog->useRecycleBin     = m_checkBoxUseRecycler->GetValue();

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
    localSyncConfiguration.exLeftSideOnly  = SyncDirRight;
    localSyncConfiguration.exRightSideOnly = SyncDirRight;
    localSyncConfiguration.leftNewer       = SyncDirRight;
    localSyncConfiguration.rightNewer      = SyncDirRight;
    localSyncConfiguration.different       = SyncDirRight;

    updateConfigIcons();

    //if event is triggered by button
    m_radioBtn1->SetValue(true);
}


void SyncDialog::OnSyncBothSides( wxCommandEvent& event )
{
    localSyncConfiguration.exLeftSideOnly  = SyncDirRight;
    localSyncConfiguration.exRightSideOnly = SyncDirLeft;
    localSyncConfiguration.leftNewer       = SyncDirRight;
    localSyncConfiguration.rightNewer      = SyncDirLeft;
    localSyncConfiguration.different       = SyncDirNone;

    updateConfigIcons();
    //if event is triggered by button
    m_radioBtn2->SetValue(true);
}


void toggleSyncDirection(SyncDirection& current)
{
    if (current == SyncDirRight)
        current = SyncDirLeft;
    else if (current == SyncDirLeft)
        current = SyncDirNone;
    else if (current== SyncDirNone)
        current = SyncDirRight;
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
