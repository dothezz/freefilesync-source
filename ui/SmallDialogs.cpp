#include "smallDialogs.h"
#include "../library/globalFunctions.h"
#include "../library/resources.h"
#include "../algorithm.h"
#include <wx/msgdlg.h>
#include "../library/customGrid.h"
#include "../library/customButton.h"
#include "../library/statistics.h"

using namespace FreeFileSync;


AboutDlg::AboutDlg(wxWindow* window) : AboutDlgGenerated(window)
{
    m_bitmap9->SetBitmap(*globalResource.bitmapWebsite);
    m_bitmap10->SetBitmap(*globalResource.bitmapEmail);
    m_bitmap11->SetBitmap(*globalResource.bitmapLogo);
    m_bitmap13->SetBitmap(*globalResource.bitmapGPL);

    //flag bitmaps
    m_bitmapFrench          ->SetBitmap(*globalResource.bitmapFrance);
    m_bitmapJapanese        ->SetBitmap(*globalResource.bitmapJapan);
    m_bitmapDutch           ->SetBitmap(*globalResource.bitmapHolland);
    m_bitmapChineseSimple   ->SetBitmap(*globalResource.bitmapChina);
    m_bitmapPolish          ->SetBitmap(*globalResource.bitmapPoland);
    m_bitmapPortuguese      ->SetBitmap(*globalResource.bitmapPortugal);
    m_bitmapItalian         ->SetBitmap(*globalResource.bitmapItaly);
    m_bitmapSlovenian       ->SetBitmap(*globalResource.bitmapSlovakia);
    m_bitmapHungarian       ->SetBitmap(*globalResource.bitmapHungary);
    m_bitmapSpanish         ->SetBitmap(*globalResource.bitmapSpain);
    m_bitmapPortugueseBrazil->SetBitmap(*globalResource.bitmapBrazil);

    //build information
    wxString build = wxString(wxT("(")) + _("Build:") + wxT(" ") + __TDATE__;
#if wxUSE_UNICODE
    build+= wxT(" - Unicode)");
#else
    build+= wxT(" - ANSI)");
#endif //wxUSE_UNICODE
    m_build->SetLabel(build);

    m_animationControl1->SetAnimation(*globalResource.animationMoney);
    m_animationControl1->Play(); //Note: The animation is created hidden(!) to not disturb constraint based window creation;
    m_animationControl1->Show(); //

    m_button8->SetFocus();
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

    m_bitmap25->SetBitmap(*globalResource.bitmapHelp);

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
    m_treeCtrl1->AppendItem(treeDifferent, _("- same date (different size)"));

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
    m_bitmap8->SetBitmap(*globalResource.bitmapInclude);
    m_bitmap9->SetBitmap(*globalResource.bitmapExclude);
    m_bitmap26->SetBitmap(*globalResource.bitmapFilter);
    m_bpButtonHelp->SetBitmapLabel(*globalResource.bitmapHelp);

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
                           bool& useRecycleBin) :
        DeleteDlgGenerated(main),
        m_folderCmp(folderCmp),
        rowsToDeleteOnLeft(rowsOnLeft),
        rowsToDeleteOnRight(rowsOnRight),
        m_deleteOnBothSides(deleteOnBothSides),
        m_useRecycleBin(useRecycleBin)
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
        m_bitmap12->SetBitmap(*globalResource.bitmapRecycler);
    }
    else
    {
        headerText = _("Do you really want to delete the following objects(s)?");
        m_bitmap12->SetBitmap(*globalResource.bitmapDeleteFile);
    }
    m_staticTextHeader->SetLabel(headerText);

    assert(m_folderCmp.size() == rowsToDeleteOnLeft.size());
    assert(m_folderCmp.size() == rowsToDeleteOnRight.size());

    wxString filesToDelete;
    for (FolderComparison::const_iterator j = m_folderCmp.begin(); j != m_folderCmp.end(); ++j)
    {
        const FileComparison& fileCmp = j->fileCmp;

        const int pairIndex = j - m_folderCmp.begin();
        if (    pairIndex < int(rowsToDeleteOnLeft.size()) && //just to be sure
                pairIndex < int(rowsToDeleteOnRight.size()))
        {
            filesToDelete += FreeFileSync::deleteFromGridAndHDPreview(fileCmp,
                             rowsToDeleteOnLeft[pairIndex],
                             rowsToDeleteOnRight[pairIndex],
                             m_checkBoxDeleteBothSides->GetValue());
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
    m_bitmap10->SetBitmap(*globalResource.bitmapError);
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
    m_bitmap10->SetBitmap(*globalResource.bitmapWarning);
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
    m_bitmap10->SetBitmap(*globalResource.bitmapQuestion);
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


QuestionDlg::~QuestionDlg() {}


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


CustomizeColsDlg::CustomizeColsDlg(wxWindow* window, xmlAccess::ColumnAttributes& attr) :
        CustomizeColsDlgGenerated(window),
        output(attr)
{
    m_bpButton29->SetBitmapLabel(*globalResource.bitmapMoveUp);
    m_bpButton30->SetBitmapLabel(*globalResource.bitmapMoveDown);

    xmlAccess::ColumnAttributes columnSettings = attr;

    sort(columnSettings.begin(), columnSettings.end(), xmlAccess::sortByPositionOnly);

    for (xmlAccess::ColumnAttributes::const_iterator i = columnSettings.begin(); i != columnSettings.end(); ++i) //love these iterators!
    {
        m_checkListColumns->Append(CustomGridRim::getTypeName(i->type));
        m_checkListColumns->Check(i - columnSettings.begin(), i->visible);
    }

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
GlobalSettingsDlg::GlobalSettingsDlg(wxWindow* window, xmlAccess::XmlGlobalSettings& globalSettings) :
        GlobalSettingsDlgGenerated(window),
        settings(globalSettings)
{
    m_bitmapSettings->SetBitmap(*globalResource.bitmapSettings);
    m_buttonResetWarnings->setBitmapFront(*globalResource.bitmapWarningSmall, 5);

    m_spinCtrlFileTimeTolerance->SetValue(globalSettings.shared.fileTimeTolerance);
    m_textCtrlFileManager->SetValue(globalSettings.gui.commandLineFileManager);

    Fit();
}


void GlobalSettingsDlg::OnOkay(wxCommandEvent& event)
{
    //write global settings only when okay-button is pressed!

    settings.shared.fileTimeTolerance = m_spinCtrlFileTimeTolerance->GetValue();
    settings.gui.commandLineFileManager = m_textCtrlFileManager->GetValue();

    EndModal(BUTTON_OKAY);
}


void GlobalSettingsDlg::OnResetWarnings(wxCommandEvent& event)
{
    wxMessageDialog* messageDlg = new wxMessageDialog(this, _("Reset all warning messages?"), _("Warning") , wxOK | wxCANCEL);
    if (messageDlg->ShowModal() == wxID_OK)
        settings.shared.resetWarnings();
}


void GlobalSettingsDlg::OnDefault(wxCommandEvent& event)
{
    m_spinCtrlFileTimeTolerance->SetValue(2);
#ifdef FFS_WIN
    m_textCtrlFileManager->SetValue(wxT("explorer /select, %name"));
#elif defined FFS_LINUX
    m_textCtrlFileManager->SetValue(wxT("konqueror \"%path\""));
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

    scannedObjects      = 0;

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

    bool screenChanged = false; //avoid screen flicker by calling layout() only if necessary

    //remove linebreaks from currentStatusText
    wxString formattedStatusText = currentStatusText.c_str();
    for (wxString::iterator i = formattedStatusText.begin(); i != formattedStatusText.end(); ++i)
        if (*i == wxChar('\n'))
            *i = wxChar(' ');

    //status texts
    if (m_textCtrlFilename->GetValue() != formattedStatusText && (screenChanged = true)) //avoid screen flicker
        m_textCtrlFilename->SetValue(formattedStatusText);

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
    m_animationControl1->SetAnimation(*globalResource.animationSync);
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
        m_bitmapStatus->SetBitmap(*globalResource.bitmapStatusError);
        m_staticTextStatus->SetLabel(_("Aborted"));
        break;

    case FINISHED_WITH_SUCCESS:
        m_bitmapStatus->SetBitmap(*globalResource.bitmapStatusSuccess);
        m_staticTextStatus->SetLabel(_("Completed"));
        break;

    case FINISHED_WITH_ERROR:
        m_bitmapStatus->SetBitmap(*globalResource.bitmapStatusWarning);
        m_staticTextStatus->SetLabel(_("Completed"));
        break;

    case PAUSE:
        m_bitmapStatus->SetBitmap(*globalResource.bitmapStatusPause);
        m_staticTextStatus->SetLabel(_("Paused"));
        break;

    case SCANNING:
        m_bitmapStatus->SetBitmap(*globalResource.bitmapStatusComparing);
        m_staticTextStatus->SetLabel(_("Scanning..."));
        break;

    case COMPARING:
        m_bitmapStatus->SetBitmap(*globalResource.bitmapStatusComparing);
        m_staticTextStatus->SetLabel(_("Comparing..."));
        break;

    case SYNCHRONIZING:
        m_bitmapStatus->SetBitmap(*globalResource.bitmapStatusSyncing);
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
