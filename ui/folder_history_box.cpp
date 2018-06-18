// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "folder_history_box.h"
#include <list>
#include <zen/scope_guard.h>
//#include <wx/scrolwin.h>
#include <wx+/string_conv.h>
#include "../lib/resolve_path.h"

using namespace zen;


FolderHistoryBox::FolderHistoryBox(wxWindow* parent,
                                   wxWindowID id,
                                   const wxString& value,
                                   const wxPoint& pos,
                                   const wxSize& size,
                                   int n,
                                   const wxString choices[],
                                   long style,
                                   const wxValidator& validator,
                                   const wxString& name) :
    wxComboBox(parent, id, value, pos, size, n, choices, style, validator, name)
{
    //#####################################
    /*##*/ SetMinSize(wxSize(150, -1)); //## workaround yet another wxWidgets bug: default minimum size is much too large for a wxComboBox
    //#####################################

    Connect(wxEVT_KEY_DOWN,   wxKeyEventHandler  (FolderHistoryBox::OnKeyEvent  ), nullptr, this);

    warn_static("mac")
    warn_static("linux")

#if defined FFS_WIN
    //on Win, this mouse click event only fires, when clicking on the small down arrow, NOT when clicking on the text field
    //thanks to wxWidgets' non-portability it's exactly the converse on Linux!
    Connect(wxEVT_LEFT_DOWN, wxEventHandler(FolderHistoryBox::OnRequireHistoryUpdate), nullptr, this);
#elif defined FFS_LINUX || defined FFS_MAC
    /*
    we can't attach to wxEVT_COMMAND_TEXT_UPDATED, since setValueAndUpdateList() will implicitly emit wxEVT_COMMAND_TEXT_UPDATED again when calling Clear()!
    => Crash on Suse/X11/wxWidgets 2.9.4 on startup (setting a flag to guard against recursion does not work, still crash)

    On OS attaching to wxEVT_LEFT_DOWN leads to occasional crashes, especially when double-clicking
    */
#endif
}


void FolderHistoryBox::OnRequireHistoryUpdate(wxEvent& event)
{
    setValueAndUpdateList(GetValue());
    event.Skip();
}

//set value and update list are technically entangled: see potential bug description below
void FolderHistoryBox::setValueAndUpdateList(const wxString& dirname)
{
    //populate selection list....
    std::vector<wxString> dirList;
    {
        //add some aliases to allow user changing to volume name and back, if possible
        std::vector<Zstring> aliases = getDirectoryAliases(toZ(dirname));
        std::transform(aliases.begin(), aliases.end(), std::back_inserter(dirList), [](const Zstring& str) { return utfCvrtTo<wxString>(str); });
    }
    if (sharedHistory_.get())
    {
        std::vector<Zstring> tmp = sharedHistory_->getList();
        std::sort(tmp.begin(), tmp.end(), LessFilename());

        if (!dirList.empty() && !tmp.empty())
            dirList.push_back(FolderHistory::separationLine());

        std::transform(tmp.begin(), tmp.end(), std::back_inserter(dirList), [](const Zstring& str) { return utfCvrtTo<wxString>(str); });
    }

    //###########################################################################################

    //attention: if the target value is not part of the dropdown list, SetValue() will look for a string that *starts with* this value:
    //e.g. if the dropdown list contains "222" SetValue("22") will erroneously set and select "222" instead, while "111" would be set correctly!
    // -> by design on Windows!
    if (std::find(dirList.begin(), dirList.end(), dirname) == dirList.end())
        dirList.insert(dirList.begin(), dirname);

    Clear(); //emits yet another wxEVT_COMMAND_TEXT_UPDATED on Suse/X11/wxWidgets 2.9.4!!!
    std::for_each(dirList.begin(), dirList.end(), [&](const wxString& dir) { this->Append(dir); });
    //this->SetSelection(wxNOT_FOUND); //don't select anything
    this->SetValue(dirname);          //preserve main text!
}


void FolderHistoryBox::OnKeyEvent(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();
    if (keyCode == WXK_DELETE || keyCode == WXK_NUMPAD_DELETE)
    {
        //try to delete the currently selected config history item
        int pos = this->GetCurrentSelection();
        if (0 <= pos && pos < static_cast<int>(this->GetCount()) &&
            //what a mess...:
            (GetValue() != GetString(pos) || //avoid problems when a character shall be deleted instead of list item
             GetValue() == wxEmptyString)) //exception: always allow removing empty entry
        {
            //save old (selected) value: deletion seems to have influence on this
            const wxString currentVal = this->GetValue();
            //this->SetSelection(wxNOT_FOUND);

            //delete selected row
            if (sharedHistory_.get())
                sharedHistory_->delItem(toZ(GetString(pos)));
            SetString(pos, wxString()); //in contrast to Delete(), this one does not kill the drop-down list and gives a nice visual feedback!
            //Delete(pos);

            //(re-)set value
            this->SetValue(currentVal);

            //eat up key event
            return;
        }
    }
    event.Skip();
}
