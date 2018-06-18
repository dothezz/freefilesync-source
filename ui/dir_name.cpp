// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "dir_name.h"
#include <zen/thread.h>
#include <zen/file_handling.h>
#include <wx/dnd.h>
#include <wx/window.h>
#include <wx/textctrl.h>
#include <wx/statbox.h>
#include <wx/dirdlg.h>
#include <wx/msgdlg.h>
#include <wx+/string_conv.h>
#include "../lib/resolve_path.h"
#include "folder_history_box.h"

#ifdef FFS_WIN
#include <zen/dll.h>
#include <zen/win_ver.h>
#include "IFileDialog_Vista\ifile_dialog.h"
#endif

using namespace zen;


const wxEventType zen::EVENT_ON_DIR_SELECTED          = wxNewEventType();
const wxEventType zen::EVENT_ON_DIR_MANUAL_CORRECTION = wxNewEventType();

namespace
{
void setDirectoryNameImpl(const wxString& dirname, wxWindow& tooltipWnd, wxStaticText* staticText)
{
    const wxString dirFormatted = utfCvrtTo<wxString>(getFormattedDirectoryName(toZ(dirname)));

    tooltipWnd.SetToolTip(nullptr); //workaround wxComboBox bug http://trac.wxwidgets.org/ticket/10512 / http://trac.wxwidgets.org/ticket/12659
    tooltipWnd.SetToolTip(dirFormatted); //who knows when the real bugfix reaches mere mortals via an official release...

    if (staticText)
    {
        //change static box label only if there is a real difference to what is shown in wxTextCtrl anyway
        wxString dirNormalized = dirname;
        trim(dirNormalized);
        if (!dirNormalized.empty() && !endsWith(dirNormalized, FILE_NAME_SEPARATOR))
            dirNormalized += FILE_NAME_SEPARATOR;

        staticText->SetLabel(dirNormalized == dirFormatted ? wxString(_("Drag && drop")) : dirFormatted);
    }
}


void setDirectoryName(const wxString&  dirname,
                      wxTextCtrl*      txtCtrl,
                      wxWindow&        tooltipWnd,
                      wxStaticText*    staticText) //pointers are optional
{
    if (txtCtrl)
        txtCtrl->ChangeValue(dirname);
    setDirectoryNameImpl(dirname, tooltipWnd, staticText);
}


void setDirectoryName(const wxString&   dirname,
                      FolderHistoryBox* comboBox,
                      wxWindow&         tooltipWnd,
                      wxStaticText*    staticText) //pointers are optional
{
    if (comboBox)
        comboBox->setValue(dirname);
    setDirectoryNameImpl(dirname, tooltipWnd, staticText);
}
}
//##############################################################################################################

template <class NameControl>
DirectoryName<NameControl>::DirectoryName(wxWindow&     dropWindow,
                                          wxButton&     selectButton,
                                          NameControl&  dirName,
                                          wxStaticText* staticText,
                                          wxWindow*     dropWindow2) :
    dropWindow_(dropWindow),
    dropWindow2_(dropWindow2),
    selectButton_(selectButton),
    dirName_(dirName),
    staticText_(staticText)
{
    //prepare drag & drop
    setupFileDrop(dropWindow_);
    dropWindow_.Connect(EVENT_DROP_FILE, FileDropEventHandler(DirectoryName::OnFilesDropped), nullptr, this);

    if (dropWindow2_)
    {
        setupFileDrop(*dropWindow2_);
        dropWindow2_->Connect(EVENT_DROP_FILE, FileDropEventHandler(DirectoryName::OnFilesDropped), nullptr, this);
    }

    //keep dirPicker and dirName synchronous
    dirName_     .Connect(wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler      (DirectoryName::OnWriteDirManually), nullptr, this);
    selectButton_.Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler      (DirectoryName::OnSelectDir       ), nullptr, this);
}


template <class NameControl>
DirectoryName<NameControl>::~DirectoryName()
{
    dropWindow_.Disconnect(EVENT_DROP_FILE, FileDropEventHandler(DirectoryName::OnFilesDropped), nullptr, this);
    if (dropWindow2_)
        dropWindow2_->Disconnect(EVENT_DROP_FILE, FileDropEventHandler(DirectoryName::OnFilesDropped), nullptr, this);

    dirName_     .Disconnect(wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler      (DirectoryName::OnWriteDirManually), nullptr, this);
    selectButton_.Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler      (DirectoryName::OnSelectDir       ), nullptr, this);
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
            setDirectoryName(fileName, &dirName_, dirName_, staticText_);
        else
        {
            wxString parentName = beforeLast(fileName, utfCvrtTo<wxString>(FILE_NAME_SEPARATOR)); //returns empty string if ch not found
#ifdef FFS_WIN
            if (endsWith(parentName, L":")) //volume name
                parentName += FILE_NAME_SEPARATOR;
#endif
            if (dirExists(toZ(parentName)))
                setDirectoryName(parentName, &dirName_, dirName_, staticText_);
            else //set original name unconditionally: usecase: inactive mapped network shares
                setDirectoryName(fileName, &dirName_, dirName_, staticText_);
        }

        //notify action invoked by user
        wxCommandEvent dummy(EVENT_ON_DIR_SELECTED);
        ProcessEvent(dummy);
    }
    else
        event.Skip(); //let other handlers try!!!
}


template <class NameControl>
void DirectoryName<NameControl>::OnWriteDirManually(wxCommandEvent& event)
{
    setDirectoryName(event.GetString(), static_cast<NameControl*>(nullptr), dirName_, staticText_);

    wxCommandEvent dummy(EVENT_ON_DIR_MANUAL_CORRECTION);
    ProcessEvent(dummy);
    event.Skip();
}


template <class NameControl>
void DirectoryName<NameControl>::OnSelectDir(wxCommandEvent& event)
{
    wxString defaultDirname; //default selection for dir picker
    {
        const Zstring dirFmt = getFormattedDirectoryName(toZ(getName()));
        if (!dirFmt.empty())
        {
            //convert to Zstring first: we don't want to pass wxString by value and risk MT issues!
            auto ft = async([=] { return zen::dirExists(dirFmt); });

            if (ft.timed_wait(boost::posix_time::milliseconds(200)) && ft.get()) //potentially slow network access: wait 200ms at most
                defaultDirname = utfCvrtTo<wxString>(dirFmt);
        }
    }

    //wxDirDialog internally uses lame-looking SHBrowseForFolder(); we better use IFileDialog() instead! (remembers size and position!)
    std::unique_ptr<wxString> newFolder;
#ifdef FFS_WIN
    if (vistaOrLater())
    {
        using namespace ifile;
        const DllFun<FunType_showFolderPicker> showFolderPicker(getDllName(), funName_showFolderPicker);
        const DllFun<FunType_freeString>       freeString      (getDllName(), funName_freeString);
        if (showFolderPicker && freeString)
        {
            wchar_t* selectedFolder = nullptr;
            wchar_t* errorMsg       = nullptr;
            bool cancelled = false;
            ZEN_ON_SCOPE_EXIT(freeString(selectedFolder));
            ZEN_ON_SCOPE_EXIT(freeString(errorMsg));

            const GuidProxy guid = { '\x0', '\x4a', '\xf9', '\x31', '\xb4', '\x92', '\x40', '\xa0',
                                     '\x8d', '\xc2', '\xc', '\xa5', '\xef', '\x59', '\x6e', '\x3b'
                                   }; //some random GUID => have Windows save IFileDialog state separately from other file/dir pickers!

            showFolderPicker(static_cast<HWND>(selectButton_.GetHWND()),                //in;  ==HWND
                             defaultDirname.empty() ? nullptr : defaultDirname.c_str(), //in, optional!
                             &guid,
                             selectedFolder, //out: call freeString() after use!
                             cancelled,      //out
                             errorMsg);      //out, optional: call freeString() after use!
            if (errorMsg)
            {
                wxMessageBox(errorMsg, _("Error"), wxOK | wxICON_ERROR);
                return;
            }
            if (cancelled || !selectedFolder)
                return;
            newFolder = make_unique<wxString>(selectedFolder);
        }
    }
#endif
    if (!newFolder.get())
    {
        wxDirDialog dirPicker(&selectButton_, _("Select a folder"), defaultDirname); //put modal wxWidgets dialogs on stack: creating on freestore leads to memleak!
        if (dirPicker.ShowModal() != wxID_OK)
            return;
        newFolder = make_unique<wxString>(dirPicker.GetPath());
    }

    setDirectoryName(*newFolder, &dirName_, dirName_, staticText_);

    //notify action invoked by user
    wxCommandEvent dummy(EVENT_ON_DIR_SELECTED);
    ProcessEvent(dummy);
}


template <class NameControl>
wxString DirectoryName<NameControl>::getName() const
{
    return dirName_.GetValue();
}


template <class NameControl>
void DirectoryName<NameControl>::setName(const wxString& dirname)
{
    setDirectoryName(dirname, &dirName_, dirName_, staticText_);
}


//explicit template instantiations
namespace zen
{
template class DirectoryName<wxTextCtrl>;
template class DirectoryName<FolderHistoryBox>;
}
