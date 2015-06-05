// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "folder_selector.h"
#include <zen/thread.h>
#include <zen/file_access.h>
#include <wx/dirdlg.h>
#include <wx/scrolwin.h>
#include <wx+/string_conv.h>
#include <wx+/popup_dlg.h>
#include "../fs/concrete.h"
#include "../fs/native.h"
#ifdef ZEN_WIN_VISTA_AND_LATER
    #include "../fs/mtp.h"
    #include "ifile_dialog.h"
#endif

using namespace zen;
using ABF = AbstractBaseFolder;


namespace
{
void setFolderPath(const Zstring& dirpath, FolderHistoryBox* comboBox, wxWindow& tooltipWnd, wxStaticText* staticText) //pointers are optional
{
    if (comboBox)
        comboBox->setValue(toWx(dirpath));

    const Zstring displayPath = getResolvedDisplayPath(dirpath); //may block when resolving [<volume name>]

    tooltipWnd.SetToolTip(nullptr); //workaround wxComboBox bug http://trac.wxwidgets.org/ticket/10512 / http://trac.wxwidgets.org/ticket/12659
    tooltipWnd.SetToolTip(toWx(displayPath)); //who knows when the real bugfix reaches mere mortals via an official release...

    if (staticText)
    {
        //change static box label only if there is a real difference to what is shown in wxTextCtrl anyway
        const Zstring dirpathFmt = trimCpy(dirpath);

        staticText->SetLabel(EqualFilePath()(appendSeparator(dirpathFmt), appendSeparator(displayPath)) ? wxString(_("Drag && drop")) : toWx(displayPath));
    }
}

#ifdef ZEN_WIN_VISTA_AND_LATER
bool acceptShellItemPaths(const std::vector<Zstring>& shellItemPaths)
{
    //accept files or folders from:
    //- file system paths
    //- MTP paths

    if (shellItemPaths.empty()) return false;
    return acceptsFolderPathPhraseNative(shellItemPaths[0]) || //
           acceptsFolderPathPhraseMtp   (shellItemPaths[0]);   //noexcept
}


bool onIFileDialogAcceptFolder(HWND wnd, const Zstring& shellFolderPath)
{
    if (acceptShellItemPaths({ shellFolderPath })) //noexcept
        return true;

    const std::wstring msg = replaceCpy(_("The selected folder %x cannot be used with FreeFileSync. Please select a folder on a local file system, network or an MTP device."), L"%x", fmtFileName(shellFolderPath));
    ::MessageBox(wnd, msg.c_str(), (_("Select a folder")).c_str(), MB_ICONWARNING);
    //showNotificationDialog would not support HWND parent
    return false;
}
#endif
}

//##############################################################################################################


const wxEventType zen::EVENT_ON_DIR_SELECTED          = wxNewEventType();
const wxEventType zen::EVENT_ON_DIR_MANUAL_CORRECTION = wxNewEventType();


FolderSelector::FolderSelector(wxWindow&          dropWindow,
                               wxButton&          selectButton,
                               FolderHistoryBox&  dirpath,
                               wxStaticText*      staticText,
                               wxWindow*          dropWindow2) :
    dropWindow_(dropWindow),
    dropWindow2_(dropWindow2),
    selectButton_(selectButton),
    dirpath_(dirpath),
    staticText_(staticText)
{
    auto setupDragDrop = [&](wxWindow& dropWin)
    {
#ifdef ZEN_WIN_VISTA_AND_LATER
        setupShellItemDrop(dropWin, acceptShellItemPaths);
#else
        setupFileDrop(dropWin);
#endif
        dropWin.Connect(EVENT_DROP_FILE, FileDropEventHandler(FolderSelector::onFilesDropped), nullptr, this);
    };

    setupDragDrop(dropWindow_);
    if (dropWindow2_) setupDragDrop(*dropWindow2_);

    //keep dirPicker and dirpath synchronous
    dirpath_     .Connect(wxEVT_MOUSEWHEEL,             wxMouseEventHandler  (FolderSelector::onMouseWheel      ), nullptr, this);
    dirpath_     .Connect(wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler(FolderSelector::onWriteDirManually), nullptr, this);
    selectButton_.Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderSelector::onSelectDir       ), nullptr, this);
}


FolderSelector::~FolderSelector()
{
    dropWindow_.Disconnect(EVENT_DROP_FILE, FileDropEventHandler(FolderSelector::onFilesDropped), nullptr, this);

    if (dropWindow2_)
        dropWindow2_->Disconnect(EVENT_DROP_FILE, FileDropEventHandler(FolderSelector::onFilesDropped), nullptr, this);

    dirpath_     .Disconnect(wxEVT_MOUSEWHEEL,             wxMouseEventHandler  (FolderSelector::onMouseWheel      ), nullptr, this);
    dirpath_     .Disconnect(wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler(FolderSelector::onWriteDirManually), nullptr, this);
    selectButton_.Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderSelector::onSelectDir       ), nullptr, this);
}


void FolderSelector::onMouseWheel(wxMouseEvent& event)
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


void FolderSelector::onFilesDropped(FileDropEvent& event)
{
    const auto& files = event.getFiles();
    if (files.empty())
        return;

    if (canSetDroppedShellPaths(files))
    {
        Zstring newFolderPathPhrase = files[0];

        if (!ABF::dirExists(createAbstractBaseFolder(files[0])->getAbstractPath()))
        {
            Zstring parentPathPhrase = beforeLast(files[0], FILE_NAME_SEPARATOR); //returns empty string if ch not found
            if (!parentPathPhrase.empty())
            {
#ifdef ZEN_WIN
                if (endsWith(parentPathPhrase, L":")) //volume root
                    parentPathPhrase += FILE_NAME_SEPARATOR;
#endif
                if (ABF::dirExists(createAbstractBaseFolder(parentPathPhrase)->getAbstractPath()))
                    newFolderPathPhrase = parentPathPhrase;
                //else: keep original name unconditionally: usecase: inactive mapped network shares
            }
        }

        //make sure FFS-specific explicit MTP-syntax is applied!
        setFolderPath(getResolvedDisplayPath(newFolderPathPhrase), &dirpath_, dirpath_, staticText_);

        //notify action invoked by user
        wxCommandEvent dummy(EVENT_ON_DIR_SELECTED);
        ProcessEvent(dummy);
    }
    else
        event.Skip(); //let other handlers try -> are there any??
}


void FolderSelector::onWriteDirManually(wxCommandEvent& event)
{
    setFolderPath(toZ(event.GetString()), nullptr, dirpath_, staticText_);

    wxCommandEvent dummy(EVENT_ON_DIR_MANUAL_CORRECTION);
    ProcessEvent(dummy);
    event.Skip();
}


void FolderSelector::onSelectDir(wxCommandEvent& event)
{
    //make sure default folder exists: don't let folder picker hang on non-existing network share!
    Zstring defaultFolderPath;
#ifdef ZEN_WIN_VISTA_AND_LATER
    std::shared_ptr<const void> /*PCIDLIST_ABSOLUTE*/ defaultFolderPidl;
#endif
    {
        auto baseFolderExisting = [](const ABF& abf)
        {
            std::function<bool()> asyncDirExists = abf.getAsyncCheckDirExists(abf.getAbstractPath()); //noexcept
            auto ft = async([asyncDirExists] { return asyncDirExists(); /*noexcept*/ });
            return ft.timed_wait(boost::posix_time::milliseconds(200)) && ft.get(); //potentially slow network access: wait 200ms at most
        };

        const Zstring folderPathPhrase = getPath();
        if (acceptsFolderPathPhraseNative(folderPathPhrase)) //noexcept
        {
            std::unique_ptr<ABF> abf = createBaseFolderNative(folderPathPhrase);
            if (baseFolderExisting(*abf))
                if (Opt<Zstring> nativeFolderPath = ABF::getNativeItemPath(abf->getAbstractPath()))
                    defaultFolderPath = *nativeFolderPath;
        }
#ifdef ZEN_WIN_VISTA_AND_LATER
        else if (acceptsFolderPathPhraseMtp(folderPathPhrase)) //noexcept
        {
            std::unique_ptr<ABF> abf = createBaseFolderMtp(folderPathPhrase);
            if (baseFolderExisting(*abf))
                defaultFolderPidl = geMtpItemAbsolutePidl(abf->getAbstractPath());
        }
#endif
    }

    //wxDirDialog internally uses lame-looking SHBrowseForFolder(); we better use IFileDialog() instead! (remembers size and position!)
#ifdef ZEN_WIN_VISTA_AND_LATER
    Zstring newFolder;
    try
    {
        //some random GUID => have Windows save IFileDialog state separately from other file/dir pickers!
        const GUID guid = { 0x31f94a00, 0x92b4, 0xa040, { 0x8d, 0xc2, 0xc, 0xa5, 0xef, 0x59, 0x6e, 0x3b } };

        const std::pair<Zstring, bool> rv = ifile::showFolderPicker(static_cast<HWND>(selectButton_.GetHWND()),
                                                                    defaultFolderPath,
                                                                    defaultFolderPidl.get(),
                                                                    &guid,
                                                                    onIFileDialogAcceptFolder); //throw FileError
        if (!rv.second) //cancelled
            return;

        //make sure FFS-specific explicit MTP-syntax is applied!
        newFolder = getResolvedDisplayPath(rv.first); //noexcept
    }
    catch (const FileError& e) { showNotificationDialog(&dropWindow_, DialogInfoType::ERROR2, PopupDialogCfg().setDetailInstructions(e.toString())); return; }
#else
    wxDirDialog dirPicker(&selectButton_, _("Select a folder"), toWx(defaultFolderPath)); //put modal wxWidgets dialogs on stack: creating on freestore leads to memleak!
    if (dirPicker.ShowModal() != wxID_OK)
        return;
    const Zstring newFolder = toZ(dirPicker.GetPath());
#endif

    setFolderPath(newFolder, &dirpath_, dirpath_, staticText_);

    //notify action invoked by user
    wxCommandEvent dummy(EVENT_ON_DIR_SELECTED);
    ProcessEvent(dummy);
}


Zstring FolderSelector::getPath() const
{
    return toZ(dirpath_.GetValue());
}


void FolderSelector::setPath(const Zstring& dirpath)
{
    setFolderPath(dirpath, &dirpath_, dirpath_, staticText_);
}
