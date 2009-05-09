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

    class DragDropOnMainDlg : public wxEvtHandler
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


    class DragDropOnDlg: public wxEvtHandler
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
