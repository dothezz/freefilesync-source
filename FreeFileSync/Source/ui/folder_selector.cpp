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
#ifdef _MSC_VER
    #include "../fs/mtp.h"
#endif
#ifdef ZEN_WIN
    #include <zen/dll.h>
    #include <zen/win_ver.h>
    #include "../dll/IFileDialog_Vista\ifile_dialog.h"
#endif

using namespace zen;


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
    //prepare drag & drop
    setupFileDrop(dropWindow_);
    dropWindow_.Connect(EVENT_DROP_FILE, FileDropEventHandler(FolderSelector::onFilesDropped), nullptr, this);

    if (dropWindow2_)
    {
        setupFileDrop(*dropWindow2_);
        dropWindow2_->Connect(EVENT_DROP_FILE, FileDropEventHandler(FolderSelector::onFilesDropped), nullptr, this);
    }

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

    if (acceptDrop(files, event.getDropPosition(), event.getDropWindow()))
    {
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

        //notify action invoked by user
        wxCommandEvent dummy(EVENT_ON_DIR_SELECTED);
        ProcessEvent(dummy);
    }
    else
        event.Skip(); //let other handlers try!!!
}


void FolderSelector::onWriteDirManually(wxCommandEvent& event)
{
    setFolderPath(toZ(event.GetString()), nullptr, dirpath_, staticText_);

    wxCommandEvent dummy(EVENT_ON_DIR_MANUAL_CORRECTION);
    ProcessEvent(dummy);
    event.Skip();
}


#ifdef ZEN_WIN
bool onIFileDialogAcceptFolder(void* wnd /*HWND*/, const wchar_t* folderPath, void* sink)
{
    if (acceptsFolderPathPhraseNative(folderPath)) //noexcept)
        return true;
#ifdef _MSC_VER
    if (acceptsFolderPathPhraseMtp   (folderPath)) //noexcept)
        return true;
#endif

    const std::wstring msg = replaceCpy(_("The selected folder %x cannot be used with FreeFileSync. Please select a folder on a local file system, network or an MTP device."), L"%x", fmtFileName(folderPath));
    ::MessageBox(static_cast<HWND>(wnd), msg.c_str(), (_("Select a folder")).c_str(), MB_ICONWARNING);
    //showNotificationDialog would not support HWND parent
    return false;
}
#endif


void FolderSelector::onSelectDir(wxCommandEvent& event)
{
    //IFileDialog requirements for default path: 1. accepts native paths only!!! 2. path must exist!
    Zstring defaultFolderPath;
    if (Opt<Zstring> nativeFolderPath = AbstractBaseFolder::getNativeItemPath(createAbstractBaseFolder(getPath())->getAbstractPath()))
        if (!nativeFolderPath->empty())
        {
            auto ft = async([nativeFolderPath] { return dirExists(*nativeFolderPath); });

            if (ft.timed_wait(boost::posix_time::milliseconds(200)) && ft.get()) //potentially slow network access: wait 200ms at most
                defaultFolderPath = *nativeFolderPath;
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

            const ifile::GuidProxy guid = { '\x0', '\x4a', '\xf9', '\x31', '\xb4', '\x92', '\x40', '\xa0',
                                            '\x8d', '\xc2', '\xc', '\xa5', '\xef', '\x59', '\x6e', '\x3b'
                                          }; //some random GUID => have Windows save IFileDialog state separately from other file/dir pickers!

            showFolderPicker(static_cast<HWND>(selectButton_.GetHWND()), //in;  ==HWND
                             defaultFolderPath.empty() ? nullptr : defaultFolderPath.c_str(), //in, optional!
                             &guid,
                             onIFileDialogAcceptFolder,
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

#ifdef _MSC_VER
            //make sure FFS-specific explicit MTP-syntax is applied!
            if (acceptsFolderPathPhraseMtp(selectedFolder)) //noexcept
                newFolder = getResolvedDisplayPathMtp(selectedFolder); //noexcept
            else
#endif
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
