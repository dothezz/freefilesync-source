// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "search.h"
#include "guiGenerated.h"
#include <wx/msgdlg.h>
#include <wx/utils.h>


class SearchDlg : public SearchDialogGenerated
{
public:
    SearchDlg(wxWindow& parentWindow, wxString& searchText, bool& respectCase);

    enum ReturnCodes
    {
        BUTTON_OKAY = 1 //mustn't be 0
    };

private:
    void OnClose(wxCloseEvent& event);
    void OnCancel(wxCommandEvent& event);
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
    m_checkBoxMatchCase->SetValue(respectCase_);
    m_textCtrlSearchTxt->SetValue(searchText_);

    CentreOnParent(); //this requires a parent window!
}


void SearchDlg::OnClose(wxCloseEvent& event)
{
    EndModal(0);
}


void SearchDlg::OnCancel(wxCommandEvent& event)
{
    EndModal(0);
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
class FindInText
{
public:
    FindInText(const wxString& textToFind);
    bool found(const wxString& phrase) const;

private:
    wxString textToFind_;
};


template <>
FindInText<true>::FindInText(const wxString& textToFind) :
    textToFind_(textToFind) {}


template <>
inline
bool FindInText<true>::found(const wxString& phrase) const
{
    return phrase.Find(textToFind_) != wxNOT_FOUND;
}


template <>
FindInText<false>::FindInText(const wxString& textToFind) :
    textToFind_(textToFind)
{
    textToFind_.MakeUpper();
}


template <>
inline
bool FindInText<false>::found(const wxString& phrase) const
{
    wxString phraseTmp = phrase; //wxWidgets::MakeUpper() is inefficient!
    phraseTmp.MakeUpper();       //But performance is not THAT important for this high-level search functionality
    return phraseTmp.Find(textToFind_) != wxNOT_FOUND;
}
//###########################################################################################


template <bool respectCase>
std::pair<int, int> searchGrid(const wxGrid& grid,
                               const wxString& searchString,
                               bool fromBeginToCursor, //specify area to search
                               bool afterCursorToEnd)  //
{
    const int rowCount    = const_cast<wxGrid&>(grid).GetNumberRows();
    const int columnCount = const_cast<wxGrid&>(grid).GetNumberCols();

    //consistency checks on ints: wxGrid uses ints, so we have to use them, too
    if (rowCount <= 0 || columnCount <= 0)
        return std::make_pair(-1, -1);

    int cursorRow    = const_cast<wxGrid&>(grid).GetGridCursorRow();
    int cursorColumn = const_cast<wxGrid&>(grid).GetGridCursorCol();

    if (    cursorRow    < 0         ||
            cursorRow    >= rowCount ||
            cursorColumn < 0         ||
            cursorColumn >= columnCount)
    {
        //cursor not on valid position...
        cursorRow    = 0;
        cursorColumn = 0;
    }

    const FindInText<respectCase> searchTxt(searchString);

    if (fromBeginToCursor)
    {
        for (int row = 0; row < cursorRow; ++row)
            for (int col = 0; col < columnCount; ++col)
                if (searchTxt.found(const_cast<wxGrid&>(grid).GetCellValue(row, col)))
                    return std::make_pair(row, col);

        for (int col = 0; col <= cursorColumn; ++col)
            if (searchTxt.found(const_cast<wxGrid&>(grid).GetCellValue(cursorRow, col)))
                return std::make_pair(cursorRow, col);
    }

    if (afterCursorToEnd)
    {
        //begin search after cursor cell...
        for (int col = cursorColumn + 1; col < columnCount; ++col)
            if (searchTxt.found(const_cast<wxGrid&>(grid).GetCellValue(cursorRow, col)))
                return std::make_pair(cursorRow, col);

        for (int row = cursorRow + 1; row < rowCount; ++row)
            for (int col = 0; col < columnCount; ++col)
                if (searchTxt.found(const_cast<wxGrid&>(grid).GetCellValue(row, col)))
                    return std::make_pair(row, col);
    }

    return std::make_pair(-1, -1);
}


//syntactic sugar...
std::pair<int, int> searchGrid(const wxGrid& grid,
                               bool respectCase,
                               const wxString& searchString,
                               bool fromBeginToCursor, //specify area to search
                               bool afterCursorToEnd)  //
{
    return respectCase ?
           searchGrid<true>( grid, searchString, fromBeginToCursor, afterCursorToEnd) :
           searchGrid<false>(grid, searchString, fromBeginToCursor, afterCursorToEnd);
}


wxString lastSearchString; //this variable really is conceptionally global...


void executeSearch(bool forceShowDialog,
                   bool& respectCase,
                   wxWindow& parentWindow,
                   wxGrid& leftGrid,
                   wxGrid& rightGrid)
{
    bool searchDialogWasShown = false;

    if (forceShowDialog || lastSearchString.IsEmpty())
    {
        SearchDlg* searchDlg = new SearchDlg(parentWindow, lastSearchString, respectCase); //wxWidgets deletion handling -> deleted by parentWindow
        if (static_cast<SearchDlg::ReturnCodes>(searchDlg->ShowModal()) != SearchDlg::BUTTON_OKAY)
            return;

        searchDialogWasShown = true;
    }

    wxGrid* targetGrid = NULL;     //filled if match is found
    std::pair<int, int> targetPos; //
    {
        wxBusyCursor showHourGlass;

        const bool startLeft = wxWindow::FindFocus() != rightGrid.GetGridWindow();
        wxGrid& firstGrid  = startLeft ? leftGrid  : rightGrid;
        wxGrid& secondGrid = startLeft ? rightGrid : leftGrid;

        //begin with first grid after cursor
        targetGrid = &firstGrid;
        targetPos  = searchGrid(firstGrid, respectCase, lastSearchString, false, true);
        if (targetPos.first == -1)
        {
            //scan second grid completely
            targetGrid = &secondGrid;
            targetPos  = searchGrid(secondGrid, respectCase, lastSearchString, true, true);

            //scan first grid up to cursor
            if (targetPos.first == -1)
            {
                targetGrid = &firstGrid;
                targetPos  = searchGrid(firstGrid, respectCase, lastSearchString, true, false);
            }
        }
    }

    if (targetPos.first != -1 && targetPos.second != -1) //new position found
    {
        targetGrid->SetFocus();
        targetGrid->SetGridCursor(  targetPos.first, targetPos.second);
        targetGrid->SelectRow(      targetPos.first);
        targetGrid->MakeCellVisible(targetPos.first, targetPos.second);
    }
    else
    {
        wxString messageNotFound = _("Cannot find %x");
        messageNotFound.Replace(wxT("%x"), wxString(wxT("\"")) + lastSearchString + wxT("\""), false);
        wxMessageBox(messageNotFound, _("Find"), wxOK);

        //show search dialog again
        if (searchDialogWasShown)
            executeSearch(true, respectCase, parentWindow, leftGrid, rightGrid);
    }
}
//###########################################################################################


void FreeFileSync::startFind(wxWindow& parentWindow, wxGrid& leftGrid, wxGrid& rightGrid, bool& respectCase) //Strg + F
{
    executeSearch(true, respectCase, parentWindow, leftGrid, rightGrid);
}


void FreeFileSync::findNext(wxWindow& parentWindow, wxGrid& leftGrid, wxGrid& rightGrid, bool& respectCase)  //F3
{
    executeSearch(false, respectCase, parentWindow, leftGrid, rightGrid);
}
