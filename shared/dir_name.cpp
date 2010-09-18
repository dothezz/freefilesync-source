// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "dir_name.h"
#include <wx/dnd.h>
#include <wx/window.h>
#include <wx/textctrl.h>
#include <wx/filename.h>
#include "file_handling.h"
#include "string_conv.h"
#include "check_exist.h"
#include "util.h"
#include "system_constants.h"
#include <wx/statbox.h>


//define new event type
const wxEventType FFS_DROP_FILE_EVENT = wxNewEventType();

typedef void (wxEvtHandler::*FFSFileDropEventFunction)(FFSFileDropEvent&);

#define FFSFileDropEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(FFSFileDropEventFunction, &func)

class FFSFileDropEvent : public wxCommandEvent
{
public:
    FFSFileDropEvent(const std::vector<wxString>& filesDropped, const wxWindow* dropWindow) :
        wxCommandEvent(FFS_DROP_FILE_EVENT),
        filesDropped_(filesDropped),
        dropWindow_(dropWindow) {}

    virtual wxEvent* Clone() const
    {
        return new FFSFileDropEvent(filesDropped_, dropWindow_);
    }

    const std::vector<wxString> filesDropped_;
    const wxWindow* dropWindow_;
};


//##############################################################################################################
namespace
{
void setDirectoryNameImpl(const wxString& dirname, wxDirPickerCtrl* dirPicker, wxWindow& tooltipWnd, wxStaticBoxSizer* staticBox, size_t timeout)
{
    using namespace ffs3;

    const wxString dirFormatted = zToWx(getFormattedDirectoryName(wxToZ(dirname)));

    tooltipWnd.SetToolTip(dirFormatted);

    if (staticBox)
    {
        //change static box label only if there is a real difference to what is shown in wxTextCtrl anyway
        wxString dirNormalized = dirname;
        dirNormalized.Trim(true);
        dirNormalized.Trim(false);
        if (!dirNormalized.empty() && !dirNormalized.EndsWith(zToWx(common::FILE_NAME_SEPARATOR)))
            dirNormalized += zToWx(common::FILE_NAME_SEPARATOR);

        staticBox->GetStaticBox()->SetLabel(dirNormalized == dirFormatted ? wxString(_("Drag && drop")) : dirFormatted);
    }

    if (dirPicker)
    {
        if (util::dirExists(wxToZ(dirFormatted), timeout) == util::EXISTING_TRUE) //potentially slow network access: wait 200ms at most
            dirPicker->SetPath(dirFormatted);
    }
}


void setDirectoryName(const wxString&  dirname,
                      wxTextCtrl*      txtCtrl,
                      wxDirPickerCtrl* dirPicker,
                      wxWindow&        tooltipWnd,
                      size_t           timeout = 200) //pointers are optional
{
    if (txtCtrl)
        txtCtrl->ChangeValue(dirname);
    setDirectoryNameImpl(dirname, dirPicker, tooltipWnd, NULL, timeout);
}


void setDirectoryName(const wxString&   dirname,
                      wxComboBox*       comboBox,
                      wxDirPickerCtrl*  dirPicker,
                      wxWindow&         tooltipWnd,
                      wxStaticBoxSizer& staticBox,
                      size_t            timeout = 200) //pointers are optional
{
    if (comboBox)
    {
        comboBox->SetSelection(wxNOT_FOUND);
        comboBox->SetValue(dirname);
    }
    setDirectoryNameImpl(dirname, dirPicker, tooltipWnd, &staticBox, timeout);
}
}


//##############################################################################################################
class WindowDropTarget : public wxFileDropTarget
{
public:
    WindowDropTarget(wxWindow* dropWindow) :
        dropWindow_(dropWindow) {}

    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& fileArray)
    {
        std::vector<wxString> filenames;
        for (size_t i = 0; i < fileArray.GetCount(); ++i)
            filenames.push_back(fileArray[i]);

        if (!filenames.empty())
        {
            //create a custom event on drop window: execute event after file dropping is completed! (e.g. after mouse is released)
            FFSFileDropEvent evt(filenames, dropWindow_);
            dropWindow_->GetEventHandler()->AddPendingEvent(evt);
        }
        return false;
    }

private:
    wxWindow* dropWindow_;
};


//##############################################################################################################
using ffs3::DirectoryNameMainDlg;

DirectoryNameMainDlg::DirectoryNameMainDlg(
    wxWindow*         dropWindow1,
    wxWindow*         dropWindow2,
    wxDirPickerCtrl*  dirPicker,
    wxComboBox*       dirName,
    wxStaticBoxSizer* staticBox) :
    dropWindow1_(dropWindow1),
    dropWindow2_(dropWindow2),
    dirPicker_(dirPicker),
    dirName_(dirName),
    staticBox_(staticBox)
{
    //prepare drag & drop
    dropWindow1->SetDropTarget(new WindowDropTarget(dropWindow1)); //takes ownership
    dropWindow2->SetDropTarget(new WindowDropTarget(dropWindow2)); //takes ownership

    //redirect drag & drop event back to this class
    dropWindow1->Connect(FFS_DROP_FILE_EVENT, FFSFileDropEventHandler(DirectoryNameMainDlg::OnFilesDropped), NULL, this);
    dropWindow2->Connect(FFS_DROP_FILE_EVENT, FFSFileDropEventHandler(DirectoryNameMainDlg::OnFilesDropped), NULL, this);

    //keep dirPicker and dirName synchronous
    dirName->  Connect( wxEVT_COMMAND_TEXT_UPDATED,      wxCommandEventHandler(      DirectoryNameMainDlg::OnWriteDirManually), NULL, this );
    dirPicker->Connect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler(DirectoryNameMainDlg::OnDirSelected),      NULL, this );
}


void DirectoryNameMainDlg::OnFilesDropped(FFSFileDropEvent& event)
{
    if (event.filesDropped_.empty())
        return;

    if (    this->dropWindow1_ == event.dropWindow_ || //file may be dropped on window 1 or 2
            this->dropWindow2_ == event.dropWindow_)
    {
        if (AcceptDrop(event.filesDropped_))
        {
            wxString fileName = event.filesDropped_[0];
            if (wxDirExists(fileName))
                setDirectoryName(fileName, dirName_, dirPicker_, *dirName_, *staticBox_);
            else
            {
                fileName = wxFileName(fileName).GetPath();
                if (wxDirExists(fileName))
                    setDirectoryName(fileName, dirName_, dirPicker_, *dirName_, *staticBox_);
            }
        }
    }
}


void DirectoryNameMainDlg::OnWriteDirManually(wxCommandEvent& event)
{
    setDirectoryName(event.GetString(), NULL, dirPicker_, *dirName_, *staticBox_, 100); //potentially slow network access: wait 100 ms at most
    event.Skip();
}


void DirectoryNameMainDlg::OnDirSelected(wxFileDirPickerEvent& event)
{
    const wxString newPath = event.GetPath();
    setDirectoryName(newPath, dirName_, NULL, *dirName_, *staticBox_);

    event.Skip();
}


Zstring DirectoryNameMainDlg::getName() const
{
    return wxToZ(dirName_->GetValue());
}


void DirectoryNameMainDlg::setName(const Zstring& dirname)
{
    setDirectoryName(zToWx(dirname), dirName_, dirPicker_, *dirName_, *staticBox_);
}


//##############################################################################################################
using ffs3::DirectoryName;

DirectoryName::DirectoryName(wxWindow*        dropWindow,
                             wxDirPickerCtrl* dirPicker,
                             wxTextCtrl*      dirName) :
    dropWindow_(dropWindow),
    dirPicker_(dirPicker),
    dirName_(dirName)
{
    //prepare drag & drop
    dropWindow->SetDropTarget(new WindowDropTarget(dropWindow)); //takes ownership

    //redirect drag & drop event back to this class
    dropWindow->Connect(FFS_DROP_FILE_EVENT, FFSFileDropEventHandler(DirectoryName::OnFilesDropped), NULL, this);

    //keep dirPicker and dirName synchronous
    dirName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DirectoryName::OnWriteDirManually ), NULL, this );
    dirPicker->Connect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( DirectoryName::OnDirSelected ), NULL, this );
}


void DirectoryName::OnFilesDropped(FFSFileDropEvent& event)
{
    if (event.filesDropped_.empty())
        return;

    if (this->dropWindow_ == event.dropWindow_)
    {
        wxString fileName = event.filesDropped_[0];
        if (wxDirExists(fileName))
            setDirectoryName(fileName, dirName_, dirPicker_, *dirName_);
        else
        {
            fileName = wxFileName(fileName).GetPath();
            if (wxDirExists(fileName))
                setDirectoryName(fileName, dirName_, dirPicker_, *dirName_);
        }
    }
}


void DirectoryName::OnWriteDirManually(wxCommandEvent& event)
{
    setDirectoryName(event.GetString(), NULL, dirPicker_, *dirName_, 100); //potentially slow network access: wait 100 ms at most
    event.Skip();
}


void DirectoryName::OnDirSelected(wxFileDirPickerEvent& event)
{
    const wxString newPath = event.GetPath();
    setDirectoryName(newPath, dirName_, NULL, *dirName_);

    event.Skip();
}


Zstring DirectoryName::getName() const
{
    return wxToZ(dirName_->GetValue());
}


void DirectoryName::setName(const Zstring& dirname)
{
    setDirectoryName(zToWx(dirname), dirName_, dirPicker_, *dirName_);
}
