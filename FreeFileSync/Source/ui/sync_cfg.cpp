// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "sync_cfg.h"
#include <memory>
#include <wx/wupdlock.h>
#include <wx+/rtl.h>
#include <wx+/no_flicker.h>
#include <wx+/choice_enum.h>
#include <wx+/image_tools.h>
#include <wx+/font_size.h>
#include <wx+/std_button_order.h>
#include <wx+/popup_dlg.h>
#include <wx+/image_resources.h>
#include "gui_generated.h"
#include "on_completion_box.h"
#include "dir_name.h"
#include "../file_hierarchy.h"
#include "../lib/help_provider.h"

#ifdef ZEN_WIN
#include <wx+/mouse_move_dlg.h>
#endif

using namespace zen;
using namespace xmlAccess;


class SyncCfgDialog : public SyncCfgDlgGenerated
{
public:
    SyncCfgDialog(wxWindow* parent,
                  CompareVariant compareVar,
                  SyncConfig&    syncCfg,
                  const wxString& caption,
                  xmlAccess::OnGuiError* handleError, //
                  OnCompletionCfg* onCompletion);     //optional input parameter

private:
    virtual void OnSyncTwoWay(wxCommandEvent& event) { directionCfg.var = DirectionConfig::TWOWAY; updateGui(); }
    virtual void OnSyncMirror(wxCommandEvent& event) { directionCfg.var = DirectionConfig::MIRROR; updateGui(); }
    virtual void OnSyncUpdate(wxCommandEvent& event) { directionCfg.var = DirectionConfig::UPDATE; updateGui(); }
    virtual void OnSyncCustom(wxCommandEvent& event) { directionCfg.var = DirectionConfig::CUSTOM; updateGui(); }

    virtual void OnToggleDetectMovedFiles(wxCommandEvent& event) { directionCfg.detectMovedFiles = !directionCfg.detectMovedFiles; updateGui(); }

    virtual void OnSyncTwoWayDouble(wxMouseEvent& event);
    virtual void OnSyncMirrorDouble(wxMouseEvent& event);
    virtual void OnSyncUpdateDouble(wxMouseEvent& event);
    virtual void OnSyncCustomDouble(wxMouseEvent& event);

    virtual void OnExLeftSideOnly (wxCommandEvent& event);
    virtual void OnExRightSideOnly(wxCommandEvent& event);
    virtual void OnLeftNewer      (wxCommandEvent& event);
    virtual void OnRightNewer     (wxCommandEvent& event);
    virtual void OnDifferent      (wxCommandEvent& event);
    virtual void OnConflict       (wxCommandEvent& event);

    virtual void OnClose (wxCloseEvent&   event) { EndModal(ReturnSyncConfig::BUTTON_CANCEL); }
    virtual void OnCancel(wxCommandEvent& event) { EndModal(ReturnSyncConfig::BUTTON_CANCEL); }
    virtual void OnOkay  (wxCommandEvent& event);

    virtual void OnParameterChange(wxCommandEvent& event) { updateGui(); }

    virtual void OnDeletionPermanent  (wxCommandEvent& event) { handleDeletion = DELETE_PERMANENTLY;   updateGui(); }
    virtual void OnDeletionRecycler   (wxCommandEvent& event) { handleDeletion = DELETE_TO_RECYCLER;   updateGui(); }
    virtual void OnDeletionVersioning (wxCommandEvent& event) { handleDeletion = DELETE_TO_VERSIONING; updateGui(); }

    virtual void OnErrorPopup (wxCommandEvent& event) { onGuiError = ON_GUIERROR_POPUP;  updateGui(); }
    virtual void OnErrorIgnore(wxCommandEvent& event) { onGuiError = ON_GUIERROR_IGNORE; updateGui(); }

    virtual void OnHelpVersioning(wxHyperlinkEvent& event) { displayHelpEntry(L"html/Versioning.html", this); }

    struct Config
    {
        SyncConfig syncCfg;
        xmlAccess::OnGuiError onGuiError;
        Zstring onCompletion;
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
    OnCompletionCfg*       outOptOnCompletion;

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
    if (directionCfg.var != DirectionConfig::TWOWAY) //automatic mode needs no sync-directions
    {
        const DirectionSet dirCfg = extractDirections(directionCfg);

        switch (dirCfg.exLeftSideOnly)
        {
            case SyncDirection::RIGHT:
                buttonLeftOnly->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_create_right")));
                buttonLeftOnly->SetToolTip(getSyncOpDescription(SO_CREATE_NEW_RIGHT));
                break;
            case SyncDirection::LEFT:
                buttonLeftOnly->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_delete_left")));
                buttonLeftOnly->SetToolTip(getSyncOpDescription(SO_DELETE_LEFT));
                break;
            case SyncDirection::NONE:
                buttonLeftOnly->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_none")));
                buttonLeftOnly->SetToolTip(getSyncOpDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.exRightSideOnly)
        {
            case SyncDirection::RIGHT:
                buttonRightOnly->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_delete_right")));
                buttonRightOnly->SetToolTip(getSyncOpDescription(SO_DELETE_RIGHT));
                break;
            case SyncDirection::LEFT:
                buttonRightOnly->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_create_left")));
                buttonRightOnly->SetToolTip(getSyncOpDescription(SO_CREATE_NEW_LEFT));
                break;
            case SyncDirection::NONE:
                buttonRightOnly->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_none")));
                buttonRightOnly->SetToolTip(getSyncOpDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.leftNewer)
        {
            case SyncDirection::RIGHT:
                buttonLeftNewer->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_update_right")));
                buttonLeftNewer->SetToolTip(getSyncOpDescription(SO_OVERWRITE_RIGHT));
                break;
            case SyncDirection::LEFT:
                buttonLeftNewer->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_update_left")));
                buttonLeftNewer->SetToolTip(getSyncOpDescription(SO_OVERWRITE_LEFT));
                break;
            case SyncDirection::NONE:
                buttonLeftNewer->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_none")));
                buttonLeftNewer->SetToolTip(getSyncOpDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.rightNewer)
        {
            case SyncDirection::RIGHT:
                buttonRightNewer->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_update_right")));
                buttonRightNewer->SetToolTip(getSyncOpDescription(SO_OVERWRITE_RIGHT));
                break;
            case SyncDirection::LEFT:
                buttonRightNewer->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_update_left")));
                buttonRightNewer->SetToolTip(getSyncOpDescription(SO_OVERWRITE_LEFT));
                break;
            case SyncDirection::NONE:
                buttonRightNewer->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_none")));
                buttonRightNewer->SetToolTip(getSyncOpDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.different)
        {
            case SyncDirection::RIGHT:
                buttonDifferent->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_update_right")));
                buttonDifferent->SetToolTip(getSyncOpDescription(SO_OVERWRITE_RIGHT));
                break;
            case SyncDirection::LEFT:
                buttonDifferent->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_update_left")));
                buttonDifferent->SetToolTip(getSyncOpDescription(SO_OVERWRITE_LEFT));
                break;
            case SyncDirection::NONE:
                buttonDifferent->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_none")));
                buttonDifferent->SetToolTip(getSyncOpDescription(SO_DO_NOTHING));
                break;
        }

        switch (dirCfg.conflict)
        {
            case SyncDirection::RIGHT:
                buttonConflict->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_update_right")));
                buttonConflict->SetToolTip(getSyncOpDescription(SO_OVERWRITE_RIGHT));
                break;
            case SyncDirection::LEFT:
                buttonConflict->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"so_update_left")));
                buttonConflict->SetToolTip(getSyncOpDescription(SO_OVERWRITE_LEFT));
                break;
            case SyncDirection::NONE:
                buttonConflict->SetBitmapLabel(mirrorIfRtl(getResourceImage(L"cat_conflict"))); //silent dependency to algorithm.cpp::Redetermine!!!
                buttonConflict->SetToolTip(_("Leave as unresolved conflict"));
                break;
        }
    }
}


SyncCfgDialog::SyncCfgDialog(wxWindow* parent,
                             CompareVariant compareVar,
                             SyncConfig&    syncCfg,
                             const wxString& title,
                             xmlAccess::OnGuiError* handleError,
                             OnCompletionCfg* onCompletion) :
    SyncCfgDlgGenerated(parent),
    handleDeletion(DELETE_TO_RECYCLER), //
    onGuiError(ON_GUIERROR_POPUP),      //dummy init
    outSyncCfg(syncCfg),
    outOptOnGuiError(handleError),
    outOptOnCompletion(onCompletion),
    compareVar_(compareVar),
    versioningFolder(*m_panelVersioning, *m_buttonSelectDirVersioning, *m_versioningFolder/*, m_staticTextResolvedPath*/)
{
#ifdef ZEN_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif
    setStandardButtonOrder(*bSizerStdButtons, StdButtons().setAffirmative(m_buttonOK).setCancel(m_buttonCancel));

    SetTitle(title);

    //set icons for this dialog
    m_bitmapLeftOnly  ->SetBitmap(mirrorIfRtl(greyScale(getResourceImage(L"cat_left_only"  ))));
    m_bitmapRightOnly ->SetBitmap(mirrorIfRtl(greyScale(getResourceImage(L"cat_right_only" ))));
    m_bitmapLeftNewer ->SetBitmap(mirrorIfRtl(greyScale(getResourceImage(L"cat_left_newer" ))));
    m_bitmapRightNewer->SetBitmap(mirrorIfRtl(greyScale(getResourceImage(L"cat_right_newer"))));
    m_bitmapDifferent ->SetBitmap(mirrorIfRtl(greyScale(getResourceImage(L"cat_different"  ))));
    m_bitmapConflict  ->SetBitmap(mirrorIfRtl(greyScale(getResourceImage(L"cat_conflict"   ))));
    m_bitmapDatabase  ->SetBitmap(getResourceImage(L"database"));

    m_toggleBtnTwoWay->SetLabel(L"<- " + _("Two way") + L" ->");
    m_toggleBtnMirror->SetLabel(         _("Mirror")  + L" ->>");
    m_toggleBtnUpdate->SetLabel(         _("Update")  + L" ->");

    setRelativeFontSize(*m_toggleBtnTwoWay, 1.25);
    setRelativeFontSize(*m_toggleBtnMirror, 1.25);
    setRelativeFontSize(*m_toggleBtnUpdate, 1.25);
    setRelativeFontSize(*m_toggleBtnCustom, 1.25);

    enumVersioningStyle.
    add(VER_STYLE_REPLACE,       _("Replace"),    _("Move files and replace if existing")).
    add(VER_STYLE_ADD_TIMESTAMP, _("Time stamp"), _("Append a time stamp to each file name"));

    //hide controls for optional parameters
    if (!handleError && !onCompletion) //currently either both or neither are bound!
    {
        bSizerExtraConfig->Show(false);
        Layout();
    }

    if (onCompletion)
        m_comboBoxOnCompletion->initHistory(*onCompletion->history, onCompletion->historyMax);

    Config newCfg = { syncCfg,
                      handleError ?* handleError : ON_GUIERROR_POPUP,
                      onCompletion ?* onCompletion->command : Zstring()
                    };
    setConfig(newCfg);

    GetSizer()->SetSizeHints(this); //~=Fit() + SetMinSize()
    //=> works like a charm for GTK2 with window resizing problems and title bar corruption; e.g. Debian!!!

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

    m_comboBoxOnCompletion->setValue(cfg.onCompletion);

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

                   //get single parameter "version limit" from both checkbox and spin ctrl:
                   //   output.syncCfg.versionCountLimit   = m_checkBoxVersionsLimit->GetValue() ? m_spinCtrlVersionsLimit->GetValue() : -1;

                   output.onGuiError = onGuiError;

    output.onCompletion = m_comboBoxOnCompletion->getValue();
    return output;
}


void SyncCfgDialog::updateGui()
{
#ifdef ZEN_WIN
    wxWindowUpdateLocker dummy(this); //leads to GUI corruption problems on Linux/OS X!
    wxWindowUpdateLocker dummy2(m_panelVersioning);
    wxWindowUpdateLocker dummy3(m_bpButtonLeftOnly);
    wxWindowUpdateLocker dummy4(m_bpButtonRightOnly);
    wxWindowUpdateLocker dummy5(m_bpButtonLeftNewer);
    wxWindowUpdateLocker dummy6(m_bpButtonRightNewer);
    wxWindowUpdateLocker dummy7(m_bpButtonDifferent);
    wxWindowUpdateLocker dummy8(m_bpButtonConflict);
#endif

    const Config cfg = getConfig(); //resolve parameter ownership: some on GUI controls, others member variables

    updateConfigIcons(cfg.syncCfg.directionCfg,
                      m_bpButtonLeftOnly,
                      m_bpButtonRightOnly,
                      m_bpButtonLeftNewer,
                      m_bpButtonRightNewer,
                      m_bpButtonDifferent,
                      m_bpButtonConflict);

    //selecting "detect move files" does not always make sense:
    m_checkBoxDetectMove->Enable(detectMovedFilesSelectable(directionCfg));
    m_checkBoxDetectMove->SetValue(detectMovedFilesEnabled(directionCfg)); //parameter NOT owned by checkbox!

    //display only relevant sync options
    m_bitmapDatabase     ->Show(cfg.syncCfg.directionCfg.var == DirectionConfig::TWOWAY);
    sbSizerSyncDirections->Show(cfg.syncCfg.directionCfg.var != DirectionConfig::TWOWAY);

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

    m_toggleBtnTwoWay->SetValue(false);
    m_toggleBtnMirror->SetValue(false);
    m_toggleBtnUpdate->SetValue(false);
    m_toggleBtnCustom->SetValue(false);

    switch (cfg.syncCfg.directionCfg.var)
    {
        case DirectionConfig::TWOWAY:
            m_toggleBtnTwoWay->SetValue(true);
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
    m_panelVersioning->Show(versioningSelected);

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
    Refresh(); //removes a few artifacts when toggling display of versioning folder

    GetSizer()->SetSizeHints(this); //~=Fit() + SetMinSize()
    //=> works like a charm for GTK2 with window resizing problems and title bar corruption; e.g. Debian!!!
}


void SyncCfgDialog::OnOkay(wxCommandEvent& event)
{
    const Config cfg = getConfig();

    //parameter validation:

    //check if user-defined directory for deletion was specified
    if (cfg.syncCfg.handleDeletion == zen::DELETE_TO_VERSIONING)
    {
        Zstring versioningDir = cfg.syncCfg.versioningDirectory;
        trim(versioningDir);
        if (versioningDir.empty())
        {
            showNotificationDialog(this, DialogInfoType::INFO, PopupDialogCfg().setMainInstructions(_("Please enter a target folder for versioning.")));
            //don't show error icon to follow "Windows' encouraging tone"
            m_panelVersioning->SetFocus();
            return;
        }
    }

    //apply config:
    outSyncCfg = cfg.syncCfg;

    if (outOptOnGuiError)
        *outOptOnGuiError = cfg.onGuiError;

    if (outOptOnCompletion)
    {
        *outOptOnCompletion->command = cfg.onCompletion;
        //a good place to commit current "on completion" history item
        m_comboBoxOnCompletion->addItemHistory();
    }

    EndModal(ReturnSyncConfig::BUTTON_OKAY);
}


void SyncCfgDialog::OnSyncTwoWayDouble(wxMouseEvent& event)
{
    wxCommandEvent dummy;
    OnSyncTwoWay(dummy);
    OnOkay(dummy);
}

void SyncCfgDialog::OnSyncMirrorDouble(wxMouseEvent& event)
{
    wxCommandEvent dummy;
    OnSyncMirror(dummy);
    OnOkay(dummy);
}

void SyncCfgDialog::OnSyncUpdateDouble(wxMouseEvent& event)
{
    wxCommandEvent dummy;
    OnSyncUpdate(dummy);
    OnOkay(dummy);
}

void SyncCfgDialog::OnSyncCustomDouble(wxMouseEvent& event)
{
    wxCommandEvent dummy;
    OnSyncCustom(dummy);
    OnOkay(dummy);
}

namespace
{
void toggleSyncDirection(SyncDirection& current)
{
    switch (current)
    {
        case SyncDirection::RIGHT:
            current = SyncDirection::LEFT;
            break;
        case SyncDirection::LEFT:
            current = SyncDirection::NONE;
            break;
        case SyncDirection::NONE:
            current = SyncDirection::RIGHT;
            break;
    }
}


void pressCustomDir(DirectionConfig& directionCfg, SyncDirection& syncdir)
{
    switch (directionCfg.var)
    {
        case DirectionConfig::TWOWAY:
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
            const DirectionSet setMirror = []
            {
                DirectionConfig mirrorCfg;
                mirrorCfg.var = DirectionConfig::MIRROR;
                return extractDirections(mirrorCfg);
            }();

            const DirectionSet setUpdate = []
            {
                DirectionConfig updateCfg;
                updateCfg.var = DirectionConfig::UPDATE;
                return extractDirections(updateCfg);
            }();

            const DirectionSet currentSet = extractDirections(directionCfg);
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
                                                       const wxString& title,
                                                       xmlAccess::OnGuiError* handleError,    //
                                                       OnCompletionCfg* onCompletion) //optional input parameter
{
    SyncCfgDialog syncDlg(parent,
                          compareVar,
                          syncCfg,
                          title,
                          handleError,
                          onCompletion);
    return static_cast<ReturnSyncConfig::ButtonPressed>(syncDlg.ShowModal());
}