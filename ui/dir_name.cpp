// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "dir_name.h"
#include <wx/dnd.h>
#include <wx/window.h>
#include <wx/textctrl.h>
#include <wx/statbox.h>
#include <zen/thread.h>
#include <zen/file_handling.h>
#include "../lib/resolve_path.h"
#include <wx+/string_conv.h>
#include "folder_history_box.h"

using namespace zen;


namespace
{
void setDirectoryNameImpl(const wxString& dirname, wxDirPickerCtrl* dirPicker, wxWindow& tooltipWnd, wxStaticText* staticText, size_t timeout)
{
    const wxString dirFormatted = toWx(getFormattedDirectoryName(toZ(dirname)));

    tooltipWnd.SetToolTip(nullptr); //workaround wxComboBox bug http://trac.wxwidgets.org/ticket/10512 / http://trac.wxwidgets.org/ticket/12659
    tooltipWnd.SetToolTip(dirFormatted); //only lord knows when the real bugfix reaches mere mortals via an official release

    if (staticText)
    {
        //change static box label only if there is a real difference to what is shown in wxTextCtrl anyway
        wxString dirNormalized = dirname;
        trim(dirNormalized);
        if (!dirNormalized.empty() && !endsWith(dirNormalized, FILE_NAME_SEPARATOR))
            dirNormalized += FILE_NAME_SEPARATOR;

        staticText->SetLabel(dirNormalized == dirFormatted ? wxString(_("Drag && drop")) : dirFormatted);
    }

    if (dirPicker && !dirFormatted.empty())
    {
        Zstring dir = toZ(dirFormatted); //convert to Zstring first: we don't want to pass wxString by value and risk MT issues!
        auto ft = async([=] { return zen::dirExists(dir); });

        if (ft.timed_wait(boost::posix_time::milliseconds(timeout)) && ft.get()) //potentially slow network access: wait 200ms at most
            dirPicker->SetPath(dirFormatted);
    }
}


void setDirectoryName(const wxString&  dirname,
                      wxTextCtrl*      txtCtrl,
                      wxDirPickerCtrl* dirPicker,
                      wxWindow&        tooltipWnd,
                      wxStaticText*    staticText,
                      size_t           timeout = 200) //pointers are optional
{
    if (txtCtrl)
        txtCtrl->ChangeValue(dirname);
    setDirectoryNameImpl(dirname, dirPicker, tooltipWnd, staticText, timeout);
}


void setDirectoryName(const wxString&   dirname,
                      FolderHistoryBox* comboBox,
                      wxDirPickerCtrl*  dirPicker,
                      wxWindow&         tooltipWnd,
                      wxStaticText*    staticText,
                      size_t            timeout = 200) //pointers are optional
{
    if (comboBox)
        comboBox->setValue(dirname);
    setDirectoryNameImpl(dirname, dirPicker, tooltipWnd, staticText, timeout);
}
}
//##############################################################################################################

template <class NameControl>
DirectoryName<NameControl>::DirectoryName(wxWindow&        dropWindow,
                                          wxDirPickerCtrl& dirPicker,
                                          NameControl&     dirName,
                                          wxStaticText*    staticText,
                                          wxWindow*        dropWindow2) :
    dropWindow_(dropWindow),
    dropWindow2_(dropWindow2),
    dirPicker_(dirPicker),
    dirName_(dirName),
    staticText_(staticText)
{
    //prepare drag & drop
    setupFileDrop(dropWindow);
    dropWindow.Connect(EVENT_DROP_FILE, FileDropEventHandler(DirectoryName::OnFilesDropped), nullptr, this);

    if (dropWindow2)
    {
        setupFileDrop(*dropWindow2);
        dropWindow2->Connect(EVENT_DROP_FILE, FileDropEventHandler(DirectoryName::OnFilesDropped), nullptr, this);
    }

    //keep dirPicker and dirName synchronous
    dirName_  .Connect(wxEVT_COMMAND_TEXT_UPDATED,      wxCommandEventHandler(      DirectoryName::OnWriteDirManually), nullptr, this);
    dirPicker_.Connect(wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler(DirectoryName::OnDirSelected     ), nullptr, this);
}


template <class NameControl>
DirectoryName<NameControl>::~DirectoryName()
{
    dirName_  .Disconnect(wxEVT_COMMAND_TEXT_UPDATED,      wxCommandEventHandler(      DirectoryName::OnWriteDirManually), nullptr, this);
    dirPicker_.Disconnect(wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler(DirectoryName::OnDirSelected     ), nullptr, this);
}


template <class NameControl>
void DirectoryName<NameControl>::OnFilesDropped(FileDropEvent& event)
{
    const auto& files = event.getFiles();
    if (files.empty())
        return;

    if (acceptDrop(files, event.getDropPosition(), event.getDropWindow()))
    {
        const wxString fileName = event.getFiles()[0];
        if (dirExists(toZ(fileName)))
            setDirectoryName(fileName, &dirName_, &dirPicker_, dirName_, staticText_);
        else
        {
            wxString parentName = beforeLast(fileName, utf8CvrtTo<wxString>(FILE_NAME_SEPARATOR)); //returns empty string if ch not found
#ifdef FFS_WIN
            if (endsWith(parentName, L":")) //volume name
                parentName += FILE_NAME_SEPARATOR;
#endif
            if (dirExists(toZ(parentName)))
                setDirectoryName(parentName, &dirName_, &dirPicker_, dirName_, staticText_);
            else //set original name unconditionally: usecase: inactive mapped network shares
                setDirectoryName(fileName, &dirName_, &dirPicker_, dirName_, staticText_);
        }
    }
    else
        event.Skip(); //let other handlers try!!!
}


template <class NameControl>
void DirectoryName<NameControl>::OnWriteDirManually(wxCommandEvent& event)
{
    setDirectoryName(event.GetString(), static_cast<NameControl*>(nullptr), &dirPicker_, dirName_, staticText_, 100); //potentially slow network access: wait 100 ms at most
    event.Skip();
}


template <class NameControl>
void DirectoryName<NameControl>::OnDirSelected(wxFileDirPickerEvent& event)
{
    const wxString newPath = event.GetPath();
    setDirectoryName(newPath, &dirName_, nullptr, dirName_, staticText_);
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
    setDirectoryName(dirname, &dirName_, &dirPicker_, dirName_, staticText_);
}


//explicit template instantiations
template class DirectoryName<wxTextCtrl>;
template class DirectoryName<FolderHistoryBox>;