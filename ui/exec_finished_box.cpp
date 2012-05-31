// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "exec_finished_box.h"
#include <deque>
#include <zen/i18n.h>
#include <algorithm>
#include <zen/stl_tools.h>
#ifdef FFS_WIN
#include <zen/win_ver.h>
#endif

using namespace zen;


namespace
{
const std::wstring closeProgressDlg = L"Close progress dialog"; //special command //mark for extraction: _("Close progress dialog")

const std::wstring separationLine(L"---------------------------------------------------------------------------------------------------------------");

std::vector<std::pair<std::wstring, std::wstring>> getDefaultCommands() //(gui name/command) pairs
{
    std::vector<std::pair<std::wstring, std::wstring>> output;

    auto addEntry = [&](const std::wstring& name, const std::wstring& value) { output.push_back(std::make_pair(name, value)); };

#ifdef FFS_WIN
    if (zen::vistaOrLater())
    {
        addEntry(_("Standby"  ), L"rundll32.exe powrprof.dll,SetSuspendState Sleep"); //suspend/Suspend to RAM/sleep
        addEntry(_("Log off"  ), L"shutdown /l");
        addEntry(_("Shut down"), L"shutdown /s /t 60");
        //addEntry(_("Hibernate"), L"shutdown /h"); //Suspend to disk -> Standby is better anyway
    }
    else //XP
    {
        addEntry(_("Standby"), L"rundll32.exe powrprof.dll,SetSuspendState"); //this triggers standby OR hibernate, depending on whether hibernate setting is active!
        addEntry(_("Log off"  ), L"shutdown -l");
        addEntry(_("Shut down"), L"shutdown -s -t 60");
        //no suspend on XP?
    }

#elif defined FFS_LINUX
    addEntry(_("Standby"  ), L"sudo pm-suspend");
    addEntry(_("Log off"  ), L"gnome-session-quit"); //alternative requiring admin: sudo killall Xorg
    addEntry(_("Shut down"), L"dbus-send --print-reply --dest=org.gnome.SessionManager /org/gnome/SessionManager org.gnome.SessionManager.RequestShutdown");
    //alternative requiring admin: sudo shutdown -h 1
    //addEntry(_("Hibernate"), L"sudo pm-hibernate");
    //alternative: "pmi action suspend" and "pmi action hibernate", require "sudo apt-get install powermanagement-interaface"

#endif
    return output;
}

const wxEventType wxEVT_REPLACE_BUILT_IN_COMMANDS = wxNewEventType();
}


bool isCloseProgressDlgCommand(const std::wstring& value)
{
    std::wstring tmp = value;
    trim(tmp);
    return tmp == closeProgressDlg;
}


void addValueToHistory(const std::wstring& value, std::vector<std::wstring>& history, size_t historyMax)
{
    std::wstring command = value;
    trim(command);

    bool skipCmd = command == separationLine   || //do not add sep. line
                   command == closeProgressDlg || //do not add special command
                   command.empty();

    //do not add built-in commands to history
    if (!skipCmd)
    {
        const auto& defaultCommands = getDefaultCommands();
        for (auto iter = defaultCommands.begin(); iter != defaultCommands.end(); ++iter)
            if (command == iter->first || command == iter->second)
            {
                skipCmd = true;
                break;
            }
    }

    if (!skipCmd)
        history.insert(history.begin(), command);

    if (history.size() > historyMax)
        history.resize(historyMax);
}


ExecFinishedBox::ExecFinishedBox(wxWindow* parent,
                                 wxWindowID id,
                                 const wxString& value,
                                 const wxPoint& pos,
                                 const wxSize& size,
                                 int n,
                                 const wxString choices[],
                                 long style,
                                 const wxValidator& validator,
                                 const wxString& name) :
    wxComboBox(parent, id, value, pos, size, n, choices, style, validator, name),
    history_(nullptr),
    defaultCommands(getDefaultCommands())
{
    //#####################################
    /*##*/ SetMinSize(wxSize(150, -1)); //## workaround yet another wxWidgets bug: default minimum size is much too large for a wxComboBox
    //#####################################

    Connect(wxEVT_KEY_DOWN,                  wxKeyEventHandler    (ExecFinishedBox::OnKeyEvent  ), nullptr, this);
    Connect(wxEVT_LEFT_DOWN,                 wxEventHandler       (ExecFinishedBox::OnUpdateList), nullptr, this);
    Connect(wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler(ExecFinishedBox::OnSelection ), nullptr, this);
    Connect(wxEVT_MOUSEWHEEL,                wxMouseEventHandler  (ExecFinishedBox::OnMouseWheel), nullptr, this);

    Connect(wxEVT_REPLACE_BUILT_IN_COMMANDS, wxCommandEventHandler(ExecFinishedBox::OnReplaceBuiltInCmds), nullptr, this);
}


std::wstring ExecFinishedBox::getValue() const
{
    const std::wstring value = zen::copyStringTo<std::wstring>(GetValue());

    {
        std::wstring tmp = value;
        trim(tmp);
        if (tmp == implementation::translate(closeProgressDlg)) //have this symbolic constant translated properly
            return closeProgressDlg;
    }

    return value;
}


void ExecFinishedBox::setValue(const std::wstring& value)
{
    std::wstring tmp = value;
    trim(tmp);

    if (tmp == closeProgressDlg)
        setValueAndUpdateList(implementation::translate(closeProgressDlg)); //have this symbolic constant translated properly
    else
        setValueAndUpdateList(value);
}

//set value and update list are technically entangled: see potential bug description below
void ExecFinishedBox::setValueAndUpdateList(const std::wstring& value)
{
    //it may be a little lame to update the list on each mouse-button click, but it should be working and we dont't have to manipulate wxComboBox internals

    std::deque<std::wstring> items;

    //1. special command
    items.push_back(implementation::translate(closeProgressDlg));

    //2. built in commands
    for (auto iter = defaultCommands.begin(); iter != defaultCommands.end(); ++iter)
        items.push_back(iter->first);

    //3. history elements
    if (history_ && !history_->empty())
    {
        items.push_back(separationLine);
        items.insert(items.end(), history_->begin(), history_->end());
        std::sort(items.end() - history_->size(), items.end());
    }

    //attention: if the target value is not part of the dropdown list, SetValue() will look for a string that *starts with* this value:
    //e.g. if the dropdown list contains "222" SetValue("22") will erroneously set and select "222" instead, while "111" would be set correctly!
    // -> by design on Windows!
    if (std::find(items.begin(), items.end(), value) == items.end())
    {
        if (!value.empty())
            items.push_front(separationLine);
        items.push_front(value);
    }

    Clear();
    std::for_each(items.begin(), items.end(), [&](const std::wstring& item) { this->Append(item); });
    //this->SetSelection(wxNOT_FOUND); //don't select anything
    SetValue(value);          //preserve main text!
}


void ExecFinishedBox::OnSelection(wxCommandEvent& event)
{
    wxCommandEvent dummy2(wxEVT_REPLACE_BUILT_IN_COMMANDS); //we cannot replace built-in commands at this position in call stack, so defer to a later time!
    if (auto handler = GetEventHandler())
        handler->AddPendingEvent(dummy2);

    event.Skip();
}


void ExecFinishedBox::OnReplaceBuiltInCmds(wxCommandEvent& event)
{
    const auto& value = getValue();

    if (value == separationLine)
        setValueAndUpdateList(std::wstring());
    else
        for (auto iter = defaultCommands.begin(); iter != defaultCommands.end(); ++iter)
            if (iter->first == value)
                return setValueAndUpdateList(iter->second); //replace GUI name by actual command string
}


void ExecFinishedBox::OnUpdateList(wxEvent& event)
{
    setValue(getValue());
    event.Skip();
}


void ExecFinishedBox::OnKeyEvent(wxKeyEvent& event)
{
    switch (event.GetKeyCode())
    {
        case WXK_DELETE:
        case WXK_NUMPAD_DELETE:
        {
            //try to delete the currently selected config history item
            int pos = this->GetCurrentSelection();
            if (0 <= pos && pos < static_cast<int>(this->GetCount()) &&
                //what a mess...:
                (GetValue() != GetString(pos) || //avoid problems when a character shall be deleted instead of list item
                 GetValue() == wxEmptyString)) //exception: always allow removing empty entry
            {
                const std::wstring selValue = copyStringTo<std::wstring>(GetString(pos));

                if (history_ && std::find(history_->begin(), history_->end(), selValue) != history_->end()) //only history elements may be deleted
                {
                    //save old (selected) value: deletion seems to have influence on this
                    const wxString currentVal = this->GetValue();
                    //this->SetSelection(wxNOT_FOUND);

                    //delete selected row
                    vector_remove_if(*history_, [&](const std::wstring& item) { return item == selValue; });

                    SetString(pos, wxString()); //in contrast to Delete(), this one does not kill the drop-down list and gives a nice visual feedback!
                    //Delete(pos);

                    //(re-)set value
                    SetValue(currentVal);
                }

                //eat up key event
                return;
            }
        }
        break;

        case WXK_UP:
        case WXK_NUMPAD_UP:
        case WXK_DOWN:
        case WXK_NUMPAD_DOWN:
        case WXK_PAGEUP:
        case WXK_NUMPAD_PAGEUP:
        case WXK_PAGEDOWN:
        case WXK_NUMPAD_PAGEDOWN:
            return; //swallow -> using these keys gives a weird effect due to this weird control
    }
    event.Skip();
}
