#include "SyncDialog.h"
#include "..\library\globalfunctions.h"

SyncDialog::SyncDialog(MainDialog* window)
        : SyncDialogGenerated(window), mainDialog(window)
{
    //set icons for this dialog
    m_bpButton18->SetBitmapLabel(*mainDialog->bitmapStartSync);
    m_bitmap13->SetBitmap(*mainDialog->bitmapLeftOnlyDeact);
    m_bitmap14->SetBitmap(*mainDialog->bitmapRightOnlyDeact);
    m_bitmap15->SetBitmap(*mainDialog->bitmapLeftNewerDeact);
    m_bitmap16->SetBitmap(*mainDialog->bitmapRightNewerDeact);
    m_bitmap17->SetBitmap(*mainDialog->bitmapDifferentDeact);

    //set sync config icons
    updateConfigIcons();

    //set radiobutton
    if (mainDialog->syncConfiguration.exLeftSideOnly  == syncDirRight &&
            mainDialog->syncConfiguration.exRightSideOnly == syncDirRight &&
            mainDialog->syncConfiguration.leftNewer       == syncDirRight &&
            mainDialog->syncConfiguration.rightNewer      == syncDirRight &&
            mainDialog->syncConfiguration.different       == syncDirRight)
        m_radioBtn1->SetValue(true);

    else if (mainDialog->syncConfiguration.exLeftSideOnly  == syncDirRight &&
             mainDialog->syncConfiguration.exRightSideOnly == syncDirLeft &&
             mainDialog->syncConfiguration.leftNewer       == syncDirRight &&
             mainDialog->syncConfiguration.rightNewer      == syncDirLeft &&
             mainDialog->syncConfiguration.different       == syncDirNone)
        m_radioBtn2->SetValue(true);
    else
        m_radioBtn3->SetValue(true);
}

//#################################################################################################################

SyncDialog::~SyncDialog()
{
}

void SyncDialog::updateConfigIcons()
{
    if (mainDialog->syncConfiguration.exLeftSideOnly == syncDirRight)
    {
        m_bpButton5->SetBitmapLabel(*mainDialog->bitmapRightArrow);
        m_bpButton5->SetToolTip(_("Copy from left to right"));
    }
    else if (mainDialog->syncConfiguration.exLeftSideOnly == syncDirLeft)
    {
        m_bpButton5->SetBitmapLabel(*mainDialog->bitmapDelete);
        m_bpButton5->SetToolTip(_("Delete files existing on left side only"));
    }
    else if (mainDialog->syncConfiguration.exLeftSideOnly == syncDirNone)
    {
        m_bpButton5->SetBitmapLabel(wxNullBitmap);
        m_bpButton5->SetToolTip(_("Do nothing"));
    }

    if (mainDialog->syncConfiguration.exRightSideOnly == syncDirRight)
    {
        m_bpButton6->SetBitmapLabel(*mainDialog->bitmapDelete);
        m_bpButton6->SetToolTip(_("Delete files existing on right side only"));
    }
    else if (mainDialog->syncConfiguration.exRightSideOnly == syncDirLeft)
    {
        m_bpButton6->SetBitmapLabel(*mainDialog->bitmapLeftArrow);
        m_bpButton6->SetToolTip(_("Copy from right to left"));
    }
    else if (mainDialog->syncConfiguration.exRightSideOnly == syncDirNone)
    {
        m_bpButton6->SetBitmapLabel(wxNullBitmap);
        m_bpButton6->SetToolTip(_("Do nothing"));
    }

    if (mainDialog->syncConfiguration.leftNewer == syncDirRight)
    {
        m_bpButton7->SetBitmapLabel(*mainDialog->bitmapRightArrow);
        m_bpButton7->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (mainDialog->syncConfiguration.leftNewer == syncDirLeft)
    {
        m_bpButton7->SetBitmapLabel(*mainDialog->bitmapLeftArrow);
        m_bpButton7->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (mainDialog->syncConfiguration.leftNewer == syncDirNone)
    {
        m_bpButton7->SetBitmapLabel(wxNullBitmap);
        m_bpButton7->SetToolTip(_("Do nothing"));
    }

    if (mainDialog->syncConfiguration.rightNewer == syncDirRight)
    {
        m_bpButton8->SetBitmapLabel(*mainDialog->bitmapRightArrow);
        m_bpButton8->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (mainDialog->syncConfiguration.rightNewer == syncDirLeft)
    {
        m_bpButton8->SetBitmapLabel(*mainDialog->bitmapLeftArrow);
        m_bpButton8->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (mainDialog->syncConfiguration.rightNewer == syncDirNone)
    {
        m_bpButton8->SetBitmapLabel(wxNullBitmap);
        m_bpButton8->SetToolTip(_("Do nothing"));
    }

    if (mainDialog->syncConfiguration.different == syncDirRight)
    {
        m_bpButton9->SetBitmapLabel(*mainDialog->bitmapRightArrow);
        m_bpButton9->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (mainDialog->syncConfiguration.different == syncDirLeft)
    {
        m_bpButton9->SetBitmapLabel(*mainDialog->bitmapLeftArrow);
        m_bpButton9->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (mainDialog->syncConfiguration.different == syncDirNone)
    {
        m_bpButton9->SetBitmapLabel(wxNullBitmap);
        m_bpButton9->SetToolTip(_("Do nothing"));
    }

    //update preview of bytes to be transferred:
    m_textCtrl5->SetValue(FreeFileSync::formatFilesizeToShortString(FreeFileSync::calcTotalBytesToTransfer(mainDialog->currentGridData, mainDialog->syncConfiguration)));

}


void SyncDialog::OnClose(wxCloseEvent& event)
{
    EndModal(0);
}

void SyncDialog::OnBack(wxCommandEvent& event)
{
    EndModal(0);
}

void SyncDialog::OnStartSync(wxCommandEvent& event)
{
    EndModal(StartSynchronizationProcess);
}

void SyncDialog::OnSyncLeftToRight( wxCommandEvent& event )
{
    mainDialog->syncConfiguration.exLeftSideOnly  = syncDirRight;
    mainDialog->syncConfiguration.exRightSideOnly = syncDirRight;
    mainDialog->syncConfiguration.leftNewer       = syncDirRight;
    mainDialog->syncConfiguration.rightNewer      = syncDirRight;
    mainDialog->syncConfiguration.different       = syncDirRight;

    updateConfigIcons();

    //if event is triggered by button
    m_radioBtn1->SetValue(true);
}

void SyncDialog::OnSyncBothSides( wxCommandEvent& event )
{
    mainDialog->syncConfiguration.exLeftSideOnly  = syncDirRight;
    mainDialog->syncConfiguration.exRightSideOnly = syncDirLeft;
    mainDialog->syncConfiguration.leftNewer       = syncDirRight;
    mainDialog->syncConfiguration.rightNewer      = syncDirLeft;
    mainDialog->syncConfiguration.different       = syncDirNone;

    updateConfigIcons();
    //if event is triggered by button
    m_radioBtn2->SetValue(true);
}

void SyncDialog::OnSyncCostum( wxCommandEvent& event )
{
    //if event is triggered by button
    m_radioBtn3->SetValue(true);
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
    toggleSyncDirection(mainDialog->syncConfiguration.exLeftSideOnly);
    updateConfigIcons();
    //set custom config button
    m_radioBtn3->SetValue(true);
}

void SyncDialog::OnExRightSideOnly( wxCommandEvent& event )
{
    toggleSyncDirection(mainDialog->syncConfiguration.exRightSideOnly);
    updateConfigIcons();
    //set custom config button
    m_radioBtn3->SetValue(true);
}

void SyncDialog::OnLeftNewer( wxCommandEvent& event )
{
    toggleSyncDirection(mainDialog->syncConfiguration.leftNewer);
    updateConfigIcons();
    //set custom config button
    m_radioBtn3->SetValue(true);
}

void SyncDialog::OnRightNewer( wxCommandEvent& event )
{
    toggleSyncDirection(mainDialog->syncConfiguration.rightNewer);
    updateConfigIcons();
    //set custom config button
    m_radioBtn3->SetValue(true);
}

void SyncDialog::OnDifferent( wxCommandEvent& event )
{
    toggleSyncDirection(mainDialog->syncConfiguration.different);
    updateConfigIcons();
    //set custom config button
    m_radioBtn3->SetValue(true);
}
