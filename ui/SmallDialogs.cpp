#include "smallDialogs.h"
#include "../shared/globalFunctions.h"
#include "../library/resources.h"
#include "../algorithm.h"
#include "../synchronization.h"
#include <wx/msgdlg.h>
#include "../library/customGrid.h"
#include "../shared/customButton.h"
#include "../library/statistics.h"
#include "../shared/localization.h"
#include "../shared/fileHandling.h"
#include "../library/statusHandler.h"
#include <wx/wupdlock.h>

using namespace FreeFileSync;


AboutDlg::AboutDlg(wxWindow* window) : AboutDlgGenerated(window)
{
    m_bitmap9->SetBitmap(*GlobalResources::getInstance().bitmapWebsite);
    m_bitmap10->SetBitmap(*GlobalResources::getInstance().bitmapEmail);
    m_bitmap11->SetBitmap(*GlobalResources::getInstance().bitmapLogo);
    m_bitmap13->SetBitmap(*GlobalResources::getInstance().bitmapGPL);

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
    build += wxT(" - Unicode)");
#else
    build += wxT(" - ANSI)");
#endif //wxUSE_UNICODE
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

    m_bitmap25->SetBitmap(*GlobalResources::getInstance().bitmapHelp);

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


HelpDlg::~HelpDlg() {}


void HelpDlg::OnClose(wxCloseEvent& event)
{
    Destroy();
}


void HelpDlg::OnOK(wxCommandEvent& event)
{
    Destroy();
}
//########################################################################################


FilterDlg::FilterDlg(wxWindow* window, wxString& filterIncl, wxString& filterExcl) :
        FilterDlgGenerated(window),
        includeFilter(filterIncl),
        excludeFilter(filterExcl)
{
    m_bitmap8->SetBitmap(*GlobalResources::getInstance().bitmapInclude);
    m_bitmap9->SetBitmap(*GlobalResources::getInstance().bitmapExclude);
    m_bitmap26->SetBitmap(*GlobalResources::getInstance().bitmapFilter);
    m_bpButtonHelp->SetBitmapLabel(*GlobalResources::getInstance().bitmapHelp);

    m_textCtrlInclude->SetValue(includeFilter);
    m_textCtrlExclude->SetValue(excludeFilter);

    m_panel13->Hide();
    m_button10->SetFocus();

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
    m_textCtrlInclude->SetValue(wxT("*"));
    m_textCtrlExclude->SetValue(wxEmptyString);

    //changes to mainDialog are only committed when the OK button is pressed
    event.Skip();
}


void FilterDlg::OnOK(wxCommandEvent& event)
{
    //only if user presses ApplyFilter, he wants the changes to be committed
    includeFilter = m_textCtrlInclude->GetValue();
    excludeFilter = m_textCtrlExclude->GetValue();

    //when leaving dialog: filter and redraw grid, if filter is active
    EndModal(BUTTON_OKAY);
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
                           const FreeFileSync::FolderComparison& folderCmp,
                           const FreeFileSync::FolderCompRef& rowsOnLeft,
                           const FreeFileSync::FolderCompRef& rowsOnRight,
                           bool& deleteOnBothSides,
                           bool& useRecycleBin,
                           int& totalDeleteCount) :
        DeleteDlgGenerated(main),
        m_folderCmp(folderCmp),
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
    wxString headerText;
    if (m_checkBoxUseRecycler->GetValue())
    {
        headerText = _("Do you really want to move the following objects(s) to the Recycle Bin?");
        m_bitmap12->SetBitmap(*GlobalResources::getInstance().bitmapRecycler);
    }
    else
    {
        headerText = _("Do you really want to delete the following objects(s)?");
        m_bitmap12->SetBitmap(*GlobalResources::getInstance().bitmapDeleteFile);
    }
    m_staticTextHeader->SetLabel(headerText);

    assert(m_folderCmp.size() == rowsToDeleteOnLeft.size());
    assert(m_folderCmp.size() == rowsToDeleteOnRight.size());

    wxString filesToDelete;
    totalDelCount = 0;
    for (FolderComparison::const_iterator j = m_folderCmp.begin(); j != m_folderCmp.end(); ++j)
    {
        const FileComparison& fileCmp = j->fileCmp;

        const int pairIndex = j - m_folderCmp.begin();
        if (    pairIndex < int(rowsToDeleteOnLeft.size()) && //just to be sure
                pairIndex < int(rowsToDeleteOnRight.size()))
        {
            const std::pair<wxString, int> delInfo = FreeFileSync::deleteFromGridAndHDPreview(fileCmp,
                    rowsToDeleteOnLeft[pairIndex],
                    rowsToDeleteOnRight[pairIndex],
                    m_checkBoxDeleteBothSides->GetValue());

            filesToDelete += delInfo.first;
            totalDelCount += delInfo.second;
        }
    }
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
            wxMessageBox(_("It was not possible to initialize the Recycle Bin!\n\nIt's likely that you are not using Windows.\nIf you want this feature included, please contact the author. :)"), _("Error") , wxOK | wxICON_ERROR);
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
    m_bitmap10->SetBitmap(*GlobalResources::getInstance().bitmapError);
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
    m_bitmap10->SetBitmap(*GlobalResources::getInstance().bitmapWarning);
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


QuestionDlg::QuestionDlg(wxWindow* parentWindow, int activeButtons, const wxString messageText, bool& dontShowDlgAgain) :
        QuestionDlgGenerated(parentWindow),
        dontShowAgain(dontShowDlgAgain)
{
    m_bitmap10->SetBitmap(*GlobalResources::getInstance().bitmapQuestion);
    m_textCtrl8->SetValue(messageText);
    m_checkBoxDontAskAgain->SetValue(dontShowAgain);

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
    dontShowAgain = m_checkBoxDontAskAgain->GetValue();
    EndModal(BUTTON_CANCEL);
}


void QuestionDlg::OnCancel(wxCommandEvent& event)
{
    dontShowAgain = m_checkBoxDontAskAgain->GetValue();
    EndModal(BUTTON_CANCEL);
}


void QuestionDlg::OnYes(wxCommandEvent& event)
{
    dontShowAgain = m_checkBoxDontAskAgain->GetValue();
    EndModal(BUTTON_YES);
}

void QuestionDlg::OnNo(wxCommandEvent& event)
{
    dontShowAgain = m_checkBoxDontAskAgain->GetValue();
    EndModal(BUTTON_NO);
}

//########################################################################################


CustomizeColsDlg::CustomizeColsDlg(wxWindow* window, xmlAccess::ColumnAttributes& attr, bool& showFileIcons) :
        CustomizeColsDlgGenerated(window),
        output(attr),
        m_showFileIcons(showFileIcons)
{
    m_bpButton29->SetBitmapLabel(*GlobalResources::getInstance().bitmapMoveUp);
    m_bpButton30->SetBitmapLabel(*GlobalResources::getInstance().bitmapMoveDown);

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

    //m_bitmapPreview->SetBitmap(*GlobalResources::getInstance().bitmapSync);
    m_buttonStartSync->setBitmapFront(*GlobalResources::getInstance().bitmapStartSync);
    m_bitmapCreate->SetBitmap(*GlobalResources::getInstance().bitmapCreate);
    m_bitmapUpdate->SetBitmap(*GlobalResources::getInstance().bitmapUpdate);
    m_bitmapDelete->SetBitmap(*GlobalResources::getInstance().bitmapDelete);
    m_bitmapData->SetBitmap(*GlobalResources::getInstance().bitmapData);

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

CompareCfgDialog::CompareCfgDialog(wxWindow* parentWindow, CompareVariant& cmpVar) :
        CmpCfgDlgGenerated(parentWindow),
        m_cmpVar(cmpVar)
{
    m_bpButtonHelp->SetBitmapLabel(*GlobalResources::getInstance().bitmapHelp);

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
    m_bitmapSettings->SetBitmap(*GlobalResources::getInstance().bitmapSettings);
    m_buttonResetWarnings->setBitmapFront(*GlobalResources::getInstance().bitmapWarningSmall, 5);

    m_spinCtrlFileTimeTolerance->SetValue(globalSettings.fileTimeTolerance);
    m_checkBoxIgnoreOneHour->SetValue(globalSettings.ignoreOneHourDiff);

    m_textCtrlCommand->SetValue(globalSettings.gui.commandLineFileManager);

    const wxString toolTip = wxString(_("This commandline will be executed on each doubleclick. The following macros are available:")) + wxT("\n\n") +
                             wxT("%name   \t") + _("- full file or directory name") + wxT("\n") +
                             wxT("%dir    \t") + _("- directory part only") + wxT("\n") +
                             wxT("%nameCo \t") + _("- sibling of %name") + wxT("\n") +
                             wxT("%dirCo  \t") + _("- sibling of %dir");

    m_staticTextCommand->SetToolTip(toolTip);
    m_textCtrlCommand->SetToolTip(toolTip);

    m_buttonOkay->SetFocus();

    Fit();
}


void GlobalSettingsDlg::OnOkay(wxCommandEvent& event)
{
    //write global settings only when okay-button is pressed!
    settings.fileTimeTolerance = m_spinCtrlFileTimeTolerance->GetValue();
    settings.ignoreOneHourDiff = m_checkBoxIgnoreOneHour->GetValue();

    settings.gui.commandLineFileManager = m_textCtrlCommand->GetValue();

    EndModal(BUTTON_OKAY);
}


void GlobalSettingsDlg::OnResetWarnings(wxCommandEvent& event)
{
    wxMessageDialog* messageDlg = new wxMessageDialog(this, _("Reset all warning messages?"), _("Warning") , wxOK | wxCANCEL);
    if (messageDlg->ShowModal() == wxID_OK)
        settings.warnings.resetWarnings();
}


void GlobalSettingsDlg::OnDefault(wxCommandEvent& event)
{
    m_spinCtrlFileTimeTolerance->SetValue(2);
    m_checkBoxIgnoreOneHour->SetValue(true);
#ifdef FFS_WIN
    m_textCtrlCommand->SetValue(wxT("explorer /select, %name"));
#elif defined FFS_LINUX
    m_textCtrlCommand->SetValue(wxT("konqueror \"%dir\""));
#endif
}


void GlobalSettingsDlg::OnCancel(wxCommandEvent& event)
{
    EndModal(0);
}


void GlobalSettingsDlg::OnClose(wxCloseEvent& event)
{
    EndModal(0);
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
        wxString formattedStatusText = currentStatusText.c_str();
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
        SyncStatusDlgGenerated(parentWindow),
        currentStatusHandler(updater),
        windowToDis(parentWindow),
        currentProcessIsRunning(true),
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

    if (windowToDis)    //disable (main) window while this status dialog is shown
        windowToDis->Disable();

    timeElapsed.Start(); //measure total time
}


SyncStatus::~SyncStatus()
{
    if (windowToDis)
    {
        windowToDis->Enable();
        windowToDis->Raise();
        windowToDis->SetFocus();
    }
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

    {
        wxWindowUpdateLocker dummy(this); //reduce display distortion

        bool screenChanged = false; //avoid screen flicker by calling layout() only if necessary

        //progress indicator
        m_gauge1->SetValue(globalFunctions::round(currentData.ToDouble() * scalingFactor));

        //status text
        if (m_textCtrlInfo->GetValue() != wxString(currentStatusText.c_str()) && (screenChanged = true)) //avoid screen flicker
            m_textCtrlInfo->SetValue(currentStatusText.c_str());

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
        const wxString timeElapsedTmp = (wxTimeSpan::Milliseconds(timeElapsed.Time())).Format();
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
    while (processPaused && currentProcessIsRunning)
    {
        wxMilliSleep(UI_UPDATE_INTERVAL);
        updateUiNow();
    }
}


void SyncStatus::setCurrentStatus(SyncStatusID id)
{
    switch (id)
    {
    case ABORTED:
        m_bitmapStatus->SetBitmap(*GlobalResources::getInstance().bitmapStatusError);
        m_staticTextStatus->SetLabel(_("Aborted"));
        break;

    case FINISHED_WITH_SUCCESS:
        m_bitmapStatus->SetBitmap(*GlobalResources::getInstance().bitmapStatusSuccess);
        m_staticTextStatus->SetLabel(_("Completed"));
        break;

    case FINISHED_WITH_ERROR:
        m_bitmapStatus->SetBitmap(*GlobalResources::getInstance().bitmapStatusWarning);
        m_staticTextStatus->SetLabel(_("Completed"));
        break;

    case PAUSE:
        m_bitmapStatus->SetBitmap(*GlobalResources::getInstance().bitmapStatusPause);
        m_staticTextStatus->SetLabel(_("Paused"));
        break;

    case SCANNING:
        m_bitmapStatus->SetBitmap(*GlobalResources::getInstance().bitmapStatusScanning);
        m_staticTextStatus->SetLabel(_("Scanning..."));
        break;

    case COMPARING_CONTENT:
        m_bitmapStatus->SetBitmap(*GlobalResources::getInstance().bitmapStatusBinCompare);
        m_staticTextStatus->SetLabel(_("Comparing content..."));
        break;

    case SYNCHRONIZING:
        m_bitmapStatus->SetBitmap(*GlobalResources::getInstance().bitmapStatusSyncing);
        m_staticTextStatus->SetLabel(_("Synchronizing..."));
        break;
    }

    currentStatus = id;
    Layout();
}


void SyncStatus::processHasFinished(SyncStatusID id) //essential to call this in StatusHandler derived class destructor
{                                                    //at the LATEST(!) to prevent access to currentStatusHandler
    currentProcessIsRunning = false; //enable okay and close events; may be set in this method ONLY

    setCurrentStatus(id);

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
    Layout();                //
}


void SyncStatus::OnOkay(wxCommandEvent& event)
{
    if (!currentProcessIsRunning) Destroy();
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
    if (currentProcessIsRunning)
    {
        m_buttonAbort->Disable();
        m_buttonAbort->Hide();
        m_buttonPause->Disable();
        m_buttonPause->Hide();

        setStatusText_NoUpdate(_("Abort requested: Waiting for current operation to finish..."));
        //no Layout() or UI-update here to avoid cascaded Yield()-call

        currentStatusHandler->requestAbortion();
    }
}


void SyncStatus::OnClose(wxCloseEvent& event)
{
    processPaused = false;
    if (currentProcessIsRunning)
        currentStatusHandler->requestAbortion();
    else
        Destroy();
}
