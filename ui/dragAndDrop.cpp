#include "dragAndDrop.h"
#include <wx/dnd.h>
#include <wx/window.h>
#include <wx/combobox.h>
#include <wx/textctrl.h>
#include <wx/filepicker.h>
#include <wx/filename.h>
#include "../algorithm.h"


//define new event type
const wxEventType FFS_DROP_FILE_EVENT = wxNewEventType();

typedef void (wxEvtHandler::*FFSFileDropEventFunction)(FFSFileDropEvent&);

#define FFSFileDropEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(FFSFileDropEventFunction, &func)

class FFSFileDropEvent : public wxCommandEvent
{
public:
    FFSFileDropEvent(const wxString& nameDropped, const wxWindow* dropWindow) :
            wxCommandEvent(FFS_DROP_FILE_EVENT),
            nameDropped_(nameDropped),
            dropWindow_(dropWindow) {}

    virtual wxEvent* Clone() const
    {
        return new FFSFileDropEvent(nameDropped_, dropWindow_);
    }

    const wxString  nameDropped_;
    const wxWindow* dropWindow_;
};

//##############################################################################################################


class WindowDropTarget : public wxFileDropTarget
{
public:
    WindowDropTarget(wxWindow* dropWindow) :
            dropWindow_(dropWindow) {}

    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
    {
        if (!filenames.IsEmpty())
        {
            const wxString droppedFileName = filenames[0];

            //create a custom event on drop window: execute event after file dropping is completed! (e.g. after mouse is released)
            FFSFileDropEvent evt(droppedFileName, dropWindow_);
            dropWindow_->AddPendingEvent(evt);
        }
        return false;
    }

private:
    wxWindow* dropWindow_;
};



//##############################################################################################################

using FreeFileSync::DragDropOnMainDlg;

DragDropOnMainDlg::DragDropOnMainDlg(wxWindow*        dropWindow1,
                                     wxWindow*        dropWindow2,
                                     wxDirPickerCtrl* dirPicker,
                                     wxComboBox*      dirName) :
        dropWindow1_(dropWindow1),
        dropWindow2_(dropWindow2),
        dirPicker_(dirPicker),
        dirName_(dirName)
{
    //prepare drag & drop
    dropWindow1->SetDropTarget(new WindowDropTarget(dropWindow1)); //takes ownership
    dropWindow2->SetDropTarget(new WindowDropTarget(dropWindow2)); //takes ownership

    //redirect drag & drop event back to this class
    dropWindow1->Connect(FFS_DROP_FILE_EVENT, FFSFileDropEventHandler(DragDropOnMainDlg::OnFilesDropped), NULL, this);
    dropWindow2->Connect(FFS_DROP_FILE_EVENT, FFSFileDropEventHandler(DragDropOnMainDlg::OnFilesDropped), NULL, this);

    //keep dirPicker and dirName synchronous
    dirName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DragDropOnMainDlg::OnWriteDirManually ), NULL, this );
    dirPicker->Connect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( DragDropOnMainDlg::OnDirSelected ), NULL, this );
}


void DragDropOnMainDlg::OnFilesDropped(FFSFileDropEvent& event)
{
    if (    this->dropWindow1_ == event.dropWindow_ || //file may be dropped on window 1 or 2
            this->dropWindow2_ == event.dropWindow_)
    {
        if (AcceptDrop(event.nameDropped_))
        {
            wxString fileName = event.nameDropped_;
            if (wxDirExists(fileName))
            {
                dirName_->SetSelection(wxNOT_FOUND);
                dirName_->SetValue(fileName);
                dirPicker_->SetPath(fileName);
            }
            else
            {
                fileName = wxFileName(fileName).GetPath();
                if (wxDirExists(fileName))
                {
                    dirName_->SetSelection(wxNOT_FOUND);
                    dirName_->SetValue(fileName);
                    dirPicker_->SetPath(fileName);
                }
            }
        }
    }
    else //should never be reached
        event.Skip();
};


void DragDropOnMainDlg::OnWriteDirManually(wxCommandEvent& event)
{
    const wxString newDir = FreeFileSync::getFormattedDirectoryName(event.GetString().c_str()).c_str();
    if (wxDirExists(newDir))
        dirPicker_->SetPath(newDir);

    event.Skip();
}


void DragDropOnMainDlg::OnDirSelected(wxFileDirPickerEvent& event)
{
    const wxString newPath = event.GetPath();
    dirName_->SetSelection(wxNOT_FOUND);
    dirName_->SetValue(newPath);

    event.Skip();
}

//##############################################################################################################


using FreeFileSync::DragDropOnDlg;

DragDropOnDlg::DragDropOnDlg(wxWindow*        dropWindow,
                             wxDirPickerCtrl* dirPicker,
                             wxTextCtrl*      dirName) :
        dropWindow_(dropWindow),
        dirPicker_(dirPicker),
        dirName_(dirName)
{
    //prepare drag & drop
    dropWindow->SetDropTarget(new WindowDropTarget(dropWindow)); //takes ownership

    //redirect drag & drop event back to this class
    dropWindow->Connect(FFS_DROP_FILE_EVENT, FFSFileDropEventHandler(DragDropOnDlg::OnFilesDropped), NULL, this);

    //keep dirPicker and dirName synchronous
    dirName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DragDropOnDlg::OnWriteDirManually ), NULL, this );
    dirPicker->Connect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( DragDropOnDlg::OnDirSelected ), NULL, this );
}


void DragDropOnDlg::OnFilesDropped(FFSFileDropEvent& event)
{
    if (this->dropWindow_ == event.dropWindow_)
    {
        wxString fileName = event.nameDropped_;
        if (wxDirExists(fileName))
        {
            dirName_->SetValue(fileName);
            dirPicker_->SetPath(fileName);
        }
        else
        {
            fileName = wxFileName(fileName).GetPath();
            if (wxDirExists(fileName))
            {
                dirName_->SetValue(fileName);
                dirPicker_->SetPath(fileName);
            }
        }
    }
    else //should never be reached
        event.Skip();
};


void DragDropOnDlg::OnWriteDirManually(wxCommandEvent& event)
{
    const wxString newDir = FreeFileSync::getFormattedDirectoryName(event.GetString().c_str()).c_str();
    if (wxDirExists(newDir))
        dirPicker_->SetPath(newDir);

    event.Skip();
}


void DragDropOnDlg::OnDirSelected(wxFileDirPickerEvent& event)
{
    const wxString newPath = event.GetPath();
    dirName_->SetValue(newPath);

    event.Skip();
}

