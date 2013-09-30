// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "small_dlgs.h"
#include <wx/wupdlock.h>
#include <zen/format_unit.h>
#include <zen/build_info.h>
#include <zen/tick_count.h>
#include <zen/stl_tools.h>
#include <wx+/choice_enum.h>
#include <wx+/bitmap_button.h>
#include <wx+/rtl.h>
#include <wx+/no_flicker.h>
#include <wx+/mouse_move_dlg.h>
#include <wx+/image_tools.h>
#include <wx+/font_size.h>
#include <wx+/std_button_order.h>
#include <wx+/popup_dlg.h>
#include <wx+/image_resources.h>
#include "gui_generated.h"
#include "custom_grid.h"
#include "../algorithm.h"
#include "../synchronization.h"
#include "../lib/help_provider.h"
#include "../lib/hard_filter.h"
#include "../version/version.h"

using namespace zen;


class AboutDlg : public AboutDlgGenerated
{
public:
    AboutDlg(wxWindow* parent);

private:
    virtual void OnClose (wxCloseEvent&   event) { EndModal(ReturnSmallDlg::BUTTON_CANCEL); }
    virtual void OnOK    (wxCommandEvent& event) { EndModal(ReturnSmallDlg::BUTTON_OKAY); }
    virtual void OnDonate(wxCommandEvent& event) { wxLaunchDefaultBrowser(L"http://freefilesync.sourceforge.net/donate.php"); }
};


AboutDlg::AboutDlg(wxWindow* parent) : AboutDlgGenerated(parent)
{
    setStandardButtonOrder(*bSizerStdButtons, StdButtons().setAffirmative(m_buttonClose));

    setRelativeFontSize(*m_buttonDonate, 1.25);

    assert(m_buttonClose->GetId() == wxID_OK); //we cannot use wxID_CLOSE else Esc key won't work: yet another wxWidgets bug??

    m_bitmap9 ->SetBitmap(getResourceImage(L"website"));
    m_bitmap10->SetBitmap(getResourceImage(L"email"));
    m_bitmap13->SetBitmap(getResourceImage(L"gpl"));
    //m_bitmapSmiley->SetBitmap(getResourceImage(L"smiley"));

    m_animCtrlWink->SetAnimation(getResourceAnimation(L"wink"));
    m_animCtrlWink->Play();

    //create language credits
    for (auto it = ExistingTranslations::get().begin(); it != ExistingTranslations::get().end(); ++it)
    {
        //flag
        wxStaticBitmap* staticBitmapFlag = new wxStaticBitmap(m_scrolledWindowTranslators, wxID_ANY, getResourceImage(it->languageFlag), wxDefaultPosition, wxSize(-1, 11), 0 );
        fgSizerTranslators->Add(staticBitmapFlag, 0, wxALIGN_CENTER);

        //translator name
        wxStaticText* staticTextTranslator = new wxStaticText(m_scrolledWindowTranslators, wxID_ANY, it->translatorName, wxDefaultPosition, wxDefaultSize, 0 );
        staticTextTranslator->Wrap(-1);
        fgSizerTranslators->Add(staticTextTranslator, 0, wxALIGN_CENTER_VERTICAL);

        staticBitmapFlag    ->SetToolTip(it->languageName);
        staticTextTranslator->SetToolTip(it->languageName);
    }
    fgSizerTranslators->Fit(m_scrolledWindowTranslators);

#ifdef ZEN_WIN
    new zen::MouseMoveWindow(*this); //-> put *after* creating credits
#endif

    //build information
    wxString build = __TDATE__;
#if wxUSE_UNICODE
    build += L" - Unicode";
#else
    build += L" - ANSI";
#endif //wxUSE_UNICODE

    //compile time info about 32/64-bit build
    if (zen::is64BitBuild)
        build += L" x64";
    else
        build += L" x86";
    assert_static(zen::is32BitBuild || zen::is64BitBuild);

    GetSizer()->SetSizeHints(this); //~=Fit() + SetMinSize()

    //generate logo: put *after* first Fit()
    Layout(); //make sure m_panelLogo has final width (required by wxGTK)

    wxBitmap bmpLogo;
    {
        wxImage tmp = getResourceImage(L"logo").ConvertToImage();
        tmp.Resize(wxSize(GetClientSize().GetWidth(), tmp.GetHeight()), wxPoint(0, 0), 255, 255, 255); //enlarge to fit full width
        bmpLogo = wxBitmap(tmp);
    }
    {
        wxMemoryDC dc(bmpLogo);

        wxImage labelImage = createImageFromText(wxString(L"FreeFileSync ") + zen::currentVersion,
                                                 wxFont(wxNORMAL_FONT->GetPointSize() * 1.8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"Tahoma"),
                                                 *wxBLACK); //accessibility: align foreground/background colors!
        wxImage buildImage = createImageFromText(replaceCpy(_("Build: %x"), L"%x", build),
                                                 *wxNORMAL_FONT,
                                                 *wxBLACK);
        wxImage dynImage = stackImages(labelImage, buildImage, ImageStackLayout::VERTICAL, ImageStackAlignment::CENTER, 0);

        dc.DrawBitmap(dynImage, wxPoint((bmpLogo.GetWidth () - dynImage.GetWidth ()) / 2,
                                        (bmpLogo.GetHeight() - dynImage.GetHeight()) / 2));
    }
    m_bitmapLogo->SetBitmap(bmpLogo);

    GetSizer()->SetSizeHints(this); //~=Fit() + SetMinSize()
    //=> works like a charm for GTK2 with window resizing problems and title bar corruption; e.g. Debian!!!

    m_buttonClose->SetFocus(); //on GTK ESC is only associated with wxID_OK correctly if we set at least *any* focus at all!!!
}


void zen::showAboutDialog(wxWindow* parent)
{
    AboutDlg aboutDlg(parent);
    aboutDlg.ShowModal();
}

//########################################################################################

class FilterDlg : public FilterDlgGenerated
{
public:
    FilterDlg(wxWindow* parent,
              FilterConfig& filter,
              const wxString& title);
    ~FilterDlg() {}

private:
    virtual void OnClose (wxCloseEvent&   event) { EndModal(ReturnSmallDlg::BUTTON_CANCEL); }
    virtual void OnCancel(wxCommandEvent& event) { EndModal(ReturnSmallDlg::BUTTON_CANCEL); }
    virtual void OnHelpShowExamples(wxHyperlinkEvent& event) { displayHelpEntry(L"html/Exclude Items.html", this); }
    virtual void OnClear (wxCommandEvent& event);
    virtual void OnOkay  (wxCommandEvent& event);
    virtual void OnUpdateChoice(wxCommandEvent& event) { updateGui(); }
    virtual void OnUpdateNameFilter(wxCommandEvent& event) { updateGui(); }

    void updateGui();
    void setFilter(const FilterConfig& filter);
    FilterConfig getFilter() const;
    void onKeyEvent(wxKeyEvent& event);

    FilterConfig& outputRef;

    EnumDescrList<UnitTime> enumTimeDescr;
    EnumDescrList<UnitSize> enumSizeDescr;
};


FilterDlg::FilterDlg(wxWindow* parent,
                     FilterConfig& filter,
                     const wxString& title) :
    FilterDlgGenerated(parent),
    outputRef(filter) //just hold reference
{
#ifdef ZEN_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif
    setStandardButtonOrder(*bSizerStdButtons, StdButtons().setAffirmative(m_buttonOk).setCancel(m_buttonCancel));

    SetTitle(title);

#ifndef __WXGTK__  //wxWidgets holds portability promise by not supporting for multi-line controls...not
    m_textCtrlInclude->SetMaxLength(0); //allow large filter entries!
    m_textCtrlExclude->SetMaxLength(0); //
#endif

    m_textCtrlInclude->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(FilterDlg::onKeyEvent), nullptr, this);
    m_textCtrlExclude->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(FilterDlg::onKeyEvent), nullptr, this);

    enumTimeDescr.
    add(UTIME_NONE, L"(" + _("None") + L")"). //meta options should be enclosed in parentheses
    add(UTIME_TODAY,       _("Today")).
    //    add(UTIME_THIS_WEEK,   _("This week")).
    add(UTIME_THIS_MONTH,  _("This month")).
    add(UTIME_THIS_YEAR,   _("This year")).
    add(UTIME_LAST_X_DAYS, _("Last x days"));

    enumSizeDescr.
    add(USIZE_NONE, L"(" + _("None") + L")"). //meta options should be enclosed in parentheses
    add(USIZE_BYTE, _("Byte")).
    add(USIZE_KB,   _("KB")).
    add(USIZE_MB,   _("MB"));

    m_bitmapFilter->SetBitmap(getResourceImage(L"filter"));

    setFilter(filter);

    m_buttonOk->SetFocus();

    GetSizer()->SetSizeHints(this); //~=Fit() + SetMinSize()
    //=> works like a charm for GTK2 with window resizing problems and title bar corruption; e.g. Debian!!!

    Layout();
}


void FilterDlg::onKeyEvent(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();

    if (event.ControlDown())
        switch (keyCode)
        {
            case 'A': //CTRL + A
                if (auto textCtrl = dynamic_cast<wxTextCtrl*>(event.GetEventObject()))
                    textCtrl->SetSelection(-1, -1); //select all
                return;
        }
    event.Skip();
}


void FilterDlg::updateGui()
{
    FilterConfig activeCfg = getFilter();

    auto setStatusBitmap = [&](wxStaticBitmap& staticBmp, const wxString& bmpName, bool active)
    {
        if (active)
            staticBmp.SetBitmap(getResourceImage(bmpName));
        else
            staticBmp.SetBitmap(greyScale(getResourceImage(bmpName)));
    };
    setStatusBitmap(*m_bitmapInclude,    L"filter_include", !NameFilter::isNull(activeCfg.includeFilter, FilterConfig().excludeFilter));
    setStatusBitmap(*m_bitmapExclude,    L"filter_exclude", !NameFilter::isNull(FilterConfig().includeFilter, activeCfg.excludeFilter));
    setStatusBitmap(*m_bitmapFilterDate, L"clock", activeCfg.unitTimeSpan != UTIME_NONE);
    setStatusBitmap(*m_bitmapFilterSize, L"size", activeCfg.unitSizeMin != USIZE_NONE || activeCfg.unitSizeMax != USIZE_NONE);

    m_spinCtrlTimespan->Enable(activeCfg.unitTimeSpan == UTIME_LAST_X_DAYS);
    m_spinCtrlMinSize ->Enable(activeCfg.unitSizeMin != USIZE_NONE);
    m_spinCtrlMaxSize ->Enable(activeCfg.unitSizeMax != USIZE_NONE);

    m_buttonClear->Enable(!(activeCfg == FilterConfig()));
}


void FilterDlg::setFilter(const FilterConfig& filter)
{
    m_textCtrlInclude->ChangeValue(utfCvrtTo<wxString>(filter.includeFilter));
    m_textCtrlExclude->ChangeValue(utfCvrtTo<wxString>(filter.excludeFilter));

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
    return FilterConfig(utfCvrtTo<Zstring>(m_textCtrlInclude->GetValue()),
                        utfCvrtTo<Zstring>(m_textCtrlExclude->GetValue()),
                        m_spinCtrlTimespan->GetValue(),
                        getEnumVal(enumTimeDescr, *m_choiceUnitTimespan),
                        m_spinCtrlMinSize->GetValue(),
                        getEnumVal(enumSizeDescr, *m_choiceUnitMinSize),
                        m_spinCtrlMaxSize->GetValue(),
                        getEnumVal(enumSizeDescr, *m_choiceUnitMaxSize));
}


void FilterDlg::OnClear(wxCommandEvent& event)
{
    setFilter(FilterConfig());
}


void FilterDlg::OnOkay(wxCommandEvent& event)
{
    FilterConfig cfg = getFilter();

    //parameter validation:

    //include filter must not be empty:
    {
        Zstring tmp = cfg.includeFilter;
        trim(tmp);
        if (tmp.empty())
            cfg.includeFilter = FilterConfig().includeFilter; //no need to show error message, just correct user input
    }

    //apply config:
    outputRef = cfg;

    //when leaving dialog: filter and redraw grid, if filter is active
    EndModal(ReturnSmallDlg::BUTTON_OKAY);
}


ReturnSmallDlg::ButtonPressed zen::showFilterDialog(wxWindow* parent, FilterConfig& filter, const wxString& title)
{
    FilterDlg filterDlg(parent,
                        filter,
                        title);
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
    virtual void OnOK(wxCommandEvent& event);
    virtual void OnCancel(wxCommandEvent& event) { EndModal(ReturnSmallDlg::BUTTON_CANCEL); }
    virtual void OnClose (wxCloseEvent&   event) { EndModal(ReturnSmallDlg::BUTTON_CANCEL); }
    virtual void OnDelOnBothSides(wxCommandEvent& event);
    virtual void OnUseRecycler(wxCommandEvent& event);

    void updateGui();

    const std::vector<zen::FileSystemObject*>& rowsToDeleteOnLeft;
    const std::vector<zen::FileSystemObject*>& rowsToDeleteOnRight;
    bool& outRefdeleteOnBothSides;
    bool& outRefuseRecycleBin;
    const TickVal tickCountStartup;
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
    outRefuseRecycleBin(useRecycleBin),
    tickCountStartup(getTicks())
{
#ifdef ZEN_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif
    setStandardButtonOrder(*bSizerStdButtons, StdButtons().setAffirmative(m_buttonOK).setCancel(m_buttonCancel));

    setMainInstructionFont(*m_staticTextHeader);

    m_checkBoxDeleteBothSides->SetValue(deleteOnBothSides);
    m_checkBoxUseRecycler->SetValue(useRecycleBin);

    //if both sides contain same rows this checkbox is superfluous
    if (rowsToDeleteOnLeft == rowsToDeleteOnRight)
    {
        m_checkBoxDeleteBothSides->Show(false);
        m_checkBoxDeleteBothSides->SetValue(true);
    }

#ifndef __WXGTK__  //wxWidgets holds portability promise by not supporting for multi-line controls...not
    m_textCtrlFileList->SetMaxLength(0); //allow large entries!
#endif

    updateGui();

    GetSizer()->SetSizeHints(this); //~=Fit() + SetMinSize()
    //=> works like a charm for GTK2 with window resizing problems and title bar corruption; e.g. Debian!!!

    Layout();

    m_buttonOK->SetFocus();
}


void DeleteDialog::updateGui()
{
#ifdef ZEN_WIN
    wxWindowUpdateLocker dummy(this); //leads to GUI corruption problems on Linux/OS X!
#endif

    const std::pair<Zstring, int> delInfo = zen::deleteFromGridAndHDPreview(rowsToDeleteOnLeft,
                                                                            rowsToDeleteOnRight,
                                                                            m_checkBoxDeleteBothSides->GetValue());
    wxString header;
    if (m_checkBoxUseRecycler->GetValue())
    {
        header = _P("Do you really want to move the following item to the recycle bin?",
                    "Do you really want to move the following %x items to the recycle bin?", delInfo.second);
        m_bitmapDeleteType->SetBitmap(getResourceImage(L"delete_recycler"));
        m_buttonOK->SetLabel(_("Move")); //no access key needed: use ENTER!
    }
    else
    {
        header = _P("Do you really want to delete the following item?",
                    "Do you really want to delete the following %x items?", delInfo.second);
        m_bitmapDeleteType->SetBitmap(getResourceImage(L"delete_permanently"));
        m_buttonOK->SetLabel(_("Delete"));
    }
    m_staticTextHeader->SetLabel(header);
    //it seems like Wrap() needs to be reapplied after SetLabel()
    m_staticTextHeader->Wrap(460);

    const wxString& fileList = utfCvrtTo<wxString>(delInfo.first);
    m_textCtrlFileList->ChangeValue(fileList);
    /*
    There is a nasty bug on wxGTK under Ubuntu: If a multi-line wxTextCtrl contains so many lines that scrollbars are shown,
    it re-enables all windows that are supposed to be disabled during the current modal loop!
    This only affects Ubuntu/wxGTK! No such issue on Debian/wxGTK or Suse/wxGTK
    => another Unity problem like the following?
    http://trac.wxwidgets.org/ticket/14823 "Menu not disabled when showing modal dialogs in wxGTK under Unity"
    */

    Layout();
    Refresh(); //needed after m_buttonOK label change
}


void DeleteDialog::OnOK(wxCommandEvent& event)
{
    //additional safety net, similar to Windows Explorer: time delta between DEL and ENTER must be at least 50ms to avoid accidental deletion!
    const TickVal now = getTicks();   //0 on error
    std::int64_t tps = ticksPerSec(); //
    if (now.isValid() && tickCountStartup.isValid() && tps != 0)
        if (dist(tickCountStartup, now) * 1000 / tps < 50)
            return;

    outRefuseRecycleBin = m_checkBoxUseRecycler->GetValue();
    if (rowsToDeleteOnLeft != rowsToDeleteOnRight)
        outRefdeleteOnBothSides = m_checkBoxDeleteBothSides->GetValue();

    EndModal(ReturnSmallDlg::BUTTON_OKAY);
}

void DeleteDialog::OnDelOnBothSides(wxCommandEvent& event)
{
    updateGui();
}

void DeleteDialog::OnUseRecycler(wxCommandEvent& event)
{
    updateGui();
}


ReturnSmallDlg::ButtonPressed zen::showDeleteDialog(wxWindow* parent,
                                                    const std::vector<zen::FileSystemObject*>& rowsOnLeft,
                                                    const std::vector<zen::FileSystemObject*>& rowsOnRight,
                                                    bool& deleteOnBothSides,
                                                    bool& useRecycleBin)
{
    DeleteDialog confirmDeletion(parent,
                                 rowsOnLeft,
                                 rowsOnRight,
                                 deleteOnBothSides,
                                 useRecycleBin);
    return static_cast<ReturnSmallDlg::ButtonPressed>(confirmDeletion.ShowModal());
}

//########################################################################################

class SyncConfirmationDlg : public SyncConfirmationDlgGenerated
{
public:
    SyncConfirmationDlg(wxWindow* parent,
                        const wxString& variantName,
                        const zen::SyncStatistics& st,
                        bool& dontShowAgain);
private:
    virtual void OnClose (wxCloseEvent&   event) { EndModal(ReturnSmallDlg::BUTTON_CANCEL); }
    virtual void OnCancel(wxCommandEvent& event) { EndModal(ReturnSmallDlg::BUTTON_CANCEL); }
    virtual void OnStartSync(wxCommandEvent& event);

    bool& m_dontShowAgain;
};


SyncConfirmationDlg::SyncConfirmationDlg(wxWindow* parent,
                                         const wxString& variantName,
                                         const SyncStatistics& st,
                                         bool& dontShowAgain) :
    SyncConfirmationDlgGenerated(parent),
    m_dontShowAgain(dontShowAgain)
{
#ifdef ZEN_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif
    setStandardButtonOrder(*bSizerStdButtons, StdButtons().setAffirmative(m_buttonStartSync).setCancel(m_buttonCancel));

    setMainInstructionFont(*m_staticTextHeader);
    m_bitmapSync->SetBitmap(getResourceImage(L"sync"));

    m_staticTextVariant->SetLabel(variantName);
    m_checkBoxDontShowAgain->SetValue(dontShowAgain);

    //update preview of item count and bytes to be transferred:
    setText(*m_staticTextData, filesizeToShortString(st.getDataToProcess()));
    if (st.getDataToProcess() == 0)
        m_bitmapData->SetBitmap(greyScale(getResourceImage(L"data")));
    else
        m_bitmapData->SetBitmap(getResourceImage(L"data"));

    auto setValue = [](wxStaticText& txtControl, int value, wxStaticBitmap& bmpControl, const wchar_t* bmpName)
    {
        setText(txtControl, toGuiString(value));

        if (value == 0)
            bmpControl.SetBitmap(greyScale(mirrorIfRtl(getResourceImage(bmpName))));
        else
            bmpControl.SetBitmap(mirrorIfRtl(getResourceImage(bmpName)));
    };

    setValue(*m_staticTextCreateLeft,  st.getCreate<LEFT_SIDE >(), *m_bitmapCreateLeft,  L"so_create_left_small");
    setValue(*m_staticTextUpdateLeft,  st.getUpdate<LEFT_SIDE >(), *m_bitmapUpdateLeft,  L"so_update_left_small");
    setValue(*m_staticTextDeleteLeft,  st.getDelete<LEFT_SIDE >(), *m_bitmapDeleteLeft,  L"so_delete_left_small");
    setValue(*m_staticTextCreateRight, st.getCreate<RIGHT_SIDE>(), *m_bitmapCreateRight, L"so_create_right_small");
    setValue(*m_staticTextUpdateRight, st.getUpdate<RIGHT_SIDE>(), *m_bitmapUpdateRight, L"so_update_right_small");
    setValue(*m_staticTextDeleteRight, st.getDelete<RIGHT_SIDE>(), *m_bitmapDeleteRight, L"so_delete_right_small");

    m_panelStatistics->Layout();

    GetSizer()->SetSizeHints(this); //~=Fit() + SetMinSize()
    //=> works like a charm for GTK2 with window resizing problems and title bar corruption; e.g. Debian!!!

    m_buttonStartSync->SetFocus();
}


void SyncConfirmationDlg::OnStartSync(wxCommandEvent& event)
{
    m_dontShowAgain = m_checkBoxDontShowAgain->GetValue();
    EndModal(ReturnSmallDlg::BUTTON_OKAY);
}


ReturnSmallDlg::ButtonPressed zen::showSyncConfirmationDlg(wxWindow* parent,
                                                           const wxString& variantName,
                                                           const zen::SyncStatistics& statistics,
                                                           bool& dontShowAgain)
{
    SyncConfirmationDlg dlg(parent,
                            variantName,
                            statistics,
                            dontShowAgain);
    return static_cast<ReturnSmallDlg::ButtonPressed>(dlg.ShowModal());
}

//########################################################################################

class CompareCfgDialog : public CmpCfgDlgGenerated
{
public:
    CompareCfgDialog(wxWindow* parent, CompConfig& cmpConfig, const wxString& caption);

private:
    virtual void OnOkay(wxCommandEvent& event);
    virtual void OnClose (wxCloseEvent&   event) { EndModal(ReturnSmallDlg::BUTTON_CANCEL); }
    virtual void OnCancel(wxCommandEvent& event) { EndModal(ReturnSmallDlg::BUTTON_CANCEL); }
    virtual void OnHelpComparisonSettings(wxHyperlinkEvent& event) { displayHelpEntry(L"html/Comparison Settings.html", this); }

    virtual void OnTimeSize(wxCommandEvent& event) { compareVar = CMP_BY_TIME_SIZE; updateGui(); }
    virtual void OnContent (wxCommandEvent& event) { compareVar = CMP_BY_CONTENT;   updateGui(); }

    virtual void OnTimeSizeDouble(wxMouseEvent& event);
    virtual void OnContentDouble(wxMouseEvent& event);

    void updateGui();

    CompConfig& cmpConfigOut; //for output only
    CompareVariant compareVar;
    zen::EnumDescrList<SymLinkHandling>  enumDescrHandleSyml;
};


CompareCfgDialog::CompareCfgDialog(wxWindow* parent,
                                   CompConfig& cmpConfig, const wxString& title) :
    CmpCfgDlgGenerated(parent),
    cmpConfigOut(cmpConfig),
    compareVar(cmpConfig.compareVar)
{
#ifdef ZEN_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif
    setStandardButtonOrder(*bSizerStdButtons, StdButtons().setAffirmative(m_buttonOkay).setCancel(m_buttonCancel));

    setRelativeFontSize(*m_toggleBtnTimeSize, 1.25);
    setRelativeFontSize(*m_toggleBtnContent,  1.25);

    SetTitle(title);

    enumDescrHandleSyml.
    add(SYMLINK_EXCLUDE,      _("Exclude")).
    add(SYMLINK_USE_DIRECTLY, _("Direct")).
    add(SYMLINK_FOLLOW_LINK,  _("Follow"));

    setEnumVal(enumDescrHandleSyml, *m_choiceHandleSymlinks, cmpConfig.handleSymlinks);

    //move dialog up so that compare-config button and first config-variant are on same level
    //   Move(wxPoint(position.x, std::max(0, position.y - (m_buttonTimeSize->GetScreenPosition() - GetScreenPosition()).y)));

    updateGui();

    GetSizer()->SetSizeHints(this); //~=Fit() + SetMinSize()
    //=> works like a charm for GTK2 with window resizing problems and title bar corruption; e.g. Debian!!!

    m_buttonOkay->SetFocus();
}


void CompareCfgDialog::updateGui()
{
    //update toggle buttons -> they have no parameter-ownership at all!
    m_toggleBtnTimeSize->SetValue(false);
    m_toggleBtnContent ->SetValue(false);

    switch (compareVar)
    {
        case CMP_BY_TIME_SIZE:
            m_toggleBtnTimeSize->SetValue(true);
            break;
        case CMP_BY_CONTENT:
            m_toggleBtnContent->SetValue(true);
            break;
    }

    auto setBitmap = [](wxStaticBitmap& bmpCtrl, bool active, const wxBitmap& bmp)
    {
        if (active)
            bmpCtrl.SetBitmap(bmp);
        else
            bmpCtrl.SetBitmap(greyScale(bmp));
    };
    setBitmap(*m_bitmapByTime,    compareVar == CMP_BY_TIME_SIZE, getResourceImage(L"clock"));
    setBitmap(*m_bitmapByContent, compareVar == CMP_BY_CONTENT,   getResourceImage(L"cmpByContent"));
}


void CompareCfgDialog::OnOkay(wxCommandEvent& event)
{
    cmpConfigOut.compareVar = compareVar;
    cmpConfigOut.handleSymlinks = getEnumVal(enumDescrHandleSyml, *m_choiceHandleSymlinks);

    EndModal(ReturnSmallDlg::BUTTON_OKAY);
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


ReturnSmallDlg::ButtonPressed zen::showCompareCfgDialog(wxWindow* parent, CompConfig& cmpConfig, const wxString& title)
{
    CompareCfgDialog syncDlg(parent, cmpConfig, title);
    return static_cast<ReturnSmallDlg::ButtonPressed>(syncDlg.ShowModal());
}

//########################################################################################

class GlobalSettingsDlg : public GlobalSettingsDlgGenerated
{
public:
    GlobalSettingsDlg(wxWindow* parent, xmlAccess::XmlGlobalSettings& globalSettings);

private:
    virtual void OnOkay(wxCommandEvent& event);
    virtual void OnResetDialogs(wxCommandEvent& event);
    virtual void OnDefault(wxCommandEvent& event);
    virtual void OnCancel(wxCommandEvent& event) { EndModal(ReturnSmallDlg::BUTTON_CANCEL); }
    virtual void OnClose (wxCloseEvent&   event) { EndModal(ReturnSmallDlg::BUTTON_CANCEL); }
    virtual void OnAddRow(wxCommandEvent& event);
    virtual void OnRemoveRow(wxCommandEvent& event);
    virtual void OnHelpShowExamples(wxHyperlinkEvent& event) { displayHelpEntry(L"html/External Applications.html", this); }
    void onResize(wxSizeEvent& event);
    void updateGui();

    virtual void OnToggleAutoRetryCount(wxCommandEvent& event) { updateGui(); }

    void setExtApp(const xmlAccess::ExternalApps& extApp);
    xmlAccess::ExternalApps getExtApp();

    xmlAccess::XmlGlobalSettings& settings;
};


GlobalSettingsDlg::GlobalSettingsDlg(wxWindow* parent, xmlAccess::XmlGlobalSettings& globalSettings) :
    GlobalSettingsDlgGenerated(parent),
    settings(globalSettings)
{
#ifdef ZEN_WIN
    new zen::MouseMoveWindow(*this); //allow moving dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif
    setStandardButtonOrder(*bSizerStdButtons, StdButtons().setAffirmative(m_buttonOkay).setCancel(m_buttonCancel));

    //setMainInstructionFont(*m_staticTextHeader);

    m_bitmapSettings    ->SetBitmap     (getResourceImage(L"settings"));
    m_bpButtonAddRow    ->SetBitmapLabel(getResourceImage(L"item_add"));
    m_bpButtonRemoveRow ->SetBitmapLabel(getResourceImage(L"item_remove"));
    setBitmapTextLabel(*m_buttonResetDialogs, getResourceImage(L"reset_dialogs").ConvertToImage(), m_buttonResetDialogs->GetLabel());

    m_checkBoxFailSafe       ->SetValue(globalSettings.failsafeFileCopy);
    m_checkBoxCopyLocked     ->SetValue(globalSettings.copyLockedFiles);
    m_checkBoxCopyPermissions->SetValue(globalSettings.copyFilePermissions);

    m_spinCtrlAutoRetryCount->SetValue(globalSettings.automaticRetryCount);
    m_spinCtrlAutoRetryDelay->SetValue(globalSettings.automaticRetryDelay);

    setExtApp(globalSettings.gui.externelApplications);

    updateGui();

#ifdef ZEN_WIN
    m_checkBoxCopyPermissions->SetLabel(_("Copy NTFS permissions"));
#elif defined ZEN_LINUX || defined ZEN_MAC
    bSizerLockedFiles->Show(false);
#endif

    const wxString toolTip = wxString(_("Integrate external applications into context menu. The following macros are available:")) + L"\n\n" +
                             L"%item_path%    \t" + _("- full file or folder name") + L"\n" +
                             L"%item_folder%  \t" + _("- folder part only") + L"\n" +
                             L"%item2_path%   \t" + _("- Other side's counterpart to %item_path%") + L"\n" +
                             L"%item2_folder% \t" + _("- Other side's counterpart to %item_folder%");

    m_gridCustomCommand->GetGridWindow()->SetToolTip(toolTip);
    m_gridCustomCommand->GetGridColLabelWindow()->SetToolTip(toolTip);
    m_gridCustomCommand->SetMargins(0, 0);

    GetSizer()->SetSizeHints(this); //~=Fit() + SetMinSize()
    //=> works like a charm for GTK2 with window resizing problems and title bar corruption; e.g. Debian!!!

    Layout();

    //automatically fit column width to match total grid width
    Connect(wxEVT_SIZE, wxSizeEventHandler(GlobalSettingsDlg::onResize), nullptr, this);
    wxSizeEvent dummy;
    onResize(dummy);

    m_buttonOkay->SetFocus();
}


void GlobalSettingsDlg::onResize(wxSizeEvent& event)
{
    const int widthTotal = m_gridCustomCommand->GetGridWindow()->GetClientSize().GetWidth();

    if (widthTotal >= 0 && m_gridCustomCommand->GetCols() == 2)
    {
        const int w0 = widthTotal * 2 / 5; //ratio 2 : 3
        const int w1 = widthTotal - w0;
        m_gridCustomCommand->SetColumnWidth(0, w0);
        m_gridCustomCommand->SetColumnWidth(1, w1);

        m_gridCustomCommand->Refresh(); //required on Ubuntu
    }

    event.Skip();
}


void GlobalSettingsDlg::updateGui()
{
    const bool autoRetryActive = m_spinCtrlAutoRetryCount->GetValue() > 0;
    m_staticTextAutoRetryDelay->Enable(autoRetryActive);
    m_spinCtrlAutoRetryDelay->Enable(autoRetryActive);
}


void GlobalSettingsDlg::OnOkay(wxCommandEvent& event)
{
    //write global settings only when okay-button is pressed!
    settings.failsafeFileCopy    = m_checkBoxFailSafe->GetValue();
    settings.copyLockedFiles     = m_checkBoxCopyLocked->GetValue();
    settings.copyFilePermissions = m_checkBoxCopyPermissions->GetValue();

    settings.automaticRetryCount = m_spinCtrlAutoRetryCount->GetValue();
    settings.automaticRetryDelay = m_spinCtrlAutoRetryDelay->GetValue();

    settings.gui.externelApplications = getExtApp();

    EndModal(ReturnSmallDlg::BUTTON_OKAY);
}


void GlobalSettingsDlg::OnResetDialogs(wxCommandEvent& event)
{
    switch (showConfirmationDialog(this, DialogInfoType::INFO,
                                   PopupDialogCfg().setMainInstructions(_("Restore all hidden windows and warnings?")),
                                   _("&Restore")))
    {
        case ConfirmationButton::DO_IT:
            settings.optDialogs.resetDialogs();
            break;
        case ConfirmationButton::CANCEL:
            break;
    }
}


void GlobalSettingsDlg::OnDefault(wxCommandEvent& event)
{
    xmlAccess::XmlGlobalSettings defaultCfg;

    m_checkBoxFailSafe       ->SetValue(defaultCfg.failsafeFileCopy);
    m_checkBoxCopyLocked     ->SetValue(defaultCfg.copyLockedFiles);
    m_checkBoxCopyPermissions->SetValue(defaultCfg.copyFilePermissions);

    m_spinCtrlAutoRetryCount->SetValue(defaultCfg.automaticRetryCount);
    m_spinCtrlAutoRetryDelay->SetValue(defaultCfg.automaticRetryDelay);

    setExtApp(defaultCfg.gui.externelApplications);

    updateGui();
}


void GlobalSettingsDlg::setExtApp(const xmlAccess::ExternalApps& extApp)
{
    auto extAppTmp = extApp;
    vector_remove_if(extAppTmp, [](decltype(extAppTmp[0])& entry) { return entry.first.empty() && entry.second.empty(); });

    extAppTmp.resize(extAppTmp.size() + 1); //append empty row to facilitate insertions

    const int rowCount = m_gridCustomCommand->GetNumberRows();
    if (rowCount > 0)
        m_gridCustomCommand->DeleteRows(0, rowCount);

    m_gridCustomCommand->AppendRows(static_cast<int>(extAppTmp.size()));
    for (auto it = extAppTmp.begin(); it != extAppTmp.end(); ++it)
    {
        const int row = it - extAppTmp.begin();
        m_gridCustomCommand->SetCellValue(row, 0, it->first);  //description
        m_gridCustomCommand->SetCellValue(row, 1, it->second); //commandline
    }
}


xmlAccess::ExternalApps GlobalSettingsDlg::getExtApp()
{
    xmlAccess::ExternalApps output;
    for (int i = 0; i < m_gridCustomCommand->GetNumberRows(); ++i)
    {
        auto description = copyStringTo<std::wstring>(m_gridCustomCommand->GetCellValue(i, 0));
        auto commandline = copyStringTo<std::wstring>(m_gridCustomCommand->GetCellValue(i, 1));

        if (!description.empty() || !commandline.empty())
            output.push_back(std::make_pair(description, commandline));
    }
    return output;
}


void GlobalSettingsDlg::OnAddRow(wxCommandEvent& event)
{
#ifdef ZEN_WIN
    wxWindowUpdateLocker dummy(this); //leads to GUI corruption problems on Linux/OS X!
#endif

    const int selectedRow = m_gridCustomCommand->GetGridCursorRow();
    if (0 <= selectedRow && selectedRow < m_gridCustomCommand->GetNumberRows())
        m_gridCustomCommand->InsertRows(selectedRow);
    else
        m_gridCustomCommand->AppendRows();
}


void GlobalSettingsDlg::OnRemoveRow(wxCommandEvent& event)
{
    if (m_gridCustomCommand->GetNumberRows() > 0)
    {
#ifdef ZEN_WIN
        wxWindowUpdateLocker dummy(this); //leads to GUI corruption problems on Linux/OS X!
#endif

        const int selectedRow = m_gridCustomCommand->GetGridCursorRow();
        if (0 <= selectedRow && selectedRow < m_gridCustomCommand->GetNumberRows())
            m_gridCustomCommand->DeleteRows(selectedRow);
        else
            m_gridCustomCommand->DeleteRows(m_gridCustomCommand->GetNumberRows() - 1);
    }
}


ReturnSmallDlg::ButtonPressed zen::showGlobalSettingsDlg(wxWindow* parent, xmlAccess::XmlGlobalSettings& globalSettings)
{
    GlobalSettingsDlg settingsDlg(parent, globalSettings);
    return static_cast<ReturnSmallDlg::ButtonPressed>(settingsDlg.ShowModal());
}

//########################################################################################

class SelectTimespanDlg : public SelectTimespanDlgGenerated
{
public:
    SelectTimespanDlg(wxWindow* parent, Int64& timeFrom, Int64& timeTo);

private:
    virtual void OnOkay(wxCommandEvent& event);
    virtual void OnCancel(wxCommandEvent& event) { EndModal(ReturnSmallDlg::BUTTON_CANCEL); }
    virtual void OnClose (wxCloseEvent&   event) { EndModal(ReturnSmallDlg::BUTTON_CANCEL); }

    virtual void OnChangeSelectionFrom(wxCalendarEvent& event)
    {
        if (m_calendarFrom->GetDate() > m_calendarTo->GetDate())
            m_calendarTo->SetDate(m_calendarFrom->GetDate());
    }
    virtual void OnChangeSelectionTo(wxCalendarEvent& event)
    {
        if (m_calendarFrom->GetDate() > m_calendarTo->GetDate())
            m_calendarFrom->SetDate(m_calendarTo->GetDate());
    }

    Int64& timeFrom_;
    Int64& timeTo_;
};


wxDateTime utcToLocalDateTime(time_t utcTime)
{
    //wxDateTime models local(!) time (in contrast to what documentation says), but this constructor takes time_t UTC
    return wxDateTime(utcTime);
}

time_t localDateTimeToUtc(const wxDateTime& localTime)
{
    return localTime.GetTicks();
}


SelectTimespanDlg::SelectTimespanDlg(wxWindow* parent, Int64& timeFrom, Int64& timeTo) :
    SelectTimespanDlgGenerated(parent),
    timeFrom_(timeFrom),
    timeTo_(timeTo)
{
#ifdef ZEN_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif
    setStandardButtonOrder(*bSizerStdButtons, StdButtons().setAffirmative(m_buttonOkay).setCancel(m_buttonCancel));

    long style = wxCAL_SHOW_HOLIDAYS | wxCAL_SHOW_SURROUNDING_WEEKS;

#ifdef ZEN_WIN
    DWORD firstDayOfWeek = 0;
    if (::GetLocaleInfo(LOCALE_USER_DEFAULT,     //__in   LCID Locale,
                        LOCALE_IFIRSTDAYOFWEEK | // first day of week specifier, 0-6, 0=Monday, 6=Sunday
                        LOCALE_RETURN_NUMBER,    //__in   LCTYPE LCType,
                        reinterpret_cast<LPTSTR>(&firstDayOfWeek),     //__out  LPTSTR lpLCData,
                        sizeof(firstDayOfWeek) / sizeof(TCHAR)) > 0 && //__in   int cchData
        firstDayOfWeek == 6)
        style |= wxCAL_SUNDAY_FIRST;
    else //default
#endif
        style |= wxCAL_MONDAY_FIRST;

    m_calendarFrom->SetWindowStyleFlag(style);
    m_calendarTo  ->SetWindowStyleFlag(style);

    //set default values
    if (timeTo_ == 0)
        timeTo_ = wxGetUTCTime(); //
    if (timeFrom_ == 0)
        timeFrom_ = timeTo_ - 7 * 24 * 3600; //default time span: one week from "now"

    m_calendarFrom->SetDate(utcToLocalDateTime(to<time_t>(timeFrom_)));
    m_calendarTo  ->SetDate(utcToLocalDateTime(to<time_t>(timeTo_)));

#if wxCHECK_VERSION(2, 9, 5)
    //doesn't seem to be a problem here:
#else
    //wxDatePickerCtrl::BestSize() does not respect year field and trims it, both wxMSW/wxGTK - why isn't there anybody testing this wxWidgets stuff???
    wxSize minSz = m_calendarFrom->GetBestSize();
    minSz.x += 30;
    m_calendarFrom->SetMinSize(minSz);
    m_calendarTo  ->SetMinSize(minSz);
#endif

    GetSizer()->SetSizeHints(this); //~=Fit() + SetMinSize()
    //=> works like a charm for GTK2 with window resizing problems and title bar corruption; e.g. Debian!!!

    m_buttonOkay->SetFocus();
}


void SelectTimespanDlg::OnOkay(wxCommandEvent& event)
{
    wxDateTime from = m_calendarFrom->GetDate();
    wxDateTime to   = m_calendarTo  ->GetDate();

    //align to full days
    from.ResetTime();
    to += wxTimeSpan::Day();
    to.ResetTime(); //reset local(!) time
    to -= wxTimeSpan::Second(); //go back to end of previous day

    timeFrom_ = localDateTimeToUtc(from);
    timeTo_   = localDateTimeToUtc(to);

    /*
    {
        time_t current = zen::to<time_t>(timeFrom_);
        struct tm* tdfewst = ::localtime(&current);
        int budfk = 3;
    }
    {
        time_t current = zen::to<time_t>(timeTo_);
        struct tm* tdfewst = ::localtime(&current);
        int budfk = 3;
    }
    */

    EndModal(ReturnSmallDlg::BUTTON_OKAY);
}


ReturnSmallDlg::ButtonPressed zen::showSelectTimespanDlg(wxWindow* parent, Int64& timeFrom, Int64& timeTo)
{
    SelectTimespanDlg timeSpanDlg(parent, timeFrom, timeTo);
    return static_cast<ReturnSmallDlg::ButtonPressed>(timeSpanDlg.ShowModal());
}
