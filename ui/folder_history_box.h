// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef CUSTOMCOMBOBOX_H_INCLUDED
#define CUSTOMCOMBOBOX_H_INCLUDED

#include <wx/combobox.h>
#include <memory>
#include <zen/zstring.h>
#include <wx+/string_conv.h>
#include <zen/stl_tools.h>

//combobox with history function + functionality to delete items (DEL)


class FolderHistory
{
public:
    FolderHistory() : maxSize_(0) {}

    FolderHistory(const std::vector<Zstring>& dirnames, size_t maxSize) :
        maxSize_(maxSize),
        dirnames_(dirnames)
    {
        if (dirnames_.size() > maxSize_) //keep maximal size of history list
            dirnames_.resize(maxSize_);
    }

    const std::vector<Zstring>& getList() const { return dirnames_; }

    static const Zstring lineSeparator() { return Zstr("---------------------------------------------------------------------------------------------------------------"); }

    void addItem(const Zstring& dirname)
    {
        if (dirname.empty() || dirname == lineSeparator())
            return;

        Zstring nameTmp = dirname;
        zen::trim(nameTmp);

        //insert new folder or put it to the front if already existing
        auto iter = std::find_if(dirnames_.begin(), dirnames_.end(),
        [&](const Zstring& entry) { return ::EqualFilename()(entry, nameTmp); });

        if (iter != dirnames_.end())
            dirnames_.erase(iter);
        dirnames_.insert(dirnames_.begin(), nameTmp);

        if (dirnames_.size() > maxSize_) //keep maximal size of history list
            dirnames_.resize(maxSize_);
    }

    void delItem(const Zstring& dirname) { zen::vector_remove_if(dirnames_, [&](const Zstring& entry) { return ::EqualFilename()(entry, dirname); }); }

private:

    size_t maxSize_;
    std::vector<Zstring> dirnames_;
};


class FolderHistoryBox : public wxComboBox
{
public:
    FolderHistoryBox(wxWindow* parent,
                     wxWindowID id,
                     const wxString& value = wxEmptyString,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     int n = 0,
                     const wxString choices[] = nullptr,
                     long style = 0,
                     const wxValidator& validator = wxDefaultValidator,
                     const wxString& name = wxComboBoxNameStr);

    void init(const std::shared_ptr<FolderHistory>& sharedHistory) { sharedHistory_ = sharedHistory; }

    void setValue(const wxString& dirname)
    {
        setValueAndUpdateList(dirname); //required for setting value correctly + Linux to ensure the dropdown is shown as being populated
    }

    // GetValue

private:
    void OnKeyEvent(wxKeyEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnSelection(wxCommandEvent& event);
    void OnUpdateList(wxEvent& event) { setValueAndUpdateList(GetValue()); event.Skip(); }

    void setValueAndUpdateList(const wxString& dirname);

#if wxCHECK_VERSION(2, 9, 1)
    void OnShowDropDown(wxCommandEvent& event);
    void OnHideDropDown(wxCommandEvent& event);

    bool dropDownShown;
#endif

    std::shared_ptr<FolderHistory> sharedHistory_;
};


#endif // CUSTOMCOMBOBOX_H_INCLUDED