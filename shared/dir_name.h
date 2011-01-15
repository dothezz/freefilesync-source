// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef DRAGANDDROP_H_INCLUDED
#define DRAGANDDROP_H_INCLUDED

#include <wx/event.h>
#include <vector>
#include <wx/sizer.h>
#include <wx/filepicker.h>
#include <wx/combobox.h>
#include "zstring.h"


class FFSFileDropEvent;
class wxCommandEvent;
class wxFileDirPickerEvent;

namespace ffs3
{
//handle drag and drop, tooltip, label and manual input, coordinating a wxWindow, wxDirPickerCtrl, and wxComboBox/wxTextCtrl

class DirectoryNameMainDlg : private wxEvtHandler
{
public:
    DirectoryNameMainDlg(wxWindow*         dropWindow1,
                         wxWindow*         dropWindow2,
                         wxDirPickerCtrl*  dirPicker,
                         wxComboBox*       dirName,
                         wxStaticBoxSizer* staticBox);

    virtual ~DirectoryNameMainDlg() {}

    Zstring getName() const;
    void setName(const Zstring& dirname);

    virtual bool AcceptDrop(const std::vector<wxString>& droppedFiles) = 0; //return true if drop should be processed

private:
    void OnFilesDropped(FFSFileDropEvent& event);
    void OnWriteDirManually(wxCommandEvent& event);
    void OnDirSelected(wxFileDirPickerEvent& event);

    const wxWindow*   dropWindow1_;
    const wxWindow*   dropWindow2_;
    wxDirPickerCtrl*  dirPicker_;
    wxComboBox*       dirName_;
    wxStaticBoxSizer* staticBox_;
};


class DirectoryName: private wxEvtHandler
{
public:
    DirectoryName(wxWindow*        dropWindow,
                  wxDirPickerCtrl* dirPicker,
                  wxTextCtrl*      dirName);

    Zstring getName() const;
    void setName(const Zstring& dirname);

private:
    void OnFilesDropped(FFSFileDropEvent& event);
    void OnWriteDirManually(wxCommandEvent& event);
    void OnDirSelected(wxFileDirPickerEvent& event);

    const wxWindow*  dropWindow_;
    wxDirPickerCtrl* dirPicker_;
    wxTextCtrl*      dirName_;
};
}


#endif // DRAGANDDROP_H_INCLUDED
