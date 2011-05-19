// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
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
#include "system_constants.h"

using namespace zen;


namespace
{
void setDirectoryNameImpl(const wxString& dirname, wxDirPickerCtrl* dirPicker, wxWindow& tooltipWnd, wxStaticBoxSizer* staticBox, size_t timeout)
{
    const wxString dirFormatted = zToWx(getFormattedDirectoryName(wxToZ(dirname)));

    tooltipWnd.SetToolTip(dirFormatted); //wxComboBox bug: the edit control is not updated... hope this will be fixed: http://trac.wxwidgets.org/ticket/12659

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
        if (!dirFormatted.empty() && util::dirExists(wxToZ(dirFormatted), timeout) == util::EXISTING_TRUE) //potentially slow network access: wait 200ms at most
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
using zen::DirectoryNameMainDlg;

DirectoryNameMainDlg::DirectoryNameMainDlg(wxWindow&         dropWindow1,
                                           wxWindow&         dropWindow2,
                                           wxDirPickerCtrl&  dirPicker,
                                           wxComboBox&       dirName,
                                           wxStaticBoxSizer& staticBox) :
    dropWindow1_(dropWindow1),
    dropWindow2_(dropWindow2),
    dirPicker_(dirPicker),
    dirName_(dirName),
    staticBox_(staticBox)
{
    //prepare drag & drop
    setupFileDrop(dropWindow1);
    setupFileDrop(dropWindow2);

    //redirect drag & drop event back to this class
    dropWindow1.Connect(FFS_DROP_FILE_EVENT, FFSFileDropEventHandler(DirectoryNameMainDlg::OnFilesDropped), NULL, this);
    dropWindow2.Connect(FFS_DROP_FILE_EVENT, FFSFileDropEventHandler(DirectoryNameMainDlg::OnFilesDropped), NULL, this);

    //keep dirPicker and dirName synchronous
    dirName  .Connect( wxEVT_COMMAND_TEXT_UPDATED,      wxCommandEventHandler(      DirectoryNameMainDlg::OnWriteDirManually), NULL, this );
    dirPicker.Connect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler(DirectoryNameMainDlg::OnDirSelected),      NULL, this );
}


void DirectoryNameMainDlg::OnFilesDropped(FFSFileDropEvent& event)
{
    if (event.getFiles().empty())
        return;

    if (AcceptDrop(event.getFiles()))
    {
        Zstring fileName = wxToZ(event.getFiles()[0]);
        if (dirExists(fileName))
            setDirectoryName(zToWx(fileName), &dirName_, &dirPicker_, dirName_, staticBox_);
        else
        {
            fileName = fileName.BeforeLast(common::FILE_NAME_SEPARATOR);
            if (dirExists(fileName))
                setDirectoryName(zToWx(fileName), &dirName_, &dirPicker_, dirName_, staticBox_);
        }
    }
}


void DirectoryNameMainDlg::OnWriteDirManually(wxCommandEvent& event)
{
    setDirectoryName(event.GetString(), NULL, &dirPicker_, dirName_, staticBox_, 100); //potentially slow network access: wait 100 ms at most
    event.Skip();
}


void DirectoryNameMainDlg::OnDirSelected(wxFileDirPickerEvent& event)
{
    const wxString newPath = event.GetPath();
    setDirectoryName(newPath, &dirName_, NULL, dirName_, staticBox_);

    event.Skip();
}


wxString DirectoryNameMainDlg::getName() const
{
    return dirName_.GetValue();
}


void DirectoryNameMainDlg::setName(const wxString& dirname)
{
    setDirectoryName(dirname, &dirName_, &dirPicker_, dirName_, staticBox_);
}


//##############################################################################################################
using zen::DirectoryName;

DirectoryName::DirectoryName(wxWindow&        dropWindow,
                             wxDirPickerCtrl& dirPicker,
                             wxTextCtrl&      dirName,
                             wxStaticBoxSizer* staticBox) :
    dropWindow_(dropWindow),
    dirPicker_(dirPicker),
    dirName_(dirName),
    staticBox_(staticBox)
{
    //prepare drag & drop
    setupFileDrop(dropWindow);

    //redirect drag & drop event back to this class
    dropWindow.Connect(FFS_DROP_FILE_EVENT, FFSFileDropEventHandler(DirectoryName::OnFilesDropped), NULL, this);

    //keep dirPicker and dirName synchronous
    dirName.Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DirectoryName::OnWriteDirManually ), NULL, this );
    dirPicker.Connect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( DirectoryName::OnDirSelected ), NULL, this );
}


void DirectoryName::OnFilesDropped(FFSFileDropEvent& event)
{
    if (event.getFiles().empty())
        return;

    Zstring fileName = wxToZ(event.getFiles()[0]);
    if (dirExists(fileName))
        setDirectoryName(zToWx(fileName), &dirName_, &dirPicker_, dirName_, staticBox_);
    else
    {
        fileName = fileName.BeforeLast(common::FILE_NAME_SEPARATOR);
        if (dirExists(fileName))
            setDirectoryName(zToWx(fileName), &dirName_, &dirPicker_, dirName_, staticBox_);
    }
}


void DirectoryName::OnWriteDirManually(wxCommandEvent& event)
{
    setDirectoryName(event.GetString(), NULL, &dirPicker_, dirName_, staticBox_, 100); //potentially slow network access: wait 100 ms at most
    event.Skip();
}


void DirectoryName::OnDirSelected(wxFileDirPickerEvent& event)
{
    const wxString newPath = event.GetPath();
    setDirectoryName(newPath, &dirName_, NULL, dirName_, staticBox_);

    event.Skip();
}


wxString DirectoryName::getName() const
{
    return dirName_.GetValue();
}


void DirectoryName::setName(const wxString& dirname)
{
    setDirectoryName(dirname, &dirName_, &dirPicker_, dirName_, staticBox_);
}
