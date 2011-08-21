// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef FILE_DROP_H_INCLUDED
#define FILE_DROP_H_INCLUDED

#include <wx/event.h>
#include <wx/dnd.h>

namespace zen
{
//register simple file drop event (without issue of freezing dialogs and without wxFileDropTarget overdesign)

//1. have a window emit FFS_DROP_FILE_EVENT
void setupFileDrop(wxWindow& wnd);

//2. register events:
//wnd.Connect   (FFS_DROP_FILE_EVENT, FFSFileDropEventHandler(MyDlg::OnFilesDropped), NULL, this);
//wnd.Disconnect(FFS_DROP_FILE_EVENT, FFSFileDropEventHandler(MyDlg::OnFilesDropped), NULL, this);

//3. do something:
//void MyDlg::OnFilesDropped(FFSFileDropEvent& event);

















inline
wxEventType createNewEventType()
{
    //inline functions have external linkage by default => this static is also extern, i.e. program wide unique! but defined in a header... ;)
    static wxEventType dummy = wxNewEventType();
    return dummy;
}

//define new event type
const wxEventType FFS_DROP_FILE_EVENT = createNewEventType();

class FFSFileDropEvent : public wxCommandEvent
{
public:
    FFSFileDropEvent(const std::vector<wxString>& filesDropped, const wxWindow& dropWindow, wxPoint dropPos) :
        wxCommandEvent(FFS_DROP_FILE_EVENT),
        filesDropped_(filesDropped),
        dropWindow_(dropWindow),
        dropPos_(dropPos) {}

    virtual wxEvent* Clone() const
    {
        return new FFSFileDropEvent(filesDropped_, dropWindow_, dropPos_);
    }

    const std::vector<wxString>& getFiles()        const { return filesDropped_; }
    const wxWindow&              getDropWindow()   const { return dropWindow_;   }
    wxPoint                      getDropPosition() const { return dropPos_;      } //position relative to drop window

private:
    const std::vector<wxString> filesDropped_;
    const wxWindow& dropWindow_;
    wxPoint dropPos_;
};

typedef void (wxEvtHandler::*FFSFileDropEventFunction)(FFSFileDropEvent&);

#define FFSFileDropEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(FFSFileDropEventFunction, &func)


class WindowDropTarget : public wxFileDropTarget
{
public:
    WindowDropTarget(wxWindow& dropWindow) : dropWindow_(dropWindow) {}

    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& fileArray)
    {
        std::vector<wxString> filenames(fileArray.begin(), fileArray.end());
        if (!filenames.empty())
        {
            //create a custom event on drop window: execute event after file dropping is completed! (after mouse is released)
            FFSFileDropEvent evt(filenames, dropWindow_, wxPoint(x, y));
            dropWindow_.GetEventHandler()->AddPendingEvent(evt);
        }
        return true;
    }

private:
    wxWindow& dropWindow_;
};


inline
void setupFileDrop(wxWindow& wnd)
{
    wnd.SetDropTarget(new WindowDropTarget(wnd)); //takes ownership
}
}

#endif // FILE_DROP_H_INCLUDED
