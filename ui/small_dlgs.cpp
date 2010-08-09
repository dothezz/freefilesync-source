// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "gui_generated.h"
#include "small_dlgs.h"
#include "msg_popup.h"
#include "../library/resources.h"
#include "../algorithm.h"
#include "../shared/string_conv.h"
#include "../shared/util.h"
#include "../synchronization.h"
#include "../library/custom_grid.h"
#include "../shared/custom_button.h"
#include "../shared/localization.h"
#include "../shared/global_func.h"
#include "../shared/build_info.h"
#include <wx/wupdlock.h>
#include <wx/msgdlg.h>

using namespace ffs3;


class AboutDlg : public AboutDlgGenerated
{
public:
    AboutDlg(wxWindow* window);

private:
    void OnClose(wxCloseEvent& event);
    void OnOK(wxCommandEvent& event);
};


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
    wxString build = __TDATE__;
#if wxUSE_UNICODE
    build += wxT(" - Unicode");
#else
    build += wxT(" - ANSI");
#endif //wxUSE_UNICODE

    //compile time info about 32/64-bit build
    if (util::is64BitBuild)
        build += wxT(" x64");
    else
        build += wxT(" x86");
    assert_static(util::is32BitBuild || util::is64BitBuild);

    wxString buildFormatted = _("(Build: %x)");
    buildFormatted.Replace(wxT("%x"), build);

    m_build->SetLabel(buildFormatted);

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


void ffs3::showAboutDialog()
{
    AboutDlg* aboutDlg = new AboutDlg(NULL);
    aboutDlg->ShowModal();
    aboutDlg->Destroy();
}
//########################################################################################


class HelpDlg : public HelpDlgGenerated
{
public:
    HelpDlg(wxWindow* window);

private:
    void OnClose(wxCloseEvent& event);
    void OnOK(wxCommandEvent& event);
};


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


void ffs3::showHelpDialog()
{
    HelpDlg* helpDlg = new HelpDlg(NULL);
    helpDlg->ShowModal();
    helpDlg->Destroy();
}
//########################################################################################


class FilterDlg : public FilterDlgGenerated
{
public:
    FilterDlg(wxWindow* window,
              bool isGlobalFilter,
              Zstring& filterIncl,
              Zstring& filterExcl);
    ~FilterDlg() {}

    enum
    {
        BUTTON_APPLY = 1
    };

private:
    void OnHelp(wxCommandEvent& event);
    void OnDefault(wxCommandEvent& event);
    void OnApply(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    const bool isGlobalFilter_;
    Zstring& includeFilter;
    Zstring& excludeFilter;
};


FilterDlg::FilterDlg(wxWindow* window,
                     bool isGlobalFilter, //global or local filter dialog?
                     Zstring& filterIncl,
                     Zstring& filterExcl) :
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

    //adapt header for global/local dialog
    if (isGlobalFilter_)
        m_staticTexHeader->SetLabel(_("Filter: All pairs"));
    else
        m_staticTexHeader->SetLabel(_("Filter: Single pair"));

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



DefaultReturnCode::Response ffs3::showFilterDialog(bool isGlobalFilter,
        Zstring& filterIncl,
        Zstring& filterExcl)
{
    DefaultReturnCode::Response rv = DefaultReturnCode::BUTTON_CANCEL;
    FilterDlg* filterDlg = new FilterDlg(NULL,
                                         isGlobalFilter, //is main filter dialog
                                         filterIncl,
                                         filterExcl);
    if (filterDlg->ShowModal() == FilterDlg::BUTTON_APPLY)
        rv = DefaultReturnCode::BUTTON_OKAY;

    filterDlg->Destroy();
    return rv;
}
//########################################################################################


class DeleteDialog : public DeleteDlgGenerated
{
public:
    DeleteDialog(wxWindow* main,
                 const std::vector<ffs3::FileSystemObject*>& rowsOnLeft,
                 const std::vector<ffs3::FileSystemObject*>& rowsOnRight,
                 bool& deleteOnBothSides,
                 bool& useRecycleBin,
                 int& totalDeleteCount);

    enum
    {
        BUTTON_OKAY = 1,
        BUTTON_CANCEL
    };

private:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnDelOnBothSides(wxCommandEvent& event);
    void OnUseRecycler(wxCommandEvent& event);

    void updateTexts();

    const std::vector<ffs3::FileSystemObject*>& rowsToDeleteOnLeft;
    const std::vector<ffs3::FileSystemObject*>& rowsToDeleteOnRight;
    bool& m_deleteOnBothSides;
    bool& m_useRecycleBin;
    int&  totalDelCount;
};


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
        m_staticTextHeader->SetLabel(_("Do you really want to move the following object(s) to the Recycle Bin?"));
        m_bitmap12->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("recycler")));
    }
    else
    {
        m_staticTextHeader->SetLabel(_("Do you really want to delete the following object(s)?"));
        m_bitmap12->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("deleteFile")));
    }

    const std::pair<wxString, int> delInfo = ffs3::deleteFromGridAndHDPreview(
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
    m_useRecycleBin = m_checkBoxUseRecycler->GetValue();
    updateTexts();
}


DefaultReturnCode::Response ffs3::showDeleteDialog(const std::vector<ffs3::FileSystemObject*>& rowsOnLeft,
        const std::vector<ffs3::FileSystemObject*>& rowsOnRight,
        bool& deleteOnBothSides,
        bool& useRecycleBin,
        int& totalDeleteCount)
{
    DefaultReturnCode::Response rv = DefaultReturnCode::BUTTON_CANCEL;

    DeleteDialog* confirmDeletion = new DeleteDialog(NULL,
            rowsOnLeft,
            rowsOnRight,
            deleteOnBothSides,
            useRecycleBin,
            totalDeleteCount);
    if (confirmDeletion->ShowModal() == DeleteDialog::BUTTON_OKAY)
        rv = DefaultReturnCode::BUTTON_OKAY;

    confirmDeletion->Destroy();
    return rv;
}
//########################################################################################


class CustomizeColsDlg : public CustomizeColsDlgGenerated
{
public:
    CustomizeColsDlg(wxWindow* window, xmlAccess::ColumnAttributes& attr);

    enum
    {
        BUTTON_OKAY = 10
    };

private:
    void OnOkay(wxCommandEvent& event);
    void OnDefault(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    void OnMoveUp(wxCommandEvent& event);
    void OnMoveDown(wxCommandEvent& event);

    xmlAccess::ColumnAttributes& output;
};


CustomizeColsDlg::CustomizeColsDlg(wxWindow* window, xmlAccess::ColumnAttributes& attr) :
    CustomizeColsDlgGenerated(window),
    output(attr)
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


DefaultReturnCode::Response ffs3::showCustomizeColsDlg(xmlAccess::ColumnAttributes& attr)
{
    DefaultReturnCode::Response rv = DefaultReturnCode::BUTTON_CANCEL;

    CustomizeColsDlg* customizeDlg = new CustomizeColsDlg(NULL, attr);
    if (customizeDlg->ShowModal() == CustomizeColsDlg::BUTTON_OKAY)
        rv = DefaultReturnCode::BUTTON_OKAY;
    customizeDlg->Destroy();

    return rv;
}
//########################################################################################


class SyncPreviewDlg : public SyncPreviewDlgGenerated
{
public:
    SyncPreviewDlg(wxWindow* parentWindow,
                   const wxString& variantName,
                   const ffs3::SyncStatistics& statistics,
                   bool& dontShowAgain);
    enum
    {
        BUTTON_START  = 1,
        BUTTON_CANCEL = 2
    };

private:
    void OnClose(wxCloseEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnStartSync(wxCommandEvent& event);

    bool& m_dontShowAgain;
};



SyncPreviewDlg::SyncPreviewDlg(wxWindow* parentWindow,
                               const wxString& variantName,
                               const ffs3::SyncStatistics& statistics,
                               bool& dontShowAgain) :
    SyncPreviewDlgGenerated(parentWindow),
    m_dontShowAgain(dontShowAgain)
{
    using ffs3::numberToStringSep;

    m_buttonStartSync->setBitmapFront(GlobalResources::getInstance().getImageByName(wxT("startSync")));
    m_bitmapCreate->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("create")));
    m_bitmapUpdate->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("update")));
    m_bitmapDelete->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("delete")));
    m_bitmapData->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("data")));

    m_staticTextVariant->SetLabel(variantName);
    m_textCtrlData->SetValue(ffs3::formatFilesizeToShortString(statistics.getDataToProcess()));

    m_textCtrlCreateL->SetValue(numberToStringSep(statistics.getCreate(   true, false)));
    m_textCtrlUpdateL->SetValue(numberToStringSep(statistics.getOverwrite(true, false)));
    m_textCtrlDeleteL->SetValue(numberToStringSep(statistics.getDelete(   true, false)));

    m_textCtrlCreateR->SetValue(numberToStringSep(statistics.getCreate(   false, true)));
    m_textCtrlUpdateR->SetValue(numberToStringSep(statistics.getOverwrite(false, true)));
    m_textCtrlDeleteR->SetValue(numberToStringSep(statistics.getDelete(   false, true)));

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


DefaultReturnCode::Response ffs3::showSyncPreviewDlg(
    const wxString& variantName,
    const ffs3::SyncStatistics& statistics,
    bool& dontShowAgain)
{
    DefaultReturnCode::Response rv = DefaultReturnCode::BUTTON_CANCEL;

    SyncPreviewDlg* preview = new SyncPreviewDlg(NULL,
            variantName,
            statistics,
            dontShowAgain);

    if (preview->ShowModal() == SyncPreviewDlg::BUTTON_START)
        rv = DefaultReturnCode::BUTTON_OKAY;

    preview->Destroy();

    return rv;
}
//########################################################################################


class CompareCfgDialog : public CmpCfgDlgGenerated
{
public:
    CompareCfgDialog(wxWindow* parentWindow,
                     const wxPoint& position,
                     ffs3::CompareVariant& cmpVar,
                     SymLinkHandling& handleSymlinks);

    enum
    {
        BUTTON_OKAY = 10
    };

private:
    void OnOkay(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnTimeSize(wxCommandEvent& event);
    void OnContent(wxCommandEvent& event);
    void OnShowHelp(wxCommandEvent& event);

    void updateView();

    ffs3::CompareVariant& cmpVarOut;
    SymLinkHandling& handleSymlinksOut;
};


namespace
{
void setValue(wxChoice& choiceCtrl, ffs3::SymLinkHandling value)
{
    choiceCtrl.Clear();
    choiceCtrl.Append(_("Ignore"));
    choiceCtrl.Append(_("Direct"));
    choiceCtrl.Append(_("Follow"));

    //default
    choiceCtrl.SetSelection(0);

    switch (value)
    {
    case ffs3::SYMLINK_IGNORE:
        choiceCtrl.SetSelection(0);
        break;
    case ffs3::SYMLINK_USE_DIRECTLY:
        choiceCtrl.SetSelection(1);
        break;
    case ffs3::SYMLINK_FOLLOW_LINK:
        choiceCtrl.SetSelection(2);
        break;
    }
}


ffs3::SymLinkHandling getValue(const wxChoice& choiceCtrl)
{
    switch (choiceCtrl.GetSelection())
    {
    case 0:
        return ffs3::SYMLINK_IGNORE;
    case 1:
        return ffs3::SYMLINK_USE_DIRECTLY;
    case 2:
        return ffs3::SYMLINK_FOLLOW_LINK;
    default:
        assert(false);
        return ffs3::SYMLINK_IGNORE;
    }
}
}

CompareCfgDialog::CompareCfgDialog(wxWindow* parentWindow,
                                   const wxPoint& position,
                                   CompareVariant& cmpVar,
                                   SymLinkHandling& handleSymlinks) :
    CmpCfgDlgGenerated(parentWindow),
    cmpVarOut(cmpVar),
    handleSymlinksOut(handleSymlinks)
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


    setValue(*m_choiceHandleSymlinks, handleSymlinks);

    updateView();
}

void CompareCfgDialog::updateView()
{
    Fit();
}

void CompareCfgDialog::OnOkay(wxCommandEvent& event)
{
    if (m_radioBtnContent->GetValue())
        cmpVarOut = CMP_BY_CONTENT;
    else
        cmpVarOut = CMP_BY_TIME_SIZE;

    handleSymlinksOut = getValue(*m_choiceHandleSymlinks);;

    EndModal(BUTTON_OKAY);
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
    m_radioBtnSizeDate->SetValue(true);
    OnOkay(event);
}


void CompareCfgDialog::OnContent(wxCommandEvent& event)
{
    m_radioBtnContent->SetValue(true);
    OnOkay(event);
}


void CompareCfgDialog::OnShowHelp(wxCommandEvent& event)
{
    HelpDlg* helpDlg = new HelpDlg(this);
    helpDlg->ShowModal();
}


DefaultReturnCode::Response ffs3::showCompareCfgDialog(
    const wxPoint& position,
    CompareVariant& cmpVar,
    SymLinkHandling& handleSymlinks)
{
    CompareCfgDialog syncDlg(NULL, position, cmpVar, handleSymlinks);

    return syncDlg.ShowModal() == CompareCfgDialog::BUTTON_OKAY ?
           DefaultReturnCode::BUTTON_OKAY :
           DefaultReturnCode::BUTTON_CANCEL;
}
//########################################################################################


class GlobalSettingsDlg : public GlobalSettingsDlgGenerated
{
public:
    GlobalSettingsDlg(wxWindow* window, xmlAccess::XmlGlobalSettings& globalSettings);

    enum
    {
        BUTTON_OKAY = 10
    };

private:
    void OnOkay(wxCommandEvent& event);
    void OnResetDialogs(wxCommandEvent& event);
    void OnDefault(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnAddRow(wxCommandEvent& event);
    void OnRemoveRow(wxCommandEvent& event);

    void set(const xmlAccess::ExternalApps& extApp);
    xmlAccess::ExternalApps getExtApp();

    xmlAccess::XmlGlobalSettings& settings;
};


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
    m_checkBoxCopyPermissions->SetValue(globalSettings.copyFilePermissions);

#ifndef FFS_WIN
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
    settings.ignoreOneHourDiff   = m_checkBoxIgnoreOneHour->GetValue();
    settings.copyLockedFiles     = m_checkBoxCopyLocked->GetValue();
    settings.copyFilePermissions = m_checkBoxCopyPermissions->GetValue();
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

    m_checkBoxIgnoreOneHour->  SetValue(defaultCfg.ignoreOneHourDiff);
    m_checkBoxCopyLocked->     SetValue(defaultCfg.copyLockedFiles);
    m_checkBoxCopyPermissions->SetValue(defaultCfg.copyFilePermissions);
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


DefaultReturnCode::Response ffs3::showGlobalSettingsDlg(xmlAccess::XmlGlobalSettings& globalSettings)
{
    DefaultReturnCode::Response rv = DefaultReturnCode::BUTTON_CANCEL;

    wxDialog* settingsDlg = new GlobalSettingsDlg(NULL, globalSettings);
    if (settingsDlg->ShowModal() == GlobalSettingsDlg::BUTTON_OKAY)
        rv = DefaultReturnCode::BUTTON_OKAY;

    settingsDlg->Destroy();

    return rv;
}
