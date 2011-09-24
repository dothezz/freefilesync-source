// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "dir_name.h"
#include <wx/dnd.h>
#include <wx/window.h>
#include <wx/textctrl.h>
#include <wx/statbox.h>
#include "file_handling.h"
#include "resolve_path.h"
#include "string_conv.h"
#include "check_exist.h"
#include "util.h"
#include "i18n.h"
#include "folder_history_box.h"

using namespace zen;


namespace
{
void setDirectoryNameImpl(const wxString& dirname, wxDirPickerCtrl* dirPicker, wxWindow& tooltipWnd, wxStaticBoxSizer* staticBox, size_t timeout)
{
    const wxString dirFormatted = toWx(getFormattedDirectoryName(toZ(dirname)));

    tooltipWnd.SetToolTip(dirFormatted); //wxComboBox bug: the edit control is not updated... hope this will be fixed: http://trac.wxwidgets.org/ticket/12659

    if (staticBox)
    {
        //change static box label only if there is a real difference to what is shown in wxTextCtrl anyway
        wxString dirNormalized = dirname;
        trim(dirNormalized);
        if (!dirNormalized.empty() && !endsWith(dirNormalized, FILE_NAME_SEPARATOR))
            dirNormalized += FILE_NAME_SEPARATOR;

        staticBox->GetStaticBox()->SetLabel(dirNormalized == dirFormatted ? wxString(_("Drag && drop")) : dirFormatted);
    }

    if (dirPicker)
    {
        if (!dirFormatted.empty() &&
            util::dirExists(toZ(dirFormatted), timeout) == util::EXISTING_TRUE) //potentially slow network access: wait 200ms at most
            dirPicker->SetPath(dirFormatted);
    }
}


void setDirectoryName(const wxString&  dirname,
                      wxTextCtrl*      txtCtrl,
                      wxDirPickerCtrl* dirPicker,
                      wxWindow&        tooltipWnd,
                      wxStaticBoxSizer* staticBox,
                      size_t           timeout = 200) //pointers are optional
{
    if (txtCtrl)
        txtCtrl->ChangeValue(dirname);
    setDirectoryNameImpl(dirname, dirPicker, tooltipWnd, staticBox, timeout);
}


void setDirectoryName(const wxString&   dirname,
                      FolderHistoryBox* comboBox,
                      wxDirPickerCtrl*  dirPicker,
                      wxWindow&         tooltipWnd,
                      wxStaticBoxSizer* staticBox,
                      size_t            timeout = 200) //pointers are optional
{
    if (comboBox)
        comboBox->setValue(dirname);
    setDirectoryNameImpl(dirname, dirPicker, tooltipWnd, staticBox, timeout);
}
}
//##############################################################################################################

template <class NameControl>
DirectoryName<NameControl>::DirectoryName(wxWindow&        dropWindow,
                                          wxDirPickerCtrl& dirPicker,
                                          NameControl&     dirName,
                                          wxStaticBoxSizer* staticBox,
                                          wxWindow*        dropWindow2) :
    dropWindow_(dropWindow),
    dropWindow2_(dropWindow2),
    dirPicker_(dirPicker),
    dirName_(dirName),
    staticBox_(staticBox)
{
    //prepare drag & drop
    setupFileDrop(dropWindow);
    if (dropWindow2)
        setupFileDrop(*dropWindow2);

    //redirect drag & drop event back to this class
    dropWindow.Connect(FFS_DROP_FILE_EVENT, FFSFileDropEventHandler(DirectoryName::OnFilesDropped), NULL, this);
    if (dropWindow2)
        dropWindow2->Connect(FFS_DROP_FILE_EVENT, FFSFileDropEventHandler(DirectoryName::OnFilesDropped), NULL, this);

    //keep dirPicker and dirName synchronous
    dirName_  .Connect(wxEVT_COMMAND_TEXT_UPDATED,      wxCommandEventHandler(      DirectoryName::OnWriteDirManually), NULL, this);
    dirPicker_.Connect(wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler(DirectoryName::OnDirSelected     ), NULL, this);
}


template <class NameControl>
DirectoryName<NameControl>::~DirectoryName()
{
    dirName_  .Disconnect(wxEVT_COMMAND_TEXT_UPDATED,      wxCommandEventHandler(      DirectoryName::OnWriteDirManually), NULL, this);
    dirPicker_.Disconnect(wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler(DirectoryName::OnDirSelected     ), NULL, this);
}


template <class NameControl>
void DirectoryName<NameControl>::OnFilesDropped(FFSFileDropEvent& event)
{
    if (event.getFiles().empty())
        return;

    if (acceptDrop(event.getFiles()))
    {
        const wxString fileName = event.getFiles()[0];
        if (dirExists(toZ(fileName)))
            setDirectoryName(fileName, &dirName_, &dirPicker_, dirName_, staticBox_);
        else
        {
            wxString parentName = beforeLast(fileName, FILE_NAME_SEPARATOR); //returns empty string if ch not found
#ifdef FFS_WIN
            if (endsWith(parentName, L":")) //volume name
                parentName += FILE_NAME_SEPARATOR;
#endif
            if (dirExists(toZ(parentName)))
                setDirectoryName(parentName, &dirName_, &dirPicker_, dirName_, staticBox_);
            else //set original name unconditionally: usecase: inactive mapped network shares
                setDirectoryName(fileName, &dirName_, &dirPicker_, dirName_, staticBox_);
        }
    }
}


template <class NameControl>
void DirectoryName<NameControl>::OnWriteDirManually(wxCommandEvent& event)
{
    setDirectoryName(event.GetString(), static_cast<NameControl*>(NULL), &dirPicker_, dirName_, staticBox_, 100); //potentially slow network access: wait 100 ms at most
    event.Skip();
}


template <class NameControl>
void DirectoryName<NameControl>::OnDirSelected(wxFileDirPickerEvent& event)
{
    const wxString newPath = event.GetPath();
    setDirectoryName(newPath, &dirName_, NULL, dirName_, staticBox_);
    event.Skip();
}


template <class NameControl>
wxString DirectoryName<NameControl>::getName() const
{
    return dirName_.GetValue();
}


template <class NameControl>
void DirectoryName<NameControl>::setName(const wxString& dirname)
{
    setDirectoryName(dirname, &dirName_, &dirPicker_, dirName_, staticBox_);
}


//explicit template instantiation
template class DirectoryName<wxTextCtrl>;
template class DirectoryName<FolderHistoryBox>;
