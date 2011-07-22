// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "gui_generated.h"
#include "small_dlgs.h"
#include "msg_popup.h"
#include "../library/resources.h"
#include "../algorithm.h"
#include "../shared/string_conv.h"
#include "../shared/util.h"
#include "../shared/wx_choice_enum.h"
#include "../synchronization.h"
#include "../library/custom_grid.h"
#include "../shared/custom_button.h"
#include "../shared/i18n.h"
#include "../shared/global_func.h"
#include "../shared/build_info.h"
#include <wx/wupdlock.h>
#include <wx/msgdlg.h>
#include "../shared/mouse_move_dlg.h"

using namespace zen;


class AboutDlg : public AboutDlgGenerated
{
public:
    AboutDlg(wxWindow* parent);

private:
    void OnClose(wxCloseEvent& event);
    void OnOK(wxCommandEvent& event);
};


AboutDlg::AboutDlg(wxWindow* parent) : AboutDlgGenerated(parent)
{
    m_bitmap9->SetBitmap(GlobalResources::instance().getImage(wxT("website")));
    m_bitmap10->SetBitmap(GlobalResources::instance().getImage(wxT("email")));
    m_bitmap11->SetBitmap(GlobalResources::instance().getImage(wxT("logo")));
    m_bitmap13->SetBitmap(GlobalResources::instance().getImage(wxT("gpl")));
    m_bitmapTransl->SetBitmap(GlobalResources::instance().getImage(wxT("translation")));

    //create language credits
    for (std::vector<ExistingTranslations::Entry>::const_iterator i = ExistingTranslations::get().begin(); i != ExistingTranslations::get().end(); ++i)
    {
        //flag
        wxStaticBitmap* staticBitmapFlag = new wxStaticBitmap(m_scrolledWindowTranslators, wxID_ANY, GlobalResources::instance().getImage(i->languageFlag), wxDefaultPosition, wxSize(-1,11), 0 );
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

    m_animationControl1->SetAnimation(*GlobalResources::instance().animationMoney);
    m_animationControl1->Play();

    m_buttonOkay->SetFocus();
    Fit();

#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif
}


void AboutDlg::OnClose(wxCloseEvent& event)
{
    EndModal(0);
}


void AboutDlg::OnOK(wxCommandEvent& event)
{
    EndModal(0);
}


void zen::showAboutDialog()
{
    AboutDlg aboutDlg(NULL);
    aboutDlg.ShowModal();
}
//########################################################################################


class HelpDlg : public HelpDlgGenerated
{
public:
    HelpDlg(wxWindow* parent);

private:
    void OnClose(wxCloseEvent& event);
    void OnOK(wxCommandEvent& event);
};


HelpDlg::HelpDlg(wxWindow* parent) : HelpDlgGenerated(parent)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    m_notebook1->SetFocus();

    m_bitmap25->SetBitmap(GlobalResources::instance().getImage(wxT("help")));

    //populate decision trees: "compare by date"
    wxTreeItemId treeRoot      = m_treeCtrl1->AddRoot(_("DECISION TREE"));
    wxTreeItemId treeBothSides = m_treeCtrl1->AppendItem(treeRoot, _("file exists on both sides"));
    wxTreeItemId treeOneSide   = m_treeCtrl1->AppendItem(treeRoot, _("on one side only"));

    m_treeCtrl1->AppendItem(treeOneSide, _("- exists left only"));
    m_treeCtrl1->AppendItem(treeOneSide, _("- exists right only"));

    wxTreeItemId treeSameDate = m_treeCtrl1->AppendItem(treeBothSides, _("same date"));
    m_treeCtrl1->AppendItem(treeSameDate, _("- equal"));
    m_treeCtrl1->AppendItem(treeSameDate, _("- conflict (same date, different size)"));


    wxTreeItemId treeDifferentDate = m_treeCtrl1->AppendItem(treeBothSides, _("different date"));
    m_treeCtrl1->AppendItem(treeDifferentDate, _("- left newer"));
    m_treeCtrl1->AppendItem(treeDifferentDate, _("- right newer"));

    m_treeCtrl1->ExpandAll();

    //populate decision trees: "compare by content"
    wxTreeItemId tree2Root      = m_treeCtrl2->AddRoot(_("DECISION TREE"));
    wxTreeItemId tree2BothSides = m_treeCtrl2->AppendItem(tree2Root, _("file exists on both sides"));
    wxTreeItemId tree2OneSide   = m_treeCtrl2->AppendItem(tree2Root, _("on one side only"));

    m_treeCtrl2->AppendItem(tree2OneSide, _("- exists left only"));
    m_treeCtrl2->AppendItem(tree2OneSide, _("- exists right only"));

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


void zen::showHelpDialog()
{
    HelpDlg helpDlg(NULL);
    helpDlg.ShowModal();
}
//########################################################################################


class FilterDlg : public FilterDlgGenerated
{
public:
    FilterDlg(wxWindow* parent,
              bool isGlobalFilter,
              FilterConfig& filter);
    ~FilterDlg() {}

private:
    void OnClose       (wxCloseEvent& event);
    void OnHelp        (wxCommandEvent& event);
    void OnDefault     (wxCommandEvent& event);
    void OnApply       (wxCommandEvent& event);
    void OnCancel      (wxCommandEvent& event);
    void OnUpdateChoice(wxCommandEvent& event) { updateGui(); }
    void OnUpdateNameFilter(wxCommandEvent& event) { updateGui(); }

    void updateGui();
    void setFilter(const FilterConfig& filter);
    FilterConfig getFilter() const;

    const bool isGlobalFilter_;
    FilterConfig& outputRef;

    EnumDescrList<UnitTime> enumTimeDescr;
    EnumDescrList<UnitSize> enumSizeDescr;
};


FilterDlg::FilterDlg(wxWindow* parent,
                     bool isGlobalFilter, //global or local filter dialog?
                     FilterConfig& filter) :
    FilterDlgGenerated(parent),
    isGlobalFilter_(isGlobalFilter),
    outputRef(filter) //just hold reference
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    enumTimeDescr.
    add(UTIME_NONE, _("Inactive")).
    add(UTIME_SEC,  _("Second")).
    add(UTIME_MIN,  _("Minute")).
    add(UTIME_HOUR, _("Hour")).
    add(UTIME_DAY,  _("Day"));

    enumSizeDescr.
    add(USIZE_NONE, _("Inactive")).
    add(USIZE_BYTE, _("Byte")).
    add(USIZE_KB,   _("KB")).
    add(USIZE_MB,   _("MB"));

    m_bitmap26->SetBitmap(GlobalResources::instance().getImage(wxT("filterOn")));
    m_bpButtonHelp->SetBitmapLabel(GlobalResources::instance().getImage(wxT("help")));

    setFilter(filter);

    m_panel13->Hide();
    m_button10->SetFocus();

    //adapt header for global/local dialog
    if (isGlobalFilter_)
        m_staticTexHeader->SetLabel(_("Filter: All pairs"));
    else
        m_staticTexHeader->SetLabel(_("Filter: Single pair"));

    Fit();
}


void FilterDlg::updateGui()
{
    FilterConfig activeCfg = getFilter();

    if (!NameFilter(activeCfg.includeFilter, FilterConfig().excludeFilter).isNull())
        m_bitmapInclude->SetBitmap(GlobalResources::instance().getImage(wxT("include")));
    else
        m_bitmapInclude->SetBitmap(GlobalResources::instance().getImage(wxT("include_grey")));

    if (!NameFilter(FilterConfig().includeFilter, activeCfg.excludeFilter).isNull())
        m_bitmapExclude->SetBitmap(GlobalResources::instance().getImage(wxT("exclude")));
    else
        m_bitmapExclude->SetBitmap(GlobalResources::instance().getImage(wxT("exclude_grey")));

    if (activeCfg.unitTimeSpan != UTIME_NONE)
        m_bitmapFilterDate->SetBitmap(GlobalResources::instance().getImage(wxT("clock")));
    else
        m_bitmapFilterDate->SetBitmap(GlobalResources::instance().getImage(wxT("clock_grey")));

    if (activeCfg.unitSizeMin != USIZE_NONE ||
        activeCfg.unitSizeMax != USIZE_NONE)
        m_bitmapFilterSize->SetBitmap(GlobalResources::instance().getImage(wxT("size")));
    else
        m_bitmapFilterSize->SetBitmap(GlobalResources::instance().getImage(wxT("size_grey")));

    m_spinCtrlTimespan->Enable(activeCfg.unitTimeSpan != UTIME_NONE);
    m_spinCtrlMinSize ->Enable(activeCfg.unitSizeMin  != USIZE_NONE);
    m_spinCtrlMaxSize ->Enable(activeCfg.unitSizeMax  != USIZE_NONE);
}


void FilterDlg::setFilter(const FilterConfig& filter)
{
    m_textCtrlInclude->ChangeValue(toWx(filter.includeFilter));
    m_textCtrlExclude->ChangeValue(toWx(filter.excludeFilter));

    setEnumVal(enumTimeDescr, *m_choiceUnitTimespan, filter.unitTimeSpan);
    setEnumVal(enumSizeDescr, *m_choiceUnitMinSize,  filter.unitSizeMin);
    setEnumVal(enumSizeDescr, *m_choiceUnitMaxSize,  filter.unitSizeMax);

    m_spinCtrlTimespan->SetValue(static_cast<int>(filter.timeSpan));
    m_spinCtrlMinSize ->SetValue(static_cast<int>(filter.sizeMin));
    m_spinCtrlMaxSize ->SetValue(static_cast<int>(filter.sizeMax));

    updateGui();
}


FilterConfig FilterDlg::getFilter() const
{
    return FilterConfig(toZ(m_textCtrlInclude->GetValue()),
                        toZ(m_textCtrlExclude->GetValue()),
                        m_spinCtrlTimespan->GetValue(),
                        getEnumVal(enumTimeDescr, *m_choiceUnitTimespan),
                        m_spinCtrlMinSize->GetValue(),
                        getEnumVal(enumSizeDescr, *m_choiceUnitMinSize),
                        m_spinCtrlMaxSize->GetValue(),
                        getEnumVal(enumSizeDescr, *m_choiceUnitMaxSize));
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
    if (isGlobalFilter_)
        setFilter(MainConfiguration().globalFilter);
    else
        setFilter(FilterConfig());

    //changes to mainDialog are only committed when the OK button is pressed
    Fit();
}


void FilterDlg::OnApply(wxCommandEvent& event)
{
    //only if user presses ApplyFilter, he wants the changes to be committed
    outputRef = getFilter();

    //when leaving dialog: filter and redraw grid, if filter is active
    EndModal(ReturnSmallDlg::BUTTON_OKAY);
}


void FilterDlg::OnCancel(wxCommandEvent& event)
{
    EndModal(0);
}


void FilterDlg::OnClose(wxCloseEvent& event)
{
    EndModal(0);
}



ReturnSmallDlg::ButtonPressed zen::showFilterDialog(bool isGlobalFilter, FilterConfig& filter)
{
    FilterDlg filterDlg(NULL,
                        isGlobalFilter, //is main filter dialog
                        filter);
    return static_cast<ReturnSmallDlg::ButtonPressed>(filterDlg.ShowModal());
}
//########################################################################################


class DeleteDialog : public DeleteDlgGenerated
{
public:
    DeleteDialog(wxWindow* parent,
                 const std::vector<zen::FileSystemObject*>& rowsOnLeft,
                 const std::vector<zen::FileSystemObject*>& rowsOnRight,
                 bool& deleteOnBothSides,
                 bool& useRecycleBin);

private:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnDelOnBothSides(wxCommandEvent& event);
    void OnUseRecycler(wxCommandEvent& event);

    void updateGui();

    const std::vector<zen::FileSystemObject*>& rowsToDeleteOnLeft;
    const std::vector<zen::FileSystemObject*>& rowsToDeleteOnRight;
    bool& outRefdeleteOnBothSides;
    bool& outRefuseRecycleBin;
};


DeleteDialog::DeleteDialog(wxWindow* parent,
                           const std::vector<FileSystemObject*>& rowsOnLeft,
                           const std::vector<FileSystemObject*>& rowsOnRight,
                           bool& deleteOnBothSides,
                           bool& useRecycleBin) :
    DeleteDlgGenerated(parent),
    rowsToDeleteOnLeft(rowsOnLeft),
    rowsToDeleteOnRight(rowsOnRight),
    outRefdeleteOnBothSides(deleteOnBothSides),
    outRefuseRecycleBin(useRecycleBin)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    m_checkBoxDeleteBothSides->SetValue(deleteOnBothSides);
    m_checkBoxUseRecycler->SetValue(useRecycleBin);

    //if both sides contain same rows this checkbox is superfluous
    if (rowsToDeleteOnLeft == rowsToDeleteOnRight)
    {
        m_checkBoxDeleteBothSides->Show(false);
        m_checkBoxDeleteBothSides->SetValue(true);
    }

    updateGui();

    m_buttonOK->SetFocus();
}


void DeleteDialog::updateGui()
{
    const std::pair<wxString, int> delInfo = zen::deleteFromGridAndHDPreview(
                                                 rowsToDeleteOnLeft,
                                                 rowsToDeleteOnRight,
                                                 m_checkBoxDeleteBothSides->GetValue());
    wxString header;
    if (m_checkBoxUseRecycler->GetValue())
    {
        header = _P("Do you really want to move the following object to the Recycle Bin?",
                    "Do you really want to move the following %x objects to the Recycle Bin?", delInfo.second);
        m_bitmap12->SetBitmap(GlobalResources::instance().getImage(wxT("recycler")));
    }
    else
    {
        header = _P("Do you really want to delete the following object?",
                    "Do you really want to delete the following %x objects?", delInfo.second);
        m_bitmap12->SetBitmap(GlobalResources::instance().getImage(wxT("deleteFile")));
    }
    header.Replace(wxT("%x"), toStringSep(delInfo.second));
    m_staticTextHeader->SetLabel(header);

    const wxString& filesToDelete = delInfo.first;
    m_textCtrlMessage->SetValue(filesToDelete);

    Layout();
}


void DeleteDialog::OnOK(wxCommandEvent& event)
{
    outRefuseRecycleBin = m_checkBoxUseRecycler->GetValue();
    if (rowsToDeleteOnLeft != rowsToDeleteOnRight)
        outRefdeleteOnBothSides = m_checkBoxDeleteBothSides->GetValue();

    EndModal(ReturnSmallDlg::BUTTON_OKAY);
}

void DeleteDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(ReturnSmallDlg::BUTTON_CANCEL);
}

void DeleteDialog::OnClose(wxCloseEvent& event)
{
    EndModal(ReturnSmallDlg::BUTTON_CANCEL);
}

void DeleteDialog::OnDelOnBothSides(wxCommandEvent& event)
{
    updateGui();
}

void DeleteDialog::OnUseRecycler(wxCommandEvent& event)
{
    updateGui();
}


ReturnSmallDlg::ButtonPressed zen::showDeleteDialog(const std::vector<zen::FileSystemObject*>& rowsOnLeft,
                                                    const std::vector<zen::FileSystemObject*>& rowsOnRight,
                                                    bool& deleteOnBothSides,
                                                    bool& useRecycleBin)
{
    DeleteDialog confirmDeletion(NULL,
                                 rowsOnLeft,
                                 rowsOnRight,
                                 deleteOnBothSides,
                                 useRecycleBin);
    return static_cast<ReturnSmallDlg::ButtonPressed>(confirmDeletion.ShowModal());
}
//########################################################################################


class CustomizeColsDlg : public CustomizeColsDlgGenerated
{
public:
    CustomizeColsDlg(wxWindow* parent, xmlAccess::ColumnAttributes& attr);

private:
    void OnOkay(wxCommandEvent& event);
    void OnDefault(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    void OnMoveUp(wxCommandEvent& event);
    void OnMoveDown(wxCommandEvent& event);

    xmlAccess::ColumnAttributes& output;
};


CustomizeColsDlg::CustomizeColsDlg(wxWindow* parent, xmlAccess::ColumnAttributes& attr) :
    CustomizeColsDlgGenerated(parent),
    output(attr)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    m_bpButton29->SetBitmapLabel(GlobalResources::instance().getImage(wxT("moveUp")));
    m_bpButton30->SetBitmapLabel(GlobalResources::instance().getImage(wxT("moveDown")));

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

    EndModal(ReturnSmallDlg::BUTTON_OKAY);
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


ReturnSmallDlg::ButtonPressed zen::showCustomizeColsDlg(xmlAccess::ColumnAttributes& attr)
{
    CustomizeColsDlg customizeDlg(NULL, attr);
    return static_cast<ReturnSmallDlg::ButtonPressed>(customizeDlg.ShowModal());
}
//########################################################################################


class SyncPreviewDlg : public SyncPreviewDlgGenerated
{
public:
    SyncPreviewDlg(wxWindow* parent,
                   const wxString& variantName,
                   const zen::SyncStatistics& statistics,
                   bool& dontShowAgain);
private:
    void OnClose(wxCloseEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnStartSync(wxCommandEvent& event);

    bool& m_dontShowAgain;
};



SyncPreviewDlg::SyncPreviewDlg(wxWindow* parent,
                               const wxString& variantName,
                               const zen::SyncStatistics& statistics,
                               bool& dontShowAgain) :
    SyncPreviewDlgGenerated(parent),
    m_dontShowAgain(dontShowAgain)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    using zen::toStringSep;

    m_buttonStartSync->setBitmapFront(GlobalResources::instance().getImage(wxT("startSync")));
    m_bitmapCreate->SetBitmap(GlobalResources::instance().getImage(wxT("create")));
    m_bitmapUpdate->SetBitmap(GlobalResources::instance().getImage(wxT("update")));
    m_bitmapDelete->SetBitmap(GlobalResources::instance().getImage(wxT("delete")));
    m_bitmapData->SetBitmap(GlobalResources::instance().getImage(wxT("data")));

    m_staticTextVariant->SetLabel(variantName);
    m_textCtrlData->SetValue(zen::formatFilesizeToShortString(statistics.getDataToProcess()));

    m_textCtrlCreateL->SetValue(toStringSep(statistics.getCreate   <LEFT_SIDE>()));
    m_textCtrlUpdateL->SetValue(toStringSep(statistics.getOverwrite<LEFT_SIDE>()));
    m_textCtrlDeleteL->SetValue(toStringSep(statistics.getDelete   <LEFT_SIDE>()));

    m_textCtrlCreateR->SetValue(toStringSep(statistics.getCreate   <RIGHT_SIDE>()));
    m_textCtrlUpdateR->SetValue(toStringSep(statistics.getOverwrite<RIGHT_SIDE>()));
    m_textCtrlDeleteR->SetValue(toStringSep(statistics.getDelete   <RIGHT_SIDE>()));

    m_checkBoxDontShowAgain->SetValue(dontShowAgain);

    m_buttonStartSync->SetFocus();
    Fit();
}


void SyncPreviewDlg::OnClose(wxCloseEvent& event)
{
    EndModal(ReturnSmallDlg::BUTTON_CANCEL);
}


void SyncPreviewDlg::OnCancel(wxCommandEvent& event)
{
    EndModal(ReturnSmallDlg::BUTTON_CANCEL);
}


void SyncPreviewDlg::OnStartSync(wxCommandEvent& event)
{
    m_dontShowAgain = m_checkBoxDontShowAgain->GetValue();
    EndModal(ReturnSmallDlg::BUTTON_OKAY);
}


ReturnSmallDlg::ButtonPressed zen::showSyncPreviewDlg(
    const wxString& variantName,
    const zen::SyncStatistics& statistics,
    bool& dontShowAgain)
{
    SyncPreviewDlg preview(NULL,
                           variantName,
                           statistics,
                           dontShowAgain);

    return static_cast<ReturnSmallDlg::ButtonPressed>(preview.ShowModal());
}
//########################################################################################


class CompareCfgDialog : public CmpCfgDlgGenerated
{
public:
    CompareCfgDialog(wxWindow* parent,
                     zen::CompareVariant& cmpVar,
                     SymLinkHandling& handleSymlinks);

private:
    void OnOkay(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event) { EndModal(0); }
    void OnCancel(wxCommandEvent& event) { EndModal(0); }
    void OnTimeSize(wxCommandEvent& event);
    void OnTimeSizeDouble(wxMouseEvent& event);
    void OnContent(wxCommandEvent& event);
    void OnContentDouble(wxMouseEvent& event);
    void OnShowHelp(wxCommandEvent& event);

    void updateView();

    zen::CompareVariant& cmpVarOut;
    SymLinkHandling& handleSymlinksOut;

    zen::EnumDescrList<SymLinkHandling>  enumDescrHandleSyml;
};


CompareCfgDialog::CompareCfgDialog(wxWindow* parent,
                                   CompareVariant& cmpVar,
                                   SymLinkHandling& handleSymlinks) :
    CmpCfgDlgGenerated(parent),
    cmpVarOut(cmpVar),
    handleSymlinksOut(handleSymlinks)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    enumDescrHandleSyml.
    add(SYMLINK_IGNORE,       _("Ignore")).
    add(SYMLINK_USE_DIRECTLY, _("Direct")).
    add(SYMLINK_FOLLOW_LINK,  _("Follow"));

    //move dialog up so that compare-config button and first config-variant are on same level
    //   Move(wxPoint(position.x, std::max(0, position.y - (m_buttonTimeSize->GetScreenPosition() - GetScreenPosition()).y)));

    m_bpButtonHelp   ->SetBitmapLabel(GlobalResources::instance().getImage(wxT("help")));
    m_bitmapByTime   ->SetBitmap     (GlobalResources::instance().getImage(wxT("clock")));
    m_bitmapByContent->SetBitmap     (GlobalResources::instance().getImage(wxT("cmpByContent")));

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

    setEnumVal(enumDescrHandleSyml, *m_choiceHandleSymlinks, handleSymlinks);

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

    handleSymlinksOut = getEnumVal(enumDescrHandleSyml, *m_choiceHandleSymlinks);

    EndModal(ReturnSmallDlg::BUTTON_OKAY);
}


void CompareCfgDialog::OnTimeSize(wxCommandEvent& event)
{
    m_radioBtnSizeDate->SetValue(true);
}


void CompareCfgDialog::OnContent(wxCommandEvent& event)
{
    m_radioBtnContent->SetValue(true);
}


void CompareCfgDialog::OnTimeSizeDouble(wxMouseEvent& event)
{
    wxCommandEvent dummy;
    OnTimeSize(dummy);
    OnOkay(dummy);
}


void CompareCfgDialog::OnContentDouble(wxMouseEvent& event)
{
    wxCommandEvent dummy;
    OnContent(dummy);
    OnOkay(dummy);
}


void CompareCfgDialog::OnShowHelp(wxCommandEvent& event)
{
    HelpDlg helpDlg(this);
    helpDlg.ShowModal();
}


ReturnSmallDlg::ButtonPressed zen::showCompareCfgDialog(CompareVariant& cmpVar,
                                                        SymLinkHandling& handleSymlinks)
{
    CompareCfgDialog syncDlg(NULL, cmpVar, handleSymlinks);

    return static_cast<ReturnSmallDlg::ButtonPressed>(syncDlg.ShowModal());
}
//########################################################################################


class GlobalSettingsDlg : public GlobalSettingsDlgGenerated
{
public:
    GlobalSettingsDlg(wxWindow* parent, xmlAccess::XmlGlobalSettings& globalSettings);

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


GlobalSettingsDlg::GlobalSettingsDlg(wxWindow* parent, xmlAccess::XmlGlobalSettings& globalSettings) :
    GlobalSettingsDlgGenerated(parent),
    settings(globalSettings)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    m_bitmapSettings->SetBitmap(GlobalResources::instance().getImage(wxT("settings")));
    m_buttonResetDialogs->setBitmapFront(GlobalResources::instance().getImage(wxT("warningSmall")), 5);
    m_bpButtonAddRow->SetBitmapLabel(GlobalResources::instance().getImage(wxT("addFolderPair")));
    m_bpButtonRemoveRow->SetBitmapLabel(GlobalResources::instance().getImage(wxT("removeFolderPair")));

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
    settings.copyLockedFiles     = m_checkBoxCopyLocked->GetValue();
    settings.copyFilePermissions = m_checkBoxCopyPermissions->GetValue();
    settings.gui.externelApplications = getExtApp();

    EndModal(ReturnSmallDlg::BUTTON_OKAY);
}


void GlobalSettingsDlg::OnResetDialogs(wxCommandEvent& event)
{
    if (showQuestionDlg(ReturnQuestionDlg::BUTTON_YES | ReturnQuestionDlg::BUTTON_CANCEL,
                        _("Restore all hidden dialogs?")) == ReturnQuestionDlg::BUTTON_YES)
        settings.optDialogs.resetDialogs();
}


void GlobalSettingsDlg::OnDefault(wxCommandEvent& event)
{
    xmlAccess::XmlGlobalSettings defaultCfg;

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


ReturnSmallDlg::ButtonPressed zen::showGlobalSettingsDlg(xmlAccess::XmlGlobalSettings& globalSettings)
{
    GlobalSettingsDlg settingsDlg(NULL, globalSettings);
    return static_cast<ReturnSmallDlg::ButtonPressed>(settingsDlg.ShowModal());
}
