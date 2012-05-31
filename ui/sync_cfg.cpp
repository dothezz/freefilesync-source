// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "sync_cfg.h"
#include <memory>
#include "../lib/resources.h"
#include "dir_name.h"
#include <wx/wupdlock.h>
#include <wx+/mouse_move_dlg.h>
#include <wx+/string_conv.h>
#include <wx+/dir_picker.h>
#include <wx+/rtl.h>
#include "gui_generated.h"
#include <wx+/choice_enum.h>
#include <wx+/image_tools.h>
#include "../file_hierarchy.h"
#include "exec_finished_box.h"

using namespace zen;
using namespace xmlAccess;


class SyncCfgDialog : public SyncCfgDlgGenerated
{
public:
    SyncCfgDialog(wxWindow* parent,
                  CompareVariant compareVar,
                  SyncConfig&    syncCfg,
                  xmlAccess::OnGuiError* handleError,     //
                  ExecWhenFinishedCfg* execWhenFinished); //optional input parameter

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

    virtual void OnClose (wxCloseEvent&   event) { EndModal(ReturnSyncConfig::BUTTON_CANCEL); }
    virtual void OnCancel(wxCommandEvent& event) { EndModal(ReturnSyncConfig::BUTTON_CANCEL); }
    virtual void OnApply (wxCommandEvent& event);

    void updateGui();

    void OnChangeErrorHandling(wxCommandEvent& event);
    void OnChangeDeletionHandling(wxCommandEvent& event);

    const zen::CompareVariant cmpVariant;

    //temporal copy of maindialog.cfg.directionCfg -> ownership NOT within GUI controls!
    DirectionConfig currentDirectionCfg;

    //changing data
    SyncConfig&            syncCfgOut;
    xmlAccess::OnGuiError* refHandleError;
    ExecWhenFinishedCfg*   refExecWhenFinished;

    DirectoryName<FolderHistoryBox> customDelFolder;

    EnumDescrList<zen::DeletionPolicy>  enumDelhandDescr;
    EnumDescrList<xmlAccess::OnGuiError> enumErrhandDescr;
};



void updateConfigIcons(const DirectionConfig& directionCfg,
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
    if (directionCfg.var != DirectionConfig::AUTOMATIC) //automatic mode needs no sync-directions
    {
        const DirectionSet dirCfg = extractDirections(directionCfg);

        switch (dirCfg.exLeftSideOnly)
        {
            case SYNC_DIR_RIGHT:
                buttonLeftOnly->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"createRight")));
                buttonLeftOnly->SetToolTip(getSyncOpDescription(SO_CREATE_NEW_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonLeftOnly->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"deleteLeft")));
                buttonLeftOnly->SetToolTip(getSyncOpDescription(SO_DELETE_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonLeftOnly->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"none")));
                buttonLeftOnly->SetToolTip(getSyncOpDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.exRightSideOnly)
        {
            case SYNC_DIR_RIGHT:
                buttonRightOnly->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"deleteRight")));
                buttonRightOnly->SetToolTip(getSyncOpDescription(SO_DELETE_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonRightOnly->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"createLeft")));
                buttonRightOnly->SetToolTip(getSyncOpDescription(SO_CREATE_NEW_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonRightOnly->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"none")));
                buttonRightOnly->SetToolTip(getSyncOpDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.leftNewer)
        {
            case SYNC_DIR_RIGHT:
                buttonLeftNewer->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"updateRight")));
                buttonLeftNewer->SetToolTip(getSyncOpDescription(SO_OVERWRITE_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonLeftNewer->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"updateLeft")));
                buttonLeftNewer->SetToolTip(getSyncOpDescription(SO_OVERWRITE_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonLeftNewer->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"none")));
                buttonLeftNewer->SetToolTip(getSyncOpDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.rightNewer)
        {
            case SYNC_DIR_RIGHT:
                buttonRightNewer->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"updateRight")));
                buttonRightNewer->SetToolTip(getSyncOpDescription(SO_OVERWRITE_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonRightNewer->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"updateLeft")));
                buttonRightNewer->SetToolTip(getSyncOpDescription(SO_OVERWRITE_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonRightNewer->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"none")));
                buttonRightNewer->SetToolTip(getSyncOpDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.different)
        {
            case SYNC_DIR_RIGHT:
                buttonDifferent->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"updateRight")));
                buttonDifferent->SetToolTip(getSyncOpDescription(SO_OVERWRITE_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonDifferent->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"updateLeft")));
                buttonDifferent->SetToolTip(getSyncOpDescription(SO_OVERWRITE_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonDifferent->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"none")));
                buttonDifferent->SetToolTip(getSyncOpDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.conflict)
        {
            case SYNC_DIR_RIGHT:
                buttonConflict->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"updateRight")));
                buttonConflict->SetToolTip(getSyncOpDescription(SO_OVERWRITE_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonConflict->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"updateLeft")));
                buttonConflict->SetToolTip(getSyncOpDescription(SO_OVERWRITE_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonConflict->SetBitmapLabel(mirrorIfRtl(GlobalResources::getImage(L"conflict")));
                buttonConflict->SetToolTip(_("Leave as unresolved conflict"));
                break;
        }
    }
}


SyncCfgDialog::SyncCfgDialog(wxWindow* parent,
                             CompareVariant compareVar,
                             SyncConfig&    syncCfg,
                             xmlAccess::OnGuiError* handleError,
                             ExecWhenFinishedCfg* execWhenFinished) :
    SyncCfgDlgGenerated(parent),
    cmpVariant(compareVar),
    currentDirectionCfg(syncCfg.directionCfg),  //make working copy
    syncCfgOut(syncCfg),
    refHandleError(handleError),
    refExecWhenFinished(execWhenFinished),
    customDelFolder(*m_panelCustomDeletionDir, *m_dirPickerCustomDelFolder, *m_customDelFolder)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    enumDelhandDescr.
    add(DELETE_PERMANENTLY,       _("Delete permanently"), _("Delete or overwrite files permanently")).
    add(MOVE_TO_RECYCLE_BIN,      _("Use Recycle Bin"),    _("Use Recycle Bin when deleting or overwriting files")).
    add(MOVE_TO_CUSTOM_DIRECTORY, _("Versioning"),         _("Move files into a time-stamped subfolder"));

    enumErrhandDescr.
    add(ON_GUIERROR_POPUP,  _("Show pop-up"),    _("Show pop-up on errors or warnings")).
    add(ON_GUIERROR_IGNORE, _("Ignore errors"), _("Hide all error and warning messages"));


    //a proper set-method may be in order some time...
    setEnumVal(enumDelhandDescr, *m_choiceHandleDeletion, syncCfg.handleDeletion);
    customDelFolder.setName(toWx(syncCfg.customDeletionDirectory));
    updateGui();

    //error handling
    if (handleError)
        setEnumVal(enumErrhandDescr, *m_choiceHandleError, *handleError);
    else
    {
        sbSizerErrorHandling->Show(false);
        Layout();
    }

    if (execWhenFinished)
    {
        m_comboBoxExecFinished->setHistoryRef(*execWhenFinished->history);
        m_comboBoxExecFinished->setValue(*execWhenFinished->command);
    }
    else
    {
        sbSizerExecFinished->Show(false);
        Layout();
    }

    //set sync config icons
    updateGui();

    //set icons for this dialog
    m_bitmapLeftOnly  ->SetBitmap(mirrorIfRtl(greyScale(GlobalResources::getImage(L"leftOnly"  ))));
    m_bitmapRightOnly ->SetBitmap(mirrorIfRtl(greyScale(GlobalResources::getImage(L"rightOnly" ))));
    m_bitmapLeftNewer ->SetBitmap(mirrorIfRtl(greyScale(GlobalResources::getImage(L"leftNewer" ))));
    m_bitmapRightNewer->SetBitmap(mirrorIfRtl(greyScale(GlobalResources::getImage(L"rightNewer"))));
    m_bitmapDifferent ->SetBitmap(mirrorIfRtl(greyScale(GlobalResources::getImage(L"different" ))));
    m_bitmapConflict  ->SetBitmap(mirrorIfRtl(greyScale(GlobalResources::getImage(L"conflict"  ))));
    m_bitmapDatabase  ->SetBitmap(GlobalResources::getImage(wxT("database")));

    bSizer201->Layout(); //wxButtonWithImage size might have changed

    m_buttonOK->SetFocus();

    Fit();
}
//#################################################################################################################


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

    updateConfigIcons(currentDirectionCfg,
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

    if (currentDirectionCfg.var == DirectionConfig::AUTOMATIC)
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
    m_staticTextAutomatic->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    m_staticTextMirror   ->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    m_staticTextUpdate   ->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    m_staticTextCustom   ->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    switch (currentDirectionCfg.var)
    {
        case DirectionConfig::AUTOMATIC:
            m_radioBtnAutomatic->SetValue(true); //automatic mode
            m_staticTextAutomatic->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
            break;
        case DirectionConfig::MIRROR:
            m_radioBtnMirror->SetValue(true);    //one way ->
            m_staticTextMirror->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
            break;
        case DirectionConfig::UPDATE:
            m_radioBtnUpdate->SetValue(true);    //Update ->
            m_staticTextUpdate->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
            break;
        case DirectionConfig::CUSTOM:
            m_radioBtnCustom->SetValue(true);    //custom
            m_staticTextCustom->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
            break;
    }

    Layout();
    GetSizer()->SetSizeHints(this); //this works like a charm for GTK2 with window resizing problems!!! (includes call to Fit())

    m_panelCustomDeletionDir->Enable(getEnumVal(enumDelhandDescr, *m_choiceHandleDeletion) == zen::MOVE_TO_CUSTOM_DIRECTORY);
}


void SyncCfgDialog::OnApply(wxCommandEvent& event)
{
    //write configuration to main dialog
    syncCfgOut.directionCfg            = currentDirectionCfg;
    syncCfgOut.handleDeletion          = getEnumVal(enumDelhandDescr, *m_choiceHandleDeletion);
    syncCfgOut.customDeletionDirectory = toZ(customDelFolder.getName());

    if (refHandleError)
        *refHandleError = getEnumVal(enumErrhandDescr, *m_choiceHandleError);

    if (refExecWhenFinished)
    {
        *refExecWhenFinished->command = m_comboBoxExecFinished->getValue();
        addValueToHistory(*refExecWhenFinished->command, *refExecWhenFinished->history, refExecWhenFinished->historyMax);
    }

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
    currentDirectionCfg.var = DirectionConfig::AUTOMATIC;
    updateGui();
}


void SyncCfgDialog::OnSyncMirror(wxCommandEvent& event)
{
    currentDirectionCfg.var = DirectionConfig::MIRROR;
    updateGui();
}


void SyncCfgDialog::OnSyncUpdate(wxCommandEvent& event)
{
    currentDirectionCfg.var = DirectionConfig::UPDATE;
    updateGui();
}


void SyncCfgDialog::OnSyncCustom(wxCommandEvent& event)
{
    currentDirectionCfg.var = DirectionConfig::CUSTOM;
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


void pressCustomDir(DirectionConfig& directionCfg, SyncDirection& syncdir)
{
    switch (directionCfg.var)
    {
        case DirectionConfig::AUTOMATIC:
            assert(false);
            break;
        case DirectionConfig::MIRROR:
        case DirectionConfig::UPDATE:
            directionCfg.custom = extractDirections(directionCfg);
            directionCfg.var = DirectionConfig::CUSTOM;
            toggleSyncDirection(syncdir);
            break;
        case DirectionConfig::CUSTOM:
            toggleSyncDirection(syncdir);

            //some config optimization: if custom settings happen to match "mirror" or "update", just switch variant
            DirectionSet currentSet = extractDirections(directionCfg);
            DirectionSet setMirror;
            DirectionSet setUpdate;
            {
                DirectionConfig mirrorCfg;
                mirrorCfg.var = DirectionConfig::MIRROR;
                setMirror = extractDirections(mirrorCfg);
            }
            {
                DirectionConfig updateCfg;
                updateCfg.var = DirectionConfig::UPDATE;
                setUpdate = extractDirections(updateCfg);
            }

            if (currentSet == setMirror)
                directionCfg.var = DirectionConfig::MIRROR;
            else if (currentSet == setUpdate)
                directionCfg.var = DirectionConfig::UPDATE;
            break;
    }
}


void SyncCfgDialog::OnExLeftSideOnly(wxCommandEvent& event )
{
    pressCustomDir(currentDirectionCfg, currentDirectionCfg.custom.exLeftSideOnly);
    updateGui();
}


void SyncCfgDialog::OnExRightSideOnly(wxCommandEvent& event )
{
    pressCustomDir(currentDirectionCfg, currentDirectionCfg.custom.exRightSideOnly);
    updateGui();
}


void SyncCfgDialog::OnLeftNewer(wxCommandEvent& event )
{
    pressCustomDir(currentDirectionCfg, currentDirectionCfg.custom.leftNewer);
    updateGui();
}


void SyncCfgDialog::OnRightNewer(wxCommandEvent& event )
{
    pressCustomDir(currentDirectionCfg, currentDirectionCfg.custom.rightNewer);
    updateGui();
}


void SyncCfgDialog::OnDifferent(wxCommandEvent& event )
{
    pressCustomDir(currentDirectionCfg, currentDirectionCfg.custom.different);
    updateGui();
}


void SyncCfgDialog::OnConflict(wxCommandEvent& event)
{
    pressCustomDir(currentDirectionCfg, currentDirectionCfg.custom.conflict);
    updateGui();
}


ReturnSyncConfig::ButtonPressed zen::showSyncConfigDlg(wxWindow* parent,
                                                       CompareVariant compareVar,
                                                       SyncConfig&    syncCfg,
                                                       xmlAccess::OnGuiError* handleError,    //
                                                       ExecWhenFinishedCfg* execWhenFinished) //optional input parameter
{
    SyncCfgDialog syncDlg(parent,
                          compareVar,
                          syncCfg,
                          handleError,
                          execWhenFinished);

    return static_cast<ReturnSyncConfig::ButtonPressed>(syncDlg.ShowModal());
}

