// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "main_dlg.h"
#include "resources.h"
#include <wx+/button.h>
#include "../lib/ffs_paths.h"
#include <wx/msgdlg.h>
#include <wx/wupdlock.h>
#include "watcher.h"
#include <wx/utils.h>
#include "xml_proc.h"
#include "tray_menu.h"
#include <zen/file_handling.h>
#include "xml_ffs.h"
#include <wx+/string_conv.h>
#include <zen/assert_static.h>
#include <zen/build_info.h>
#include "../lib/help_provider.h"
#include "../lib/process_xml.h"
#include <wx+/mouse_move_dlg.h>

using namespace zen;


MainDialog::MainDialog(wxDialog* dlg, const wxString& cfgFileName)
    : MainDlgGenerated(dlg)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    m_bpButtonRemoveTopFolder->Hide();
    m_panelMainFolder->Layout();

    m_bpButtonAddFolder      ->SetBitmapLabel(GlobalResources::getImage(L"addFolderPair"));
    m_bpButtonRemoveTopFolder->SetBitmapLabel(GlobalResources::getImage(L"removeFolderPair"));
    m_buttonStart            ->setBitmapFront(GlobalResources::getImage(L"startRed"));

    //register key event
    Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(MainDialog::OnKeyPressed), nullptr, this);

    //prepare drag & drop
    dirNameFirst.reset(new DirectoryName<wxTextCtrl>(*m_panelMainFolder, *m_dirPickerMain, *m_txtCtrlDirectoryMain, m_staticTextFinalPath));

#ifdef FFS_WIN
    new MouseMoveWindow(*this); //ownership passed to "this"
#endif

    //--------------------------- load config values ------------------------------------
    xmlAccess::XmlRealConfig newConfig;

    const wxString currentConfigFile = cfgFileName.empty() ? lastConfigFileName() : cfgFileName;

    bool loadCfgSuccess = false;
    if (!cfgFileName.empty() || wxFileExists(lastConfigFileName()))
        try
        {
            rts::readRealOrBatchConfig(toZ(currentConfigFile), newConfig);
            loadCfgSuccess = true;
        }
        catch (const xmlAccess::FfsXmlError& error)
        {
            if (error.getSeverity() == xmlAccess::FfsXmlError::WARNING)
                wxMessageBox(error.toString(), _("Warning"), wxOK | wxICON_WARNING, this);
            else
                wxMessageBox(error.toString(), _("Error"), wxOK | wxICON_ERROR, this);
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
    }
    else
        m_buttonStart->SetFocus(); //don't "steal" focus if program is running from sys-tray"
}


MainDialog::~MainDialog()
{
    //save current configuration
    const xmlAccess::XmlRealConfig currentCfg = getConfiguration();

    try //write config to XML
    {
        writeRealConfig(currentCfg, toZ(lastConfigFileName()));
    }
    catch (const xmlAccess::FfsXmlError& error)
    {
        wxMessageBox(error.toString().c_str(), _("Error"), wxOK | wxICON_ERROR, this);
    }
}


const wxString& MainDialog::lastConfigFileName()
{
    static wxString instance = toWx(zen::getConfigDir()) + L"LastRun.ffs_real";
    return instance;
}


void MainDialog::OnShowHelp(wxCommandEvent& event)
{
    zen::displayHelpEntry(L"html/RealtimeSync.html");
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

    wxMessageBox(L"RealtimeSync" L"\n\n" + replaceCpy(_("(Build: %x)"), L"%x", build), _("About"), wxOK, this);
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

    switch (rts::startDirectoryMonitor(currentCfg, xmlAccess::extractJobName(utfCvrtTo<Zstring>(currentConfigFileName))))
    {
        case rts::EXIT_APP:
            Close();
            return;

        case rts::SHOW_GUI:
            break;
    }
    Show(); //don't show for EXIT_APP
}


void MainDialog::OnConfigSave(wxCommandEvent& event)
{
    wxString defaultFileName = currentConfigFileName.empty() ? L"Realtime.ffs_real" : currentConfigFileName;
    //attention: currentConfigFileName may be an imported *.ffs_batch file! We don't want to overwrite it with a GUI config!
    if (endsWith(defaultFileName, L".ffs_batch"))
        replace(defaultFileName, L".ffs_batch", L".ffs_real", false);


    wxFileDialog filePicker(this, wxEmptyString, wxEmptyString, defaultFileName,
                            wxString(L"RealtimeSync (*.ffs_real)|*.ffs_real") + L"|" +_("All files") + L" (*.*)|*",
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (filePicker.ShowModal() != wxID_OK)
        return;

    const wxString newFileName = filePicker.GetPath();

    //write config to XML
    const xmlAccess::XmlRealConfig currentCfg = getConfiguration();
    try
    {
        writeRealConfig(currentCfg, toZ(newFileName)); //throw FfsXmlError
        setLastUsedConfig(newFileName);
    }
    catch (const xmlAccess::FfsXmlError& e)
    {
        wxMessageBox(e.toString().c_str(), _("Error"), wxOK | wxICON_ERROR, this);
    }
}


void MainDialog::loadConfig(const wxString& filename)
{
    xmlAccess::XmlRealConfig newConfig;

    try
    {
        rts::readRealOrBatchConfig(toZ(filename), newConfig);
    }
    catch (const xmlAccess::FfsXmlError& error)
    {
        if (error.getSeverity() == xmlAccess::FfsXmlError::WARNING)
            wxMessageBox(error.toString(), _("Warning"), wxOK | wxICON_WARNING);
        else
        {
            wxMessageBox(error.toString(), _("Error"), wxOK | wxICON_ERROR);
            return;
        }
    }

    setConfiguration(newConfig);
    setLastUsedConfig(filename);
}


void MainDialog::setLastUsedConfig(const wxString& filename)
{
    //set title
    if (filename == lastConfigFileName())
    {
        SetTitle(_("RealtimeSync - Automated Synchronization"));
        currentConfigFileName.clear();
    }
    else
    {
        SetTitle(filename);
        currentConfigFileName = filename;
    }
}


void MainDialog::OnConfigLoad(wxCommandEvent& event)
{
    wxFileDialog filePicker(this, wxEmptyString, wxEmptyString, wxEmptyString,
                            wxString(L"RealtimeSync (*.ffs_real;*.ffs_batch)|*.ffs_real;*.ffs_batch") + L"|" +_("All files") + L" (*.*)|*",
                            wxFD_OPEN);
    if (filePicker.ShowModal() == wxID_OK)
        loadConfig(filePicker.GetPath());
}


void MainDialog::setConfiguration(const xmlAccess::XmlRealConfig& cfg)
{
    //clear existing folders
    dirNameFirst->setName(wxString());

    clearAddFolders();

    if (!cfg.directories.empty())
    {
        //fill top folder
        dirNameFirst->setName(*cfg.directories.begin());

        //fill additional folders
        addFolder(std::vector<wxString>(cfg.directories.begin() + 1, cfg.directories.end()));
    }

    //fill commandline
    m_textCtrlCommand->SetValue(cfg.commandline);

    //set delay
    m_spinCtrlDelay->SetValue(static_cast<int>(cfg.delay));
}


xmlAccess::XmlRealConfig MainDialog::getConfiguration()
{
    xmlAccess::XmlRealConfig output;

    output.directories.push_back(dirNameFirst->getName());
    for (std::vector<DirectoryPanel*>::const_iterator i = dirNamesExtra.begin(); i != dirNamesExtra.end(); ++i)
        output.directories.push_back((*i)->getName());

    output.commandline = m_textCtrlCommand->GetValue();
    output.delay       = m_spinCtrlDelay->GetValue();;

    return output;
}


void MainDialog::OnAddFolder(wxCommandEvent& event)
{
    const wxString topFolder = dirNameFirst->getName();

    //clear existing top folder first
    dirNameFirst->setName(wxString());

    std::vector<wxString> newFolders;
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


#ifdef FFS_WIN
static const size_t MAX_ADD_FOLDERS = 8;
#elif defined FFS_LINUX
static const size_t MAX_ADD_FOLDERS = 6;
#endif


void MainDialog::addFolder(const std::vector<wxString>& newFolders, bool addFront)
{
    if (newFolders.size() == 0)
        return;

    wxWindowUpdateLocker dummy(this); //avoid display distortion

    int folderHeight = 0;
    for (std::vector<wxString>::const_iterator i = newFolders.begin(); i != newFolders.end(); ++i)
    {
        //add new folder pair
        DirectoryPanel* newFolder = new DirectoryPanel(m_scrolledWinFolders);
        newFolder->m_bpButtonRemoveFolder->SetBitmapLabel(GlobalResources::getImage(wxT("removeFolderPair")));

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
        newFolder->setName(*i);
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


void MainDialog::removeAddFolder(int pos)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    if (0 <= pos && pos < static_cast<int>(dirNamesExtra.size()))
    {
        //remove folder pairs from window
        DirectoryPanel* dirToDelete = dirNamesExtra[pos];
        const int folderHeight = dirToDelete->GetSize().GetHeight();

        bSizerFolders->Detach(dirToDelete); //Remove() does not work on Window*, so do it manually
        dirToDelete->Destroy();                 //
        dirNamesExtra.erase(dirNamesExtra.begin() + pos); //remove last element in vector


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
