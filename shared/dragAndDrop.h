// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef DRAGANDDROP_H_INCLUDED
#define DRAGANDDROP_H_INCLUDED

#include <wx/event.h>

class wxWindow;
class wxDirPickerCtrl;
class wxComboBox;
class wxTextCtrl;
class wxString;
class FFSFileDropEvent;
class wxCommandEvent;
class wxFileDirPickerEvent;


namespace FreeFileSync
{
//add drag and drop functionality, coordinating a wxWindow, wxDirPickerCtrl, and wxComboBox/wxTextCtrl

class DragDropOnMainDlg : private wxEvtHandler
{
public:
    DragDropOnMainDlg(wxWindow*        dropWindow1,
                      wxWindow*        dropWindow2,
                      wxDirPickerCtrl* dirPicker,
                      wxComboBox*      dirName);

    virtual ~DragDropOnMainDlg() {}

    virtual bool AcceptDrop(const wxString& dropName) = 0; //return true if drop should be processed

private:
    void OnFilesDropped(FFSFileDropEvent& event);
    void OnWriteDirManually(wxCommandEvent& event);
    void OnDirSelected(wxFileDirPickerEvent& event);

    const wxWindow*  dropWindow1_;
    const wxWindow*  dropWindow2_;
    wxDirPickerCtrl* dirPicker_;
    wxComboBox*      dirName_;
};


class DragDropOnDlg: private wxEvtHandler
{
public:
    DragDropOnDlg(wxWindow*        dropWindow,
                  wxDirPickerCtrl* dirPicker,
                  wxTextCtrl*      dirName);

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
