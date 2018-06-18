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
#include <wx+/image_resources.h>
#include "../fs/concrete.h"
#include "../fs/native.h"
#ifdef ZEN_WIN_VISTA_AND_LATER
    #include "small_dlgs.h"
    #include "ifile_dialog.h"
    #include "../fs/mtp.h"
#endif

using namespace zen;
using ABF = AbstractBaseFolder;


namespace
{
void setFolderPathPhrase(const Zstring& folderPathPhrase, FolderHistoryBox* comboBox, wxWindow& tooltipWnd, wxStaticText* staticText) //pointers are optional
{
    if (comboBox)
        comboBox->setValue(toWx(folderPathPhrase));

    const Zstring folderPathPhraseFmt = createAbstractBaseFolder(folderPathPhrase)->getInitPathPhrase(); //noexcept
    //may block when resolving [<volume name>]

    tooltipWnd.SetToolTip(nullptr); //workaround wxComboBox bug http://trac.wxwidgets.org/ticket/10512 / http://trac.wxwidgets.org/ticket/12659
    tooltipWnd.SetToolTip(toWx(folderPathPhraseFmt)); //who knows when the real bugfix reaches mere mortals via an official release...

    if (staticText)
    {
        //change static box label only if there is a real difference to what is shown in wxTextCtrl anyway
        staticText->SetLabel(EqualFilePath()(appendSeparator(trimCpy(folderPathPhrase)), appendSeparator(folderPathPhraseFmt)) ?
                             wxString(_("Drag && drop")) : toWx(folderPathPhraseFmt));
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

    const std::wstring msg = replaceCpy(_("The selected folder %x cannot be used with FreeFileSync. Please select a folder on a local file system, network or an MTP device."), L"%x", fmtPath(shellFolderPath));
    ::MessageBox(wnd, msg.c_str(), (_("Select a folder")).c_str(), MB_ICONWARNING);
    //showNotificationDialog would not support HWND parent
    return false;
}
#endif
}

//##############################################################################################################

const wxEventType zen::EVENT_ON_FOLDER_SELECTED    = wxNewEventType();
const wxEventType zen::EVENT_ON_FOLDER_MANUAL_EDIT = wxNewEventType();


FolderSelector::FolderSelector(wxWindow&         dropWindow,
                               wxButton&         selectFolderButton,
                               wxButton&         selectSftpButton,
                               FolderHistoryBox& folderComboBox,
                               wxStaticText*     staticText,
                               wxWindow*         dropWindow2) :
    dropWindow_(dropWindow),
    dropWindow2_(dropWindow2),
    selectFolderButton_(selectFolderButton),
    selectSftpButton_(selectSftpButton),
    folderComboBox_(folderComboBox),
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

#ifdef ZEN_WIN_VISTA_AND_LATER
    selectSftpButton.SetBitmapLabel(getResourceImage(L"sftp_small"));
#else
    selectSftpButton_.Hide();
#endif

    //keep dirPicker and dirpath synchronous
    folderComboBox_    .Connect(wxEVT_MOUSEWHEEL,             wxMouseEventHandler  (FolderSelector::onMouseWheel    ), nullptr, this);
    folderComboBox_    .Connect(wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler(FolderSelector::onEditFolderPath), nullptr, this);
    selectFolderButton_.Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderSelector::onSelectFolder  ), nullptr, this);
    selectSftpButton_  .Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderSelector::onSelectSftp    ), nullptr, this);
}


FolderSelector::~FolderSelector()
{
    dropWindow_.Disconnect(EVENT_DROP_FILE, FileDropEventHandler(FolderSelector::onFilesDropped), nullptr, this);

    if (dropWindow2_)
        dropWindow2_->Disconnect(EVENT_DROP_FILE, FileDropEventHandler(FolderSelector::onFilesDropped), nullptr, this);

    folderComboBox_    .Disconnect(wxEVT_MOUSEWHEEL,             wxMouseEventHandler  (FolderSelector::onMouseWheel    ), nullptr, this);
    folderComboBox_    .Disconnect(wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler(FolderSelector::onEditFolderPath), nullptr, this);
    selectFolderButton_.Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderSelector::onSelectFolder  ), nullptr, this);
    selectSftpButton_  .Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderSelector::onSelectSftp    ), nullptr, this);
}


void FolderSelector::onMouseWheel(wxMouseEvent& event)
{
    //for combobox: although switching through available items is wxWidgets default, this is NOT windows default, e.g. explorer
    //additionally this will delete manual entries, although all the users wanted is scroll the parent window!

    //redirect to parent scrolled window!
    wxWindow* wnd = &folderComboBox_;
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
    const auto& itemPaths = event.getPaths();
    if (itemPaths.empty())
        return;

    if (canSetDroppedShellPaths(itemPaths))
    {
        auto fmtShellPath = [](const Zstring& shellItemPath)
        {
            std::unique_ptr<ABF> abf = createAbstractBaseFolder(shellItemPath);

            if (!ABF::folderExists(abf->getAbstractPath()))
            {
                Zstring parentShellPath = beforeLast(shellItemPath, FILE_NAME_SEPARATOR, IF_MISSING_RETURN_NONE);
                if (!parentShellPath.empty())
                {
#ifdef ZEN_WIN
                    if (endsWith(parentShellPath, L":")) //volume root
                        parentShellPath += FILE_NAME_SEPARATOR;
#endif
                    std::unique_ptr<ABF> abfParent = createAbstractBaseFolder(parentShellPath);

                    if (ABF::folderExists(abfParent->getAbstractPath()))
                        return abfParent->getInitPathPhrase();
                    //else: keep original name unconditionally: usecase: inactive mapped network shares
                }
            }
            //make sure FFS-specific explicit MTP-syntax is applied!
            return abf->getInitPathPhrase();
        };

        setPath(fmtShellPath(itemPaths[0]));
        //drop two folder paths at once:
        if (siblingSelector && itemPaths.size() >= 2)
            siblingSelector->setPath(fmtShellPath(itemPaths[1]));

        //notify action invoked by user
        wxCommandEvent dummy(EVENT_ON_FOLDER_SELECTED);
        ProcessEvent(dummy);
    }
    else
        event.Skip(); //let other handlers try -> are there any??
}


void FolderSelector::onEditFolderPath(wxCommandEvent& event)
{
    setFolderPathPhrase(toZ(event.GetString()), nullptr, folderComboBox_, staticText_);

    wxCommandEvent dummy(EVENT_ON_FOLDER_MANUAL_EDIT);
    ProcessEvent(dummy);
    event.Skip();
}


void FolderSelector::onSelectFolder(wxCommandEvent& event)
{
    //make sure default folder exists: don't let folder picker hang on non-existing network share!
    Zstring defaultFolderPath;
#ifdef ZEN_WIN_VISTA_AND_LATER
    std::shared_ptr<const void> /*PCIDLIST_ABSOLUTE*/ defaultFolderPidl;
#endif
    {
        auto baseFolderExisting = [](const ABF& abf)
        {
            std::function<bool()> /*throw FileError*/ dirExists = ABF::getAsyncCheckFolderExists(abf.getAbstractPath()); //noexcept

            auto ft = runAsync([dirExists]
            {
                try
                {
                    return dirExists(); //throw FileError
                }
                catch (FileError&) { return false; }
            });
            return ft.wait_for(std::chrono::milliseconds(200)) == std::future_status::ready && ft.get(); //potentially slow network access: wait 200ms at most
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
    Zstring newFolderPathPhrase;
    try
    {
        //some random GUID => have Windows save IFileDialog state separately from other file/dir pickers!
        const GUID guid = { 0x31f94a00, 0x92b4, 0xa040, { 0x8d, 0xc2, 0xc, 0xa5, 0xef, 0x59, 0x6e, 0x3b } };

        const std::pair<Zstring, bool> rv = ifile::showFolderPicker(static_cast<HWND>(selectFolderButton_.GetHWND()),
                                                                    defaultFolderPath,
                                                                    defaultFolderPidl.get(),
                                                                    &guid,
                                                                    onIFileDialogAcceptFolder); //throw FileError
        if (!rv.second) //cancelled
            return;

        //make sure FFS-specific explicit MTP-syntax is applied!
        newFolderPathPhrase = createAbstractBaseFolder(rv.first)->getInitPathPhrase(); //noexcept
    }
    catch (const FileError& e) { showNotificationDialog(&dropWindow_, DialogInfoType::ERROR2, PopupDialogCfg().setDetailInstructions(e.toString())); return; }
#else
    wxDirDialog dirPicker(&selectFolderButton_, _("Select a folder"), toWx(defaultFolderPath)); //put modal wxWidgets dialogs on stack: creating on freestore leads to memleak!
    if (dirPicker.ShowModal() != wxID_OK)
        return;
    const Zstring newFolderPathPhrase = toZ(dirPicker.GetPath());
#endif

    setFolderPathPhrase(newFolderPathPhrase, &folderComboBox_, folderComboBox_, staticText_);

    //notify action invoked by user
    wxCommandEvent dummy(EVENT_ON_FOLDER_SELECTED);
    ProcessEvent(dummy);
}


void FolderSelector::onSelectSftp(wxCommandEvent& event)
{
#ifdef ZEN_WIN_VISTA_AND_LATER
    Zstring folderPathPhrase = getPath();
    if (showSftpSetupDialog(&selectSftpButton_, folderPathPhrase) != ReturnSmallDlg::BUTTON_OKAY)
        return;

    setFolderPathPhrase(folderPathPhrase, &folderComboBox_, folderComboBox_, staticText_);

    //notify action invoked by user
    wxCommandEvent dummy(EVENT_ON_FOLDER_SELECTED);
    ProcessEvent(dummy);
#endif
}


Zstring FolderSelector::getPath() const
{
    return toZ(folderComboBox_.GetValue());
}


void FolderSelector::setPath(const Zstring& folderPathPhrase)
{
    setFolderPathPhrase(folderPathPhrase, &folderComboBox_, folderComboBox_, staticText_);
}
