// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "main_dlg.h"
#include <wx/wupdlock.h>
#include <wx/filedlg.h>
#include <wx+/bitmap_button.h>
#include <wx+/string_conv.h>
#include <wx+/mouse_move_dlg.h>
#include <wx+/font_size.h>
#include <wx+/popup_dlg.h>
#include <wx+/image_resources.h>
#include <zen/assert_static.h>
#include <zen/file_handling.h>
#include <zen/build_info.h>
#include "xml_proc.h"
#include "tray_menu.h"
#include "xml_ffs.h"
#include "app_icon.h"
#include "../lib/help_provider.h"
#include "../lib/process_xml.h"
#include "../lib/ffs_paths.h"
#ifdef ZEN_LINUX
#include <gtk/gtk.h>
#elif defined ZEN_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

using namespace zen;


class DirectoryPanel : public FolderGenerated
{
public:
    DirectoryPanel(wxWindow* parent) :
        FolderGenerated(parent),
        dirName(*this, *m_buttonSelectDir, *m_txtCtrlDirectory)
    {
#ifdef ZEN_LINUX
        //file drag and drop directly into the text control unhelpfully inserts in format "file://..<cr><nl>"; see folder_history_box.cpp
        if (GtkWidget* widget = m_txtCtrlDirectory->GetConnectWidget())
            ::gtk_drag_dest_unset(widget);
#endif
    }

    void setName(const wxString& dirname) { dirName.setName(dirname); }
    wxString getName() const { return dirName.getName(); }

private:
    zen::DirectoryName<wxTextCtrl> dirName;
};


void MainDialog::create(const Zstring& cfgFile)
{
    /*MainDialog* frame = */ new MainDialog(nullptr, cfgFile);
}


MainDialog::MainDialog(wxDialog* dlg, const Zstring& cfgFileName)
    : MainDlgGenerated(dlg)
{
#ifdef ZEN_WIN
    new MouseMoveWindow(*this); //ownership passed to "this"
    wxWindowUpdateLocker dummy(this); //leads to GUI corruption problems on Linux/OS X!
#endif

#ifdef ZEN_LINUX
    //file drag and drop directly into the text control unhelpfully inserts in format "file://..<cr><nl>"; see folder_history_box.cpp
    if (GtkWidget* widget = m_txtCtrlDirectoryMain->GetConnectWidget())
        ::gtk_drag_dest_unset(widget);
#endif

    SetIcon(getRtsIcon()); //set application icon

    setRelativeFontSize(*m_buttonStart, 1.5);

    m_bpButtonRemoveTopFolder->Hide();
    m_panelMainFolder->Layout();

    m_bpButtonAddFolder      ->SetBitmapLabel(getResourceImage(L"item_add"));
    m_bpButtonRemoveTopFolder->SetBitmapLabel(getResourceImage(L"item_remove"));
    setBitmapTextLabel(*m_buttonStart, getResourceImage(L"startRts").ConvertToImage(), m_buttonStart->GetLabel(), 5, 8);


    //register key event
    Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(MainDialog::OnKeyPressed), nullptr, this);

    //prepare drag & drop
    dirNameFirst.reset(new DirectoryName<wxTextCtrl>(*m_panelMainFolder, *m_buttonSelectDirMain, *m_txtCtrlDirectoryMain, m_staticTextFinalPath));

    //--------------------------- load config values ------------------------------------
    xmlAccess::XmlRealConfig newConfig;

    const Zstring currentConfigFile = cfgFileName.empty() ? lastConfigFileName() : cfgFileName;
    bool loadCfgSuccess = false;
    if (!cfgFileName.empty() || fileExists(lastConfigFileName()))
        try
        {
            rts::readRealOrBatchConfig(currentConfigFile, newConfig); //throw FfsXmlError
            loadCfgSuccess = true;
        }
        catch (const xmlAccess::FfsXmlError& e)
        {
            if (e.getSeverity() == xmlAccess::FfsXmlError::WARNING)
                showNotificationDialog(this, DialogInfoType::WARNING, PopupDialogCfg().setDetailInstructions(e.toString()));
            else
                showNotificationDialog(this, DialogInfoType::ERROR2, PopupDialogCfg().setDetailInstructions(e.toString()));
        }

    const bool startWatchingImmediately = loadCfgSuccess && !cfgFileName.empty();

    setConfiguration(newConfig);
    setLastUsedConfig(currentConfigFile);
    //-----------------------------------------------------------------------------------------

    if (startWatchingImmediately) //start watch mode directly
    {
        wxCommandEvent dummy2(wxEVT_COMMAND_BUTTON_CLICKED);
        this->OnStart(dummy2);
        //don't Show()!
    }
    else
    {
        m_buttonStart->SetFocus(); //don't "steal" focus if program is running from sys-tray"
        Show();
#ifdef ZEN_MAC
        ProcessSerialNumber psn = { 0, kCurrentProcess };
        ::TransformProcessType(&psn, kProcessTransformToForegroundApplication); //show dock icon, even if we're not an application bundle
        ::SetFrontProcess(&psn); //call before TransformProcessType() so that OSX menu is updated correctly
        //if the executable is not yet in a bundle or if it is called through a launcher, we need to set focus manually:
#endif
    }

    //drag and drop .ffs_real and .ffs_batch on main dialog
    setupFileDrop(*m_panelMain);
    m_panelMain->Connect(EVENT_DROP_FILE, FileDropEventHandler(MainDialog::onFilesDropped), nullptr, this);

    timerForAsyncTasks.Connect(wxEVT_TIMER, wxEventHandler(MainDialog::onProcessAsyncTasks), nullptr, this);
}


MainDialog::~MainDialog()
{
    //save current configuration
    const xmlAccess::XmlRealConfig currentCfg = getConfiguration();

    try //write config to XML
    {
        writeRealConfig(currentCfg, lastConfigFileName()); //throw FfsXmlError
    }
    catch (const xmlAccess::FfsXmlError& e)
    {
        showNotificationDialog(this, DialogInfoType::ERROR2, PopupDialogCfg().setDetailInstructions(e.toString()));
    }
}


void MainDialog::onQueryEndSession()
{
    try { writeRealConfig(getConfiguration(), lastConfigFileName()); } //throw FfsXmlError
    catch (const xmlAccess::FfsXmlError&) {} //we try our best do to something useful in this extreme situation - no reason to notify or even log errors here!
}


void MainDialog::onProcessAsyncTasks(wxEvent& event)
{
    //schedule and run long-running tasks asynchronously
    asyncTasks.evalResults(); //process results on GUI queue
    if (asyncTasks.empty())
        timerForAsyncTasks.Stop();
}


const Zstring& MainDialog::lastConfigFileName()
{
    static Zstring instance = zen::getConfigDir() + Zstr("LastRun.ffs_real");
    return instance;
}


void MainDialog::OnShowHelp(wxCommandEvent& event)
{
    zen::displayHelpEntry(L"html/RealtimeSync.html", this);
}


void MainDialog::OnMenuAbout(wxCommandEvent& event)
{
    wxString build = __TDATE__;
    build += L" - Unicode";
#ifndef wxUSE_UNICODE
#error what is going on?
#endif

	build += zen::is64BitBuild ? L" x64" : L" x86";
    assert_static(zen::is32BitBuild || zen::is64BitBuild);

    showNotificationDialog(this, DialogInfoType::INFO, PopupDialogCfg().
                           setTitle(_("About")).
                           setMainInstructions(L"RealtimeSync" L"\n\n" + replaceCpy(_("Build: %x"), L"%x", build)));
}


void MainDialog::OnKeyPressed(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();
    if (keyCode == WXK_ESCAPE)
    {
        Close();
        return;
    }
    event.Skip();
}


void MainDialog::OnStart(wxCommandEvent& event)
{
    xmlAccess::XmlRealConfig currentCfg = getConfiguration();

    Hide();
#ifdef ZEN_MAC
    //hide dock icon: else user is able to forcefully show the hidden main dialog by clicking on the icon!!
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    ::TransformProcessType(&psn, kProcessTransformToUIElementApplication);
#endif

    switch (rts::startDirectoryMonitor(currentCfg, xmlAccess::extractJobName(utfCvrtTo<Zstring>(currentConfigFileName))))
    {
        case rts::EXIT_APP:
            Close();
            return;

        case rts::SHOW_GUI:
            break;
    }
    Show(); //don't show for EXIT_APP
#ifdef ZEN_MAC
    //why isn't this covered by wxWindows::Raise()??
    ::TransformProcessType(&psn, kProcessTransformToForegroundApplication); //show dock icon again
    ::SetFrontProcess(&psn); //call before TransformProcessType() so that OSX menu is updated correctly
#endif
    Raise();
}


void MainDialog::OnConfigSave(wxCommandEvent& event)
{
    Zstring defaultFileName = currentConfigFileName.empty() ? Zstr("Realtime.ffs_real") : currentConfigFileName;
    //attention: currentConfigFileName may be an imported *.ffs_batch file! We don't want to overwrite it with a GUI config!
    if (endsWith(defaultFileName, Zstr(".ffs_batch")))
        replace(defaultFileName, Zstr(".ffs_batch"), Zstr(".ffs_real"), false);


    wxFileDialog filePicker(this,
                            wxEmptyString,
                            //OS X really needs dir/file separated like this:
                            utfCvrtTo<wxString>(beforeLast(defaultFileName, FILE_NAME_SEPARATOR)), //default dir; empty string if / not found
                            utfCvrtTo<wxString>(afterLast (defaultFileName, FILE_NAME_SEPARATOR)), //default file; whole string if / not found
                            wxString(L"RealtimeSync (*.ffs_real)|*.ffs_real") + L"|" +_("All files") + L" (*.*)|*",
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (filePicker.ShowModal() != wxID_OK)
        return;

    const Zstring newFileName = utfCvrtTo<Zstring>(filePicker.GetPath());

    //write config to XML
    const xmlAccess::XmlRealConfig currentCfg = getConfiguration();
    try
    {
        writeRealConfig(currentCfg, newFileName); //throw FfsXmlError
        setLastUsedConfig(newFileName);
    }
    catch (const xmlAccess::FfsXmlError& e)
    {
        showNotificationDialog(this, DialogInfoType::ERROR2, PopupDialogCfg().setDetailInstructions(e.toString()));
    }
}


void MainDialog::loadConfig(const Zstring& filename)
{
    xmlAccess::XmlRealConfig newConfig;

    try
    {
        rts::readRealOrBatchConfig(filename, newConfig);
    }
    catch (const xmlAccess::FfsXmlError& e)
    {
        if (e.getSeverity() == xmlAccess::FfsXmlError::WARNING)
            showNotificationDialog(this, DialogInfoType::WARNING, PopupDialogCfg().setDetailInstructions(e.toString()));
        else
        {
            showNotificationDialog(this, DialogInfoType::ERROR2, PopupDialogCfg().setDetailInstructions(e.toString()));
            return;
        }
    }

    setConfiguration(newConfig);
    setLastUsedConfig(filename);
}


void MainDialog::setLastUsedConfig(const Zstring& filename)
{
    //set title
    if (filename == lastConfigFileName())
    {
        SetTitle(L"RealtimeSync - " + _("Automated Synchronization"));
        currentConfigFileName.clear();
    }
    else
    {
        SetTitle(utfCvrtTo<wxString>(filename));
        currentConfigFileName = filename;
    }
}


void MainDialog::OnConfigLoad(wxCommandEvent& event)
{
    wxFileDialog filePicker(this,
                            wxEmptyString,
                            utfCvrtTo<wxString>(beforeLast(currentConfigFileName, FILE_NAME_SEPARATOR)), //default dir; empty string if / not found
                            wxEmptyString,
                            wxString(L"RealtimeSync (*.ffs_real; *.ffs_batch)|*.ffs_real;*.ffs_batch") + L"|" +_("All files") + L" (*.*)|*",
                            wxFD_OPEN);
    if (filePicker.ShowModal() == wxID_OK)
        loadConfig(utfCvrtTo<Zstring>(filePicker.GetPath()));
}


void MainDialog::onFilesDropped(FileDropEvent& event)
{
    const auto& files = event.getFiles();
    if (!files.empty())
        loadConfig(utfCvrtTo<Zstring>(files[0]));
}


void MainDialog::setConfiguration(const xmlAccess::XmlRealConfig& cfg)
{
    //clear existing folders
    dirNameFirst->setName(wxString());
    clearAddFolders();

    if (!cfg.directories.empty())
    {
        //fill top folder
        dirNameFirst->setName(utfCvrtTo<wxString>(*cfg.directories.begin()));

        //fill additional folders
        addFolder(std::vector<Zstring>(cfg.directories.begin() + 1, cfg.directories.end()));
    }

    //fill commandline
    m_textCtrlCommand->SetValue(utfCvrtTo<wxString>(cfg.commandline));

    //set delay
    m_spinCtrlDelay->SetValue(static_cast<int>(cfg.delay));
}


xmlAccess::XmlRealConfig MainDialog::getConfiguration()
{
    xmlAccess::XmlRealConfig output;

    output.directories.push_back(utfCvrtTo<Zstring>(dirNameFirst->getName()));
    for (const DirectoryPanel* dne : dirNamesExtra)
        output.directories.push_back(utfCvrtTo<Zstring>(dne->getName()));

    output.commandline = utfCvrtTo<Zstring>(m_textCtrlCommand->GetValue());
    output.delay       = m_spinCtrlDelay->GetValue();

    return output;
}


void MainDialog::OnAddFolder(wxCommandEvent& event)
{
    const Zstring topFolder = utfCvrtTo<Zstring>(dirNameFirst->getName());

    //clear existing top folder first
    dirNameFirst->setName(wxString());

    std::vector<Zstring> newFolders;
    newFolders.push_back(topFolder);

    addFolder(newFolders, true); //add pair in front of additonal pairs
}


void MainDialog::OnRemoveFolder(wxCommandEvent& event)
{
    //find folder pair originating the event
    const wxObject* const eventObj = event.GetEventObject();
    for (auto it = dirNamesExtra.begin(); it != dirNamesExtra.end(); ++it)
        if (eventObj == static_cast<wxObject*>((*it)->m_bpButtonRemoveFolder))
        {
            removeAddFolder(it - dirNamesExtra.begin());
            return;
        }
}


void MainDialog::OnRemoveTopFolder(wxCommandEvent& event)
{
    if (dirNamesExtra.size() > 0)
    {
        dirNameFirst->setName(dirNamesExtra[0]->getName());
        removeAddFolder(0); //remove first of additional folders
    }
}


#ifdef ZEN_WIN
static const size_t MAX_ADD_FOLDERS = 8;
#elif defined ZEN_LINUX || defined ZEN_MAC
static const size_t MAX_ADD_FOLDERS = 6;
#endif


void MainDialog::addFolder(const std::vector<Zstring>& newFolders, bool addFront)
{
    if (newFolders.size() == 0)
        return;

#ifdef ZEN_WIN
    wxWindowUpdateLocker dummy(this); //leads to GUI corruption problems on Linux/OS X!
#endif

    int folderHeight = 0;
    for (const Zstring& dirname : newFolders)
    {
        //add new folder pair
        DirectoryPanel* newFolder = new DirectoryPanel(m_scrolledWinFolders);
        newFolder->m_bpButtonRemoveFolder->SetBitmapLabel(getResourceImage(L"item_remove"));

        //get size of scrolled window
        folderHeight = newFolder->GetSize().GetHeight();

        if (addFront)
        {
            bSizerFolders->Insert(0, newFolder, 0, wxEXPAND, 5);
            dirNamesExtra.insert(dirNamesExtra.begin(), newFolder);
        }
        else
        {
            bSizerFolders->Add(newFolder, 0, wxEXPAND, 5);
            dirNamesExtra.push_back(newFolder);
        }

        //register events
        newFolder->m_bpButtonRemoveFolder->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainDialog::OnRemoveFolder), nullptr, this );

        //insert directory name
        newFolder->setName(utfCvrtTo<wxString>(dirname));
    }

    //set size of scrolled window
    const size_t additionalRows = std::min(dirNamesExtra.size(), MAX_ADD_FOLDERS); //up to MAX_ADD_FOLDERS additional folders shall be shown
    m_scrolledWinFolders->SetMinSize(wxSize( -1, folderHeight * static_cast<int>(additionalRows)));

    //adapt delete top folder pair button
    m_bpButtonRemoveTopFolder->Show();

    GetSizer()->SetSizeHints(this); //~=Fit() + SetMinSize()
    Layout();
    Refresh(); //remove a little flicker near the start button
}


void MainDialog::removeAddFolder(size_t pos)
{
#ifdef ZEN_WIN
    wxWindowUpdateLocker dummy(this); //leads to GUI corruption problems on Linux/OS X!
#endif

    if (pos < dirNamesExtra.size())
    {
        //remove folder pairs from window
        DirectoryPanel* pairToDelete = dirNamesExtra[pos];
        const int folderHeight = pairToDelete->GetSize().GetHeight();

        bSizerFolders->Detach(pairToDelete); //Remove() does not work on Window*, so do it manually
        dirNamesExtra.erase(dirNamesExtra.begin() + pos); //remove last element in vector
        //more (non-portable) wxWidgets bullshit: on OS X wxWindow::Destroy() screws up and calls "operator delete" directly rather than
        //the deferred deletion it is expected to do (and which is implemented correctly on Windows and Linux)
        //http://bb10.com/python-wxpython-devel/2012-09/msg00004.html
        //=> since we're in a mouse button callback of a sub-component of "pairToDelete" we need to delay deletion ourselves:
        processAsync2([] {}, [pairToDelete] { pairToDelete->Destroy(); });

        //set size of scrolled window
        const size_t additionalRows = std::min(dirNamesExtra.size(), MAX_ADD_FOLDERS); //up to MAX_ADD_FOLDERS additional folders shall be shown
        m_scrolledWinFolders->SetMinSize(wxSize( -1, folderHeight * static_cast<int>(additionalRows)));

        //adapt delete top folder pair button
        if (dirNamesExtra.size() == 0)
        {
            m_bpButtonRemoveTopFolder->Hide();
            m_panelMainFolder->Layout();
        }

        GetSizer()->SetSizeHints(this); //~=Fit() + SetMinSize()
        Layout();
        Refresh(); //remove a little flicker near the start button
    }
}


void MainDialog::clearAddFolders()
{
#ifdef ZEN_WIN
    wxWindowUpdateLocker dummy(this); //leads to GUI corruption problems on Linux/OS X!
#endif

    bSizerFolders->Clear(true);
    dirNamesExtra.clear();

    m_scrolledWinFolders->SetMinSize(wxSize(-1, 0));

    m_bpButtonRemoveTopFolder->Hide();
    m_panelMainFolder->Layout();

    GetSizer()->SetSizeHints(this); //~=Fit()
    Layout();
    Refresh(); //remove a little flicker near the start button
}
