// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "syncConfig.h"

#include "../library/resources.h"
#include "../shared/dragAndDrop.h"
#include <wx/wupdlock.h>

using namespace FreeFileSync;


SyncCfgDialog::SyncCfgDialog(wxWindow* window,
                             const CompareVariant compareVar,
                             SyncConfiguration& syncConfiguration,
                             DeletionPolicy&    handleDeletion,
                             wxString&          customDeletionDirectory,
                             bool*              ignoreErrors) :
    SyncCfgDlgGenerated(window),
    cmpVariant(compareVar),
    currentSyncConfig(syncConfiguration),  //make working copy of syncConfiguration
    refSyncConfiguration(syncConfiguration),
    refHandleDeletion(handleDeletion),
    refCustomDeletionDirectory(customDeletionDirectory),
    refIgnoreErrors(ignoreErrors),
    dragDropCustomDelFolder(new DragDropOnDlg(m_panelCustomDeletionDir, m_dirPickerCustomDelFolder, m_textCtrlCustomDelFolder))
{
    setDeletionHandling(handleDeletion);
    m_textCtrlCustomDelFolder->SetValue(customDeletionDirectory);

    //error handling
    if (ignoreErrors)
        setErrorHandling(*ignoreErrors);
    else
    {
        sbSizerErrorHandling->Show(false);
        Layout();
    }

    //set sync config icons
    updateConfigIcons(cmpVariant, currentSyncConfig);

    //set icons for this dialog
    m_bitmapLeftOnly->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("leftOnly")));
    m_bitmapRightOnly->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("rightOnly")));
    m_bitmapLeftNewer->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("leftNewer")));
    m_bitmapRightNewer->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("rightNewer")));
    m_bitmapDifferent->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("different")));
    m_bitmapConflict->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("conflictGrey")));

    bSizer201->Layout(); //wxButtonWithImage size might have changed

    m_buttonOK->SetFocus();

    Fit();
}

//#################################################################################################################

SyncCfgDialog::~SyncCfgDialog() {} //non-inline destructor for std::auto_ptr to work with forward declaration


void SyncCfgDialog::updateConfigIcons(const FreeFileSync::CompareVariant cmpVar, const FreeFileSync::SyncConfiguration& syncConfig)
{
    //wxWindowUpdateLocker dummy(this); //avoid display distortion
    wxWindowUpdateLocker dummy2(m_panelCustomDeletionDir); //avoid display distortion
    wxWindowUpdateLocker dummy3(m_bpButtonLeftOnly);
    wxWindowUpdateLocker dummy4(m_bpButtonRightOnly);
    wxWindowUpdateLocker dummy5(m_bpButtonLeftNewer);
    wxWindowUpdateLocker dummy6(m_bpButtonRightNewer);
    wxWindowUpdateLocker dummy7(m_bpButtonDifferent);
    wxWindowUpdateLocker dummy8(m_bpButtonConflict);


    updateConfigIcons(cmpVar,
                      syncConfig,
                      m_bpButtonLeftOnly,
                      m_bpButtonRightOnly,
                      m_bpButtonLeftNewer,
                      m_bpButtonRightNewer,
                      m_bpButtonDifferent,
                      m_bpButtonConflict,
                      m_bitmapLeftOnly,
                      m_bitmapRightOnly,
                      m_bitmapLeftNewer,
                      m_bitmapRightNewer,
                      m_bitmapDifferent,
                      m_bitmapConflict,
                      sbSizerSyncDirections);

    //set radiobuttons -> have no parameter-ownership at all!
    switch (FreeFileSync::getVariant(currentSyncConfig))
    {
    case SyncConfiguration::AUTOMATIC:
        m_radioBtnAutomatic->SetValue(true); //automatic mode
        break;
    case SyncConfiguration::MIRROR:
        m_radioBtnMirror->SetValue(true);    //one way ->
        break;
    case SyncConfiguration::UPDATE:
        m_radioBtnUpdate->SetValue(true);    //Update ->
        break;
    case SyncConfiguration::CUSTOM:
        m_radioBtnCustom->SetValue(true);    //custom
        break;
    }

    GetSizer()->SetSizeHints(this); //this works like a charm for GTK2 with window resizing problems!!! (includes call to Fit())
}


void SyncCfgDialog::updateConfigIcons(const CompareVariant compareVar,
                                      const SyncConfiguration& syncConfig,
                                      wxBitmapButton* buttonLeftOnly,
                                      wxBitmapButton* buttonRightOnly,
                                      wxBitmapButton* buttonLeftNewer,
                                      wxBitmapButton* buttonRightNewer,
                                      wxBitmapButton* buttonDifferent,
                                      wxBitmapButton* buttonConflict,
                                      wxStaticBitmap* bitmapLeftOnly,
                                      wxStaticBitmap* bitmapRightOnly,
                                      wxStaticBitmap* bitmapLeftNewer,
                                      wxStaticBitmap* bitmapRightNewer,
                                      wxStaticBitmap* bitmapDifferent,
                                      wxStaticBitmap* bitmapConflict,
                                      wxSizer*        syncDirections) //sizer containing all sync-directions
{
    //display only relevant sync options
    syncDirections->Show(true);

    buttonLeftOnly  ->Show(); //
    buttonRightOnly ->Show(); //
    buttonLeftNewer ->Show(); //
    buttonRightNewer->Show(); // enable everything by default
    buttonDifferent ->Show(); //
    buttonConflict  ->Show(); //

    bitmapLeftOnly  ->Show(); //
    bitmapRightOnly ->Show(); //
    bitmapLeftNewer ->Show(); //
    bitmapRightNewer->Show(); //
    bitmapDifferent ->Show(); //
    bitmapConflict  ->Show(); //

    switch (compareVar)
    {
    case CMP_BY_TIME_SIZE:
        buttonDifferent ->Hide();

        bitmapDifferent ->Hide();
        break;

    case CMP_BY_CONTENT:
        buttonLeftNewer ->Hide();
        buttonRightNewer->Hide();

        bitmapLeftNewer ->Hide();
        bitmapRightNewer->Hide();
        break;
    }

    if (syncConfig.automatic) //automatic mode needs no sync-directions
        syncDirections->Show(false);

    switch (syncConfig.exLeftSideOnly)
    {
    case SYNC_DIR_RIGHT:
        buttonLeftOnly->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("arrowRightCr")));
        buttonLeftOnly->SetToolTip(getDescription(SO_CREATE_NEW_RIGHT));
        break;
    case SYNC_DIR_LEFT:
        buttonLeftOnly->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("deleteLeft")));
        buttonLeftOnly->SetToolTip(getDescription(SO_DELETE_LEFT));
        break;
    case SYNC_DIR_NONE:
        buttonLeftOnly->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("arrowNone")));
        buttonLeftOnly->SetToolTip(getDescription(SO_DO_NOTHING));
        break;
    }

    switch (syncConfig.exRightSideOnly)
    {
    case SYNC_DIR_RIGHT:
        buttonRightOnly->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("deleteRight")));
        buttonRightOnly->SetToolTip(getDescription(SO_DELETE_RIGHT));
        break;
    case SYNC_DIR_LEFT:
        buttonRightOnly->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("arrowLeftCr")));
        buttonRightOnly->SetToolTip(getDescription(SO_CREATE_NEW_LEFT));
        break;
    case SYNC_DIR_NONE:
        buttonRightOnly->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("arrowNone")));
        buttonRightOnly->SetToolTip(getDescription(SO_DO_NOTHING));
        break;
    }

    switch (syncConfig.leftNewer)
    {
    case SYNC_DIR_RIGHT:
        buttonLeftNewer->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("arrowRight")));
        buttonLeftNewer->SetToolTip(getDescription(SO_OVERWRITE_RIGHT));
        break;
    case SYNC_DIR_LEFT:
        buttonLeftNewer->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("arrowLeft")));
        buttonLeftNewer->SetToolTip(getDescription(SO_OVERWRITE_LEFT));
        break;
    case SYNC_DIR_NONE:
        buttonLeftNewer->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("arrowNone")));
        buttonLeftNewer->SetToolTip(getDescription(SO_DO_NOTHING));
        break;
    }

    switch (syncConfig.rightNewer)
    {
    case SYNC_DIR_RIGHT:
        buttonRightNewer->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("arrowRight")));
        buttonRightNewer->SetToolTip(getDescription(SO_OVERWRITE_RIGHT));
        break;
    case SYNC_DIR_LEFT:
        buttonRightNewer->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("arrowLeft")));
        buttonRightNewer->SetToolTip(getDescription(SO_OVERWRITE_LEFT));
        break;
    case SYNC_DIR_NONE:
        buttonRightNewer->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("arrowNone")));
        buttonRightNewer->SetToolTip(getDescription(SO_DO_NOTHING));
        break;
    }

    switch (syncConfig.different)
    {
    case SYNC_DIR_RIGHT:
        buttonDifferent->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("arrowRight")));
        buttonDifferent->SetToolTip(getDescription(SO_OVERWRITE_RIGHT));
        break;
    case SYNC_DIR_LEFT:
        buttonDifferent->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("arrowLeft")));
        buttonDifferent->SetToolTip(getDescription(SO_OVERWRITE_LEFT));
        break;
    case SYNC_DIR_NONE:
        buttonDifferent->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("arrowNone")));
        buttonDifferent->SetToolTip(getDescription(SO_DO_NOTHING));
        break;
    }

    switch (syncConfig.conflict)
    {
    case SYNC_DIR_RIGHT:
        buttonConflict->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("arrowRight")));
        buttonConflict->SetToolTip(getDescription(SO_OVERWRITE_RIGHT));
        break;
    case SYNC_DIR_LEFT:
        buttonConflict->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("arrowLeft")));
        buttonConflict->SetToolTip(getDescription(SO_OVERWRITE_LEFT));
        break;
    case SYNC_DIR_NONE:
        buttonConflict->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("conflict")));
        buttonConflict->SetToolTip(_("Leave as unresolved conflict"));
        break;
    }
}


void SyncCfgDialog::OnClose(wxCloseEvent& event)
{
    EndModal(0);
}


void SyncCfgDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(0);
}


void SyncCfgDialog::OnApply(wxCommandEvent& event)
{
    //write configuration to main dialog
    refSyncConfiguration = currentSyncConfig;
    refHandleDeletion    = getDeletionHandling();
    refCustomDeletionDirectory = m_textCtrlCustomDelFolder->GetValue();
    if (refIgnoreErrors)
        *refIgnoreErrors = getErrorHandling();

    EndModal(BUTTON_APPLY);
}


void SyncCfgDialog::updateToolTipErrorHandling(bool ignoreErrors)
{
    if (ignoreErrors)
        m_choiceHandleError->SetToolTip(_("Hide all error and warning messages"));
    else
        m_choiceHandleError->SetToolTip(_("Show popup on errors or warnings"));
}


bool SyncCfgDialog::getErrorHandling()
{
    if (m_choiceHandleError->GetSelection() == 1) //Ignore errors
        return true;
    else
        return false; // Show popup
}


void SyncCfgDialog::setErrorHandling(bool ignoreErrors)
{
    m_choiceHandleError->Clear();
    m_choiceHandleError->Append(_("Show popup"));
    m_choiceHandleError->Append(_("Ignore errors"));

    if (ignoreErrors)
        m_choiceHandleError->SetSelection(1);
    else
        m_choiceHandleError->SetSelection(0);

    updateToolTipErrorHandling(ignoreErrors);
}


void SyncCfgDialog::OnChangeErrorHandling(wxCommandEvent& event)
{
    updateToolTipErrorHandling(getErrorHandling());
}

//-------------------

void updateToolTipDeletionHandling(wxChoice* choiceHandleError, wxPanel* customDir, const FreeFileSync::DeletionPolicy value)
{
    customDir->Disable();

    switch (value)
    {
    case FreeFileSync::DELETE_PERMANENTLY:
        choiceHandleError->SetToolTip(_("Delete or overwrite files permanently"));
        break;

    case FreeFileSync::MOVE_TO_RECYCLE_BIN:
        choiceHandleError->SetToolTip(_("Use Recycle Bin when deleting or overwriting files"));
        break;

    case FreeFileSync::MOVE_TO_CUSTOM_DIRECTORY:
        choiceHandleError->SetToolTip(_("Move files into a time-stamped subdirectory"));
        customDir->Enable();
        break;
    }
}


FreeFileSync::DeletionPolicy SyncCfgDialog::getDeletionHandling()
{
    switch (m_choiceHandleDeletion->GetSelection())
    {
    case 0:
        return FreeFileSync::DELETE_PERMANENTLY;
    case 1:
        return FreeFileSync::MOVE_TO_RECYCLE_BIN;
    case 2:
        return FreeFileSync::MOVE_TO_CUSTOM_DIRECTORY;
    default:
        assert(false);
        return FreeFileSync::MOVE_TO_RECYCLE_BIN;
    }
}


void SyncCfgDialog::setDeletionHandling(FreeFileSync::DeletionPolicy newValue)
{
    m_choiceHandleDeletion->Clear();
    m_choiceHandleDeletion->Append(_("Delete permanently"));
    m_choiceHandleDeletion->Append(_("Use Recycle Bin"));
    m_choiceHandleDeletion->Append(_("User-defined directory"));

    switch (newValue)
    {
    case FreeFileSync::DELETE_PERMANENTLY:
        m_choiceHandleDeletion->SetSelection(0);
        break;
    case FreeFileSync::MOVE_TO_RECYCLE_BIN:
        m_choiceHandleDeletion->SetSelection(1);
        break;
    case FreeFileSync::MOVE_TO_CUSTOM_DIRECTORY:
        m_choiceHandleDeletion->SetSelection(2);
        break;
    }

    updateToolTipDeletionHandling(m_choiceHandleDeletion, m_panelCustomDeletionDir, newValue);
}


void SyncCfgDialog::OnChangeDeletionHandling(wxCommandEvent& event)
{
    updateToolTipDeletionHandling(m_choiceHandleDeletion, m_panelCustomDeletionDir, getDeletionHandling());
}


void SyncCfgDialog::OnSyncAutomatic(wxCommandEvent& event)
{
    FreeFileSync::setVariant(currentSyncConfig, SyncConfiguration::AUTOMATIC);
    updateConfigIcons(cmpVariant, currentSyncConfig);
}


void SyncCfgDialog::OnSyncLeftToRight(wxCommandEvent& event)
{
    FreeFileSync::setVariant(currentSyncConfig, SyncConfiguration::MIRROR);
    updateConfigIcons(cmpVariant, currentSyncConfig);
}


void SyncCfgDialog::OnSyncUpdate(wxCommandEvent& event)
{
    FreeFileSync::setVariant(currentSyncConfig, SyncConfiguration::UPDATE);
    updateConfigIcons(cmpVariant, currentSyncConfig);
}


void toggleSyncDirection(SyncDirection& current)
{
    switch (current)
    {
    case SYNC_DIR_RIGHT:
        current = SYNC_DIR_LEFT;
        break;
    case SYNC_DIR_LEFT:
        current = SYNC_DIR_NONE;
        break;
    case SYNC_DIR_NONE:
        current = SYNC_DIR_RIGHT;
        break;
    }
}


void SyncCfgDialog::OnExLeftSideOnly(wxCommandEvent& event )
{
    toggleSyncDirection(currentSyncConfig.exLeftSideOnly);
    updateConfigIcons(cmpVariant, currentSyncConfig);
}


void SyncCfgDialog::OnExRightSideOnly(wxCommandEvent& event )
{
    toggleSyncDirection(currentSyncConfig.exRightSideOnly);
    updateConfigIcons(cmpVariant, currentSyncConfig);
}


void SyncCfgDialog::OnLeftNewer(wxCommandEvent& event )
{
    toggleSyncDirection(currentSyncConfig.leftNewer);
    updateConfigIcons(cmpVariant, currentSyncConfig);
}


void SyncCfgDialog::OnRightNewer(wxCommandEvent& event )
{
    toggleSyncDirection(currentSyncConfig.rightNewer);
    updateConfigIcons(cmpVariant, currentSyncConfig);
}


void SyncCfgDialog::OnDifferent(wxCommandEvent& event )
{
    toggleSyncDirection(currentSyncConfig.different);
    updateConfigIcons(cmpVariant, currentSyncConfig);
}


void SyncCfgDialog::OnConflict(wxCommandEvent& event)
{
    toggleSyncDirection(currentSyncConfig.conflict);
    updateConfigIcons(cmpVariant, currentSyncConfig);
}
