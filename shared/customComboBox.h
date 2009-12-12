#ifndef CUSTOMCOMBOBOX_H_INCLUDED
#define CUSTOMCOMBOBOX_H_INCLUDED

#include <wx/combobox.h>

//combobox with history function + functionality to delete items (DEL)

class CustomComboBox : public wxComboBox
{
public:
    CustomComboBox(wxWindow* parent,
                   wxWindowID id,
                   const wxString& value = wxEmptyString,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   int n = 0,
                   const wxString choices[] = (const wxString *) NULL,
                   long style = 0,
                   const wxValidator& validator = wxDefaultValidator,
                   const wxString& name = wxComboBoxNameStr);

    void addPairToFolderHistory(const wxString& newFolder, unsigned int maxHistSize);

private:
    void OnKeyEvent(wxKeyEvent& event);
};


#endif // CUSTOMCOMBOBOX_H_INCLUDED
