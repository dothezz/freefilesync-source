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
#include <wx+/file_drop.h>

namespace zen
{
//handle drag and drop, tooltip, label and manual input, coordinating a wxWindow, wxDirPickerCtrl, and wxComboBox/wxTextCtrl

template <class NameControl>  //NameControl may be wxTextCtrl, FolderHistoryBox
class DirectoryName: private wxEvtHandler
{
public:
    DirectoryName(wxWindow&         dropWindow,
                  wxDirPickerCtrl&  dirPicker,
                  NameControl&      dirName,
                  wxStaticBoxSizer* staticBox = NULL,
                  wxWindow*        dropWindow2 = NULL); //optional

    ~DirectoryName();

    wxString getName() const;
    void setName(const wxString& dirname);

private:
    virtual bool acceptDrop(const std::vector<wxString>& droppedFiles) { return true; }; //return true if drop should be processed

    void OnFilesDropped(FFSFileDropEvent& event);
    void OnWriteDirManually(wxCommandEvent& event);
    void OnDirSelected(wxFileDirPickerEvent& event);

    const wxWindow&   dropWindow_;
    const wxWindow*   dropWindow2_;
    wxDirPickerCtrl&  dirPicker_;
    NameControl&      dirName_;
    wxStaticBoxSizer* staticBox_; //optional
};
}


#endif // DRAGANDDROP_H_INCLUDED
