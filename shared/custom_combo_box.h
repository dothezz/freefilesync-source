// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef CUSTOMCOMBOBOX_H_INCLUDED
#define CUSTOMCOMBOBOX_H_INCLUDED

#include <wx/combobox.h>

//combobox with history function + functionality to delete items (DEL)

class CustomComboBox : public wxComboBox
{
public:
    CustomComboBox(wxWindow* parent,
                   wxWindowID id,
                   const wxString& value = wxEmptyString,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   int n = 0,
                   const wxString choices[] = NULL,
                   long style = 0,
                   const wxValidator& validator = wxDefaultValidator,
                   const wxString& name = wxComboBoxNameStr);

    void addPairToFolderHistory(const wxString& newFolder, unsigned int maxHistSize);

private:
    void OnKeyEvent(wxKeyEvent& event);

#if wxCHECK_VERSION(2, 9, 1)
    void OnShowDropDown(wxCommandEvent& event);
    void OnHideDropDown(wxCommandEvent& event);

    bool dropDownShown;
#endif
};


#endif // CUSTOMCOMBOBOX_H_INCLUDED
