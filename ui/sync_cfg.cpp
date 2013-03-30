// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "sync_cfg.h"
#include <memory>
#include <zen/format_unit.h>
#include <wx/wupdlock.h>
#include <wx+/mouse_move_dlg.h>
#include <wx+/rtl.h>
#include <wx+/no_flicker.h>
#include <wx+/choice_enum.h>
#include <wx+/image_tools.h>
#include <wx+/font_size.h>
#include "gui_generated.h"
#include "exec_finished_box.h"
#include "dir_name.h"
#include "../file_hierarchy.h"
#include "../lib/resources.h"

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
    virtual void OnSyncAutomatic(wxCommandEvent& event) { directionCfg.var = DirectionConfig::AUTOMATIC; updateGui(); }
    virtual void OnSyncMirror   (wxCommandEvent& event) { directionCfg.var = DirectionConfig::MIRROR;    updateGui(); }
    virtual void OnSyncUpdate   (wxCommandEvent& event) { directionCfg.var = DirectionConfig::UPDATE;    updateGui(); }
    virtual void OnSyncCustom   (wxCommandEvent& event) { directionCfg.var = DirectionConfig::CUSTOM;    updateGui(); }

    virtual void OnSyncAutomaticDouble(wxMouseEvent& event);
    virtual void OnSyncMirrorDouble   (wxMouseEvent& event);
    virtual void OnSyncUpdateDouble   (wxMouseEvent& event);
    virtual void OnSyncCustomDouble   (wxMouseEvent& event);

    virtual void OnExLeftSideOnly (wxCommandEvent& event);
    virtual void OnExRightSideOnly(wxCommandEvent& event);
    virtual void OnLeftNewer      (wxCommandEvent& event);
    virtual void OnRightNewer     (wxCommandEvent& event);
    virtual void OnDifferent      (wxCommandEvent& event);
    virtual void OnConflict       (wxCommandEvent& event);

    virtual void OnClose (wxCloseEvent&   event) { EndModal(ReturnSyncConfig::BUTTON_CANCEL); }
    virtual void OnCancel(wxCommandEvent& event) { EndModal(ReturnSyncConfig::BUTTON_CANCEL); }
    virtual void OnApply (wxCommandEvent& event);

    virtual void OnParameterChange(wxCommandEvent& event) { updateGui(); }

    virtual void OnDeletionPermanent  (wxCommandEvent& event) { handleDeletion = DELETE_PERMANENTLY;   updateGui(); }
    virtual void OnDeletionRecycler   (wxCommandEvent& event) { handleDeletion = DELETE_TO_RECYCLER;   updateGui(); }
    virtual void OnDeletionVersioning (wxCommandEvent& event) { handleDeletion = DELETE_TO_VERSIONING; updateGui(); }

    virtual void OnErrorPopup (wxCommandEvent& event) { onGuiError = ON_GUIERROR_POPUP;  updateGui(); }
    virtual void OnErrorIgnore(wxCommandEvent& event) { onGuiError = ON_GUIERROR_IGNORE; updateGui(); }

    struct Config
    {
        SyncConfig syncCfg;
        xmlAccess::OnGuiError onGuiError;
        std::wstring onCompletion;
    };
    void setConfig(const Config& cfg);
    Config getConfig() const;

    void updateGui();

    //parameters with ownership NOT within GUI controls!
    DirectionConfig directionCfg;
    DeletionPolicy handleDeletion; //use Recycler, delete permanently or move to user-defined location
    OnGuiError onGuiError;

    //output data
    SyncConfig&            outSyncCfg;
    xmlAccess::OnGuiError* outOptOnGuiError;
    ExecWhenFinishedCfg*   outOptExecWhenFinished;

    CompareVariant compareVar_;
    DirectoryName<FolderHistoryBox> versioningFolder;

    EnumDescrList<VersioningStyle> enumVersioningStyle;
};


void updateConfigIcons(const DirectionConfig& directionCfg,
                       wxBitmapButton* buttonLeftOnly,
                       wxBitmapButton* buttonRightOnly,
                       wxBitmapButton* buttonLeftNewer,
                       wxBitmapButton* buttonRightNewer,
                       wxBitmapButton* buttonDifferent,
                       wxBitmapButton* buttonConflict)
{
    if (directionCfg.var != DirectionConfig::AUTOMATIC) //automatic mode needs no sync-directions
    {
        const DirectionSet dirCfg = extractDirections(directionCfg);

        switch (dirCfg.exLeftSideOnly)
        {
            case SYNC_DIR_RIGHT:
                buttonLeftOnly->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"createRight")));
                buttonLeftOnly->SetToolTip(getSyncOpDescription(SO_CREATE_NEW_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonLeftOnly->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"deleteLeft")));
                buttonLeftOnly->SetToolTip(getSyncOpDescription(SO_DELETE_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonLeftOnly->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"none")));
                buttonLeftOnly->SetToolTip(getSyncOpDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.exRightSideOnly)
        {
            case SYNC_DIR_RIGHT:
                buttonRightOnly->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"deleteRight")));
                buttonRightOnly->SetToolTip(getSyncOpDescription(SO_DELETE_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonRightOnly->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"createLeft")));
                buttonRightOnly->SetToolTip(getSyncOpDescription(SO_CREATE_NEW_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonRightOnly->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"none")));
                buttonRightOnly->SetToolTip(getSyncOpDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.leftNewer)
        {
            case SYNC_DIR_RIGHT:
                buttonLeftNewer->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"updateRight")));
                buttonLeftNewer->SetToolTip(getSyncOpDescription(SO_OVERWRITE_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonLeftNewer->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"updateLeft")));
                buttonLeftNewer->SetToolTip(getSyncOpDescription(SO_OVERWRITE_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonLeftNewer->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"none")));
                buttonLeftNewer->SetToolTip(getSyncOpDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.rightNewer)
        {
            case SYNC_DIR_RIGHT:
                buttonRightNewer->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"updateRight")));
                buttonRightNewer->SetToolTip(getSyncOpDescription(SO_OVERWRITE_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonRightNewer->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"updateLeft")));
                buttonRightNewer->SetToolTip(getSyncOpDescription(SO_OVERWRITE_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonRightNewer->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"none")));
                buttonRightNewer->SetToolTip(getSyncOpDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.different)
        {
            case SYNC_DIR_RIGHT:
                buttonDifferent->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"updateRight")));
                buttonDifferent->SetToolTip(getSyncOpDescription(SO_OVERWRITE_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonDifferent->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"updateLeft")));
                buttonDifferent->SetToolTip(getSyncOpDescription(SO_OVERWRITE_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonDifferent->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"none")));
                buttonDifferent->SetToolTip(getSyncOpDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.conflict)
        {
            case SYNC_DIR_RIGHT:
                buttonConflict->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"updateRight")));
                buttonConflict->SetToolTip(getSyncOpDescription(SO_OVERWRITE_RIGHT));
                break;
            case SYNC_DIR_LEFT:
                buttonConflict->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"updateLeft")));
                buttonConflict->SetToolTip(getSyncOpDescription(SO_OVERWRITE_LEFT));
                break;
            case SYNC_DIR_NONE:
                buttonConflict->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"conflict"))); //silent dependency to algorithm.cpp::Redetermine!!!
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
    handleDeletion(DELETE_TO_RECYCLER), //
    onGuiError(ON_GUIERROR_POPUP), //dummy init
    outSyncCfg(syncCfg),
    outOptOnGuiError(handleError),
    outOptExecWhenFinished(execWhenFinished),
    compareVar_(compareVar),
    versioningFolder(*m_panelVersioning, *m_buttonSelectDirVersioning, *m_versioningFolder/*, m_staticTextResolvedPath*/)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif
    //set icons for this dialog
    m_bitmapLeftOnly  ->SetBitmap(mirrorIfRtl(greyScale(getResourceImage(L"leftOnly"  ))));
    m_bitmapRightOnly ->SetBitmap(mirrorIfRtl(greyScale(getResourceImage(L"rightOnly" ))));
    m_bitmapLeftNewer ->SetBitmap(mirrorIfRtl(greyScale(getResourceImage(L"leftNewer" ))));
    m_bitmapRightNewer->SetBitmap(mirrorIfRtl(greyScale(getResourceImage(L"rightNewer"))));
    m_bitmapDifferent ->SetBitmap(mirrorIfRtl(greyScale(getResourceImage(L"different" ))));
    m_bitmapConflict  ->SetBitmap(mirrorIfRtl(greyScale(getResourceImage(L"conflict"  ))));
    m_bitmapDatabase  ->SetBitmap(getResourceImage(L"database"));

    setRelativeFontSize(*m_toggleBtnAutomatic, 1.25);
    setRelativeFontSize(*m_toggleBtnMirror,    1.25);
    setRelativeFontSize(*m_toggleBtnUpdate,    1.25);
    setRelativeFontSize(*m_toggleBtnCustom,    1.25);
    setRelativeFontSize(*m_staticTextHeaderCategory, 0.90);
    setRelativeFontSize(*m_staticTextHeaderAction,   0.90);

    enumVersioningStyle.
    add(VER_STYLE_ADD_TIMESTAMP, _("Versioning"), _("Append a timestamp to each file name")).
    add(VER_STYLE_REPLACE,       _("Replace"),    _("Move files and replace if existing"));

    //hide controls for optional parameters
    if (!handleError && !execWhenFinished) //currently either both or neither are bound!
    {
        bSizerExtraConfig->Show(false);
        Layout();
    }

    if (execWhenFinished)
        m_comboBoxExecFinished->initHistory(*execWhenFinished->history, execWhenFinished->historyMax);

    Config newCfg = { syncCfg,
                      handleError ?* handleError : ON_GUIERROR_POPUP,
                      execWhenFinished ?* execWhenFinished->command : std::wstring()
                    };
    setConfig(newCfg);

    Fit();

    m_buttonOK->SetFocus();
}

//#################################################################################################################

void SyncCfgDialog::setConfig(const Config& cfg)
{
    directionCfg   = cfg.syncCfg.directionCfg;  //make working copy; ownership *not* on GUI
    handleDeletion = cfg.syncCfg.handleDeletion;

    versioningFolder.setName(utfCvrtTo<wxString>(cfg.syncCfg.versioningDirectory));
    setEnumVal(enumVersioningStyle, *m_choiceVersioningStyle,  cfg.syncCfg.versioningStyle);

    ////map single parameter "version limit" to both checkbox and spin ctrl:
    //m_checkBoxVersionsLimit->SetValue(cfg.syncCfg.versionCountLimit >= 0);
    //m_spinCtrlVersionsLimit->SetValue(cfg.syncCfg.versionCountLimit >= 0 ? cfg.syncCfg.versionCountLimit : 10 /*SyncConfig().versionCountLimit*/);

    onGuiError = cfg.onGuiError;

    m_comboBoxExecFinished->setValue(cfg.onCompletion);

    updateGui();
}


SyncCfgDialog::Config SyncCfgDialog::getConfig() const
{
    Config output;

    //write configuration to main dialog
    output.syncCfg.directionCfg        = directionCfg;
    output.syncCfg.handleDeletion      = handleDeletion;
    output.syncCfg.versioningDirectory = utfCvrtTo<Zstring>(versioningFolder.getName());
    output.syncCfg.versioningStyle     = getEnumVal(enumVersioningStyle, *m_choiceVersioningStyle),

                   ////get single parameter "version limit" from both checkbox and spin ctrl:
                   //   output.syncCfg.versionCountLimit   = m_checkBoxVersionsLimit->GetValue() ? m_spinCtrlVersionsLimit->GetValue() : -1;

                   output.onGuiError = onGuiError;

    output.onCompletion = m_comboBoxExecFinished->getValue();
    return output;
}


void SyncCfgDialog::updateGui()
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion
    wxWindowUpdateLocker dummy2(m_panelVersioning); //avoid display distortion
    wxWindowUpdateLocker dummy3(m_bpButtonLeftOnly);
    wxWindowUpdateLocker dummy4(m_bpButtonRightOnly);
    wxWindowUpdateLocker dummy5(m_bpButtonLeftNewer);
    wxWindowUpdateLocker dummy6(m_bpButtonRightNewer);
    wxWindowUpdateLocker dummy7(m_bpButtonDifferent);
    wxWindowUpdateLocker dummy8(m_bpButtonConflict);

    const Config cfg = getConfig(); //resolve parameter ownership: some on GUI controls, others member variables

    updateConfigIcons(cfg.syncCfg.directionCfg,
                      m_bpButtonLeftOnly,
                      m_bpButtonRightOnly,
                      m_bpButtonLeftNewer,
                      m_bpButtonRightNewer,
                      m_bpButtonDifferent,
                      m_bpButtonConflict);

    //display only relevant sync options
    m_bitmapDatabase     ->Show(cfg.syncCfg.directionCfg.var == DirectionConfig::AUTOMATIC);
    sbSizerSyncDirections->Show(cfg.syncCfg.directionCfg.var != DirectionConfig::AUTOMATIC);

    switch (compareVar_) //sbSizerSyncDirections->Show resets child sizers!
    {
        case CMP_BY_TIME_SIZE:
            bSizerDifferent ->Show(false);
            break;

        case CMP_BY_CONTENT:
            bSizerLeftNewer ->Show(false);
            bSizerRightNewer->Show(false);
            break;
    }
    bSizerConfig->Layout(); //[!]

    //update toggle buttons -> they have no parameter-ownership at all!
    m_staticTextAutomatic->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    m_staticTextMirror   ->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    m_staticTextUpdate   ->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    m_staticTextCustom   ->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));

    m_toggleBtnAutomatic->SetValue(false);
    m_toggleBtnMirror   ->SetValue(false);
    m_toggleBtnUpdate   ->SetValue(false);
    m_toggleBtnCustom   ->SetValue(false);

    switch (cfg.syncCfg.directionCfg.var)
    {
        case DirectionConfig::AUTOMATIC:
            m_toggleBtnAutomatic->SetValue(true);
            m_staticTextAutomatic->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
            break;
        case DirectionConfig::MIRROR:
            m_toggleBtnMirror->SetValue(true);
            m_staticTextMirror->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
            break;
        case DirectionConfig::UPDATE:
            m_toggleBtnUpdate->SetValue(true);
            m_staticTextUpdate->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
            break;
        case DirectionConfig::CUSTOM:
            m_toggleBtnCustom->SetValue(true);
            m_staticTextCustom->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
            break;
    }

    m_toggleBtnPermanent ->SetValue(false);
    m_toggleBtnRecycler  ->SetValue(false);
    m_toggleBtnVersioning->SetValue(false);
    switch (cfg.syncCfg.handleDeletion)
    {
        case DELETE_PERMANENTLY:
            m_toggleBtnPermanent->SetValue(true);
            break;
        case DELETE_TO_RECYCLER:
            m_toggleBtnRecycler->SetValue(true);
            break;
        case DELETE_TO_VERSIONING:
            m_toggleBtnVersioning->SetValue(true);
            break;
    }

    const bool versioningSelected = cfg.syncCfg.handleDeletion == DELETE_TO_VERSIONING;
    bSizerVersioningNamingConvention->Show(versioningSelected);
    bSizerVersioningStyle           ->Show(versioningSelected);
    m_panelVersioning               ->Show(versioningSelected);

    if (versioningSelected)
    {
        updateTooltipEnumVal(enumVersioningStyle, *m_choiceVersioningStyle);

        const std::wstring pathSep = utfCvrtTo<std::wstring>(FILE_NAME_SEPARATOR);
        switch (cfg.syncCfg.versioningStyle)
        {
            case VER_STYLE_REPLACE:
                setText(*m_staticTextNamingCvtPart1, pathSep + _("Folder") + pathSep + _("File") + L".doc");
                setText(*m_staticTextNamingCvtPart2Bold, L"");
                setText(*m_staticTextNamingCvtPart3, L"");
                break;

            case VER_STYLE_ADD_TIMESTAMP:
                setText(*m_staticTextNamingCvtPart1, pathSep + _("Folder") + pathSep + _("File") + L".doc ");
                setText(*m_staticTextNamingCvtPart2Bold, _("YYYY-MM-DD hhmmss"));
                setText(*m_staticTextNamingCvtPart3, L".doc");
                break;
        }
    }

    //m_spinCtrlVersionsLimit->Enable(m_checkBoxVersionsLimit->GetValue()); //enabled status is *not* directly dependent from resolved config! (but transitively)

    m_toggleBtnErrorIgnore->SetValue(false);
    m_toggleBtnErrorPopup ->SetValue(false);
    switch (cfg.onGuiError)
    {
        case ON_GUIERROR_IGNORE:
            m_toggleBtnErrorIgnore->SetValue(true);
            break;
        case ON_GUIERROR_POPUP:
            m_toggleBtnErrorPopup->SetValue(true);
            break;
    }

    Layout();
    bSizerNamingConvention->Layout(); //[!] not handled by previous "Layout"
    Refresh(); //removes a few artifacts when toggling display of versioning folder
    GetSizer()->SetSizeHints(this); //this works like a charm for GTK2 with window resizing problems!!! (includes call to Fit())
}


void SyncCfgDialog::OnApply(wxCommandEvent& event)
{
    const Config cfg = getConfig();

    //write configuration to main dialog
    outSyncCfg = cfg.syncCfg;

    if (outOptOnGuiError)
        *outOptOnGuiError = cfg.onGuiError;

    if (outOptExecWhenFinished)
    {
        *outOptExecWhenFinished->command = cfg.onCompletion;
        //a good place to commit current "on completion" history item
        m_comboBoxExecFinished->addItemHistory();
    }

    EndModal(ReturnSyncConfig::BUTTON_OKAY);
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

namespace
{
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
}

void SyncCfgDialog::OnExLeftSideOnly(wxCommandEvent& event )
{
    pressCustomDir(directionCfg, directionCfg.custom.exLeftSideOnly);
    updateGui();
}


void SyncCfgDialog::OnExRightSideOnly(wxCommandEvent& event )
{
    pressCustomDir(directionCfg, directionCfg.custom.exRightSideOnly);
    updateGui();
}


void SyncCfgDialog::OnLeftNewer(wxCommandEvent& event )
{
    pressCustomDir(directionCfg, directionCfg.custom.leftNewer);
    updateGui();
}


void SyncCfgDialog::OnRightNewer(wxCommandEvent& event )
{
    pressCustomDir(directionCfg, directionCfg.custom.rightNewer);
    updateGui();
}


void SyncCfgDialog::OnDifferent(wxCommandEvent& event )
{
    pressCustomDir(directionCfg, directionCfg.custom.different);
    updateGui();
}


void SyncCfgDialog::OnConflict(wxCommandEvent& event)
{
    pressCustomDir(directionCfg, directionCfg.custom.conflict);
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
