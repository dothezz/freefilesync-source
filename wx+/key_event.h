// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef KEY_EVENT_H_086130871086708354674
#define KEY_EVENT_H_086130871086708354674

#include <functional>
#include <zen/scope_guard.h>
#include <wx/toplevel.h>
#include <wx/window.h>
#include <wx/event.h>

namespace zen
{
//wxWidgets provides no elegant way to register shortcut keys scoped for dialog windows =>
//enable dialog-specific local key events!

//setup in wxDialog-derived class' constructor, e.g.:
//	setupLocalKeyEvents(*this, [this](wxKeyEvent& event){ this->onLocalKeyEvent(event); });
//
// => redirects local key events to:
//	void MyDlg::onLocalKeyEvent(wxKeyEvent& event);
void setupLocalKeyEvents(wxWindow& wnd, const std::function<void(wxKeyEvent& event)>& callback); //callback held during life time of "wnd"!







//pretty much the same like "bool wxWindowBase::IsDescendant(wxWindowBase* child) const" but without the obvious misnaming
inline
bool isComponentOf(const wxWindow* child, const wxWindow* top)
{
    for (const wxWindow* wnd = child; wnd != nullptr; wnd = wnd->GetParent())
        if (wnd == top)
            return true;
    return false;
}


namespace impl
{
inline
const wxTopLevelWindow* getTopLevelWindow(const wxWindow* child)
{
    for (const wxWindow* wnd = child; wnd != nullptr; wnd = wnd->GetParent())
        if (auto tlw = dynamic_cast<const wxTopLevelWindow*>(wnd))
            return tlw;
    return nullptr;
}


class LokalKeyEventHandler : public wxWindow //private wxEvtHandler
{
public:
    LokalKeyEventHandler(wxWindow& parent, const std::function<void(wxKeyEvent& event)>& callback) : wxWindow(&parent, wxID_ANY), //use a dummy child window to bind instance life time to parent
        parent_(parent),
        callback_(callback),
        processingCallback(false)
    {
		    Hide();    //this is just a dummy window so that its parent can have ownership
            Disable(); //

        //register global hotkeys (without needing explicit menu entry)
        wxTheApp->Connect(wxEVT_KEY_DOWN,  wxKeyEventHandler(LokalKeyEventHandler::onGlobalKeyEvent), nullptr, this);
        wxTheApp->Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(LokalKeyEventHandler::onGlobalKeyEvent), nullptr, this); //capture direction keys
    }

    ~LokalKeyEventHandler()
    {
        //important! event source wxTheApp lives longer than this instance -> disconnect!
        wxTheApp->Disconnect(wxEVT_KEY_DOWN,  wxKeyEventHandler(LokalKeyEventHandler::onGlobalKeyEvent), nullptr, this);
        wxTheApp->Disconnect(wxEVT_CHAR_HOOK, wxKeyEventHandler(LokalKeyEventHandler::onGlobalKeyEvent), nullptr, this);
    }

private:
    void onGlobalKeyEvent(wxKeyEvent& event)
    {
        const wxWindow* focus = wxWindow::FindFocus();
        const wxTopLevelWindow* tlw = getTopLevelWindow(&parent_);

        //avoid recursion!!! -> this ugly construct seems to be the only (portable) way to avoid re-entrancy
        //recursion may happen in multiple situations: e.g. modal dialogs, Grid::ProcessEvent()!
        if (processingCallback ||
            !isComponentOf(focus, &parent_) ||
            !parent_.IsEnabled() || //only handle if window is in use and no modal dialog is shown:
            !tlw || !const_cast<wxTopLevelWindow*>(tlw)->IsActive())    //thanks to wxWidgets non-portability we need both checks:
            //IsEnabled() is sufficient for Windows, IsActive() is needed on OS X since it does NOT disable the parent when showing a modal dialog
        {
            event.Skip();
            return;
        }
        processingCallback = true;
        ZEN_ON_SCOPE_EXIT(processingCallback = false;)

        callback_(event);
    }

    wxWindow& parent_;
    const std::function<void(wxKeyEvent& event)> callback_;
    bool processingCallback;
};
}


inline
void setupLocalKeyEvents(wxWindow& wnd, const std::function<void(wxKeyEvent& event)>& callback)
{
    new impl::LokalKeyEventHandler(wnd, callback); //ownership passed to "wnd"!
}

}

#endif //KEY_EVENT_H_086130871086708354674
