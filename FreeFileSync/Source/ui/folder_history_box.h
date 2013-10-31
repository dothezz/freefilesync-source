// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef CUSTOMCOMBOBOX_H_INCLUDED
#define CUSTOMCOMBOBOX_H_INCLUDED

#include <wx/combobox.h>
#include <memory>
#include <zen/zstring.h>
#include <zen/stl_tools.h>
#include <zen/utf.h>

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

    static const wxString separationLine() { return L"---------------------------------------------------------------------------------------------------------------"; }

    void addItem(const Zstring& dirname)
    {
        if (dirname.empty() || dirname == zen::utfCvrtTo<Zstring>(separationLine()))
            return;

        Zstring nameTmp = dirname;
        zen::trim(nameTmp);

        //insert new folder or put it to the front if already existing
        auto it = std::find_if(dirnames_.begin(), dirnames_.end(),
        [&](const Zstring& entry) { return ::EqualFilename()(entry, nameTmp); });

        if (it != dirnames_.end())
            dirnames_.erase(it);
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
        setValueAndUpdateList(dirname); //required for setting value correctly; Linux: ensure the dropdown is shown as being populated
    }

    // GetValue

private:
    void OnKeyEvent(wxKeyEvent& event);
    void OnRequireHistoryUpdate(wxEvent& event);
    void setValueAndUpdateList(const wxString& dirname);

    std::shared_ptr<FolderHistory> sharedHistory_;
};


#endif // CUSTOMCOMBOBOX_H_INCLUDED
