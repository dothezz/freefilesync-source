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
    #include <gtk/gtk.h>


using namespace zen;


namespace
{
void setFolderPath(const Zstring& dirpath, wxTextCtrl* txtCtrl, wxWindow& tooltipWnd, wxStaticText* staticText) //pointers are optional
{
    if (txtCtrl)
        txtCtrl->ChangeValue(toWx(dirpath));

    const Zstring folderPathFmt = getResolvedFilePath(dirpath); //may block when resolving [<volume name>]

    tooltipWnd.SetToolTip(nullptr); //workaround wxComboBox bug http://trac.wxwidgets.org/ticket/10512 / http://trac.wxwidgets.org/ticket/12659
    tooltipWnd.SetToolTip(toWx(folderPathFmt)); //who knows when the real bugfix reaches mere mortals via an official release...

    if (staticText) //change static box label only if there is a real difference to what is shown in wxTextCtrl anyway
        staticText->SetLabel(equalFilePath(appendSeparator(trimCpy(dirpath)), appendSeparator(folderPathFmt)) ? wxString(_("Drag && drop")) : toWx(folderPathFmt));
}
}

//##############################################################################################################

FolderSelector2::FolderSelector2(wxWindow&     dropWindow,
                                 wxButton&     selectButton,
                                 wxTextCtrl&   folderPathCtrl,
                                 wxStaticText* staticText) :
    dropWindow_(dropWindow),
    selectButton_(selectButton),
    folderPathCtrl_(folderPathCtrl),
    staticText_(staticText)
{
    //file drag and drop directly into the text control unhelpfully inserts in format "file://..<cr><nl>"; see folder_history_box.cpp
    if (GtkWidget* widget = folderPathCtrl.GetConnectWidget())
        ::gtk_drag_dest_unset(widget);

    //prepare drag & drop
    setupFileDrop(dropWindow_);
    dropWindow_.Connect(EVENT_DROP_FILE, FileDropEventHandler(FolderSelector2::onFilesDropped), nullptr, this);

    //keep dirPicker and dirpath synchronous
    folderPathCtrl_.Connect(wxEVT_MOUSEWHEEL,             wxMouseEventHandler  (FolderSelector2::onMouseWheel    ), nullptr, this);
    folderPathCtrl_.Connect(wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler(FolderSelector2::onEditFolderPath), nullptr, this);
    selectButton_  .Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderSelector2::onSelectDir     ), nullptr, this);
}


FolderSelector2::~FolderSelector2()
{
    dropWindow_.Disconnect(EVENT_DROP_FILE, FileDropEventHandler(FolderSelector2::onFilesDropped), nullptr, this);

    folderPathCtrl_.Disconnect(wxEVT_MOUSEWHEEL,             wxMouseEventHandler  (FolderSelector2::onMouseWheel    ), nullptr, this);
    folderPathCtrl_.Disconnect(wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler(FolderSelector2::onEditFolderPath), nullptr, this);
    selectButton_  .Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderSelector2::onSelectDir     ), nullptr, this);
}


void FolderSelector2::onMouseWheel(wxMouseEvent& event)
{
    //for combobox: although switching through available items is wxWidgets default, this is NOT windows default, e.g. explorer
    //additionally this will delete manual entries, although all the users wanted is scroll the parent window!

    //redirect to parent scrolled window!
    wxWindow* wnd = &folderPathCtrl_;
    while ((wnd = wnd->GetParent()) != nullptr) //silence MSVC warning
        if (dynamic_cast<wxScrolledWindow*>(wnd) != nullptr)
            if (wxEvtHandler* evtHandler = wnd->GetEventHandler())
            {
                evtHandler->AddPendingEvent(event);
                break;
            }
    //  event.Skip();
}


void FolderSelector2::onFilesDropped(FileDropEvent& event)
{
    const auto& itemPaths = event.getPaths();
    if (itemPaths.empty())
        return;

    Zstring itemPath = itemPaths[0];
    if (!dirExists(itemPath))
    {
        Zstring parentPath = beforeLast(itemPath, FILE_NAME_SEPARATOR, IF_MISSING_RETURN_NONE);
        if (dirExists(parentPath))
            itemPath = parentPath;
        //else: keep original name unconditionally: usecase: inactive mapped network shares
    }
    setPath(itemPath);

    //event.Skip();
}


void FolderSelector2::onEditFolderPath(wxCommandEvent& event)
{
    setFolderPath(toZ(event.GetString()), nullptr, folderPathCtrl_, staticText_);
    event.Skip();
}




void FolderSelector2::onSelectDir(wxCommandEvent& event)
{
    //IFileDialog requirements for default path: 1. accepts native paths only!!! 2. path must exist!
    Zstring defaultFolderPath;
    {
        const Zstring folderPath = getResolvedFilePath(getPath());
        if (!folderPath.empty())
        {
            auto ft = runAsync([folderPath] { return dirExists(folderPath); });

            if (ft.wait_for(std::chrono::milliseconds(200)) == std::future_status::ready && ft.get()) //potentially slow network access: wait 200ms at most
                defaultFolderPath = folderPath;
        }
    }

    wxDirDialog dirPicker(&selectButton_, _("Select a folder"), toWx(defaultFolderPath)); //put modal wxWidgets dialogs on stack: creating on freestore leads to memleak!
    if (dirPicker.ShowModal() != wxID_OK)
        return;
    const Zstring newFolder = toZ(dirPicker.GetPath());

    setFolderPath(newFolder, &folderPathCtrl_, folderPathCtrl_, staticText_);
}


Zstring FolderSelector2::getPath() const
{
    Zstring path = utfCvrtTo<Zstring>(folderPathCtrl_.GetValue());

    return path;
}


void FolderSelector2::setPath(const Zstring& dirpath)
{
    setFolderPath(dirpath, &folderPathCtrl_, folderPathCtrl_, staticText_);
}
