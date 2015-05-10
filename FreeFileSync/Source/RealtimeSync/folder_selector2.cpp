// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "folder_selector2.h"
#include <zen/thread.h>
#include <zen/file_access.h>
#include <zen/optional.h>
#include <wx/dirdlg.h>
#include <wx/scrolwin.h>
#include <wx+/string_conv.h>
#include <wx+/popup_dlg.h>
#include "../lib/resolve_path.h"

#ifdef ZEN_WIN
    #include <zen/dll.h>
    #include <zen/win_ver.h>
    #include "../dll/IFileDialog_Vista\ifile_dialog.h"
#endif

using namespace zen;


namespace
{
void setFolderPath(const Zstring& dirpath, wxTextCtrl* txtCtrl, wxWindow& tooltipWnd, wxStaticText* staticText) //pointers are optional
{
    if (txtCtrl)
        txtCtrl->ChangeValue(toWx(dirpath));

    const Zstring displayPath = getResolvedDirectoryPath(dirpath); //may block when resolving [<volume name>]

    tooltipWnd.SetToolTip(nullptr); //workaround wxComboBox bug http://trac.wxwidgets.org/ticket/10512 / http://trac.wxwidgets.org/ticket/12659
    tooltipWnd.SetToolTip(toWx(displayPath)); //who knows when the real bugfix reaches mere mortals via an official release...

    if (staticText) //change static box label only if there is a real difference to what is shown in wxTextCtrl anyway
        staticText->SetLabel(EqualFilePath()(appendSeparator(trimCpy(dirpath)), appendSeparator(displayPath)) ? wxString(_("Drag && drop")) : toWx(displayPath));
}
}

//##############################################################################################################

FolderSelector2::FolderSelector2(wxWindow&     dropWindow,
                                 wxButton&     selectButton,
                                 wxTextCtrl&   dirpath,
                                 wxStaticText* staticText) :
    dropWindow_(dropWindow),
    selectButton_(selectButton),
    dirpath_(dirpath),
    staticText_(staticText)
{
    //prepare drag & drop
    setupFileDrop(dropWindow_);
    dropWindow_.Connect(EVENT_DROP_FILE, FileDropEventHandler(FolderSelector2::onFilesDropped), nullptr, this);

    //keep dirPicker and dirpath synchronous
    dirpath_     .Connect(wxEVT_MOUSEWHEEL,             wxMouseEventHandler  (FolderSelector2::onMouseWheel      ), nullptr, this);
    dirpath_     .Connect(wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler(FolderSelector2::onWriteDirManually), nullptr, this);
    selectButton_.Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderSelector2::onSelectDir       ), nullptr, this);
}


FolderSelector2::~FolderSelector2()
{
    dropWindow_.Disconnect(EVENT_DROP_FILE, FileDropEventHandler(FolderSelector2::onFilesDropped), nullptr, this);

    dirpath_     .Disconnect(wxEVT_MOUSEWHEEL,             wxMouseEventHandler  (FolderSelector2::onMouseWheel      ), nullptr, this);
    dirpath_     .Disconnect(wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler(FolderSelector2::onWriteDirManually), nullptr, this);
    selectButton_.Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderSelector2::onSelectDir       ), nullptr, this);
}


void FolderSelector2::onMouseWheel(wxMouseEvent& event)
{
    //for combobox: although switching through available items is wxWidgets default, this is NOT windows default, e.g. explorer
    //additionally this will delete manual entries, although all the users wanted is scroll the parent window!

    //redirect to parent scrolled window!
    wxWindow* wnd = &dirpath_;
    while ((wnd = wnd->GetParent()) != nullptr) //silence MSVC warning
        if (dynamic_cast<wxScrolledWindow*>(wnd) != nullptr)
            if (wxEvtHandler* evtHandler = wnd->GetEventHandler())
            {
                evtHandler->AddPendingEvent(event);
                break;
            }
    //	event.Skip();
}


void FolderSelector2::onFilesDropped(FileDropEvent& event)
{
    const auto& files = event.getFiles();
    if (files.empty())
        return;

    const Zstring fileName = toZ(event.getFiles()[0]);
    if (dirExists(fileName))
        setFolderPath(fileName, &dirpath_, dirpath_, staticText_);
    else
    {
        Zstring parentName = beforeLast(fileName, FILE_NAME_SEPARATOR); //returns empty string if ch not found
#ifdef ZEN_WIN
        if (endsWith(parentName, L":")) //volume root
            parentName += FILE_NAME_SEPARATOR;
#endif
        if (dirExists(parentName))
            setFolderPath(parentName, &dirpath_, dirpath_, staticText_);
        else //set original name unconditionally: usecase: inactive mapped network shares
            setFolderPath(fileName, &dirpath_, dirpath_, staticText_);
    }
    //event.Skip();
}


void FolderSelector2::onWriteDirManually(wxCommandEvent& event)
{
    setFolderPath(toZ(event.GetString()), nullptr, dirpath_, staticText_);
    event.Skip();
}


void FolderSelector2::onSelectDir(wxCommandEvent& event)
{
    //IFileDialog requirements for default path: 1. accepts native paths only!!! 2. path must exist!
    Zstring defaultFolderPath;
    {
        const Zstring folderPath = getResolvedDirectoryPath(getPath());

        auto ft = async([folderPath] { return dirExists(folderPath); });

        if (ft.timed_wait(boost::posix_time::milliseconds(200)) && ft.get()) //potentially slow network access: wait 200ms at most
            defaultFolderPath = folderPath;
    }

    //wxDirDialog internally uses lame-looking SHBrowseForFolder(); we better use IFileDialog() instead! (remembers size and position!)
    Opt<Zstring> newFolder;
#ifdef ZEN_WIN
    if (vistaOrLater())
    {
#define DEF_DLL_FUN(name) const DllFun<ifile::FunType_##name> name(ifile::getDllName(), ifile::funName_##name);
        DEF_DLL_FUN(showFolderPicker);
        DEF_DLL_FUN(freeString);
#undef DEF_DLL_FUN

        if (showFolderPicker && freeString)
        {
            wchar_t* selectedFolder = nullptr;
            wchar_t* errorMsg       = nullptr;
            bool cancelled = false;
            ZEN_ON_SCOPE_EXIT(freeString(selectedFolder));
            ZEN_ON_SCOPE_EXIT(freeString(errorMsg));

            const ifile::GuidProxy guid = { '\x5d', '\x1f', '\x9c', '\xe8', '\x17', '\xb2', '\x46', '\x55',
                                            '\xa3', '\xc0', '\xdc', '\xcb', '\x37', '\xbb', '\x4e', '\x35'
                                          }; //some random GUID => have Windows save IFileDialog state separately from other file/dir pickers!

            showFolderPicker(static_cast<HWND>(selectButton_.GetHWND()), //in;  ==HWND
                             defaultFolderPath.empty() ? nullptr : defaultFolderPath.c_str(), //in, optional!
                             &guid,
                             nullptr,
                             nullptr /*callbackSink*/,
                             selectedFolder, //out: call freeString() after use!
                             cancelled,      //out
                             errorMsg);      //out, optional: call freeString() after use!
            if (errorMsg)
            {
                showNotificationDialog(&dropWindow_, DialogInfoType::ERROR2, PopupDialogCfg().setDetailInstructions(errorMsg));
                return;
            }
            if (cancelled || !selectedFolder)
                return;

            newFolder = Zstring(selectedFolder);
        }
    }
#endif
    if (!newFolder)
    {
        wxDirDialog dirPicker(&selectButton_, _("Select a folder"), toWx(defaultFolderPath)); //put modal wxWidgets dialogs on stack: creating on freestore leads to memleak!
        if (dirPicker.ShowModal() != wxID_OK)
            return;
        newFolder = toZ(dirPicker.GetPath());
    }

    setFolderPath(*newFolder, &dirpath_, dirpath_, staticText_);
}


Zstring FolderSelector2::getPath() const
{
    return toZ(dirpath_.GetValue());
}


void FolderSelector2::setPath(const Zstring& dirpath)
{
    setFolderPath(dirpath, &dirpath_, dirpath_, staticText_);
}
