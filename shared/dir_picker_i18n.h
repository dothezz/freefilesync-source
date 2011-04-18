#ifndef DIR_PICKER_I18N_H_INCLUDED
#define DIR_PICKER_I18N_H_INCLUDED

#include <wx/filepicker.h>
#include "i18n.h"


class FfsDirPickerCtrl : public wxDirPickerCtrl
{
public:
    FfsDirPickerCtrl(wxWindow* parent, wxWindowID id,
                     const wxString& path = wxEmptyString,
                     const wxString& message = wxDirSelectorPromptStr,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxDIRP_DEFAULT_STYLE,
                     const wxValidator& validator = wxDefaultValidator,
                     const wxString& name = wxDirPickerCtrlNameStr) :
        wxDirPickerCtrl(parent, id, path, message, pos, size, style, validator, name)
    {
#ifdef FFS_WIN
        //fix wxWidgets localization gap:
        wxButton* button = dynamic_cast<wxButton*>(m_pickerIface);
        if (button) button->SetLabel(_("Browse"));
#endif
    }
};


#endif // DIR_PICKER_I18N_H_INCLUDED
