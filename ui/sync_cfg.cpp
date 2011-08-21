// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "sync_cfg.h"
#include "../library/resources.h"
#include "../shared/dir_name.h"
#include <wx/wupdlock.h>
#include "../shared/mouse_move_dlg.h"
#include "../shared/string_conv.h"
#include "../shared/dir_picker_i18n.h"
#include "gui_generated.h"
#include <memory>
#include "../shared/wx_choice_enum.h"
#include "../shared/dir_name.h"

using namespace zen;
using namespace xmlAccess;




class SyncCfgDialog : public SyncCfgDlgGenerated
{
public:
    SyncCfgDialog(wxWindow* window,
                  zen::CompareVariant   compareVar,
                  zen::SyncConfig&      syncConfiguration,
                  zen::DeletionPolicy&  handleDeletion,
                  wxString&              customDeletionDirectory,
                  xmlAccess::OnGuiError* handleError); //optional input parameter

    ~SyncCfgDialog();

private:
    virtual void OnSyncAutomatic(   wxCommandEvent& event);
    virtual void OnSyncMirror(      wxCommandEvent& event);
    virtual void OnSyncUpdate(      wxCommandEvent& event);
    virtual void OnSyncCustom(      wxCommandEvent& event);

    virtual void OnSyncAutomaticDouble(   wxMouseEvent& event);
    virtual void OnSyncMirrorDouble(      wxMouseEvent& event);
    virtual void OnSyncUpdateDouble(      wxMouseEvent& event);
    virtual void OnSyncCustomDouble(      wxMouseEvent& event);

    virtual void OnExLeftSideOnly(  wxCommandEvent& event);
    virtual void OnExRightSideOnly( wxCommandEvent& event);
    virtual void OnLeftNewer(       wxCommandEvent& event);
    virtual void OnRightNewer(      wxCommandEvent& event);
    virtual void OnDifferent(       wxCommandEvent& event);
    virtual void OnConflict(        wxCommandEvent& event);

    virtual void OnClose(           wxCloseEvent&   event) { EndModal(0); }
    virtual void OnCancel(          wxCommandEvent& event) { EndModal(0); }
    virtual void OnApply(           wxCommandEvent& event);

    void updateGui();

    void OnChangeErrorHandling(wxCommandEvent& event);
    void OnChangeDeletionHandling(wxCommandEvent& event);

    const zen::CompareVariant cmpVariant;

    //temporal copy of maindialog.cfg.syncConfiguration -> ownership NOT within GUI controls!
    zen::SyncConfig currentSyncConfig;

    //changing data
    zen::SyncConfig&        refSyncConfiguration;
    zen::DeletionPolicy&    refHandleDeletion;
    wxString&                refCustomDeletionDirectory;
    xmlAccess::OnGuiError*   refHandleError;

    zen::DirectoryName customDelFolder;

    zen::EnumDescrList<zen::DeletionPolicy>  enumDelhandDescr;
    zen::EnumDescrList<xmlAccess::OnGuiError> enumErrhandDescr;
};



void updateConfigIcons(const SyncConfig& syncConfig,
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
                       wxStaticBitmap* bitmapConflict) //sizer containing all sync-directions
{
    if (syncConfig.var != SyncConfig::AUTOMATIC) //automatic mode needs no sync-directions
    {
        const DirectionSet dirCfg = extractDirections(syncConfig);

        switch (dirCfg.exLeftSideOnly)
        {
            case SYNC_DIR_RIGHT:
                buttonLeftOnly->SetBitmapLabel(GlobalResources::instance().getImage(wxT("arrowRightCr")));
                buttonLeftOnly->SetToolTip(getDescription(SO_CREATE_NEW_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonLeftOnly->SetBitmapLabel(GlobalResources::instance().getImage(wxT("deleteLeft")));
                buttonLeftOnly->SetToolTip(getDescription(SO_DELETE_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonLeftOnly->SetBitmapLabel(GlobalResources::instance().getImage(wxT("arrowNone")));
                buttonLeftOnly->SetToolTip(getDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.exRightSideOnly)
        {
            case SYNC_DIR_RIGHT:
                buttonRightOnly->SetBitmapLabel(GlobalResources::instance().getImage(wxT("deleteRight")));
                buttonRightOnly->SetToolTip(getDescription(SO_DELETE_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonRightOnly->SetBitmapLabel(GlobalResources::instance().getImage(wxT("arrowLeftCr")));
                buttonRightOnly->SetToolTip(getDescription(SO_CREATE_NEW_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonRightOnly->SetBitmapLabel(GlobalResources::instance().getImage(wxT("arrowNone")));
                buttonRightOnly->SetToolTip(getDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.leftNewer)
        {
            case SYNC_DIR_RIGHT:
                buttonLeftNewer->SetBitmapLabel(GlobalResources::instance().getImage(wxT("arrowRight")));
                buttonLeftNewer->SetToolTip(getDescription(SO_OVERWRITE_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonLeftNewer->SetBitmapLabel(GlobalResources::instance().getImage(wxT("arrowLeft")));
                buttonLeftNewer->SetToolTip(getDescription(SO_OVERWRITE_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonLeftNewer->SetBitmapLabel(GlobalResources::instance().getImage(wxT("arrowNone")));
                buttonLeftNewer->SetToolTip(getDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.rightNewer)
        {
            case SYNC_DIR_RIGHT:
                buttonRightNewer->SetBitmapLabel(GlobalResources::instance().getImage(wxT("arrowRight")));
                buttonRightNewer->SetToolTip(getDescription(SO_OVERWRITE_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonRightNewer->SetBitmapLabel(GlobalResources::instance().getImage(wxT("arrowLeft")));
                buttonRightNewer->SetToolTip(getDescription(SO_OVERWRITE_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonRightNewer->SetBitmapLabel(GlobalResources::instance().getImage(wxT("arrowNone")));
                buttonRightNewer->SetToolTip(getDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.different)
        {
            case SYNC_DIR_RIGHT:
                buttonDifferent->SetBitmapLabel(GlobalResources::instance().getImage(wxT("arrowRight")));
                buttonDifferent->SetToolTip(getDescription(SO_OVERWRITE_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonDifferent->SetBitmapLabel(GlobalResources::instance().getImage(wxT("arrowLeft")));
                buttonDifferent->SetToolTip(getDescription(SO_OVERWRITE_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonDifferent->SetBitmapLabel(GlobalResources::instance().getImage(wxT("arrowNone")));
                buttonDifferent->SetToolTip(getDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.conflict)
        {
            case SYNC_DIR_RIGHT:
                buttonConflict->SetBitmapLabel(GlobalResources::instance().getImage(wxT("arrowRight")));
                buttonConflict->SetToolTip(getDescription(SO_OVERWRITE_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonConflict->SetBitmapLabel(GlobalResources::instance().getImage(wxT("arrowLeft")));
                buttonConflict->SetToolTip(getDescription(SO_OVERWRITE_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonConflict->SetBitmapLabel(GlobalResources::instance().getImage(wxT("conflict")));
                buttonConflict->SetToolTip(_("Leave as unresolved conflict"));
                break;
        }
    }
}


SyncCfgDialog::SyncCfgDialog(wxWindow* window,
                             CompareVariant  compareVar,
                             SyncConfig&     syncConfiguration,
                             DeletionPolicy& handleDeletion,
                             wxString&       customDeletionDirectory,
                             OnGuiError*     handleError) :
    SyncCfgDlgGenerated(window),
    cmpVariant(compareVar),
    currentSyncConfig(syncConfiguration),  //make working copy of syncConfiguration
    refSyncConfiguration(syncConfiguration),
    refHandleDeletion(handleDeletion),
    refCustomDeletionDirectory(customDeletionDirectory),
    refHandleError(handleError),
    customDelFolder(*m_panelCustomDeletionDir, *m_dirPickerCustomDelFolder, *m_textCtrlCustomDelFolder)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    enumDelhandDescr.
    add(DELETE_PERMANENTLY,       _("Delete permanently"), _("Delete or overwrite files permanently")).
    add(MOVE_TO_RECYCLE_BIN,      _("Use Recycle Bin"),    _("Use Recycle Bin when deleting or overwriting files")).
    add(MOVE_TO_CUSTOM_DIRECTORY, _("Versioning"),         _("Move files into a time-stamped subdirectory"));

    enumErrhandDescr.
    add(ON_GUIERROR_POPUP,  _("Show popup"),    _("Show popup on errors or warnings")).
    add(ON_GUIERROR_IGNORE, _("Ignore errors"), _("Hide all error and warning messages"));


    //a proper set-method may be in order some time...
    setEnumVal(enumDelhandDescr, *m_choiceHandleDeletion, handleDeletion);
    customDelFolder.setName(customDeletionDirectory);
    updateGui();

    //error handling
    if (handleError)
        setEnumVal(enumErrhandDescr, *m_choiceHandleError, *handleError);
    else
    {
        sbSizerErrorHandling->Show(false);
        Layout();
    }

    //set sync config icons
    updateGui();

    //set icons for this dialog
    m_bitmapLeftOnly  ->SetBitmap(GlobalResources::instance().getImage(wxT("leftOnly")));
    m_bitmapRightOnly ->SetBitmap(GlobalResources::instance().getImage(wxT("rightOnly")));
    m_bitmapLeftNewer ->SetBitmap(GlobalResources::instance().getImage(wxT("leftNewer")));
    m_bitmapRightNewer->SetBitmap(GlobalResources::instance().getImage(wxT("rightNewer")));
    m_bitmapDifferent ->SetBitmap(GlobalResources::instance().getImage(wxT("different")));
    m_bitmapConflict  ->SetBitmap(GlobalResources::instance().getImage(wxT("conflictGrey")));
    m_bitmapDatabase  ->SetBitmap(GlobalResources::instance().getImage(wxT("database")));

    bSizer201->Layout(); //wxButtonWithImage size might have changed

    m_buttonOK->SetFocus();

    Fit();
}

//#################################################################################################################

SyncCfgDialog::~SyncCfgDialog() {} //non-inline destructor for std::auto_ptr to work with forward declaration


void SyncCfgDialog::updateGui()
{
    //wxWindowUpdateLocker dummy(this); //avoid display distortion
    wxWindowUpdateLocker dummy2(m_panelCustomDeletionDir); //avoid display distortion
    wxWindowUpdateLocker dummy3(m_bpButtonLeftOnly);
    wxWindowUpdateLocker dummy4(m_bpButtonRightOnly);
    wxWindowUpdateLocker dummy5(m_bpButtonLeftNewer);
    wxWindowUpdateLocker dummy6(m_bpButtonRightNewer);
    wxWindowUpdateLocker dummy7(m_bpButtonDifferent);
    wxWindowUpdateLocker dummy8(m_bpButtonConflict);

    updateConfigIcons(currentSyncConfig,
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
                      m_bitmapConflict);

    //display only relevant sync options
    m_bitmapDatabase->Show(true);
    sbSizerSyncDirections->Show(true);

    if (currentSyncConfig.var == SyncConfig::AUTOMATIC)
    {
        sbSizerSyncDirections->Show(false);
    }
    else
    {
        m_bitmapDatabase->Show(false);
        switch (cmpVariant)
        {
            case CMP_BY_TIME_SIZE:
                bSizerDifferent ->Show(false);
                break;

            case CMP_BY_CONTENT:
                bSizerLeftNewer ->Show(false);
                bSizerRightNewer->Show(false);
                break;
        }
    }

    //set radiobuttons -> have no parameter-ownership at all!
    switch (currentSyncConfig.var)
    {
        case SyncConfig::AUTOMATIC:
            m_radioBtnAutomatic->SetValue(true); //automatic mode
            break;
        case SyncConfig::MIRROR:
            m_radioBtnMirror->SetValue(true);    //one way ->
            break;
        case SyncConfig::UPDATE:
            m_radioBtnUpdate->SetValue(true);    //Update ->
            break;
        case SyncConfig::CUSTOM:
            m_radioBtnCustom->SetValue(true);    //custom
            break;
    }

    Layout();
    GetSizer()->SetSizeHints(this); //this works like a charm for GTK2 with window resizing problems!!! (includes call to Fit())

    m_panelCustomDeletionDir->Enable(getEnumVal(enumDelhandDescr, *m_choiceHandleDeletion) == zen::MOVE_TO_CUSTOM_DIRECTORY);
}


void SyncCfgDialog::OnApply(wxCommandEvent& event)
{
    //write configuration to main dialog
    refSyncConfiguration = currentSyncConfig;
    refHandleDeletion    = getEnumVal(enumDelhandDescr, *m_choiceHandleDeletion);

    refCustomDeletionDirectory = customDelFolder.getName();
    if (refHandleError)
        *refHandleError = getEnumVal(enumErrhandDescr, *m_choiceHandleError);

    EndModal(ReturnSyncConfig::BUTTON_OKAY);
}


void SyncCfgDialog::OnChangeErrorHandling(wxCommandEvent& event)
{
    updateTooltipEnumVal(enumErrhandDescr, *m_choiceHandleError);
}


void SyncCfgDialog::OnChangeDeletionHandling(wxCommandEvent& event)
{
    updateTooltipEnumVal(enumDelhandDescr, *m_choiceHandleDeletion);
    updateGui();
}


void SyncCfgDialog::OnSyncAutomatic(wxCommandEvent& event)
{
    currentSyncConfig.var = SyncConfig::AUTOMATIC;
    updateGui();
}


void SyncCfgDialog::OnSyncMirror(wxCommandEvent& event)
{
    currentSyncConfig.var = SyncConfig::MIRROR;
    updateGui();
}


void SyncCfgDialog::OnSyncUpdate(wxCommandEvent& event)
{
    currentSyncConfig.var = SyncConfig::UPDATE;
    updateGui();
}


void SyncCfgDialog::OnSyncCustom(wxCommandEvent& event)
{
    currentSyncConfig.var = SyncConfig::CUSTOM;
    updateGui();
}


void SyncCfgDialog::OnSyncAutomaticDouble(wxMouseEvent& event)
{
    wxCommandEvent dummy;
    OnSyncAutomatic(dummy);
    OnApply(dummy);
}

void SyncCfgDialog::OnSyncMirrorDouble(wxMouseEvent& event)
{
    wxCommandEvent dummy;
    OnSyncMirror(dummy);
    OnApply(dummy);
}

void SyncCfgDialog::OnSyncUpdateDouble(wxMouseEvent& event)
{
    wxCommandEvent dummy;
    OnSyncUpdate(dummy);
    OnApply(dummy);
}

void SyncCfgDialog::OnSyncCustomDouble(wxMouseEvent& event)
{
    wxCommandEvent dummy;
    OnSyncCustom(dummy);
    OnApply(dummy);
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


void pressCustomDir(SyncConfig& syncCfg, SyncDirection& syncdir)
{
    switch (syncCfg.var)
    {
        case SyncConfig::AUTOMATIC:
            assert(false);
            break;
        case SyncConfig::MIRROR:
        case SyncConfig::UPDATE:
            syncCfg.custom = extractDirections(syncCfg);
            syncCfg.var = SyncConfig::CUSTOM;
            toggleSyncDirection(syncdir);
            break;
        case SyncConfig::CUSTOM:
            toggleSyncDirection(syncdir);

            //some config optimization: if custom settings happen to match "mirror" or "update", just switch variant
            DirectionSet currentSet = extractDirections(syncCfg);
            DirectionSet setMirror;
            DirectionSet setUpdate;
            {
                SyncConfig mirrorCfg;
                mirrorCfg.var = SyncConfig::MIRROR;
                setMirror = extractDirections(mirrorCfg);
            }
            {
                SyncConfig updateCfg;
                updateCfg.var = SyncConfig::UPDATE;
                setUpdate = extractDirections(updateCfg);
            }

            if (currentSet == setMirror)
                syncCfg.var = SyncConfig::MIRROR;
            else if (currentSet == setUpdate)
                syncCfg.var = SyncConfig::UPDATE;
            break;
    }
}


void SyncCfgDialog::OnExLeftSideOnly(wxCommandEvent& event )
{
    pressCustomDir(currentSyncConfig, currentSyncConfig.custom.exLeftSideOnly);
    updateGui();
}


void SyncCfgDialog::OnExRightSideOnly(wxCommandEvent& event )
{
    pressCustomDir(currentSyncConfig, currentSyncConfig.custom.exRightSideOnly);
    updateGui();
}


void SyncCfgDialog::OnLeftNewer(wxCommandEvent& event )
{
    pressCustomDir(currentSyncConfig, currentSyncConfig.custom.leftNewer);
    updateGui();
}


void SyncCfgDialog::OnRightNewer(wxCommandEvent& event )
{
    pressCustomDir(currentSyncConfig, currentSyncConfig.custom.rightNewer);
    updateGui();
}


void SyncCfgDialog::OnDifferent(wxCommandEvent& event )
{
    pressCustomDir(currentSyncConfig, currentSyncConfig.custom.different);
    updateGui();
}


void SyncCfgDialog::OnConflict(wxCommandEvent& event)
{
    pressCustomDir(currentSyncConfig, currentSyncConfig.custom.conflict);
    updateGui();
}





ReturnSyncConfig::ButtonPressed zen::showSyncConfigDlg(zen::CompareVariant   compareVar,
                                                       zen::SyncConfig&      syncConfiguration,
                                                       zen::DeletionPolicy&  handleDeletion,
                                                       wxString&              customDeletionDirectory,
                                                       xmlAccess::OnGuiError* handleError) //optional input parameter
{
    SyncCfgDialog syncDlg(NULL,
                          compareVar,
                          syncConfiguration,
                          handleDeletion,
                          customDeletionDirectory,
                          handleError);

    return static_cast<ReturnSyncConfig::ButtonPressed>(syncDlg.ShowModal());
}

