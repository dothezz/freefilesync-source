// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "folder_history_box.h"
#include "resolve_path.h"
#include <list>

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
#if wxCHECK_VERSION(2, 9, 1)
    , dropDownShown(false)
#endif
{
    //#####################################
    /*##*/ SetMinSize(wxSize(150, -1)); //## workaround yet another wxWidgets bug: default minimum size is much too large for a wxComboBox
    //#####################################

    //register key event to enable item deletion
    Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(FolderHistoryBox::OnKeyEvent), NULL, this);

    //refresh history list on mouse click
    Connect(wxEVT_LEFT_DOWN, wxEventHandler(FolderHistoryBox::OnUpdateList), NULL, this);

    Connect(wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler(FolderHistoryBox::OnSelection), NULL, this);

#if wxCHECK_VERSION(2, 9, 1)
    Connect(wxEVT_COMMAND_COMBOBOX_DROPDOWN, wxCommandEventHandler(FolderHistoryBox::OnShowDropDown), NULL, this);
    Connect(wxEVT_COMMAND_COMBOBOX_CLOSEUP, wxCommandEventHandler(FolderHistoryBox::OnHideDropDown), NULL, this);
#endif
}


#if wxCHECK_VERSION(2, 9, 1)
void FolderHistoryBox::OnShowDropDown(wxCommandEvent& event)
{
    dropDownShown = true;
    event.Skip();
}


void FolderHistoryBox::OnHideDropDown(wxCommandEvent& event)
{
    dropDownShown = false;
    event.Skip();
}
#endif


void FolderHistoryBox::update()
{
    std::list<Zstring> dirList;

    //add some aliases to allow user changing to volume name and back, if possible
#ifdef FFS_WIN
    const Zstring activePath = toZ(GetValue());
    std::vector<Zstring> aliases = getDirectoryAliases(activePath);
    dirList.insert(dirList.end(), aliases.begin(), aliases.end());
#endif

    if (sharedHistory_.get())
    {
        auto tmp = sharedHistory_->getList();
        //std::sort(tmp.begin(), tmp.end(), LessFilename());

        if (!dirList.empty() && !tmp.empty())
            dirList.push_back(FolderHistory::lineSeparator());

        dirList.insert(dirList.end(), tmp.begin(), tmp.end());
    }
    //###########################################################################################


    //it may be a little lame to update the list on each mouse-button click, but it should be working and we dont't have to manipulate wxComboBox internals
    const wxString oldVal = this->GetValue();

    Clear();
    std::for_each(dirList.begin(), dirList.end(),
    [&](const Zstring& dir) { this->Append(toWx(dir)); });

    this->SetSelection(wxNOT_FOUND); //don't select anything
    this->SetValue(oldVal);          //but preserve main text!
}


void FolderHistoryBox::OnSelection(wxCommandEvent& event)
{
    event.Skip();
}


void FolderHistoryBox::OnKeyEvent(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();
    if (keyCode == WXK_DELETE || keyCode == WXK_NUMPAD_DELETE)
    {
        //try to delete the currently selected config history item
        int pos = this->GetCurrentSelection();
        if (0 <= pos && pos < static_cast<int>(this->GetCount()) &&
#if wxCHECK_VERSION(2, 9, 1)
            dropDownShown)
#else
            //what a mess...:
            (GetValue() != GetString(pos) || //avoid problems when a character shall be deleted instead of list item
             GetValue() == wxEmptyString)) //exception: always allow removing empty entry
#endif
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
