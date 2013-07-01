// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "search.h"
#include "gui_generated.h"
#include <wx/msgdlg.h>
#include <wx/utils.h>
#include <utility>
#include <zen/string_tools.h>
#include <wx+/mouse_move_dlg.h>

using namespace zen;


class SearchDlg : public SearchDialogGenerated
{
public:
    SearchDlg(wxWindow* parent, wxString& searchText, bool& respectCase);

    enum ReturnCodes
    {
        BUTTON_CANCEL,
        BUTTON_OKAY
    };

private:
    void OnClose (wxCloseEvent&   event) { EndModal(BUTTON_CANCEL); }
    void OnCancel(wxCommandEvent& event) { EndModal(BUTTON_CANCEL); }
    void OnFindNext(wxCommandEvent& event);
    void OnText(wxCommandEvent& event);

    wxString& searchText_;
    bool& respectCase_;
};


SearchDlg::SearchDlg(wxWindow* parent, wxString& searchText, bool& respectCase) :
    SearchDialogGenerated(parent),
    searchText_(searchText),
    respectCase_(respectCase)
{
#ifdef ZEN_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    m_checkBoxMatchCase->SetValue(respectCase_);
    m_textCtrlSearchTxt->SetValue(searchText_);

    m_textCtrlSearchTxt->SetFocus();
}


void SearchDlg::OnFindNext(wxCommandEvent& event)
{
    respectCase_ = m_checkBoxMatchCase->GetValue();
    searchText_  = m_textCtrlSearchTxt->GetValue();
    EndModal(BUTTON_OKAY);
}


void SearchDlg::OnText(wxCommandEvent& event)
{
    if (m_textCtrlSearchTxt->GetValue().Trim().IsEmpty())
        m_buttonFindNext->Disable();
    else
        m_buttonFindNext->Enable();

    event.Skip();
}
//###########################################################################################

template <bool respectCase>
class FindInText;

template <bool respectCase>
class FindInText
{
public:
    FindInText(const wxString& textToFind) : textToFind_(textToFind) {}
    bool found(const wxString& phrase) const { return contains(phrase, textToFind_); }

private:
    wxString textToFind_;
};


template <>
class FindInText<false>
{
public:
    FindInText(const wxString& textToFind) : textToFind_(textToFind) { textToFind_.MakeUpper(); }
    bool found(wxString&& phrase) const
    {
        //wxWidgets::MakeUpper() is inefficient! But performance is not THAT important for this high-level search functionality
        phrase.MakeUpper();
        return contains(phrase, textToFind_);
    }

private:
    wxString textToFind_;
};

//###########################################################################################

template <bool respectCase>
ptrdiff_t findRow(const Grid& grid, //return -1 if no matching row found
                  const wxString& searchString,
                  size_t rowFirst, //specify area to search:
                  size_t rowLast)  // [rowFirst, rowLast)
{
    auto prov = grid.getDataProvider();
    std::vector<Grid::ColumnAttribute> colAttr = grid.getColumnConfig();
    vector_remove_if(colAttr, [](const Grid::ColumnAttribute& ca) { return !ca.visible_; });
    if (!colAttr.empty() && prov)
    {
        const FindInText<respectCase> searchTxt(searchString);

        for (size_t row = rowFirst; row < rowLast; ++row)
            for (auto iterCol = colAttr.begin(); iterCol != colAttr.end(); ++iterCol)
                if (searchTxt.found(prov->getValue(row, iterCol->type_)))
                    return row;
    }
    return -1;
}


//syntactic sugar...
ptrdiff_t findRow(Grid& grid,
                  bool respectCase,
                  const wxString& searchString,
                  size_t rowFirst, //specify area to search:
                  size_t rowLast)  // [rowFirst, rowLast)
{
    return respectCase ?
           findRow<true>( grid, searchString, rowFirst, rowLast) :
           findRow<false>(grid, searchString, rowFirst, rowLast);
}


wxString lastSearchString; //this variable really is conceptionally global...


void executeSearch(bool forceShowDialog,
                   bool& respectCase,
                   wxWindow* parent,
                   Grid* gridL, Grid* gridR)
{
    bool searchDialogWasShown = false;

    if (forceShowDialog || lastSearchString.IsEmpty())
    {
        SearchDlg searchDlg(parent, lastSearchString, respectCase); //wxWidgets deletion handling -> deleted by parentWindow
        if (static_cast<SearchDlg::ReturnCodes>(searchDlg.ShowModal()) != SearchDlg::BUTTON_OKAY)
            return;

        searchDialogWasShown = true;
    }

    if (wxWindow::FindFocus() == &gridR->getMainWin())
        std::swap(gridL, gridR); //select side to start with

    const size_t rowCountL = gridL->getRowCount();
    const size_t rowCountR = gridR->getRowCount();
    auto cursorPos = gridL->getGridCursor(); //(row, component pos)

    size_t cursorRowL = cursorPos.first;
    if (cursorRowL >= rowCountL)
        cursorRowL = 0;
    {
        wxBusyCursor showHourGlass;

        auto finishSearch = [&](Grid& grid, size_t rowFirst, size_t rowLast) -> bool
        {
            const ptrdiff_t targetRow = findRow(grid, respectCase, lastSearchString, rowFirst, rowLast);
            if (targetRow >= 0)
            {
                //gridOther.clearSelection(); -> not needed other grids are automatically cleared after selection
                grid.setGridCursor(targetRow);
                grid.SetFocus();
                return true;
            }
            return false;
        };

        if (finishSearch(*gridL, cursorRowL + 1, rowCountL) ||
            finishSearch(*gridR, 0, rowCountR)              ||
            finishSearch(*gridL, 0, cursorRowL + 1))
            return;
    }

    wxMessageBox(replaceCpy(_("Cannot find %x"), L"%x", L"\"" + lastSearchString + L"\"", false), _("Find"), wxOK, parent);

    //show search dialog again
    if (searchDialogWasShown)
        executeSearch(true, respectCase, parent, gridL, gridR);
}
//###########################################################################################


void zen::startFind(wxWindow* parent, Grid& gridL, Grid& gridR, bool& respectCase) //Strg + F
{
    executeSearch(true, respectCase, parent, &gridL, &gridR);
}


void zen::findNext(wxWindow* parent, Grid& gridL, Grid& gridR, bool& respectCase)  //F3
{
    executeSearch(false, respectCase, parent, &gridL, &gridR);
}
