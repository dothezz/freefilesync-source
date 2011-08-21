// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef DRAGANDDROP_H_INCLUDED
#define DRAGANDDROP_H_INCLUDED

#include <vector>
#include <wx/event.h>
#include <wx/sizer.h>
#include <wx/filepicker.h>
#include <wx/combobox.h>
#include "file_drop.h"


class wxFileDirPickerEvent;

namespace zen
{
//handle drag and drop, tooltip, label and manual input, coordinating a wxWindow, wxDirPickerCtrl, and wxComboBox/wxTextCtrl

class DirectoryNameMainDlg : private wxEvtHandler
{
public:
    DirectoryNameMainDlg(wxWindow&         dropWindow1,
                         wxWindow&         dropWindow2,
                         wxDirPickerCtrl&  dirPicker,
                         wxComboBox&       dirName,
                         wxStaticBoxSizer& staticBox);

    virtual ~DirectoryNameMainDlg() {}

    wxString getName() const;
    void setName(const wxString& dirname);

    virtual bool AcceptDrop(const std::vector<wxString>& droppedFiles) = 0; //return true if drop should be processed

private:
    void OnFilesDropped(FFSFileDropEvent& event);
    void OnWriteDirManually(wxCommandEvent& event);
    void OnDirSelected(wxFileDirPickerEvent& event);

    const wxWindow&   dropWindow1_;
    const wxWindow&   dropWindow2_;
    wxDirPickerCtrl&  dirPicker_;
    wxComboBox&       dirName_;
    wxStaticBoxSizer& staticBox_;
};


class DirectoryName: private wxEvtHandler
{
public:
    DirectoryName(wxWindow&        dropWindow,
                  wxDirPickerCtrl& dirPicker,
                  wxTextCtrl&      dirName,
                  wxStaticBoxSizer* staticBox = NULL); //optional

    wxString getName() const;
    void setName(const wxString& dirname);

private:
    void OnFilesDropped(FFSFileDropEvent& event);
    void OnWriteDirManually(wxCommandEvent& event);
    void OnDirSelected(wxFileDirPickerEvent& event);

    const wxWindow&  dropWindow_;
    wxDirPickerCtrl& dirPicker_;
    wxTextCtrl&      dirName_;
    wxStaticBoxSizer* staticBox_; //optional
};
}


#endif // DRAGANDDROP_H_INCLUDED
