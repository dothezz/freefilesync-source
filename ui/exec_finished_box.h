// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef EXEC_FINISHED_BOX_18947773210473214
#define EXEC_FINISHED_BOX_18947773210473214

#include <vector>
#include <string>
#include <map>
#include <wx/combobox.h>
#include <zen/string_tools.h>


//combobox with history function + functionality to delete items (DEL)

//special command
bool isCloseProgressDlgCommand(const std::wstring& value);

void addValueToHistory(const std::wstring& value, std::vector<std::wstring>& history, size_t historyMax);


class ExecFinishedBox : public wxComboBox
{
public:
    ExecFinishedBox(wxWindow* parent,
                    wxWindowID id,
                    const wxString& value = wxEmptyString,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    int n = 0,
                    const wxString choices[] = NULL,
                    long style = 0,
                    const wxValidator& validator = wxDefaultValidator,
                    const wxString& name = wxComboBoxNameStr);

    void setHistoryRef(std::vector<std::wstring>& history) { history_ = &history; }

    // use these two accessors instead of GetValue()/SetValue():
    std::wstring getValue() const;
    void setValue(const std::wstring& value);
    //required for setting value correctly + Linux to ensure the dropdown is shown as being populated

private:
    void OnKeyEvent(wxKeyEvent& event);
    void OnMouseWheel(wxMouseEvent& event) {} //swallow! this gives confusing UI feedback anyway
    void OnSelection(wxCommandEvent& event);
    void OnReplaceBuiltInCmds(wxCommandEvent& event);
    void OnUpdateList(wxEvent& event);

    void setValueAndUpdateList(const std::wstring& value);

    std::vector<std::wstring>* history_;

    const std::vector<std::pair<std::wstring, std::wstring>> defaultCommands;
};


#endif //EXEC_FINISHED_BOX_18947773210473214
