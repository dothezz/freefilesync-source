#include "smallDialogs.h"
#include "../library/resources.h"
#include "../algorithm.h"
#include "../shared/stringConv.h"
#include "util.h"
#include "../synchronization.h"
#include <wx/msgdlg.h>
#include "../library/customGrid.h"
#include "../shared/customButton.h"
#include "../library/statistics.h"
#include "../shared/localization.h"
#include "../shared/fileHandling.h"
#include "../library/statusHandler.h"
#include <wx/wupdlock.h>
#include "../shared/globalFunctions.h"
#include "trayIcon.h"
#include "../shared/staticAssert.h"
#include "../shared/buildInfo.h"

using namespace FreeFileSync;


AboutDlg::AboutDlg(wxWindow* window) : AboutDlgGenerated(window)
{
    m_bitmap9->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("website")));
    m_bitmap10->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("email")));
    m_bitmap11->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("logo")));
    m_bitmap13->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("gpl")));

    //create language credits
    for (std::vector<LocInfoLine>::const_iterator i = LocalizationInfo::getMapping().begin(); i != LocalizationInfo::getMapping().end(); ++i)
    {
        //flag
        wxStaticBitmap* staticBitmapFlag = new wxStaticBitmap(m_scrolledWindowTranslators, wxID_ANY, GlobalResources::getInstance().getImageByName(i->languageFlag), wxDefaultPosition, wxSize(-1,11), 0 );
        fgSizerTranslators->Add(staticBitmapFlag, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

        //language name
        wxStaticText* staticTextLanguage = new wxStaticText(m_scrolledWindowTranslators, wxID_ANY, i->languageName, wxDefaultPosition, wxDefaultSize, 0 );
        staticTextLanguage->Wrap( -1 );
        fgSizerTranslators->Add(staticTextLanguage, 0, wxALIGN_CENTER_VERTICAL, 5);

        //translator name
        wxStaticText* staticTextTranslator = new wxStaticText(m_scrolledWindowTranslators, wxID_ANY, i->translatorName, wxDefaultPosition, wxDefaultSize, 0 );
        staticTextTranslator->Wrap( -1 );
        fgSizerTranslators->Add(staticTextTranslator, 0, wxALIGN_CENTER_VERTICAL, 5);
    }

    bSizerTranslators->Fit(m_scrolledWindowTranslators);


    //build information
    wxString build = wxString(wxT("(")) + _("Build:") + wxT(" ") + __TDATE__;
#if wxUSE_UNICODE
    build += wxT(" - Unicode");
#else
    build += wxT(" - ANSI");
#endif //wxUSE_UNICODE

    //compile time info about 32/64-bit build
    if (Utility::is64BitBuild)
        build += wxT(" x64)");
    else
        build += wxT(" x86)");
    assert_static(Utility::is32BitBuild || Utility::is64BitBuild);


    m_build->SetLabel(build);

    m_animationControl1->SetAnimation(*GlobalResources::getInstance().animationMoney);
    m_animationControl1->Play();

    m_buttonOkay->SetFocus();
    Fit();
}


void AboutDlg::OnClose(wxCloseEvent& event)
{
    EndModal(0);
}


void AboutDlg::OnOK(wxCommandEvent& event)
{
    EndModal(0);
}
//########################################################################################


HelpDlg::HelpDlg(wxWindow* window) : HelpDlgGenerated(window)
{
    m_notebook1->SetFocus();

    m_bitmap25->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("help")));

    //populate decision trees: "compare by date"
    wxTreeItemId treeRoot      = m_treeCtrl1->AddRoot(_("DECISION TREE"));
    wxTreeItemId treeBothSides = m_treeCtrl1->AppendItem(treeRoot, _("file exists on both sides"));
    wxTreeItemId treeOneSide   = m_treeCtrl1->AppendItem(treeRoot, _("on one side only"));

    m_treeCtrl1->AppendItem(treeOneSide, _("- left"));
    m_treeCtrl1->AppendItem(treeOneSide, _("- right"));

    m_treeCtrl1->AppendItem(treeBothSides, _("- equal"));
    wxTreeItemId treeDifferent = m_treeCtrl1->AppendItem(treeBothSides, _("different"));

    m_treeCtrl1->AppendItem(treeDifferent, _("- left newer"));
    m_treeCtrl1->AppendItem(treeDifferent, _("- right newer"));
    m_treeCtrl1->AppendItem(treeDifferent, _("- conflict (same date, different size)"));

    m_treeCtrl1->ExpandAll();

    //populate decision trees: "compare by content"
    wxTreeItemId tree2Root      = m_treeCtrl2->AddRoot(_("DECISION TREE"));
    wxTreeItemId tree2BothSides = m_treeCtrl2->AppendItem(tree2Root, _("file exists on both sides"));
    wxTreeItemId tree2OneSide   = m_treeCtrl2->AppendItem(tree2Root, _("on one side only"));

    m_treeCtrl2->AppendItem(tree2OneSide, _("- left"));
    m_treeCtrl2->AppendItem(tree2OneSide, _("- right"));

    m_treeCtrl2->AppendItem(tree2BothSides, _("- equal"));
    m_treeCtrl2->AppendItem(tree2BothSides, _("- different"));

    m_treeCtrl2->ExpandAll();
}


void HelpDlg::OnClose(wxCloseEvent& event)
{
    Destroy();
}


void HelpDlg::OnOK(wxCommandEvent& event)
{
    Destroy();
}


//########################################################################################
FilterDlg::FilterDlg(wxWindow* window,
                     bool isGlobalFilter, //global or local filter dialog?
                     Zstring& filterIncl,
                     Zstring& filterExcl,
                     bool filterActive) :
    FilterDlgGenerated(window),
    isGlobalFilter_(isGlobalFilter),
    includeFilter(filterIncl),
    excludeFilter(filterExcl)
{
    m_bitmap8->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("include")));
    m_bitmap9->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("exclude")));
    m_bitmap26->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("filterOn")));
    m_bpButtonHelp->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("help")));

    m_textCtrlInclude->SetValue(zToWx(includeFilter));
    m_textCtrlExclude->SetValue(zToWx(excludeFilter));

    m_panel13->Hide();
    m_button10->SetFocus();

    if (filterActive)
        m_staticTextFilteringInactive->Hide();

    //adapt header for global/local dialog
    if (isGlobalFilter_)
        m_staticTexHeader->SetLabel(_("Global filter"));
    else
        m_staticTexHeader->SetLabel(_("Local filter"));

    Fit();
}


void FilterDlg::OnHelp(wxCommandEvent& event)
{
    m_bpButtonHelp->Hide();
    m_panel13->Show();
    Fit();
    Refresh();

    event.Skip();
}


void FilterDlg::OnDefault(wxCommandEvent& event)
{
    const FilterConfig nullFilter;

    if (isGlobalFilter_)
    {
        m_textCtrlInclude->SetValue(zToWx(nullFilter.includeFilter));
        //exclude various recycle bin directories with global filter
        m_textCtrlExclude->SetValue(zToWx(standardExcludeFilter()));
    }
    else
    {
        m_textCtrlInclude->SetValue(zToWx(nullFilter.includeFilter));
        m_textCtrlExclude->SetValue(zToWx(nullFilter.excludeFilter));
    }

    //changes to mainDialog are only committed when the OK button is pressed
    Fit();
}


void FilterDlg::OnApply(wxCommandEvent& event)
{
    //only if user presses ApplyFilter, he wants the changes to be committed
    includeFilter = wxToZ(m_textCtrlInclude->GetValue());
    excludeFilter = wxToZ(m_textCtrlExclude->GetValue());

    //when leaving dialog: filter and redraw grid, if filter is active
    EndModal(BUTTON_APPLY);
}


void FilterDlg::OnCancel(wxCommandEvent& event)
{
    EndModal(0);
}


void FilterDlg::OnClose(wxCloseEvent& event)
{
    EndModal(0);
}


//########################################################################################
DeleteDialog::DeleteDialog(wxWindow* main,
                           const std::vector<FileSystemObject*>& rowsOnLeft,
                           const std::vector<FileSystemObject*>& rowsOnRight,
                           bool& deleteOnBothSides,
                           bool& useRecycleBin,
                           int& totalDeleteCount) :
    DeleteDlgGenerated(main),
    rowsToDeleteOnLeft(rowsOnLeft),
    rowsToDeleteOnRight(rowsOnRight),
    m_deleteOnBothSides(deleteOnBothSides),
    m_useRecycleBin(useRecycleBin),
    totalDelCount(totalDeleteCount)
{
    m_checkBoxDeleteBothSides->SetValue(deleteOnBothSides);
    m_checkBoxUseRecycler->SetValue(useRecycleBin);
    updateTexts();

    m_buttonOK->SetFocus();
}


void DeleteDialog::updateTexts()
{
    if (m_checkBoxUseRecycler->GetValue())
    {
        m_staticTextHeader->SetLabel(_("Do you really want to move the following objects(s) to the Recycle Bin?"));
        m_bitmap12->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("recycler")));
    }
    else
    {
        m_staticTextHeader->SetLabel(_("Do you really want to delete the following objects(s)?"));
        m_bitmap12->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("deleteFile")));
    }

    const std::pair<wxString, int> delInfo = FreeFileSync::deleteFromGridAndHDPreview(
                rowsToDeleteOnLeft,
                rowsToDeleteOnRight,
                m_checkBoxDeleteBothSides->GetValue());

    const wxString filesToDelete = delInfo.first;
    totalDelCount = delInfo.second;

    m_textCtrlMessage->SetValue(filesToDelete);

    Layout();
}


void DeleteDialog::OnOK(wxCommandEvent& event)
{
    EndModal(BUTTON_OKAY);
}

void DeleteDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(BUTTON_CANCEL);
}

void DeleteDialog::OnClose(wxCloseEvent& event)
{
    EndModal(BUTTON_CANCEL);
}

void DeleteDialog::OnDelOnBothSides(wxCommandEvent& event)
{
    m_deleteOnBothSides = m_checkBoxDeleteBothSides->GetValue();
    updateTexts();
}

void DeleteDialog::OnUseRecycler(wxCommandEvent& event)
{
    if (m_checkBoxUseRecycler->GetValue())
    {
        if (!FreeFileSync::recycleBinExists())
        {
            wxMessageBox(_("Unable to initialize Recycle Bin!"), _("Error") , wxOK | wxICON_ERROR);
            m_checkBoxUseRecycler->SetValue(false);
        }
    }

    m_useRecycleBin = m_checkBoxUseRecycler->GetValue();
    updateTexts();
}
//########################################################################################


ErrorDlg::ErrorDlg(wxWindow* parentWindow, const int activeButtons, const wxString messageText, bool& ignoreNextErrors) :
    ErrorDlgGenerated(parentWindow),
    ignoreErrors(ignoreNextErrors)
{
    m_bitmap10->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("error")));
    m_textCtrl8->SetValue(messageText);
    m_checkBoxIgnoreErrors->SetValue(ignoreNextErrors);

    if (~activeButtons & BUTTON_IGNORE)
    {
        m_buttonIgnore->Hide();
        m_checkBoxIgnoreErrors->Hide();
    }

    if (~activeButtons & BUTTON_RETRY)
        m_buttonRetry->Hide();

    if (~activeButtons & BUTTON_ABORT)
        m_buttonAbort->Hide();

    //set button focus precedence
    if (activeButtons & BUTTON_RETRY)
        m_buttonRetry->SetFocus();
    else if (activeButtons & BUTTON_IGNORE)
        m_buttonIgnore->SetFocus();
    else if (activeButtons & BUTTON_ABORT)
        m_buttonAbort->SetFocus();
}

ErrorDlg::~ErrorDlg() {}


void ErrorDlg::OnClose(wxCloseEvent& event)
{
    ignoreErrors = m_checkBoxIgnoreErrors->GetValue();
    EndModal(BUTTON_ABORT);
}


void ErrorDlg::OnIgnore(wxCommandEvent& event)
{
    ignoreErrors = m_checkBoxIgnoreErrors->GetValue();
    EndModal(BUTTON_IGNORE);
}


void ErrorDlg::OnRetry(wxCommandEvent& event)
{
    ignoreErrors = m_checkBoxIgnoreErrors->GetValue();
    EndModal(BUTTON_RETRY);
}


void ErrorDlg::OnAbort(wxCommandEvent& event)
{
    ignoreErrors = m_checkBoxIgnoreErrors->GetValue();
    EndModal(BUTTON_ABORT);
}
//########################################################################################


WarningDlg::WarningDlg(wxWindow* parentWindow,  int activeButtons, const wxString messageText, bool& dontShowDlgAgain) :
    WarningDlgGenerated(parentWindow),
    dontShowAgain(dontShowDlgAgain)
{
    m_bitmap10->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("warning")));
    m_textCtrl8->SetValue(messageText);
    m_checkBoxDontShowAgain->SetValue(dontShowAgain);

    if (~activeButtons & BUTTON_IGNORE)
    {
        m_buttonIgnore->Hide();
        m_checkBoxDontShowAgain->Hide();
    }

    if (~activeButtons & BUTTON_ABORT)
        m_buttonAbort->Hide();

    //set button focus precedence
    if (activeButtons & BUTTON_IGNORE)
        m_buttonIgnore->SetFocus();
    else if (activeButtons & BUTTON_ABORT)
        m_buttonAbort->SetFocus();
}

WarningDlg::~WarningDlg() {}


void WarningDlg::OnClose(wxCloseEvent& event)
{
    dontShowAgain = m_checkBoxDontShowAgain->GetValue();
    EndModal(BUTTON_ABORT);
}


void WarningDlg::OnIgnore(wxCommandEvent& event)
{
    dontShowAgain = m_checkBoxDontShowAgain->GetValue();
    EndModal(BUTTON_IGNORE);
}


void WarningDlg::OnAbort(wxCommandEvent& event)
{
    dontShowAgain = m_checkBoxDontShowAgain->GetValue();
    EndModal(BUTTON_ABORT);
}

//########################################################################################


QuestionDlg::QuestionDlg(wxWindow* parentWindow, int activeButtons, const wxString messageText, bool* dontShowDlgAgain) :
    QuestionDlgGenerated(parentWindow),
    dontShowAgain(dontShowDlgAgain)
{
    m_bitmap10->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("question")));
    m_textCtrl8->SetValue(messageText);
    if (dontShowAgain)
        m_checkBoxDontAskAgain->SetValue(*dontShowAgain);
    else
        m_checkBoxDontAskAgain->Hide();

    if (~activeButtons & BUTTON_YES)
        m_buttonYes->Hide();

    if (~activeButtons & BUTTON_NO)
    {
        m_buttonNo->Hide();
        m_checkBoxDontAskAgain->Hide();
    }

    if (~activeButtons & BUTTON_CANCEL)
        m_buttonCancel->Hide();

    //set button focus precedence
    if (activeButtons & BUTTON_YES)
        m_buttonYes->SetFocus();
    else if (activeButtons & BUTTON_CANCEL)
        m_buttonCancel->SetFocus();
    else if (activeButtons & BUTTON_NO)
        m_buttonNo->SetFocus();
}


void QuestionDlg::OnClose(wxCloseEvent& event)
{
    if (dontShowAgain)
        *dontShowAgain = m_checkBoxDontAskAgain->GetValue();
    EndModal(BUTTON_CANCEL);
}


void QuestionDlg::OnCancel(wxCommandEvent& event)
{
    if (dontShowAgain)
        *dontShowAgain = m_checkBoxDontAskAgain->GetValue();
    EndModal(BUTTON_CANCEL);
}


void QuestionDlg::OnYes(wxCommandEvent& event)
{
    if (dontShowAgain)
        *dontShowAgain = m_checkBoxDontAskAgain->GetValue();
    EndModal(BUTTON_YES);
}

void QuestionDlg::OnNo(wxCommandEvent& event)
{
    if (dontShowAgain)
        *dontShowAgain = m_checkBoxDontAskAgain->GetValue();
    EndModal(BUTTON_NO);
}

//########################################################################################


CustomizeColsDlg::CustomizeColsDlg(wxWindow* window, xmlAccess::ColumnAttributes& attr, bool& showFileIcons) :
    CustomizeColsDlgGenerated(window),
    output(attr),
    m_showFileIcons(showFileIcons)
{
    m_bpButton29->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("moveUp")));
    m_bpButton30->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("moveDown")));

    xmlAccess::ColumnAttributes columnSettings = attr;

    sort(columnSettings.begin(), columnSettings.end(), xmlAccess::sortByPositionOnly);

    for (xmlAccess::ColumnAttributes::const_iterator i = columnSettings.begin(); i != columnSettings.end(); ++i) //love these iterators!
    {
        m_checkListColumns->Append(CustomGridRim::getTypeName(i->type));
        m_checkListColumns->Check(i - columnSettings.begin(), i->visible);
    }

#ifdef FFS_LINUX //file icons currently supported on Windows only
    m_checkBoxShowFileIcons->Hide();
#endif

    m_checkBoxShowFileIcons->SetValue(m_showFileIcons);

    m_checkListColumns->SetSelection(0);
    Fit();
}


void CustomizeColsDlg::OnOkay(wxCommandEvent& event)
{
    for (int i = 0; i < int(m_checkListColumns->GetCount()); ++i)
    {
        const wxString label = m_checkListColumns->GetString(i);
        for (xmlAccess::ColumnAttributes::iterator j = output.begin(); j != output.end(); ++j)
        {
            if (CustomGridRim::getTypeName(j->type) == label) //not nice but short and no performance issue
            {
                j->position = i;
                j->visible  = m_checkListColumns->IsChecked(i);;
                break;
            }
        }
    }

    m_showFileIcons = m_checkBoxShowFileIcons->GetValue();

    EndModal(BUTTON_OKAY);
}


void CustomizeColsDlg::OnDefault(wxCommandEvent& event)
{
    xmlAccess::ColumnAttributes defaultColumnAttr = CustomGridRim::getDefaultColumnAttributes();

    m_checkListColumns->Clear();
    for (xmlAccess::ColumnAttributes::const_iterator i = defaultColumnAttr.begin(); i != defaultColumnAttr.end(); ++i)
    {
        m_checkListColumns->Append(CustomGridRim::getTypeName(i->type));
        m_checkListColumns->Check(i - defaultColumnAttr.begin(), i->visible);
    }

    m_checkBoxShowFileIcons->SetValue(true);
}


void CustomizeColsDlg::OnCancel(wxCommandEvent& event)
{
    EndModal(0);
}


void CustomizeColsDlg::OnClose(wxCloseEvent& event)
{
    EndModal(0);
}


void CustomizeColsDlg::OnMoveUp(wxCommandEvent& event)
{
    const int pos = m_checkListColumns->GetSelection();
    if (1 <= pos && pos < int(m_checkListColumns->GetCount()))
    {
        const bool checked    = m_checkListColumns->IsChecked(pos);
        const wxString label  = m_checkListColumns->GetString(pos);

        m_checkListColumns->SetString(pos, m_checkListColumns->GetString(pos - 1));
        m_checkListColumns->Check(pos, m_checkListColumns->IsChecked(pos - 1));
        m_checkListColumns->SetString(pos - 1, label);
        m_checkListColumns->Check(pos - 1, checked);
        m_checkListColumns->Select(pos - 1);
    }
}


void CustomizeColsDlg::OnMoveDown(wxCommandEvent& event)
{
    const int pos = m_checkListColumns->GetSelection();
    if (0 <= pos && pos < int(m_checkListColumns->GetCount()) - 1)
    {
        const bool checked    = m_checkListColumns->IsChecked(pos);
        const wxString label  = m_checkListColumns->GetString(pos);

        m_checkListColumns->SetString(pos, m_checkListColumns->GetString(pos + 1));
        m_checkListColumns->Check(pos, m_checkListColumns->IsChecked(pos + 1));
        m_checkListColumns->SetString(pos + 1, label);
        m_checkListColumns->Check(pos + 1, checked);
        m_checkListColumns->Select(pos + 1);
    }
}

//########################################################################################


SyncPreviewDlg::SyncPreviewDlg(wxWindow* parentWindow,
                               const wxString& variantName,
                               const FreeFileSync::SyncStatistics& statistics,
                               bool& dontShowAgain) :
    SyncPreviewDlgGenerated(parentWindow),
    m_dontShowAgain(dontShowAgain)
{
    using FreeFileSync::includeNumberSeparator;
    using globalFunctions::numberToWxString;

    m_buttonStartSync->setBitmapFront(GlobalResources::getInstance().getImageByName(wxT("startSync")));
    m_bitmapCreate->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("create")));
    m_bitmapUpdate->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("update")));
    m_bitmapDelete->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("delete")));
    m_bitmapData->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("data")));

    m_staticTextVariant->SetLabel(variantName);
    m_textCtrlData->SetValue(FreeFileSync::formatFilesizeToShortString(statistics.getDataToProcess()));

    m_textCtrlCreateL->SetValue(includeNumberSeparator(numberToWxString(statistics.getCreate(   true, false))));
    m_textCtrlUpdateL->SetValue(includeNumberSeparator(numberToWxString(statistics.getOverwrite(true, false))));
    m_textCtrlDeleteL->SetValue(includeNumberSeparator(numberToWxString(statistics.getDelete(   true, false))));

    m_textCtrlCreateR->SetValue(includeNumberSeparator(numberToWxString(statistics.getCreate(   false, true))));
    m_textCtrlUpdateR->SetValue(includeNumberSeparator(numberToWxString(statistics.getOverwrite(false, true))));
    m_textCtrlDeleteR->SetValue(includeNumberSeparator(numberToWxString(statistics.getDelete(   false, true))));

    m_checkBoxDontShowAgain->SetValue(dontShowAgain);

    m_buttonStartSync->SetFocus();
    Fit();
}


void SyncPreviewDlg::OnClose(wxCloseEvent& event)
{
    EndModal(BUTTON_CANCEL);
}


void SyncPreviewDlg::OnCancel(wxCommandEvent& event)
{
    EndModal(BUTTON_CANCEL);
}


void SyncPreviewDlg::OnStartSync(wxCommandEvent& event)
{
    m_dontShowAgain = m_checkBoxDontShowAgain->GetValue();
    EndModal(BUTTON_START);
}


//########################################################################################

CompareCfgDialog::CompareCfgDialog(wxWindow* parentWindow, const wxPoint& position, CompareVariant& cmpVar) :
    CmpCfgDlgGenerated(parentWindow),
    m_cmpVar(cmpVar)
{
    //move dialog up so that compare-config button and first config-variant are on same level
    Move(wxPoint(position.x, std::max(0, position.y - (m_buttonTimeSize->GetScreenPosition() - GetScreenPosition()).y)));

    m_bpButtonHelp->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("help")));
    m_bitmapByTime->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("cmpByTime")));
    m_bitmapByContent->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("cmpByContent")));

    switch (cmpVar)
    {
    case CMP_BY_TIME_SIZE:
        m_radioBtnSizeDate->SetValue(true);
        m_buttonContent->SetFocus(); //set focus on the other button
        break;
    case CMP_BY_CONTENT:
        m_radioBtnContent->SetValue(true);
        m_buttonTimeSize->SetFocus(); //set focus on the other button
        break;
    }
    Fit();
}


void CompareCfgDialog::OnClose(wxCloseEvent& event)
{
    EndModal(0);
}


void CompareCfgDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(0);
}


void CompareCfgDialog::OnTimeSize(wxCommandEvent& event)
{
    m_cmpVar = CMP_BY_TIME_SIZE;
    EndModal(BUTTON_OKAY);
}


void CompareCfgDialog::OnContent(wxCommandEvent& event)
{
    m_cmpVar = CMP_BY_CONTENT;
    EndModal(BUTTON_OKAY);
}


void CompareCfgDialog::OnShowHelp(wxCommandEvent& event)
{
    HelpDlg* helpDlg = new HelpDlg(this);
    helpDlg->ShowModal();
}


//########################################################################################
GlobalSettingsDlg::GlobalSettingsDlg(wxWindow* window, xmlAccess::XmlGlobalSettings& globalSettings) :
    GlobalSettingsDlgGenerated(window),
    settings(globalSettings)
{
    m_bitmapSettings->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("settings")));
    m_buttonResetDialogs->setBitmapFront(GlobalResources::getInstance().getImageByName(wxT("warningSmall")), 5);
    m_bpButtonAddRow->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("addFolderPair")));
    m_bpButtonRemoveRow->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("removeFolderPair")));

    m_checkBoxIgnoreOneHour->SetValue(globalSettings.ignoreOneHourDiff);
    m_checkBoxCopyLocked->SetValue(globalSettings.copyLockedFiles);

#ifndef FFS_WIN
    m_staticTextCopyLocked->Hide();
    m_checkBoxCopyLocked->Hide();
#endif

    set(globalSettings.gui.externelApplications);

    const wxString toolTip = wxString(_("Integrate external applications into context menu. The following macros are available:")) + wxT("\n\n") +
                             wxT("%name   \t") + _("- full file or directory name") + wxT("\n") +
                             wxT("%dir        \t") + _("- directory part only") + wxT("\n") +
                             wxT("%nameCo \t") + _("- Other side's counterpart to %name") + wxT("\n") +
                             wxT("%dirCo   \t") + _("- Other side's counterpart to %dir");

    m_gridCustomCommand->GetGridWindow()->SetToolTip(toolTip);
    m_gridCustomCommand->GetGridColLabelWindow()->SetToolTip(toolTip);

    m_buttonOkay->SetFocus();

    Fit();
}


void GlobalSettingsDlg::OnOkay(wxCommandEvent& event)
{
    //write global settings only when okay-button is pressed!
    settings.ignoreOneHourDiff = m_checkBoxIgnoreOneHour->GetValue();
    settings.copyLockedFiles   = m_checkBoxCopyLocked->GetValue();

    settings.gui.externelApplications = getExtApp();

    EndModal(BUTTON_OKAY);
}


void GlobalSettingsDlg::OnResetDialogs(wxCommandEvent& event)
{
    QuestionDlg* messageDlg = new QuestionDlg(this,
            QuestionDlg::BUTTON_YES | QuestionDlg::BUTTON_CANCEL,
            _("Re-enable all hidden dialogs?"));

    if (messageDlg->ShowModal() == QuestionDlg::BUTTON_YES)
        settings.optDialogs.resetDialogs();
}


void GlobalSettingsDlg::OnDefault(wxCommandEvent& event)
{
    xmlAccess::XmlGlobalSettings defaultCfg;

    m_checkBoxIgnoreOneHour->SetValue(defaultCfg.ignoreOneHourDiff);
    m_checkBoxCopyLocked->SetValue(defaultCfg.copyLockedFiles);
    set(defaultCfg.gui.externelApplications);
}


void GlobalSettingsDlg::OnCancel(wxCommandEvent& event)
{
    EndModal(0);
}


void GlobalSettingsDlg::OnClose(wxCloseEvent& event)
{
    EndModal(0);
}


void GlobalSettingsDlg::set(const xmlAccess::ExternalApps& extApp)
{
    const int rowCount = m_gridCustomCommand->GetNumberRows();
    if (rowCount > 0)
        m_gridCustomCommand->DeleteRows(0, rowCount);

    m_gridCustomCommand->AppendRows(static_cast<int>(extApp.size()));
    for (xmlAccess::ExternalApps::const_iterator i = extApp.begin(); i != extApp.end(); ++i)
    {
        const int row = i - extApp.begin();
        m_gridCustomCommand->SetCellValue(row, 0, i->first);  //description
        m_gridCustomCommand->SetCellValue(row, 1, i->second); //commandline
    }
    Fit();
}


xmlAccess::ExternalApps GlobalSettingsDlg::getExtApp()
{
    xmlAccess::ExternalApps output;
    for (int i = 0; i < m_gridCustomCommand->GetNumberRows(); ++i)
        output.push_back(
            std::make_pair(m_gridCustomCommand->GetCellValue(i, 0),   //description
                           m_gridCustomCommand->GetCellValue(i, 1))); //commandline
    return output;
}


void GlobalSettingsDlg::OnAddRow(wxCommandEvent& event)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    const int selectedRow = m_gridCustomCommand->GetGridCursorRow();
    if (0 <= selectedRow && selectedRow < m_gridCustomCommand->GetNumberRows())
        m_gridCustomCommand->InsertRows(selectedRow);
    else
        m_gridCustomCommand->AppendRows();

    Fit();
}


void GlobalSettingsDlg::OnRemoveRow(wxCommandEvent& event)
{
    if (m_gridCustomCommand->GetNumberRows() > 0)
    {
        wxWindowUpdateLocker dummy(this); //avoid display distortion

        const int selectedRow = m_gridCustomCommand->GetGridCursorRow();
        if (0 <= selectedRow && selectedRow < m_gridCustomCommand->GetNumberRows())
            m_gridCustomCommand->DeleteRows(selectedRow);
        else
            m_gridCustomCommand->DeleteRows(m_gridCustomCommand->GetNumberRows() - 1);

        Fit();
    }
}

//########################################################################################

CompareStatus::CompareStatus(wxWindow* parentWindow) :
    CompareStatusGenerated(parentWindow),
    scannedObjects(0),
    totalObjects(0),
    totalData(0),
    currentObjects(0),
    currentData(0),
    scalingFactor(0),
    statistics(NULL),
    lastStatCallSpeed(-1000000), //some big number
    lastStatCallRemTime(-1000000)
{
    init();
}


void CompareStatus::init()
{
    //initialize gauge
    m_gauge2->SetRange(50000);
    m_gauge2->SetValue(0);

    //initially hide status that's relevant for comparing bytewise only
    bSizer42->Hide(sbSizer13);
    m_gauge2->Hide();
    bSizer42->Layout();

    scannedObjects = 0;
    currentStatusText.clear();

    totalObjects   = 0;
    totalData      = 0;
    currentObjects = 0;
    currentData    = 0;
    scalingFactor  = 0;

    statistics.reset();

    timeElapsed.Start(); //measure total time

    updateStatusPanelNow();
}


void CompareStatus::switchToCompareBytewise(int totalObjectsToProcess, wxLongLong totalDataToProcess)
{
    currentData = 0;
    totalData = totalDataToProcess;

    currentObjects = 0;
    totalObjects   = totalObjectsToProcess;

    if (totalData != 0)
        scalingFactor = 50000 / totalData.ToDouble(); //let's normalize to 50000
    else
        scalingFactor = 0;

    //set new statistics handler: 10 seconds "window" for remaining time, 5 seconds for speed
    statistics.reset(new Statistics(totalObjectsToProcess, totalDataToProcess.ToDouble(), 10000, 5000));
    lastStatCallSpeed   = -1000000; //some big number
    lastStatCallRemTime = -1000000;

    //show status for comparing bytewise
    bSizer42->Show(sbSizer13);
    m_gauge2->Show();
    bSizer42->Layout();
}


void CompareStatus::incScannedObjects_NoUpdate(int number)
{
    scannedObjects += number;
}


void CompareStatus::incProcessedCmpData_NoUpdate(int objectsProcessed, wxLongLong dataProcessed)
{
    currentData    +=    dataProcessed;
    currentObjects += objectsProcessed;
}


void CompareStatus::setStatusText_NoUpdate(const Zstring& text)
{
    currentStatusText = text;
}


void CompareStatus::updateStatusPanelNow()
{
    //static RetrieveStatistics statistic;
    //statistic.writeEntry(currentData, currentObjects);
    {
        wxWindowUpdateLocker dummy(this); //reduce display distortion

        bool screenChanged = false; //avoid screen flicker by calling layout() only if necessary

        //remove linebreaks from currentStatusText
        wxString formattedStatusText = zToWx(currentStatusText);
        for (wxString::iterator i = formattedStatusText.begin(); i != formattedStatusText.end(); ++i)
            if (*i == wxChar('\n'))
                *i = wxChar(' ');

        //status texts
        if (m_textCtrlStatus->GetValue() != formattedStatusText && (screenChanged = true)) //avoid screen flicker
            m_textCtrlStatus->SetValue(formattedStatusText);

        //nr of scanned objects
        const wxString scannedObjTmp = globalFunctions::numberToWxString(scannedObjects);
        if (m_staticTextScanned->GetLabel() != scannedObjTmp && (screenChanged = true)) //avoid screen flicker
            m_staticTextScanned->SetLabel(scannedObjTmp);

        //progress indicator for "compare file content"
        m_gauge2->SetValue(int(currentData.ToDouble() * scalingFactor));

        //remaining files left for file comparison
        const wxString filesToCompareTmp = globalFunctions::numberToWxString(totalObjects - currentObjects);
        if (m_staticTextFilesRemaining->GetLabel() != filesToCompareTmp && (screenChanged = true)) //avoid screen flicker
            m_staticTextFilesRemaining->SetLabel(filesToCompareTmp);

        //remaining bytes left for file comparison
        const wxString remainingBytesTmp = FreeFileSync::formatFilesizeToShortString(totalData - currentData);
        if (m_staticTextDataRemaining->GetLabel() != remainingBytesTmp && (screenChanged = true)) //avoid screen flicker
            m_staticTextDataRemaining->SetLabel(remainingBytesTmp);

        if (statistics.get())
        {
            if (timeElapsed.Time() - lastStatCallSpeed >= 500) //call method every 500 ms
            {
                lastStatCallSpeed = timeElapsed.Time();

                statistics->addMeasurement(currentObjects, currentData.ToDouble());

                //current speed
                const wxString speedTmp = statistics->getBytesPerSecond();
                if (m_staticTextSpeed->GetLabel() != speedTmp && (screenChanged = true)) //avoid screen flicker
                    m_staticTextSpeed->SetLabel(speedTmp);

                if (timeElapsed.Time() - lastStatCallRemTime >= 2000) //call method every two seconds only
                {
                    lastStatCallRemTime = timeElapsed.Time();

                    //remaining time
                    const wxString timeRemainingTmp = statistics->getRemainingTime();
                    if (m_staticTextTimeRemaining->GetLabel() != timeRemainingTmp && (screenChanged = true)) //avoid screen flicker
                        m_staticTextTimeRemaining->SetLabel(timeRemainingTmp);
                }
            }
        }

        //time elapsed
        const wxString timeElapsedTmp = (wxTimeSpan::Milliseconds(timeElapsed.Time())).Format();
        if (m_staticTextTimeElapsed->GetLabel() != timeElapsedTmp && (screenChanged = true)) //avoid screen flicker
            m_staticTextTimeElapsed->SetLabel(timeElapsedTmp);

        //do the ui update
        if (screenChanged)
            bSizer42->Layout();
    }
    updateUiNow();
}


//########################################################################################

SyncStatus::SyncStatus(StatusHandler* updater, wxWindow* parentWindow) :
    SyncStatusDlgGenerated(parentWindow,
                           wxID_ANY,
                           parentWindow ? wxEmptyString : _("FreeFileSync - Folder Comparison and Synchronization"),
                           wxDefaultPosition, wxSize(638, 376),
                           parentWindow ?
                           wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT :
                           wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL),
    processStatusHandler(updater),
    mainDialog(parentWindow),
    totalObjects(0),
    totalData(0),
    currentObjects(0),
    currentData(0),
    scalingFactor(0),
    processPaused(false),
    currentStatus(SyncStatus::ABORTED),
    statistics(NULL),
    lastStatCallSpeed(-1000000), //some big number
    lastStatCallRemTime(-1000000)
{
    m_animationControl1->SetAnimation(*GlobalResources::getInstance().animationSync);
    m_animationControl1->Play();

    //initialize gauge
    m_gauge1->SetRange(50000);
    m_gauge1->SetValue(0);

    m_buttonAbort->SetFocus();

    if (mainDialog)    //disable (main) window while this status dialog is shown
        mainDialog->Disable();

    timeElapsed.Start(); //measure total time


    SetIcon(*GlobalResources::getInstance().programIcon); //set application icon

    //register key event
    Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(SyncStatus::OnKeyPressed), NULL, this);

}


SyncStatus::~SyncStatus()
{
    if (mainDialog)
    {
        mainDialog->Enable();
        mainDialog->Raise();
        mainDialog->SetFocus();
    }

    if (minimizedToSysTray.get())
        minimizedToSysTray->keepHidden(); //avoid window flashing shortly before it is destroyed
}


void SyncStatus::OnKeyPressed(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();
    if (keyCode == WXK_ESCAPE)
        Close(); //generate close event: do NOT destroy window unconditionally!

    event.Skip();
}


void SyncStatus::resetGauge(int totalObjectsToProcess, wxLongLong totalDataToProcess)
{
    currentData = 0;
    totalData = totalDataToProcess;

    currentObjects = 0;
    totalObjects   = totalObjectsToProcess;

    if (totalData != 0)
        scalingFactor = 50000 / totalData.ToDouble(); //let's normalize to 50000
    else
        scalingFactor = 0;

    //set new statistics handler: 10 seconds "window" for remaining time, 5 seconds for speed
    statistics.reset(new Statistics(totalObjectsToProcess, totalDataToProcess.ToDouble(), 10000, 5000));
    lastStatCallSpeed   = -1000000; //some big number
    lastStatCallRemTime = -1000000;
}


void SyncStatus::incProgressIndicator_NoUpdate(int objectsProcessed, wxLongLong dataProcessed)
{
    currentData    +=    dataProcessed;
    currentObjects += objectsProcessed;
}


void SyncStatus::setStatusText_NoUpdate(const Zstring& text)
{
    currentStatusText = text;
}


void SyncStatus::updateStatusDialogNow()
{
    //static RetrieveStatistics statistic;
    //statistic.writeEntry(currentData, currentObjects);

    //write status information systray, too, if window is minimized
    if (minimizedToSysTray.get())
        switch (currentStatus)
        {
        case SCANNING:
            minimizedToSysTray->setToolTip(wxString(wxT("FreeFileSync - ")) + wxString(_("Scanning...")));
            //+ wxT(" ") + globalFunctions::numberToWxString(currentObjects));
            break;
        case COMPARING_CONTENT:
            minimizedToSysTray->setToolTip(wxString(wxT("FreeFileSync - ")) + wxString(_("Comparing content...")) + wxT(" ") +
                                           fromatPercentage(currentData, totalData), currentData.ToDouble() * 100 / totalData.ToDouble());
            break;
        case SYNCHRONIZING:
            minimizedToSysTray->setToolTip(wxString(wxT("FreeFileSync - ")) + wxString(_("Synchronizing...")) + wxT(" ") +
                                           fromatPercentage(currentData, totalData), currentData.ToDouble() * 100 / totalData.ToDouble());
            break;
        case ABORTED:
        case FINISHED_WITH_SUCCESS:
        case FINISHED_WITH_ERROR:
        case PAUSE:
            minimizedToSysTray->setToolTip(wxT("FreeFileSync"));
        }

    //write regular status information (if dialog is visible or not)
    {
        wxWindowUpdateLocker dummy(this); //reduce display distortion

        bool screenChanged = false; //avoid screen flicker by calling layout() only if necessary

        //progress indicator
        if (currentStatus == SCANNING)
            m_gauge1->Pulse();
        else
            m_gauge1->SetValue(globalFunctions::round(currentData.ToDouble() * scalingFactor));

        //status text
        const wxString statusTxt = zToWx(currentStatusText);
        if (m_textCtrlInfo->GetValue() != statusTxt && (screenChanged = true)) //avoid screen flicker
            m_textCtrlInfo->SetValue(statusTxt);

        //remaining objects
        const wxString remainingObjTmp = globalFunctions::numberToWxString(totalObjects - currentObjects);
        if (m_staticTextRemainingObj->GetLabel() != remainingObjTmp && (screenChanged = true)) //avoid screen flicker
            m_staticTextRemainingObj->SetLabel(remainingObjTmp);

        //remaining bytes left for copy
        const wxString remainingBytesTmp = FreeFileSync::formatFilesizeToShortString(totalData - currentData);
        if (m_staticTextDataRemaining->GetLabel() != remainingBytesTmp && (screenChanged = true)) //avoid screen flicker
            m_staticTextDataRemaining->SetLabel(remainingBytesTmp);

        if (statistics.get())
        {
            if (timeElapsed.Time() - lastStatCallSpeed >= 500) //call method every 500 ms
            {
                lastStatCallSpeed = timeElapsed.Time();

                statistics->addMeasurement(currentObjects, currentData.ToDouble());

                //current speed
                const wxString speedTmp = statistics->getBytesPerSecond();
                if (m_staticTextSpeed->GetLabel() != speedTmp && (screenChanged = true)) //avoid screen flicker
                    m_staticTextSpeed->SetLabel(speedTmp);

                if (timeElapsed.Time() - lastStatCallRemTime >= 2000) //call method every two seconds only
                {
                    lastStatCallRemTime = timeElapsed.Time();

                    //remaining time
                    const wxString timeRemainingTmp = statistics->getRemainingTime();
                    if (m_staticTextTimeRemaining->GetLabel() != timeRemainingTmp && (screenChanged = true)) //avoid screen flicker
                        m_staticTextTimeRemaining->SetLabel(timeRemainingTmp);
                }
            }
        }

        //time elapsed
        const wxString timeElapsedTmp = wxTimeSpan::Milliseconds(timeElapsed.Time()).Format();
        if (m_staticTextTimeElapsed->GetLabel() != timeElapsedTmp && (screenChanged = true)) //avoid screen flicker
            m_staticTextTimeElapsed->SetLabel(timeElapsedTmp);


        //do the ui update
        if (screenChanged)
        {
            bSizer28->Layout();
            bSizer31->Layout();
        }
    }
    updateUiNow();

//support for pause button
    while (processPaused && currentProcessIsRunning())
    {
        wxMilliSleep(UI_UPDATE_INTERVAL);
        updateUiNow();
    }
}


bool SyncStatus::currentProcessIsRunning()
{
    return processStatusHandler != NULL;
}


void SyncStatus::setCurrentStatus(SyncStatusID id)
{
    switch (id)
    {
    case ABORTED:
        m_bitmapStatus->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("statusError")));
        m_staticTextStatus->SetLabel(_("Aborted"));
        break;

    case FINISHED_WITH_SUCCESS:
        m_bitmapStatus->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("statusSuccess")));
        m_staticTextStatus->SetLabel(_("Completed"));
        break;

    case FINISHED_WITH_ERROR:
        m_bitmapStatus->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("statusWarning")));
        m_staticTextStatus->SetLabel(_("Completed"));
        break;

    case PAUSE:
        m_bitmapStatus->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("statusPause")));
        m_staticTextStatus->SetLabel(_("Paused"));
        break;

    case SCANNING:
        m_bitmapStatus->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("statusScanning")));
        m_staticTextStatus->SetLabel(_("Scanning..."));
        break;

    case COMPARING_CONTENT:
        m_bitmapStatus->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("statusBinaryCompare")));
        m_staticTextStatus->SetLabel(_("Comparing content..."));
        break;

    case SYNCHRONIZING:
        m_bitmapStatus->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("statusSyncing")));
        m_staticTextStatus->SetLabel(_("Synchronizing..."));
        break;
    }

    currentStatus = id;
    Layout();
}


void SyncStatus::processHasFinished(SyncStatusID id, const wxString& finalMessage) //essential to call this in StatusHandler derived class destructor
{
    //at the LATEST(!) to prevent access to currentStatusHandler
    //enable okay and close events; may be set in this method ONLY

    processStatusHandler = NULL; //avoid callback to (maybe) deleted parent process

    setCurrentStatus(id);

    resumeFromSystray(); //if in tray mode...

    m_buttonAbort->Disable();
    m_buttonAbort->Hide();
    m_buttonPause->Disable();
    m_buttonPause->Hide();
    m_buttonOK->Show();
    m_buttonOK->SetFocus();

    m_animationControl1->Stop();
    m_animationControl1->Hide();

    bSizerSpeed->Show(false);
    bSizerRemTime->Show(false);

    updateStatusDialogNow(); //keep this sequence to avoid display distortion, if e.g. only 1 item is sync'ed
    m_textCtrlInfo->SetValue(finalMessage);
    Layout();                //
}


void SyncStatus::OnOkay(wxCommandEvent& event)
{
    if (!currentProcessIsRunning()) Destroy();
}


void SyncStatus::OnPause(wxCommandEvent& event)
{
    static SyncStatusID previousStatus = SyncStatus::ABORTED;

    if (processPaused)
    {
        setCurrentStatus(previousStatus);
        processPaused = false;
        m_buttonPause->SetLabel(_("Pause"));
        m_animationControl1->Play();

        //resume timers
        timeElapsed.Resume();
        if (statistics.get())
            statistics->resumeTimer();
    }
    else
    {
        previousStatus = currentStatus; //save current status

        setCurrentStatus(SyncStatus::PAUSE);
        processPaused = true;
        m_buttonPause->SetLabel(_("Continue"));
        m_animationControl1->Stop();

        //pause timers
        timeElapsed.Pause();
        if (statistics.get())
            statistics->pauseTimer();
    }
}


void SyncStatus::OnAbort(wxCommandEvent& event)
{
    processPaused = false;
    if (currentProcessIsRunning())
    {
        m_buttonAbort->Disable();
        m_buttonAbort->Hide();
        m_buttonPause->Disable();
        m_buttonPause->Hide();

        setStatusText_NoUpdate(wxToZ(_("Abort requested: Waiting for current operation to finish...")));
        //no Layout() or UI-update here to avoid cascaded Yield()-call

        processStatusHandler->requestAbortion();
    }
}


void SyncStatus::OnClose(wxCloseEvent& event)
{
    processPaused = false;
    if (processStatusHandler)
        processStatusHandler->requestAbortion();
    else
        Destroy();
}


void SyncStatus::OnIconize(wxIconizeEvent& event)
{
    if (event.Iconized()) //ATTENTION: iconize event is also triggered on "Restore"! (at least under Linux)
        minimizeToTray();
}


void SyncStatus::minimizeToTray()
{
    minimizedToSysTray.reset(new MinimizeToTray(this, mainDialog));
}


void SyncStatus::resumeFromSystray()
{
    minimizedToSysTray.reset();
}

