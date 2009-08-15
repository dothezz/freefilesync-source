#include "mainDialog.h"
#include "resources.h"
#include "../shared/customButton.h"
#include "../shared/standardPaths.h"
#include "../shared/globalFunctions.h"
#include <wx/msgdlg.h>
#include <wx/wupdlock.h>
#include "watcher.h"
#include <wx/utils.h>
#include "xmlProcessing.h"
#include "trayMenu.h"
#include "../shared/fileHandling.h"
#include "xmlFreeFileSync.h"


MainDialog::MainDialog(wxDialog *dlg, const wxString& cfgFilename)
        : MainDlgGenerated(dlg)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    m_bpButtonRemoveTopFolder->Hide();
    m_panelMainFolder->Layout();

    m_bpButtonAddFolder->SetBitmapLabel(*GlobalResources::getInstance().bitmapAddFolderPair);
    m_bpButtonRemoveTopFolder->SetBitmapLabel(*GlobalResources::getInstance().bitmapRemoveFolderPair);
    m_buttonStart->setBitmapFront(*GlobalResources::getInstance().bitmapStart);
    m_buttonStart->SetFocus();

    //register key event
    Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(MainDialog::OnKeyPressed), NULL, this);

    //prepare drag & drop
    dragDropOnFolder.reset(new FreeFileSync::DragDropOnDlg(m_panelMainFolder, m_dirPickerMain, m_txtCtrlDirectoryMain));

    //load config values
    xmlAccess::XmlRealConfig newConfig;
    bool startWatchingImmediately = false;


    if (cfgFilename.empty())
        try
        {
            RealtimeSync::readRealOrBatchConfig(lastConfigFileName(), newConfig);
        }
        catch (const xmlAccess::XmlError& error)
        {
            if (wxFileExists(lastConfigFileName())) //show error only if it's a parsing problem
            {
                if (error.getSeverity() == xmlAccess::XmlError::WARNING)
                    wxMessageBox(error.show(), _("Warning"), wxOK | wxICON_WARNING);
                else
                    wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
            }
        }
    else
        try
        {
            RealtimeSync::readRealOrBatchConfig(cfgFilename, newConfig);
            startWatchingImmediately = true;
        }
        catch (const xmlAccess::XmlError& error)
        {
            if (error.getSeverity() == xmlAccess::XmlError::WARNING)
                wxMessageBox(error.show(), _("Warning"), wxOK | wxICON_WARNING);
            else
                wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
        }

    setConfiguration(newConfig);

    m_buttonStart->SetFocus();
    Fit();
    Center();

    if (startWatchingImmediately) //start watch mode directly
    {
        wxCommandEvent dummy(wxEVT_COMMAND_BUTTON_CLICKED);
        this->OnStart(dummy);
    }
}


MainDialog::~MainDialog()
{
    //save current configuration
    const xmlAccess::XmlRealConfig currentCfg = getConfiguration();

    try //write config to XML
    {
        writeRealConfig(currentCfg, lastConfigFileName());
    }
    catch (const FreeFileSync::FileError& error)
    {
        wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
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
    static wxString instance = FreeFileSync::getConfigDir().EndsWith(wxString(globalFunctions::FILE_NAME_SEPARATOR)) ?
                               FreeFileSync::getConfigDir() + wxT("LastRun.ffs_real") :
                               FreeFileSync::getConfigDir() + globalFunctions::FILE_NAME_SEPARATOR + wxT("LastRun.ffs_real");
    return instance;
}


void MainDialog::OnMenuAbout(wxCommandEvent& event)
{
    //build information
    wxString build = wxString(wxT("(")) + _("Build:") + wxT(" ") + __TDATE__;
#if wxUSE_UNICODE
    build += wxT(" - Unicode)");
#else
    build += wxT(" - ANSI)");
#endif //wxUSE_UNICODE

    wxMessageDialog* aboutDlg = new wxMessageDialog(this, wxString(wxT("RealtimeSync")) + wxT("\n\n") + build, _("About"), wxOK);
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

    switch (RealtimeSync::startDirectoryMonitor(currentCfg))
    {
    case RealtimeSync::QUIT:
    {
        Destroy();
        return;
    }
    break;

    case RealtimeSync::RESUME:
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
        catch (const FreeFileSync::FileError& error)
        {
            wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
        }
    }
}


void MainDialog::loadConfig(const wxString& filename)
{
    xmlAccess::XmlRealConfig newConfig;

    try
    {
        RealtimeSync::readRealOrBatchConfig(filename, newConfig);
    }
    catch (const xmlAccess::XmlError& error)
    {
        if (error.getSeverity() == xmlAccess::XmlError::WARNING)
            wxMessageBox(error.show(), _("Warning"), wxOK | wxICON_WARNING);
        else
        {
            wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
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
    m_txtCtrlDirectoryMain->ChangeValue(wxEmptyString);
    m_dirPickerMain->SetPath(wxEmptyString);

    clearAddFolders();

    if (!cfg.directories.empty())
    {
        //fill top folder
        m_txtCtrlDirectoryMain->SetValue(*cfg.directories.begin());

        const wxString dirFormatted = FreeFileSync::getFormattedDirectoryName(cfg.directories.begin()->c_str()).c_str();
        if (wxDirExists(dirFormatted))
            m_dirPickerMain->SetPath(dirFormatted);

        //fill additional folders
        addFolder(std::vector<wxString>(cfg.directories.begin() + 1, cfg.directories.end()));
    }

    //fill commandline
    m_textCtrlCommand->SetValue(cfg.commandline);

    //set delay
    m_spinCtrlDelay->SetValue(cfg.delay);
}


xmlAccess::XmlRealConfig MainDialog::getConfiguration()
{
    xmlAccess::XmlRealConfig output;

    output.directories.push_back(m_txtCtrlDirectoryMain->GetValue());
    for (std::vector<FolderPanel*>::const_iterator i = additionalFolders.begin(); i != additionalFolders.end(); ++i)
        output.directories.push_back((*i)->m_txtCtrlDirectory->GetValue());

    output.commandline = m_textCtrlCommand->GetValue();
    output.delay       = m_spinCtrlDelay->GetValue();;

    return output;
}


void MainDialog::OnAddFolder(wxCommandEvent& event)
{
    const wxString topFolder = m_txtCtrlDirectoryMain->GetValue();

    //clear existing top folder first
    RealtimeSync::setDirectoryName(wxEmptyString, m_txtCtrlDirectoryMain, m_dirPickerMain);

    std::vector<wxString> newFolders;
    newFolders.push_back(topFolder.c_str());

    addFolder(newFolders, true); //add pair in front of additonal pairs
}


void MainDialog::OnRemoveFolder(wxCommandEvent& event)
{
    //find folder pair originating the event
    const wxObject* const eventObj = event.GetEventObject();
    for (std::vector<FolderPanel*>::const_iterator i = additionalFolders.begin(); i != additionalFolders.end(); ++i)
    {
        if (eventObj == static_cast<wxObject*>((*i)->m_bpButtonRemoveFolder))
        {
            removeAddFolder(i - additionalFolders.begin());
            return;
        }
    }
}


void MainDialog::OnRemoveTopFolder(wxCommandEvent& event)
{
    if (additionalFolders.size() > 0)
    {
        const wxString topDir = (*additionalFolders.begin())->m_txtCtrlDirectory->GetValue().c_str();
        RealtimeSync::setDirectoryName(topDir, m_txtCtrlDirectoryMain, m_dirPickerMain);

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
        FolderPanel* newFolder = new FolderPanel(m_scrolledWinFolders);
        newFolder->m_bpButtonRemoveFolder->SetBitmapLabel(*GlobalResources::getInstance().bitmapRemoveFolderPair);

        //get size of scrolled window
        folderHeight = newFolder->GetSize().GetHeight();

        if (addFront)
        {
            bSizerFolders->Insert(0, newFolder, 0, wxEXPAND, 5);
            additionalFolders.insert(additionalFolders.begin(), newFolder);
        }
        else
        {
            bSizerFolders->Add(newFolder, 0, wxEXPAND, 5);
            additionalFolders.push_back(newFolder);
        }

        //register events
        newFolder->m_bpButtonRemoveFolder->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainDialog::OnRemoveFolder), NULL, this );

        //insert directory name
        RealtimeSync::setDirectoryName(*i, newFolder->m_txtCtrlDirectory, newFolder->m_dirPicker);
    }

    //set size of scrolled window
    const int additionalRows = std::min(additionalFolders.size(), MAX_ADD_FOLDERS); //up to MAX_ADD_FOLDERS additional folders shall be shown
    m_scrolledWinFolders->SetMinSize(wxSize( -1, folderHeight * additionalRows));

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

    if (0 <= pos && pos < int(additionalFolders.size()))
    {
        //remove folder pairs from window
        FolderPanel* dirToDelete = additionalFolders[pos];
        const int folderHeight = dirToDelete->GetSize().GetHeight();

        bSizerFolders->Detach(dirToDelete); //Remove() does not work on Window*, so do it manually
        dirToDelete->Destroy();                 //
        additionalFolders.erase(additionalFolders.begin() + pos); //remove last element in vector


        //set size of scrolled window
        const int additionalRows = std::min(additionalFolders.size(), MAX_ADD_FOLDERS); //up to MAX_ADD_FOLDERS additional folders shall be shown
        m_scrolledWinFolders->SetMinSize(wxSize( -1, folderHeight * additionalRows));

        //adapt delete top folder pair button
        if (additionalFolders.size() == 0)
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

    additionalFolders.clear();
    bSizerFolders->Clear(true);

    m_bpButtonRemoveTopFolder->Hide();
    m_panelMainFolder->Layout();

    m_scrolledWinFolders->SetMinSize(wxSize(-1, 0));
    Fit();                       //adapt dialog size
}
