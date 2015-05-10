// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef DIR_NAME_H_24857842375234523463425
#define DIR_NAME_H_24857842375234523463425

#include <zen/zstring.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx+/file_drop.h>
#include "folder_history_box.h"

namespace zen
{
//handle drag and drop, tooltip, label and manual input, coordinating a wxWindow, wxButton, and wxComboBox/wxTextCtrl
/*
Reasons NOT to use wxDirPickerCtrl, but wxButton instead:
	- Crash on GTK 2: http://favapps.wordpress.com/2012/06/11/freefilesync-crash-in-linux-when-syncing-solved/
	- still uses outdated ::SHBrowseForFolder() (even on Windows 7)
	- selection dialog remembers size, but NOT position => if user enlarges window, the next time he opens the dialog it may leap out of visible screen
	- hard-codes "Browse" button label
*/

extern const wxEventType EVENT_ON_DIR_SELECTED;			 //directory is changed by the user (except manual type-in)
extern const wxEventType EVENT_ON_DIR_MANUAL_CORRECTION; //manual type-in
//example: wnd.Connect(EVENT_ON_DIR_SELECTED, wxCommandEventHandler(MyDlg::OnDirSelected), nullptr, this);

class FolderSelector: public wxEvtHandler
{
public:
    FolderSelector(wxWindow&          dropWindow,
                   wxButton&          selectButton,
                   FolderHistoryBox&  dirpath,
                   wxStaticText*      staticText  = nullptr,  //optional
                   wxWindow*          dropWindow2 = nullptr); //

    ~FolderSelector();

    Zstring getPath() const;
    void setPath(const Zstring& dirpath);

private:
    virtual bool acceptDrop(const std::vector<wxString>& droppedFiles, const wxPoint& clientPos, const wxWindow& wnd) { return true; }; //return true if drop should be processed

    void onMouseWheel      (wxMouseEvent& event);
    void onFilesDropped    (FileDropEvent& event);
    void onWriteDirManually(wxCommandEvent& event);
    void onSelectDir       (wxCommandEvent& event);

    wxWindow&          dropWindow_;
    wxWindow*          dropWindow2_;
    wxButton&          selectButton_;
    FolderHistoryBox&  dirpath_;
    wxStaticText*      staticText_; //optional
};
}

#endif //DIR_NAME_H_24857842375234523463425
