// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "main_dlg.h"
#include "resources.h"
#include <wx/msgdlg.h>
#include <wx/wupdlock.h>
#include <wx/filedlg.h>
#include <wx+/bitmap_button.h>
#include <wx+/string_conv.h>
#include <wx+/mouse_move_dlg.h>
#include <wx+/font_size.h>
#include <zen/assert_static.h>
#include <zen/file_handling.h>
#include <zen/build_info.h>
#include "xml_proc.h"
#include "tray_menu.h"
#include "xml_ffs.h"
#include "../lib/help_provider.h"
#include "../lib/process_xml.h"
#include "../lib/ffs_paths.h"
#ifdef ZEN_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

using namespace zen;


class DirectoryPanel : public FolderGenerated
{
public:
    DirectoryPanel(wxWindow* parent) :
        FolderGenerated(parent),
        dirName(*this, *m_buttonSelectDir, *m_txtCtrlDirectory) {}

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
#endif
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    SetIcon(GlobalResources::instance().programIconRTS); //set application icon

    setRelativeFontSize(*m_buttonStart, 1.5);

    m_bpButtonRemoveTopFolder->Hide();
    m_panelMainFolder->Layout();

    m_bpButtonAddFolder      ->SetBitmapLabel(getResourceImage(L"item_add"));
    m_bpButtonRemoveTopFolder->SetBitmapLabel(getResourceImage(L"item_remove"));
    ///m_buttonStart            ->setBitmapFront(getResourceImage(L"startRts"), 5);

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
        catch (const xmlAccess::FfsXmlError& error)
        {
            if (error.getSeverity() == xmlAccess::FfsXmlError::WARNING)
                wxMessageBox(error.toString(),L"RealtimeSync" +  _("Warning"), wxOK | wxICON_WARNING, this);
            else
                wxMessageBox(error.toString(), L"RealtimeSync" + _("Error"), wxOK | wxICON_ERROR, this);
        }

    const bool startWatchingImmediately = loadCfgSuccess && !cfgFileName.empty();

    setConfiguration(newConfig);
    setLastUsedConfig(currentConfigFile);
    //-----------------------------------------------------------------------------------------
    //Layout();
    //Fit();

    m_scrolledWinFolders->Fit(); //adjust scrolled window size
    m_scrolledWinFolders->Layout(); //fix small layout problem
    m_panelMain->Layout();       //adjust stuff inside scrolled window
    Fit();                       //adapt dialog size

    Center();

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
        //if the executable is not yet in a bundle or if it is called through a launcher, we need to set focus manually:
        ::SetFrontProcess(&psn);
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
    catch (const xmlAccess::FfsXmlError& error)
    {
        wxMessageBox(error.toString().c_str(), L"RealtimeSync" + _("Error"), wxOK | wxICON_ERROR, this);
    }
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
    //build information
    wxString build = __TDATE__;
#if wxUSE_UNICODE
    build += L" - Unicode";
#else
    build += L" - ANSI";
#endif //wxUSE_UNICODE

    //compile time info about 32/64-bit build
    if (zen::is64BitBuild)
        build += L" x64";
    else
        build += L" x86";
    assert_static(zen::is32BitBuild || zen::is64BitBuild);

    wxMessageBox(L"RealtimeSync" L"\n\n" + replaceCpy(_("Build: %x"), L"%x", build), _("About"), wxOK, this);
}


void MainDialog::OnKeyPressed(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();

    if (keyCode == WXK_ESCAPE)
        Close();

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
    ::TransformProcessType(&psn, kProcessTransformToForegroundApplication); //show dock icon again
    ::SetFrontProcess(&psn); //why isn't this covered by wxWindows::Raise()??
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
        wxMessageBox(e.toString().c_str(), L"RealtimeSync" + _("Error"), wxOK | wxICON_ERROR, this);
    }
}


void MainDialog::loadConfig(const Zstring& filename)
{
    xmlAccess::XmlRealConfig newConfig;

    try
    {
        rts::readRealOrBatchConfig(filename, newConfig);
    }
    catch (const xmlAccess::FfsXmlError& error)
    {
        if (error.getSeverity() == xmlAccess::FfsXmlError::WARNING)
            wxMessageBox(error.toString(), L"RealtimeSync" + _("Warning"), wxOK | wxICON_WARNING, this);
        else
        {
            wxMessageBox(error.toString(), L"RealtimeSync" + _("Error"), wxOK | wxICON_ERROR, this);
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
        SetTitle(_("RealtimeSync - Automated Synchronization"));
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
                            wxString(L"RealtimeSync (*.ffs_real;*.ffs_batch)|*.ffs_real;*.ffs_batch") + L"|" +_("All files") + L" (*.*)|*",
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
    for (auto it = dirNamesExtra.begin(); it != dirNamesExtra.end(); ++it)
        output.directories.push_back(utfCvrtTo<Zstring>((*it)->getName()));

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
    for (std::vector<DirectoryPanel*>::const_iterator i = dirNamesExtra.begin(); i != dirNamesExtra.end(); ++i)
        if (eventObj == static_cast<wxObject*>((*i)->m_bpButtonRemoveFolder))
        {
            removeAddFolder(i - dirNamesExtra.begin());
            return;
        }
}


void MainDialog::OnRemoveTopFolder(wxCommandEvent& event)
{
    if (dirNamesExtra.size() > 0)
    {
        const wxString topDir = (*dirNamesExtra.begin())->getName();

        dirNameFirst->setName(topDir);

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

    wxWindowUpdateLocker dummy(this); //avoid display distortion

    int folderHeight = 0;
    for (auto it = newFolders.begin(); it != newFolders.end(); ++it)
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
        newFolder->setName(utfCvrtTo<wxString>(*it));
    }

    //set size of scrolled window
    const size_t additionalRows = std::min(dirNamesExtra.size(), MAX_ADD_FOLDERS); //up to MAX_ADD_FOLDERS additional folders shall be shown
    m_scrolledWinFolders->SetMinSize(wxSize( -1, folderHeight * static_cast<int>(additionalRows)));

    //adapt delete top folder pair button
    m_bpButtonRemoveTopFolder->Show();
    m_panelMainFolder->Layout();

    //update controls
    m_scrolledWinFolders->Fit(); //adjust scrolled window size
    m_scrolledWinFolders->Layout(); //fix small layout problem
    m_panelMain->Layout();       //adjust stuff inside scrolled window
    Fit();                       //adapt dialog size
}


void MainDialog::removeAddFolder(size_t pos)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

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

        //update controls
        m_scrolledWinFolders->Fit(); //adjust scrolled window size
        m_panelMain->Layout();       //adjust stuff inside scrolled window
        Fit();                       //adapt dialog size
    }
}


void MainDialog::clearAddFolders()
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    dirNamesExtra.clear();
    bSizerFolders->Clear(true);

    m_bpButtonRemoveTopFolder->Hide();
    m_panelMainFolder->Layout();

    m_scrolledWinFolders->SetMinSize(wxSize(-1, 0));
    Fit();                       //adapt dialog size
}
