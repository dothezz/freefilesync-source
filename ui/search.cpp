// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "search.h"
#include "gui_generated.h"
#include <wx/msgdlg.h>
#include <wx/utils.h>
#include <utility>
#include <wx+/mouse_move_dlg.h>

using namespace zen;


class SearchDlg : public SearchDialogGenerated
{
public:
    SearchDlg(wxWindow& parentWindow, wxString& searchText, bool& respectCase);

    enum ReturnCodes
    {
        BUTTON_OKAY = 1 //mustn't be 0
    };

private:
    void OnClose (wxCloseEvent&   event) { EndModal(0); }
    void OnCancel(wxCommandEvent& event) { EndModal(0); }
    void OnFindNext(wxCommandEvent& event);
    void OnText(wxCommandEvent& event);

    wxString& searchText_;
    bool& respectCase_;
};


SearchDlg::SearchDlg(wxWindow& parentWindow, wxString& searchText, bool& respectCase) :
    SearchDialogGenerated(&parentWindow),
    searchText_(searchText),
    respectCase_(respectCase)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    m_checkBoxMatchCase->SetValue(respectCase_);
    m_textCtrlSearchTxt->SetValue(searchText_);

    CentreOnParent(); //this requires a parent window!
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
    bool found(const wxString& phrase) const { return phrase.Find(textToFind_) != wxNOT_FOUND; }

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
        return phrase.Find(textToFind_) != wxNOT_FOUND;
    }

private:
    wxString textToFind_;
};

//###########################################################################################


template <bool respectCase>
ptrdiff_t findRow(const Grid& grid, //return -1 if no matching row found
                  size_t compPos,
                  const wxString& searchString,
                  size_t rowFirst, //specify area to search:
                  size_t rowLast)  // [rowFirst, rowLast)
{
    auto prov = grid.getDataProvider(compPos);
    std::vector<Grid::ColumnAttribute> colAttr = grid.getColumnConfig(compPos);
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
ptrdiff_t findRow(const Grid& grid,
                  size_t compPos,
                  bool respectCase,
                  const wxString& searchString,
                  size_t rowFirst, //specify area to search:
                  size_t rowLast)  // [rowFirst, rowLast)
{
    return respectCase ?
           findRow<true>( grid, compPos, searchString, rowFirst, rowLast) :
           findRow<false>(grid, compPos, searchString, rowFirst, rowLast);
}


wxString lastSearchString; //this variable really is conceptionally global...


void executeSearch(bool forceShowDialog,
                   bool& respectCase,
                   wxWindow& parentWindow,
                   Grid& grid,
                   size_t compPosLeft, size_t compPosRight)
{
    bool searchDialogWasShown = false;

    if (forceShowDialog || lastSearchString.IsEmpty())
    {
        SearchDlg searchDlg(parentWindow, lastSearchString, respectCase); //wxWidgets deletion handling -> deleted by parentWindow
        if (static_cast<SearchDlg::ReturnCodes>(searchDlg.ShowModal()) != SearchDlg::BUTTON_OKAY)
            return;

        searchDialogWasShown = true;
    }

    const size_t rowCount = grid.getRowCount();
    auto cursorPos = grid.getGridCursor(); //(row, component pos)

    size_t cursorRow = cursorPos.first;
    if (cursorRow >= rowCount)
        cursorRow = 0;

    if (cursorPos.second == compPosRight)
        std::swap(compPosLeft, compPosRight); //select side to start with
    else if (cursorPos.second != compPosLeft)
        cursorRow = 0;

    {
        wxBusyCursor showHourGlass;

        auto finishSearch = [&](size_t compPos, size_t rowFirst, size_t rowLast) -> bool
        {
            const ptrdiff_t targetRow = findRow(grid, compPos, respectCase, lastSearchString, rowFirst, rowLast);
            if (targetRow >= 0)
            {
                grid.setGridCursor(targetRow, compPos);
                grid.SetFocus();
                return true;
            }
            return false;
        };

        if (finishSearch(compPosLeft , cursorRow + 1, rowCount) ||
            finishSearch(compPosRight, 0, rowCount)             ||
            finishSearch(compPosLeft , 0, cursorRow + 1))
            return;
    }

    wxString messageNotFound = _("Cannot find %x");
    messageNotFound.Replace(wxT("%x"), wxString(wxT("\"")) + lastSearchString + wxT("\""), false);
    wxMessageBox(messageNotFound, _("Find"), wxOK);

    //show search dialog again
    if (searchDialogWasShown)
        executeSearch(true, respectCase, parentWindow, grid, compPosLeft, compPosRight);
}
//###########################################################################################


void zen::startFind(wxWindow& parentWindow, Grid& grid, size_t compPosLeft, size_t compPosRight, bool& respectCase) //Strg + F
{
    executeSearch(true, respectCase, parentWindow, grid, compPosLeft, compPosRight);
}


void zen::findNext(wxWindow& parentWindow, Grid& grid, size_t compPosLeft, size_t compPosRight, bool& respectCase)  //F3
{
    executeSearch(false, respectCase, parentWindow, grid, compPosLeft, compPosRight);
}
