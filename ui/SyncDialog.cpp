#include "syncDialog.h"
#include "../library/globalFunctions.h"
#include "../library/resources.h"
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <wx/ffile.h>
#ifdef FFS_WIN
#include <windows.h>
#include <shlobj.h>
#endif  // FFS_WIN

using namespace std;

SyncDialog::SyncDialog(wxWindow* window,
                       const FileCompareResult& gridDataRef,
                       Configuration& config,
                       bool synchronizationEnabled) :
        SyncDlgGenerated(window),
        gridData(gridDataRef),
        cfg(config)
{
    //make working copy of mainDialog.cfg.syncConfiguration and recycler setting
    localSyncConfiguration = config.syncConfiguration;
    m_checkBoxUseRecycler->SetValue(cfg.useRecycleBin);
    m_checkBoxContinueError->SetValue(cfg.continueOnError);

    //set sync config icons
    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    //update preview
    calculatePreview();

    //set icons for this dialog
    m_bpButton18->SetBitmapLabel(*GlobalResources::bitmapStartSync);
    m_bpButton18->SetBitmapDisabled(*GlobalResources::bitmapStartSyncDis);
    m_bitmap13->SetBitmap(*GlobalResources::bitmapLeftOnlyDeact);
    m_bitmap14->SetBitmap(*GlobalResources::bitmapRightOnlyDeact);
    m_bitmap15->SetBitmap(*GlobalResources::bitmapLeftNewerDeact);
    m_bitmap16->SetBitmap(*GlobalResources::bitmapRightNewerDeact);
    m_bitmap17->SetBitmap(*GlobalResources::bitmapDifferentDeact);

    if (synchronizationEnabled)
        m_bpButton18->Enable();
    else
    {
        m_bpButton18->Disable();
        m_button6->SetFocus();
    }

    //set radiobutton
    if (localSyncConfiguration.exLeftSideOnly  == SYNC_DIR_RIGHT &&
            localSyncConfiguration.exRightSideOnly == SYNC_DIR_RIGHT &&
            localSyncConfiguration.leftNewer       == SYNC_DIR_RIGHT &&
            localSyncConfiguration.rightNewer      == SYNC_DIR_RIGHT &&
            localSyncConfiguration.different       == SYNC_DIR_RIGHT)
        m_radioBtn1->SetValue(true);    //one way ->

    else if (localSyncConfiguration.exLeftSideOnly  == SYNC_DIR_RIGHT &&
             localSyncConfiguration.exRightSideOnly == SYNC_DIR_LEFT &&
             localSyncConfiguration.leftNewer       == SYNC_DIR_RIGHT &&
             localSyncConfiguration.rightNewer      == SYNC_DIR_LEFT &&
             localSyncConfiguration.different       == SYNC_DIR_NONE)
        m_radioBtn2->SetValue(true);    //two way <->

    else
        m_radioBtn3->SetValue(true);    //other

    m_bpButton18->SetLabel(_("&Start"));

    //set tooltip for ambivalent category "different"
    adjustToolTips(m_bitmap17, config.compareVar);
}

//#################################################################################################################

SyncDialog::~SyncDialog() {}


void SyncDialog::updateConfigIcons(wxBitmapButton* button1,
                                   wxBitmapButton* button2,
                                   wxBitmapButton* button3,
                                   wxBitmapButton* button4,
                                   wxBitmapButton* button5,
                                   const SyncConfiguration& syncConfig)
{
    if (syncConfig.exLeftSideOnly == SYNC_DIR_RIGHT)
    {
        button1->SetBitmapLabel(*GlobalResources::bitmapArrowRightCr);
        button1->SetToolTip(_("Copy from left to right"));
    }
    else if (syncConfig.exLeftSideOnly == SYNC_DIR_LEFT)
    {
        button1->SetBitmapLabel(*GlobalResources::bitmapDeleteLeft);
        button1->SetToolTip(_("Delete files/folders existing on left side only"));
    }
    else if (syncConfig.exLeftSideOnly == SYNC_DIR_NONE)
    {
        button1->SetBitmapLabel(*GlobalResources::bitmapArrowNone);
        button1->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.exRightSideOnly == SYNC_DIR_RIGHT)
    {
        button2->SetBitmapLabel(*GlobalResources::bitmapDeleteRight);
        button2->SetToolTip(_("Delete files/folders existing on right side only"));
    }
    else if (syncConfig.exRightSideOnly == SYNC_DIR_LEFT)
    {
        button2->SetBitmapLabel(*GlobalResources::bitmapArrowLeftCr);
        button2->SetToolTip(_("Copy from right to left"));
    }
    else if (syncConfig.exRightSideOnly == SYNC_DIR_NONE)
    {
        button2->SetBitmapLabel(*GlobalResources::bitmapArrowNone);
        button2->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.leftNewer == SYNC_DIR_RIGHT)
    {
        button3->SetBitmapLabel(*GlobalResources::bitmapArrowRight);
        button3->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (syncConfig.leftNewer == SYNC_DIR_LEFT)
    {
        button3->SetBitmapLabel(*GlobalResources::bitmapArrowLeft);
        button3->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (syncConfig.leftNewer == SYNC_DIR_NONE)
    {
        button3->SetBitmapLabel(*GlobalResources::bitmapArrowNone);
        button3->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.rightNewer == SYNC_DIR_RIGHT)
    {
        button4->SetBitmapLabel(*GlobalResources::bitmapArrowRight);
        button4->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (syncConfig.rightNewer == SYNC_DIR_LEFT)
    {
        button4->SetBitmapLabel(*GlobalResources::bitmapArrowLeft);
        button4->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (syncConfig.rightNewer == SYNC_DIR_NONE)
    {
        button4->SetBitmapLabel(*GlobalResources::bitmapArrowNone);
        button4->SetToolTip(_("Do nothing"));
    }

    if (syncConfig.different == SYNC_DIR_RIGHT)
    {
        button5->SetBitmapLabel(*GlobalResources::bitmapArrowRight);
        button5->SetToolTip(_("Copy from left to right overwriting"));
    }
    else if (syncConfig.different == SYNC_DIR_LEFT)
    {
        button5->SetBitmapLabel(*GlobalResources::bitmapArrowLeft);
        button5->SetToolTip(_("Copy from right to left overwriting"));
    }
    else if (syncConfig.different == SYNC_DIR_NONE)
    {
        button5->SetBitmapLabel(*GlobalResources::bitmapArrowNone);
        button5->SetToolTip(_("Do nothing"));
    }
}


void SyncDialog::adjustToolTips(wxStaticBitmap* bitmap, const CompareVariant var)
{
    //set tooltip for ambivalent category "different"
    if (var == CMP_BY_TIME_SIZE)
    {
        bitmap->SetToolTip(_("Files that exist on both sides, have same date but different filesizes"));
    }
    else if (var == CMP_BY_CONTENT)
    {
        bitmap->SetToolTip(_("Files that exist on both sides and have different content"));
    }
    else
        assert(false);
}


void SyncDialog::calculatePreview()
{
    //update preview of bytes to be transferred:
    int objectsToCreate    = 0;
    int objectsToOverwrite = 0;
    int objectsToDelete    = 0;
    double dataToProcess   = 0;
    FreeFileSync::calcTotalBytesToSync(objectsToCreate,
                                       objectsToOverwrite,
                                       objectsToDelete,
                                       dataToProcess,
                                       gridData,
                                       localSyncConfiguration);

    wxString toCreate = globalFunctions::numberToWxString(objectsToCreate);
    wxString toUpdate = globalFunctions::numberToWxString(objectsToOverwrite);
    wxString toDelete = globalFunctions::numberToWxString(objectsToDelete);
    wxString data     = FreeFileSync::formatFilesizeToShortString(dataToProcess);

    globalFunctions::includeNumberSeparator(toCreate);
    globalFunctions::includeNumberSeparator(toUpdate);
    globalFunctions::includeNumberSeparator(toDelete);

    m_textCtrlCreate->SetValue(toCreate);
    m_textCtrlUpdate->SetValue(toUpdate);
    m_textCtrlDelete->SetValue(toDelete);
    m_textCtrlData->SetValue(data);
}


void SyncDialog::OnClose(wxCloseEvent& event)
{
    EndModal(0);
}


void SyncDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(0);
}


void SyncDialog::OnBack(wxCommandEvent& event)
{
    //write configuration to main dialog
    cfg.syncConfiguration = localSyncConfiguration;
    cfg.useRecycleBin     = m_checkBoxUseRecycler->GetValue();
    cfg.continueOnError   = m_checkBoxContinueError->GetValue();

    EndModal(0);
}

void SyncDialog::OnStartSync(wxCommandEvent& event)
{
    //write configuration to main dialog
    cfg.syncConfiguration = localSyncConfiguration;
    cfg.useRecycleBin     = m_checkBoxUseRecycler->GetValue();
    cfg.continueOnError   = m_checkBoxContinueError->GetValue();

    EndModal(StartSynchronizationProcess);
}


void SyncDialog::OnSelectRecycleBin(wxCommandEvent& event)
{
    if (event.IsChecked())
    {
        if (!FreeFileSync::recycleBinExists())
        {
            wxMessageBox(_("It was not possible to initialize the Recycle Bin!\n\nIt's likely that you are not using Windows.\nIf you want this feature included, please contact the author. :)"), _("Error") , wxOK | wxICON_ERROR);
            m_checkBoxUseRecycler->SetValue(false);
        }
    }
}


void SyncDialog::OnSyncLeftToRight(wxCommandEvent& event)
{
    localSyncConfiguration.exLeftSideOnly  = SYNC_DIR_RIGHT;
    localSyncConfiguration.exRightSideOnly = SYNC_DIR_RIGHT;
    localSyncConfiguration.leftNewer       = SYNC_DIR_RIGHT;
    localSyncConfiguration.rightNewer      = SYNC_DIR_RIGHT;
    localSyncConfiguration.different       = SYNC_DIR_RIGHT;

    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();

    //if event is triggered by button
    m_radioBtn1->SetValue(true);
}


void SyncDialog::OnSyncBothSides(wxCommandEvent& event)
{
    localSyncConfiguration.exLeftSideOnly  = SYNC_DIR_RIGHT;
    localSyncConfiguration.exRightSideOnly = SYNC_DIR_LEFT;
    localSyncConfiguration.leftNewer       = SYNC_DIR_RIGHT;
    localSyncConfiguration.rightNewer      = SYNC_DIR_LEFT;
    localSyncConfiguration.different       = SYNC_DIR_NONE;

    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();

    //if event is triggered by button
    m_radioBtn2->SetValue(true);
}


void toggleSyncDirection(SyncDirection& current)
{
    if (current == SYNC_DIR_RIGHT)
        current = SYNC_DIR_LEFT;
    else if (current == SYNC_DIR_LEFT)
        current = SYNC_DIR_NONE;
    else if (current== SYNC_DIR_NONE)
        current = SYNC_DIR_RIGHT;
    else
        assert (false);
}

void SyncDialog::OnExLeftSideOnly( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.exLeftSideOnly);
    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();
    //set custom config button
    m_radioBtn3->SetValue(true);
}

void SyncDialog::OnExRightSideOnly( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.exRightSideOnly);
    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();
    //set custom config button
    m_radioBtn3->SetValue(true);
}

void SyncDialog::OnLeftNewer( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.leftNewer);
    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();
    //set custom config button
    m_radioBtn3->SetValue(true);
}

void SyncDialog::OnRightNewer( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.rightNewer);
    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();
    //set custom config button
    m_radioBtn3->SetValue(true);
}

void SyncDialog::OnDifferent( wxCommandEvent& event )
{
    toggleSyncDirection(localSyncConfiguration.different);
    updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
    calculatePreview();
    //set custom config button
    m_radioBtn3->SetValue(true);
}

//###################################################################################################################################


BatchDialog::BatchDialog(wxWindow* window,
                         const Configuration& config,
                         const wxString& leftDir,
                         const wxString& rightDir) :
        BatchDlgGenerated(window)
{
    //make working copy of mainDialog.cfg.syncConfiguration and recycler setting
    localSyncConfiguration = config.syncConfiguration;
    SyncDialog::updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);

    m_checkBoxUseRecycler->SetValue(config.useRecycleBin);
    m_checkBoxContinueError->SetValue(config.continueOnError);

    switch (config.compareVar)
    {
    case CMP_BY_TIME_SIZE:
        m_radioBtnSizeDate->SetValue(true);
        break;
    case CMP_BY_CONTENT:
        m_radioBtnContent->SetValue(true);
        break;
    default:
        assert (false);
    }
    //adjust toolTip
    SyncDialog::adjustToolTips(m_bitmap17, config.compareVar);

    filterIsActive = config.filterIsActive;
    updateFilterButton();

    m_textCtrlInclude->SetValue(config.includeFilter);
    m_textCtrlExclude->SetValue(config.excludeFilter);

    m_directoryPanel1->SetValue(leftDir);
    m_directoryPanel2->SetValue(rightDir);

    //set icons for this dialog
    m_bitmap13->SetBitmap(*GlobalResources::bitmapLeftOnlyDeact);
    m_bitmap14->SetBitmap(*GlobalResources::bitmapRightOnlyDeact);
    m_bitmap15->SetBitmap(*GlobalResources::bitmapLeftNewerDeact);
    m_bitmap16->SetBitmap(*GlobalResources::bitmapRightNewerDeact);
    m_bitmap17->SetBitmap(*GlobalResources::bitmapDifferentDeact);
    m_bitmap8->SetBitmap(*GlobalResources::bitmapInclude);
    m_bitmap9->SetBitmap(*GlobalResources::bitmapExclude);

    m_buttonCreate->SetFocus();
}

BatchDialog::~BatchDialog()
{}


void BatchDialog::updateFilterButton()
{
    if (filterIsActive)
    {
        m_bpButtonFilter->SetBitmapLabel(*GlobalResources::bitmapFilterOn);
        m_bpButtonFilter->SetToolTip(_("Filter active: Press again to deactivate"));

        m_textCtrlInclude->Enable();
        m_textCtrlExclude->Enable();
    }
    else
    {
        m_bpButtonFilter->SetBitmapLabel(*GlobalResources::bitmapFilterOff);
        m_bpButtonFilter->SetToolTip(_("Press button to activate filter"));

        m_textCtrlInclude->Disable();
        m_textCtrlExclude->Disable();
    }
}


void BatchDialog::OnExLeftSideOnly(wxCommandEvent& event)
{
    toggleSyncDirection(localSyncConfiguration.exLeftSideOnly);
    SyncDialog::updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
}

void BatchDialog::OnExRightSideOnly(wxCommandEvent& event)
{
    toggleSyncDirection(localSyncConfiguration.exRightSideOnly);
    SyncDialog::updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
}

void BatchDialog::OnLeftNewer(wxCommandEvent& event)
{
    toggleSyncDirection(localSyncConfiguration.leftNewer);
    SyncDialog::updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
}

void BatchDialog::OnRightNewer(wxCommandEvent& event)
{
    toggleSyncDirection(localSyncConfiguration.rightNewer);
    SyncDialog::updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
}

void BatchDialog::OnDifferent(wxCommandEvent& event)
{
    toggleSyncDirection(localSyncConfiguration.different);
    SyncDialog::updateConfigIcons(m_bpButton5, m_bpButton6, m_bpButton7, m_bpButton8, m_bpButton9, localSyncConfiguration);
}


void BatchDialog::OnFilterButton(wxCommandEvent& event)
{
    filterIsActive = !filterIsActive;
    updateFilterButton();
}


void BatchDialog::OnSelectRecycleBin(wxCommandEvent& event)
{
    if (event.IsChecked())
    {
        if (!FreeFileSync::recycleBinExists())
        {
            wxMessageBox(_("It was not possible to initialize the Recycle Bin!\n\nIt's likely that you are not using Windows.\nIf you want this feature included, please contact the author. :)"), _("Error") , wxOK | wxICON_ERROR);
            m_checkBoxUseRecycler->SetValue(false);
        }
    }
}


void BatchDialog::OnChangeCompareVar(wxCommandEvent& event)
{
    CompareVariant var;
    if (m_radioBtnSizeDate->GetValue())
        var = CMP_BY_TIME_SIZE;
    else if (m_radioBtnContent->GetValue())
        var = CMP_BY_CONTENT;
    else
    {
        assert(false);
        var = CMP_BY_TIME_SIZE;
    }

    //set tooltip for ambivalent category "different"
    SyncDialog::adjustToolTips(m_bitmap17, var);
}


void BatchDialog::OnClose(wxCloseEvent&   event)
{
    EndModal(0);
}


void BatchDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(0);
}


void BatchDialog::OnCreateBatchJob(wxCommandEvent& event)
{
    if (m_directoryPanel1->GetValue().IsEmpty() || m_directoryPanel2->GetValue().IsEmpty())
    {
        wxMessageBox(_("Please select both left and right directories!"), _("Information"));
        return;
    }

    //check if directories exist if loaded by config file
    if (!wxDirExists(FreeFileSync::getFormattedDirectoryName(m_directoryPanel1->GetValue())))
    {
        wxMessageBox(_("Left directory does not exist. Please select a new one!"), _("Warning"), wxICON_WARNING);
        return;
    }
    else if (!wxDirExists(FreeFileSync::getFormattedDirectoryName(m_directoryPanel2->GetValue())))
    {
        wxMessageBox(_("Right directory does not exist. Please select a new one!"), _("Warning"), wxICON_WARNING);
        return;
    }

    //get a filename
#ifdef FFS_WIN
    wxString fileName = _("SyncJob.lnk"); //proposal
    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, fileName, wxString(_("Shell link")) + wxT(" (*.lnk)|*.lnk"), wxFD_SAVE);
#elif defined FFS_LINUX
    wxString fileName = _("SyncJob.sh");  //proposal
    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, fileName, wxString(_("Shell script")) + wxT(" (*.sh)|*.sh"), wxFD_SAVE);
#else
    assert(false);
#endif

    if (filePicker->ShowModal() == wxID_OK)
    {
        fileName = filePicker->GetPath();
        if (wxFileExists(fileName))
        {
            wxMessageDialog* messageDlg = new wxMessageDialog(this, wxString(wxT("\"")) + fileName + wxT("\"") + _(" already exists. Overwrite?"), _("Warning") , wxOK | wxCANCEL);

            if (messageDlg->ShowModal() != wxID_OK)
                return;
        }

        //create batch file
        if (createBatchFile(fileName))
            EndModal(batchFileCreated);
        else
            wxMessageBox(wxString(_("Could not create file ")) + wxT("\"") + fileName + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
    }
}


wxString getFormattedSyncDirection(const SyncDirection direction)
{
    if (direction == SYNC_DIR_RIGHT)
        return wxChar('R');
    else if (direction == SYNC_DIR_LEFT)
        return wxChar('L');
    else if (direction == SYNC_DIR_NONE)
        return wxChar('N');
    else
    {
        assert (false);
        return wxEmptyString;
    }
}


wxString BatchDialog::getCommandlineArguments()
{
    wxString output;

    output+= wxString(wxT("-")) + GlobalResources::paramCompare + wxT(" ");
    if (m_radioBtnSizeDate->GetValue())
        output+= GlobalResources::valueSizeDate;
    else if (m_radioBtnContent->GetValue())
        output+= GlobalResources::valueContent;
    else
        assert(false);

    output+= wxString(wxT(" -")) + GlobalResources::paramSync + wxT(" ") +
             getFormattedSyncDirection(localSyncConfiguration.exLeftSideOnly) +
             getFormattedSyncDirection(localSyncConfiguration.exRightSideOnly) +
             getFormattedSyncDirection(localSyncConfiguration.leftNewer) +
             getFormattedSyncDirection(localSyncConfiguration.rightNewer) +
             getFormattedSyncDirection(localSyncConfiguration.different);

    if (filterIsActive)
    {
        output+= wxString(wxT(" -")) + GlobalResources::paramInclude + wxT(" ") +
                 wxT("\"") + m_textCtrlInclude->GetValue() + wxT("\"");

        output+= wxString(wxT(" -")) + GlobalResources::paramExclude + wxT(" ") +
                 wxT("\"") + m_textCtrlExclude->GetValue() + wxT("\"");
    }

    if (m_checkBoxUseRecycler->GetValue())
        output+= wxString(wxT(" -")) + GlobalResources::paramRecycler;

    if (m_checkBoxContinueError->GetValue())
        output+= wxString(wxT(" -")) + GlobalResources::paramContinueError;

    if (m_checkBoxSilent->GetValue())
        output+= wxString(wxT(" -")) + GlobalResources::paramSilent;

    wxString leftDir  = FreeFileSync::getFormattedDirectoryName(m_directoryPanel1->GetValue());
    wxString rightDir = FreeFileSync::getFormattedDirectoryName(m_directoryPanel2->GetValue());

    output+= wxString(wxT(" \"")) + wxDir(leftDir).GetName() + wxT("\""); //directory WITHOUT trailing path separator
    output+= wxString(wxT(" \"")) + wxDir(rightDir).GetName() + wxT("\""); //needed since e.g. "C:\" isn't parsed correctly by commandline

    return output;
}


template <typename T>
struct CleanUp
{
    CleanUp(T* element) : m_element(element) {}

    ~CleanUp()
    {
        m_element->Release();
    }

    T* m_element;
};


bool BatchDialog::createBatchFile(const wxString& filename)
{
#ifdef FFS_WIN
    //create shell link (instead of batch file) for full Unicode support
    HRESULT hResult = E_FAIL;
    IShellLink* pShellLink = NULL;

    if (FAILED(CoCreateInstance(CLSID_ShellLink,       //class identifier
                                NULL,                  //object isn't part of an aggregate
                                CLSCTX_INPROC_SERVER,  //context for running executable code
                                IID_IShellLink,        //interface identifier
                                (void**)&pShellLink))) //pointer to storage of interface pointer
        return false;
    CleanUp<IShellLink> cleanOnExit(pShellLink);

    wxString freeFileSyncExe = wxStandardPaths::Get().GetExecutablePath();
    if (FAILED(pShellLink->SetPath(freeFileSyncExe.c_str())))
        return false;

    if (FAILED(pShellLink->SetArguments(getCommandlineArguments().c_str())))
        return false;

    if (FAILED(pShellLink->SetIconLocation(freeFileSyncExe.c_str(), 1))) //second icon from executable file is used
        return false;

    if (FAILED(pShellLink->SetDescription(_("FreeFileSync Batch Job"))))
        return false;

    IPersistFile*  pPersistFile = NULL;
    if (FAILED(pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile)))
        return false;
    CleanUp<IPersistFile> cleanOnExit2(pPersistFile);

    //pPersistFile->Save accepts unicode input only
#ifdef _UNICODE
    hResult = pPersistFile->Save(filename.c_str(), TRUE);
#else
    WCHAR wszTemp [MAX_PATH];
    if (MultiByteToWideChar(CP_ACP, 0, filename.c_str(), -1, wszTemp, MAX_PATH) == 0)
        return false;

    hResult = pPersistFile->Save(wszTemp, TRUE);
#endif
    if (FAILED(hResult))
        return false;

    return true;

//############################################################################
#elif defined FFS_LINUX
    //create shell script
    wxFFile output(filename, wxT("w"));
    if (output.IsOpened())
    {
        wxString outputString;
        outputString+= wxT("#!/bin/bash\n"); //shell script identifier
        outputString+= wxString(wxT("\"")) + wxStandardPaths::Get().GetExecutablePath() + wxT("\" ");
        outputString+= getCommandlineArguments() + wxT("\n");

        if (!output.Write(outputString))
            return false;

        //for linux the batch file needs the executable flag
        output.Close();
        wxExecute(wxString(wxT("chmod +x ")) + filename);
        return true;
    }
    else
        return false;
#else
    adapt this!
#endif  // FFS_LINUX
}
