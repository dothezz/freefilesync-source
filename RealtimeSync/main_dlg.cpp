// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "main_dlg.h"
#include "resources.h"
#include "../shared/custom_button.h"
#include "../shared/standard_paths.h"
//#include "functions.h"
#include <wx/msgdlg.h>
#include <wx/wupdlock.h>
#include "watcher.h"
#include <wx/utils.h>
#include "xml_proc.h"
#include "tray_menu.h"
#include "../shared/file_handling.h"
#include "xml_ffs.h"
#include "../shared/system_constants.h"
#include "../shared/string_conv.h"
#include "../shared/assert_static.h"
#include "../shared/build_info.h"
#include "../shared/help_provider.h"

using namespace ffs3;


MainDialog::MainDialog(wxDialog *dlg,
                       const wxString& cfgFilename)
    : MainDlgGenerated(dlg)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    m_bpButtonRemoveTopFolder->Hide();
    m_panelMainFolder->Layout();

    m_bpButtonAddFolder->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("addFolderPair")));
    m_bpButtonRemoveTopFolder->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("removeFolderPair")));
    m_buttonStart->setBitmapFront(GlobalResources::getInstance().getImageByName(wxT("startRed")));

    //register key event
    Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(MainDialog::OnKeyPressed), NULL, this);

    //prepare drag & drop
    dirNameFirst.reset(new ffs3::DirectoryName(m_panelMainFolder, m_dirPickerMain, m_txtCtrlDirectoryMain));

    //load config values
    xmlAccess::XmlRealConfig newConfig;
    bool startWatchingImmediately = false;


    if (cfgFilename.empty())
        try
        {
            rts::readRealOrBatchConfig(lastConfigFileName(), newConfig);
        }
        catch (const xmlAccess::XmlError& error)
        {
            if (wxFileExists(lastConfigFileName())) //show error only if it's a parsing problem
            {
                if (error.getSeverity() == xmlAccess::XmlError::WARNING)
                    wxMessageBox(error.msg(), _("Warning"), wxOK | wxICON_WARNING);
                else
                    wxMessageBox(error.msg(), _("Error"), wxOK | wxICON_ERROR);
            }
        }
    else
        try
        {
            rts::readRealOrBatchConfig(cfgFilename, newConfig);
            startWatchingImmediately = true;
        }
        catch (const xmlAccess::XmlError& error)
        {
            if (error.getSeverity() == xmlAccess::XmlError::WARNING)
                wxMessageBox(error.msg(), _("Warning"), wxOK | wxICON_WARNING);
            else
                wxMessageBox(error.msg(), _("Error"), wxOK | wxICON_ERROR);
        }

    setConfiguration(newConfig);

    Fit();
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
        writeRealConfig(currentCfg, lastConfigFileName());
    }
    catch (const xmlAccess::XmlError& error)
    {
        wxMessageBox(error.msg().c_str(), _("Error"), wxOK | wxICON_ERROR);
    }
}


void MainDialog::OnClose(wxCloseEvent &event)
{
    Destroy();
}


void MainDialog::OnQuit(wxCommandEvent &event)
{
    Destroy();
}


const wxString& MainDialog::lastConfigFileName()
{
    static wxString instance = ffs3::getConfigDir() + wxT("LastRun.ffs_real");
    return instance;
}


void MainDialog::OnShowHelp(wxCommandEvent& event)
{
#ifdef FFS_WIN
    ffs3::displayHelpEntry(wxT("html\\advanced\\RealtimeSync.html"));
#elif defined FFS_LINUX
    ffs3::displayHelpEntry(wxT("html/advanced/RealtimeSync.html"));
#endif
}


void MainDialog::OnMenuAbout(wxCommandEvent& event)
{
    //build information
    wxString build = __TDATE__;
#if wxUSE_UNICODE
    build += wxT(" - Unicode");
#else
    build += wxT(" - ANSI");
#endif //wxUSE_UNICODE

    //compile time info about 32/64-bit build
    if (util::is64BitBuild)
        build += wxT(" x64");
    else
        build += wxT(" x86");
    assert_static(util::is32BitBuild || util::is64BitBuild);

    wxString buildFormatted = _("(Build: %x)");
    buildFormatted.Replace(wxT("%x"), build);

    wxMessageDialog* aboutDlg = new wxMessageDialog(this, wxString(wxT("RealtimeSync")) + wxT("\n\n") + buildFormatted, _("About"), wxOK);
    aboutDlg->ShowModal();
}


void MainDialog::OnKeyPressed(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();

    if (keyCode == WXK_ESCAPE)
        Destroy();

    event.Skip();
}


void MainDialog::OnStart(wxCommandEvent& event)
{
    xmlAccess::XmlRealConfig currentCfg = getConfiguration();

    Hide();

    wxWindowDisabler dummy; //avoid unwanted re-entrance in the following process

    switch (rts::startDirectoryMonitor(currentCfg))
    {
    case rts::QUIT:
    {
        Destroy();
        return;
    }
    break;

    case rts::RESUME:
        break;
    }

    Show();
}


void MainDialog::OnSaveConfig(wxCommandEvent& event)
{
    const wxString defaultFileName = wxT("Realtime.ffs_real");

    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, defaultFileName, wxString(_("RealtimeSync configuration")) + wxT(" (*.ffs_real)|*.ffs_real"), wxFD_SAVE);
    if (filePicker->ShowModal() == wxID_OK)
    {
        const wxString newFileName = filePicker->GetPath();

        if (wxFileExists(newFileName))
        {
            wxMessageDialog* messageDlg = new wxMessageDialog(this, wxString(_("File already exists. Overwrite?")) + wxT(" \"") + newFileName + wxT("\""), _("Warning") , wxOK | wxCANCEL);

            if (messageDlg->ShowModal() != wxID_OK)
            {
                OnSaveConfig(event); //retry
                return;
            }
        }

        const xmlAccess::XmlRealConfig currentCfg = getConfiguration();

        try //write config to XML
        {
            writeRealConfig(currentCfg, newFileName);
        }
        catch (const ffs3::FileError& error)
        {
            wxMessageBox(error.msg().c_str(), _("Error"), wxOK | wxICON_ERROR);
        }
    }
}


void MainDialog::loadConfig(const wxString& filename)
{
    xmlAccess::XmlRealConfig newConfig;

    try
    {
        rts::readRealOrBatchConfig(filename, newConfig);
    }
    catch (const xmlAccess::XmlError& error)
    {
        if (error.getSeverity() == xmlAccess::XmlError::WARNING)
            wxMessageBox(error.msg(), _("Warning"), wxOK | wxICON_WARNING);
        else
        {
            wxMessageBox(error.msg(), _("Error"), wxOK | wxICON_ERROR);
            return;
        }
    }

    setConfiguration(newConfig);
}


void MainDialog::OnLoadConfig(wxCommandEvent& event)
{
    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, wxEmptyString,
            wxString(_("RealtimeSync configuration")) + wxT(" (*.ffs_real;*.ffs_batch)|*.ffs_real;*.ffs_batch"),
            wxFD_OPEN);
    if (filePicker->ShowModal() == wxID_OK)
        loadConfig(filePicker->GetPath());
}


void MainDialog::setConfiguration(const xmlAccess::XmlRealConfig& cfg)
{
    //clear existing folders
    dirNameFirst->setName(Zstring());

    clearAddFolders();

    if (!cfg.directories.empty())
    {
        //fill top folder
        dirNameFirst->setName(wxToZ(*cfg.directories.begin()));

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

    output.directories.push_back(zToWx(dirNameFirst->getName()));
    for (std::vector<DirectoryPanel*>::const_iterator i = dirNamesExtra.begin(); i != dirNamesExtra.end(); ++i)
        output.directories.push_back(zToWx((*i)->getName()));

    output.commandline = m_textCtrlCommand->GetValue();
    output.delay       = m_spinCtrlDelay->GetValue();;

    return output;
}


void MainDialog::OnAddFolder(wxCommandEvent& event)
{
    const wxString topFolder = zToWx(dirNameFirst->getName());

    //clear existing top folder first
    dirNameFirst->setName(Zstring());

    std::vector<wxString> newFolders;
    newFolders.push_back(topFolder);

    addFolder(newFolders, true); //add pair in front of additonal pairs
}


void MainDialog::OnRemoveFolder(wxCommandEvent& event)
{
    //find folder pair originating the event
    const wxObject* const eventObj = event.GetEventObject();
    for (std::vector<DirectoryPanel*>::const_iterator i = dirNamesExtra.begin(); i != dirNamesExtra.end(); ++i)
    {
        if (eventObj == static_cast<wxObject*>((*i)->m_bpButtonRemoveFolder))
        {
            removeAddFolder(i - dirNamesExtra.begin());
            return;
        }
    }
}


void MainDialog::OnRemoveTopFolder(wxCommandEvent& event)
{
    if (dirNamesExtra.size() > 0)
    {
        const wxString topDir = zToWx((*dirNamesExtra.begin())->getName());

        dirNameFirst->setName(wxToZ(topDir));

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
        newFolder->m_bpButtonRemoveFolder->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("removeFolderPair")));

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
        newFolder->m_bpButtonRemoveFolder->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainDialog::OnRemoveFolder), NULL, this );

        //insert directory name
        newFolder->setName(wxToZ(*i));
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


void MainDialog::removeAddFolder(const int pos)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    if (0 <= pos && pos < int(dirNamesExtra.size()))
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
