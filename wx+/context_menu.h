// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef CONTEXT_HEADER_18047302153418174632141234
#define CONTEXT_HEADER_18047302153418174632141234

#include <vector>
#include <functional>
#include <wx/menu.h>
#include <wx/app.h>

/*
A context menu supporting C++11 lambda callbacks!

Usage:
	ContextMenu menu;
	menu.addItem(L"Some Label", [&]{ ...do something... }); -> capture by reference is fine, as long as captured variables have at least scope of ContextMenu::show()!
	...
	menu.popup(wnd);
*/

namespace zen
{
class ContextMenu : private wxEvtHandler
{
public:
    void addItem(const wxString& label, const std::function<void()>& command, const wxBitmap* bmp = nullptr, bool enabled = true)
    {
        wxMenuItem* newItem = new wxMenuItem(&menu, wxID_ANY, label);
        if (bmp) newItem->SetBitmap(*bmp); //do not set AFTER appending item! wxWidgets screws up for yet another crappy reason
        menu.Append(newItem);
        if (!enabled) newItem->Enable(false); //do not enable BEFORE appending item! wxWidgets screws up for yet another crappy reason
        menu.Connect(newItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ContextMenu::onSelection), new GenericCommand(command), /*pass ownership*/ this);
    }

    void addCheckBox(const wxString& label, const std::function<void()>& command, bool checked, bool enabled = true)
    {
        wxMenuItem* newItem = menu.AppendCheckItem(wxID_ANY, label);
        newItem->Check(checked);
        if (!enabled) newItem->Enable(false);
        menu.Connect(newItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ContextMenu::onSelection), new GenericCommand(command), /*pass ownership*/ this);
    }

    void addRadio(const wxString& label, const std::function<void()>& command, bool checked, bool enabled = true)
    {
        wxMenuItem* newItem = menu.AppendRadioItem(wxID_ANY, label);
        newItem->Check(checked);
        if (!enabled) newItem->Enable(false);
        menu.Connect(newItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ContextMenu::onSelection), new GenericCommand(command), /*pass ownership*/ this);
    }

    void addSeparator() { menu.AppendSeparator(); }

    void popup(wxWindow& wnd) //show popup menu + process lambdas
    {
        wnd.PopupMenu(&menu);
        wxTheApp->ProcessPendingEvents(); //make sure lambdas are evaluated before going out of scope;
        //although all events seem to be processed within wxWindows::PopupMenu, we shouldn't trust wxWidgets in this regard
    }

private:
    void onSelection(wxCommandEvent& event)
    {
        if (auto cmd = dynamic_cast<GenericCommand*>(event.m_callbackUserData))
            (cmd->fun_)();
    }

    struct GenericCommand : public wxObject
    {
        GenericCommand(const std::function<void()>& fun) : fun_(fun) {}
        std::function<void()> fun_;
    };

    wxMenu menu;
};
}

#endif //CONTEXT_HEADER_18047302153418174632141234
