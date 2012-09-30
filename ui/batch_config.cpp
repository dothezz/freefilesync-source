// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "batch_config.h"
#include <iterator>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/wupdlock.h>
#include <wx+/button.h>
#include <wx+/choice_enum.h>
#include <wx+/mouse_move_dlg.h>
#include <wx+/context_menu.h>
#include <zen/file_handling.h>
#include "../lib/help_provider.h"
#include "folder_pair.h"
#include "msg_popup.h"
#include "gui_generated.h"

using namespace zen;
using namespace xmlAccess;

class DirectoryPairBatchFirst;
class DirectoryPairBatch;


class BatchDialog: public BatchDlgGenerated
{
    template <class GuiPanel>
    friend class FolderPairCallback;
    friend class DirectoryPairBatchFirst;

public:
    BatchDialog(wxWindow* parent,
                const wxString& referenceFile,
                const XmlBatchConfig& batchCfg,
                const std::shared_ptr<FolderHistory>& folderHistLeft,
                const std::shared_ptr<FolderHistory>& folderHistRight,
                std::vector<std::wstring>& onCompletionHistory,
                size_t onCompletionHistoryMax);

private:
    virtual void OnCmpSettings    (wxCommandEvent& event);
    virtual void OnSyncSettings   (wxCommandEvent& event);
    virtual void OnConfigureFilter(wxCommandEvent& event);
    virtual void OnHelp           (wxCommandEvent& event) { displayHelpEntry(L"html/Schedule a Batch Job.html"); }

    virtual void OnCompSettingsContext(wxCommandEvent& event);
    virtual void OnSyncSettingsContext(wxCommandEvent& event);
    virtual void OnGlobalFilterContext(wxCommandEvent& event);
    virtual void OnCheckSaveLog        (wxCommandEvent& event);
    virtual void OnClose               (wxCloseEvent&   event) { EndModal(0); }
    virtual void OnCancel              (wxCommandEvent& event) { EndModal(0); }
    virtual void OnSaveBatchJob        (wxCommandEvent& event);
    virtual void OnLoadBatchJob        (wxCommandEvent& event);
    virtual void OnAddFolderPair       (wxCommandEvent& event);
    virtual void OnRemoveFolderPair    (wxCommandEvent& event);
    virtual void OnRemoveTopFolderPair (wxCommandEvent& event);
    void OnFilesDropped(FileDropEvent& event);

    virtual void OnErrorPopup (wxCommandEvent& event) { localBatchCfg.handleError = ON_ERROR_POPUP;  updateGui(); }
    virtual void OnErrorIgnore(wxCommandEvent& event) { localBatchCfg.handleError = ON_ERROR_IGNORE; updateGui(); }
    virtual void OnErrorExit  (wxCommandEvent& event) { localBatchCfg.handleError = ON_ERROR_EXIT;   updateGui(); }

    virtual void OnToggleGenerateLogfile(wxCommandEvent& event) { updateGui(); }
    virtual void OnToggleLogfilesLimit  (wxCommandEvent& event) { updateGui(); }

    void addFolderPair(const std::vector<zen::FolderPairEnh>& newPairs, bool addFront = false);
    void removeAddFolderPair(int pos);
    void clearAddFolderPairs();

    void updateGuiForFolderPair();

    void updateGui(); //re-evaluate gui after config changes

    bool saveBatchFile(const wxString& filename);
    void loadBatchFile(const wxString& filename);
    void loadBatchFile(const std::vector<wxString>& filenames);

    void setConfig(const XmlBatchConfig& batchCfg);
    XmlBatchConfig getConfig() const;

    std::unique_ptr<DirectoryPairBatchFirst> firstFolderPair; //always bound!!!
    std::vector<DirectoryPairBatch*> additionalFolderPairs;

    //used when saving batch file
    wxString proposedBatchFileName;

    XmlBatchConfig localBatchCfg; //a mixture of settings some of which have OWNERSHIP WITHIN GUI CONTROLS! use getConfig() to resolve

    std::unique_ptr<DirectoryName<FolderHistoryBox>> logfileDir; //always bound, solve circular compile-time dependency

    const std::shared_ptr<FolderHistory> folderHistLeft_;
    const std::shared_ptr<FolderHistory> folderHistRight_;
};

//###################################################################################################################################

//------------------------------------------------------------------
/*    class hierarchy:

            template<>
            FolderPairPanelBasic
                    /|\
                     |
             template<>
             FolderPairCallback      BatchFolderPairGenerated
                    /|\                        /|\
            _________|______________    ________|
           |                        |  |
  DirectoryPairBatchFirst    DirectoryPairBatch
*/

template <class GuiPanel>
class FolderPairCallback : public FolderPairPanelBasic<GuiPanel> //implements callback functionality to BatchDialog as imposed by FolderPairPanelBasic
{
public:
    FolderPairCallback(GuiPanel& basicPanel, BatchDialog& batchDialog) :
        FolderPairPanelBasic<GuiPanel>(basicPanel), //pass FolderPairGenerated part...
        batchDlg(batchDialog) {}

private:
    virtual wxWindow* getParentWindow() { return &batchDlg; }

    virtual MainConfiguration getMainConfig() const { return batchDlg.getConfig().mainCfg; }

    virtual void OnAltCompCfgChange() { batchDlg.updateGui(); }

    virtual void OnAltSyncCfgChange() { batchDlg.updateGui(); }

    virtual void removeAltCompCfg()
    {
        FolderPairPanelBasic<GuiPanel>::removeAltCompCfg();
        batchDlg.updateGui();
    }

    virtual void removeAltSyncCfg()
    {
        FolderPairPanelBasic<GuiPanel>::removeAltSyncCfg();
        batchDlg.updateGui();
    }

    virtual void OnLocalFilterCfgChange() {}

    BatchDialog& batchDlg;
};


class DirectoryPairBatch:
    public BatchFolderPairGenerated, //DirectoryPairBatch "owns" BatchFolderPairGenerated!
    public FolderPairCallback<BatchFolderPairGenerated>
{
public:
    DirectoryPairBatch(wxWindow* parent, BatchDialog& batchDialog) :
        BatchFolderPairGenerated(parent),
        FolderPairCallback<BatchFolderPairGenerated>(static_cast<BatchFolderPairGenerated&>(*this), batchDialog), //pass BatchFolderPairGenerated part...
        dirNameLeft (*m_panelLeft,  *m_buttonSelectDirLeft,  *m_directoryLeft),
        dirNameRight(*m_panelRight, *m_buttonSelectDirRight, *m_directoryRight) {}

    void setValues(const wxString& leftDir,
                   const wxString& rightDir,
                   AltCompCfgPtr cmpCfg,
                   AltSyncCfgPtr syncCfg,
                   const FilterConfig& filter)
    {
        setConfig(cmpCfg, syncCfg, filter);
        dirNameLeft.setName(leftDir);
        dirNameRight.setName(rightDir);
    }
    wxString getLeftDir () const { return dirNameLeft .getName(); }
    wxString getRightDir() const { return dirNameRight.getName(); }

private:
    //support for drag and drop
    DirectoryName<FolderHistoryBox> dirNameLeft;
    DirectoryName<FolderHistoryBox> dirNameRight;
};


class DirectoryPairBatchFirst : public FolderPairCallback<BatchDlgGenerated>
{
public:
    DirectoryPairBatchFirst(BatchDialog& batchDialog) :
        FolderPairCallback<BatchDlgGenerated>(batchDialog, batchDialog),

        //prepare drag & drop
        dirNameLeft(*batchDialog.m_panelLeft,
                    *batchDialog.m_buttonSelectDirLeft,
                    *batchDialog.m_directoryLeft),
        dirNameRight(*batchDialog.m_panelRight,
                     *batchDialog.m_buttonSelectDirRight,
                     *batchDialog.m_directoryRight) {}

    void setValues(const wxString& leftDir,
                   const wxString& rightDir,
                   AltCompCfgPtr cmpCfg,
                   AltSyncCfgPtr syncCfg,
                   const FilterConfig& filter)
    {
        setConfig(cmpCfg, syncCfg, filter);
        dirNameLeft.setName(leftDir);
        dirNameRight.setName(rightDir);
    }
    wxString getLeftDir () const { return dirNameLeft .getName(); }
    wxString getRightDir() const { return dirNameRight.getName(); }

private:
    //support for drag and drop
    DirectoryName<FolderHistoryBox> dirNameLeft;
    DirectoryName<FolderHistoryBox> dirNameRight;
};
//###################################################################################################################################


BatchDialog::BatchDialog(wxWindow* parent,
                         const wxString& referenceFile,
                         const XmlBatchConfig& batchCfg,
                         const std::shared_ptr<FolderHistory>& folderHistLeft,
                         const std::shared_ptr<FolderHistory>& folderHistRight,
                         std::vector<std::wstring>& onCompletionHistory,
                         size_t onCompletionHistoryMax) :
    BatchDlgGenerated(parent),
    folderHistLeft_(folderHistLeft),
    folderHistRight_(folderHistRight)
{
    m_directoryLeft ->init(folderHistLeft_);
    m_directoryRight->init(folderHistRight_);
    m_comboBoxExecFinished->initHistory(onCompletionHistory, onCompletionHistoryMax);

#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    m_bpButtonCmpConfig ->SetBitmapLabel(GlobalResources::getImage(L"cmpConfig"));
    m_bpButtonSyncConfig->SetBitmapLabel(GlobalResources::getImage(L"syncConfig"));
    m_bpButtonHelp      ->SetBitmapLabel(GlobalResources::getImage(L"help"));

    //init handling of first folder pair
    firstFolderPair.reset(new DirectoryPairBatchFirst(*this));

    m_bpButtonCmpConfig ->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(BatchDialog::OnCompSettingsContext), nullptr, this);
    m_bpButtonSyncConfig->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(BatchDialog::OnSyncSettingsContext), nullptr, this);
    m_bpButtonFilter    ->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(BatchDialog::OnGlobalFilterContext), nullptr, this);

    //prepare drag & drop for loading of *.ffs_batch files
    setupFileDrop(*this);
    Connect(EVENT_DROP_FILE, FileDropEventHandler(BatchDialog::OnFilesDropped), nullptr, this);

    logfileDir.reset(new DirectoryName<FolderHistoryBox>(*m_panelBatchSettings, *m_buttonSelectLogfileDir, *m_comboBoxLogfileDir));

    //set icons for this dialog
    m_bpButtonAddPair->SetBitmapLabel(GlobalResources::getImage(L"item_add"));
    m_bitmap27->SetBitmap(GlobalResources::getImage(L"batch"));

    m_buttonSave->SetFocus();

    if (!referenceFile.empty())
    {
        SetTitle(referenceFile);
        proposedBatchFileName = referenceFile; //may be used on next save
    }

    setConfig(batchCfg);
}

//------------------- error handling --------------------------

void BatchDialog::OnCmpSettings(wxCommandEvent& event)
{
    //show window right next to the compare-config button
    //wxPoint windowPos = m_bpButtonCmpConfig->GetScreenPosition();
    //windowPos.x += m_bpButtonCmpConfig->GetSize().GetWidth() + 5;

    if (zen::showCompareCfgDialog(this, localBatchCfg.mainCfg.cmpConfig) == ReturnSmallDlg::BUTTON_OKAY)
        updateGui();
}


void BatchDialog::OnSyncSettings(wxCommandEvent& event)
{
    //ExecWhenFinishedCfg ewfCfg = { &localBatchCfg.mainCfg.onCompletion,
    //                               &onCompletionHistory_,
    //                               onCompletionHistoryMax_
    //                             };

    if (showSyncConfigDlg(this,
                          localBatchCfg.mainCfg.cmpConfig.compareVar,
                          localBatchCfg.mainCfg.syncCfg,
                          nullptr,
                          nullptr) == ReturnSyncConfig::BUTTON_OKAY) //optional input parameter
        updateGui();
}


void BatchDialog::OnConfigureFilter(wxCommandEvent& event)
{
    if (showFilterDialog(this,
                         true, //is main filter dialog
                         localBatchCfg.mainCfg.globalFilter) == ReturnSmallDlg::BUTTON_OKAY)
        updateGui();
}


void BatchDialog::updateGui() //re-evaluate gui after config changes
{
    XmlBatchConfig cfg = getConfig();

    m_panelLogfile        ->Enable(m_checkBoxGenerateLogfile->GetValue());
    m_spinCtrlLogfileLimit->Enable(m_checkBoxGenerateLogfile->GetValue() && m_checkBoxLogfilesLimit->GetValue());

    //update compare variant name
    m_staticTextCmpVariant->SetLabel(cfg.mainCfg.getCompVariantName());

    //update sync variant name
    m_staticTextSyncVariant->SetLabel(cfg.mainCfg.getSyncVariantName());

    //set filter icon
    if (!isNullFilter(cfg.mainCfg.globalFilter))
    {
        setImage(*m_bpButtonFilter, GlobalResources::getImage(L"filter"));
        m_bpButtonFilter->SetToolTip(_("Filter is active"));
    }
    else
    {
        setImage(*m_bpButtonFilter, greyScale(GlobalResources::getImage(L"filter")));
        m_bpButtonFilter->SetToolTip(_("No filter selected"));
    }

    m_toggleBtnErrorIgnore->SetValue(false);
    m_toggleBtnErrorPopup ->SetValue(false);
    m_toggleBtnErrorExit  ->SetValue(false);
    switch (cfg.handleError)
    {
        case ON_ERROR_IGNORE:
            m_toggleBtnErrorIgnore->SetValue(true);
            break;
        case ON_ERROR_POPUP:
            m_toggleBtnErrorPopup->SetValue(true);
            break;
        case ON_ERROR_EXIT:
            m_toggleBtnErrorExit->SetValue(true);
            break;
    }

    m_panelOverview->Layout (); //adjust stuff inside scrolled window
    m_panelOverview->Refresh(); //refresh filter button (if nothing else)
}


void BatchDialog::OnCompSettingsContext(wxCommandEvent& event)
{
    ContextMenu menu;

    auto setVariant = [&](CompareVariant var)
    {
        localBatchCfg.mainCfg.cmpConfig.compareVar = var;
        updateGui();
    };

    auto currentVar = localBatchCfg.mainCfg.cmpConfig.compareVar;

    menu.addRadio(_("File time and size"), [&] { setVariant(CMP_BY_TIME_SIZE); }, currentVar == CMP_BY_TIME_SIZE);
    menu.addRadio(_("File content"      ), [&] { setVariant(CMP_BY_CONTENT);   }, currentVar == CMP_BY_CONTENT);

    menu.popup(*this);
}


void BatchDialog::OnSyncSettingsContext(wxCommandEvent& event)
{

    ContextMenu menu;

    auto setVariant = [&](DirectionConfig::Variant var)
    {
        localBatchCfg.mainCfg.syncCfg.directionCfg.var = var;
        updateGui();
    };

    const auto currentVar = localBatchCfg.mainCfg.syncCfg.directionCfg.var;

    menu.addRadio(_("<Automatic>"), [&] { setVariant(DirectionConfig::AUTOMATIC); }, currentVar == DirectionConfig::AUTOMATIC);
    menu.addRadio(_("Mirror ->>") , [&] { setVariant(DirectionConfig::MIRROR);    }, currentVar == DirectionConfig::MIRROR);
    menu.addRadio(_("Update ->")  , [&] { setVariant(DirectionConfig::UPDATE);    }, currentVar == DirectionConfig::UPDATE);
    menu.addRadio(_("Custom")     , [&] { setVariant(DirectionConfig::CUSTOM);    }, currentVar == DirectionConfig::CUSTOM);

    menu.popup(*this);
}


void BatchDialog::OnGlobalFilterContext(wxCommandEvent& event)
{
    ContextMenu menu;

    auto clearFilter = [&]
    {
        localBatchCfg.mainCfg.globalFilter = FilterConfig();
        updateGui();
    };
    menu.addItem( _("Clear filter settings"), clearFilter, nullptr, !isNullFilter(localBatchCfg.mainCfg.globalFilter));

    menu.popup(*this);
}


void BatchDialog::OnCheckSaveLog(wxCommandEvent& event)
{
    updateGui();

    //reset error handling depending on "m_checkBoxSilent"
    //setSelectionHandleError(getEnumVal(enumDescrMap, *m_choiceHandleError));
}


void BatchDialog::OnFilesDropped(FileDropEvent& event)
{
    loadBatchFile(event.getFiles());
}


/*
void BatchDialog::showNotebookpage(wxWindow* page, const wxString& pageName, bool show)
{
    int windowPosition = -1;
    for (size_t i = 0; i < m_notebookSettings->GetPageCount(); ++i)
        if (m_notebookSettings->GetPage(i) == page)
        {
            windowPosition = static_cast<int>(i);
            break;
        }

    if (show)
    {
        if (windowPosition == -1)
            m_notebookSettings->AddPage(page, pageName, false);
    }
    else
    {
        if (windowPosition != -1)
        {
            //do not delete currently selected tab!!
            if (m_notebookSettings->GetCurrentPage() == m_notebookSettings->GetPage(windowPosition))
                m_notebookSettings->ChangeSelection(0);

            m_notebookSettings->RemovePage(windowPosition);
        }
    }
}
*/

void BatchDialog::OnSaveBatchJob(wxCommandEvent& event)
{
    //get a filename
    wxString defaultFileName = proposedBatchFileName.empty() ? wxT("SyncJob.ffs_batch") : proposedBatchFileName;

    //attention: proposedBatchFileName may be an imported *.ffs_gui file! We don't want to overwrite it with a BATCH config!
    if (endsWith(defaultFileName, L".ffs_gui"))
        replace(defaultFileName, L".ffs_gui", L".ffs_batch");

    wxFileDialog filePicker(this,
                            wxEmptyString,
                            wxEmptyString,
                            defaultFileName,
                            _("FreeFileSync batch") + L" (*.ffs_batch)|*.ffs_batch" + L"|" +_("All files") + L" (*.*)|*",
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT); //creating this on freestore leads to memleak!
    if (filePicker.ShowModal() == wxID_OK)
    {
        const wxString newFileName = filePicker.GetPath();

        saveBatchFile(newFileName);
        //if ()
        //    EndModal(ReturnBatchConfig::BATCH_FILE_SAVED);
    }
}


void BatchDialog::OnLoadBatchJob(wxCommandEvent& event)
{
    wxFileDialog filePicker(this,
                            wxEmptyString,
                            beforeLast(proposedBatchFileName, utfCvrtTo<wxString>(FILE_NAME_SEPARATOR)), //set default dir: empty string if "currentConfigFileName" is empty or has no path separator
                            wxEmptyString,
                            _("FreeFileSync batch") + L" (*.ffs_batch;*.ffs_gui)|*.ffs_batch;*.ffs_gui" + L"|" +_("All files") + L" (*.*)|*",
                            wxFD_OPEN | wxFD_MULTIPLE); //creating this on freestore leads to memleak!
    if (filePicker.ShowModal() == wxID_OK)
    {
        wxArrayString tmp;
        filePicker.GetPaths(tmp);
        std::vector<wxString> fileNames(tmp.begin(), tmp.end());

        loadBatchFile(fileNames);
    }
}



inline
FolderPairEnh getPairCfg(const DirectoryPairBatch& panel)
{
    return FolderPairEnh(toZ(panel.getLeftDir()),
                         toZ(panel.getRightDir()),
                         panel.getAltCompConfig(),
                         panel.getAltSyncConfig(),
                         panel.getAltFilterConfig());
}


bool BatchDialog::saveBatchFile(const wxString& filename)
{
    const XmlBatchConfig batchCfg = getConfig();

    //a good place to commit current "on completion" history item
    m_comboBoxExecFinished->addItemHistory();

    //write config to XML
    try
    {
        writeConfig(batchCfg, toZ(filename));
    }
    catch (const FfsXmlError& error)
    {
        wxMessageBox(error.toString().c_str(), _("Error"), wxOK | wxICON_ERROR, this);
        return false;
    }

    SetTitle(filename);
    proposedBatchFileName = filename; //may be used on next save

    return true;
}


void BatchDialog::loadBatchFile(const wxString& filename)
{
    std::vector<wxString> filenames;
    filenames.push_back(filename);
    loadBatchFile(filenames);
}


void BatchDialog::loadBatchFile(const std::vector<wxString>& filenames)
{
    if (filenames.empty())
        return;

    //load XML settings
    XmlBatchConfig batchCfg;  //structure to receive gui settings
    try
    {
        //open a *.ffs_gui or *.ffs_batch file!
        mergeConfigs(toZ(filenames), batchCfg); //throw FfsXmlError

        //readConfig(filename, batchCfg);
    }
    catch (const FfsXmlError& error)
    {
        if (error.getSeverity() == FfsXmlError::WARNING)
            wxMessageBox(error.toString(), _("Warning"), wxOK | wxICON_WARNING, this);
        else
        {
            wxMessageBox(error.toString(), _("Error"), wxOK | wxICON_ERROR, this);
            return;
        }
    }

    const wxString activeFile = filenames.size() == 1 ? filenames[0] : wxString();

    if (activeFile.empty())
        SetTitle(_("Create a batch job"));
    else
        SetTitle(activeFile);

    proposedBatchFileName = activeFile; //may be used on next save

    setConfig(batchCfg);
}


void BatchDialog::setConfig(const XmlBatchConfig& batchCfg)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    localBatchCfg = batchCfg; //contains some parameters not owned by GUI controls

    //transfer parameter ownership to GUI

    m_checkBoxShowProgress->SetValue(batchCfg.showProgress);
    logfileDir->setName(utfCvrtTo<wxString>(batchCfg.logFileDirectory));
    m_comboBoxExecFinished->setValue(batchCfg.mainCfg.onCompletion);

    //map single parameter "logfiles limit" to all three checkboxs and spin ctrl:
    m_checkBoxGenerateLogfile->SetValue(batchCfg.logfilesCountLimit != 0);
    m_checkBoxLogfilesLimit  ->SetValue(batchCfg.logfilesCountLimit >= 0);
    m_spinCtrlLogfileLimit   ->SetValue(batchCfg.logfilesCountLimit >= 0 ? batchCfg.logfilesCountLimit : 100 /*XmlBatchConfig().logfilesCountLimit*/);
    //attention: emits a "change value" event!! => updateGui() called implicitly!

    //set first folder pair
    firstFolderPair->setValues(toWx(batchCfg.mainCfg.firstPair.leftDirectory),
                               toWx(batchCfg.mainCfg.firstPair.rightDirectory),
                               batchCfg.mainCfg.firstPair.altCmpConfig,
                               batchCfg.mainCfg.firstPair.altSyncConfig,
                               batchCfg.mainCfg.firstPair.localFilter);

    //set additional pairs
    clearAddFolderPairs();
    addFolderPair(batchCfg.mainCfg.additionalPairs);

    updateGui(); //re-evaluate gui after config changes

    Fit(); //needed
    Refresh(); //needed
    Centre();
}


XmlBatchConfig BatchDialog::getConfig() const
{
    XmlBatchConfig batchCfg = localBatchCfg;

    //load parameter with ownership within wxWidgets controls...

    //first folder pair
    batchCfg.mainCfg.firstPair = FolderPairEnh(toZ(firstFolderPair->getLeftDir()),
                                               toZ(firstFolderPair->getRightDir()),
                                               firstFolderPair->getAltCompConfig(),
                                               firstFolderPair->getAltSyncConfig(),
                                               firstFolderPair->getAltFilterConfig());
    //add additional pairs
    batchCfg.mainCfg.additionalPairs.clear();
    std::transform(additionalFolderPairs.begin(), additionalFolderPairs.end(),
    std::back_inserter(batchCfg.mainCfg.additionalPairs), [](const DirectoryPairBatch* dp) { return getPairCfg(*dp); });

    //load structure with batch settings "batchCfg"
    batchCfg.showProgress     = m_checkBoxShowProgress->GetValue();
    batchCfg.logFileDirectory = utfCvrtTo<Zstring>(logfileDir->getName());
    batchCfg.mainCfg.onCompletion = m_comboBoxExecFinished->getValue();
    //get single parameter "logfiles limit" from all three checkboxes and spin ctrl:
    batchCfg.logfilesCountLimit = m_checkBoxGenerateLogfile->GetValue() ? (m_checkBoxLogfilesLimit->GetValue() ? m_spinCtrlLogfileLimit->GetValue() : -1) : 0;

    return batchCfg;
}


void BatchDialog::OnAddFolderPair(wxCommandEvent& event)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    std::vector<FolderPairEnh> newPairs;
    newPairs.push_back(getConfig().mainCfg.firstPair);

    //clear first pair
    const FolderPairEnh cfgEmpty;
    firstFolderPair->setValues(toWx(cfgEmpty.leftDirectory),
                               toWx(cfgEmpty.rightDirectory),
                               cfgEmpty.altCmpConfig,
                               cfgEmpty.altSyncConfig,
                               cfgEmpty.localFilter);

    //keep sequence to update GUI as last step
    addFolderPair(newPairs, true); //add pair in front of additonal pairs
}


void BatchDialog::OnRemoveFolderPair(wxCommandEvent& event)
{
    //find folder pair originating the event
    const wxObject* const eventObj = event.GetEventObject();
    for (std::vector<DirectoryPairBatch*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
    {
        if (eventObj == static_cast<wxObject*>((*i)->m_bpButtonRemovePair))
        {
            removeAddFolderPair(i - additionalFolderPairs.begin());
            return;
        }
    }
}


void BatchDialog::OnRemoveTopFolderPair(wxCommandEvent& event)
{
    if (!additionalFolderPairs.empty())
    {
        //get settings from second folder pair
        const FolderPairEnh cfgSecond = getPairCfg(*additionalFolderPairs[0]);

        //reset first pair
        firstFolderPair->setValues(toWx(cfgSecond.leftDirectory),
                                   toWx(cfgSecond.rightDirectory),
                                   cfgSecond.altCmpConfig,
                                   cfgSecond.altSyncConfig,
                                   cfgSecond.localFilter);

        removeAddFolderPair(0); //remove second folder pair (first of additional folder pairs)
    }
}


void BatchDialog::updateGuiForFolderPair()
{
    //adapt delete top folder pair button
    if (additionalFolderPairs.empty())
        m_bpButtonRemovePair->Hide();
    else
        m_bpButtonRemovePair->Show();

    //adapt local filter and sync cfg for first folder pair
    const bool showLocalCfgFirstPair = !additionalFolderPairs.empty()                       ||
                                       firstFolderPair->getAltCompConfig().get() != nullptr ||
                                       firstFolderPair->getAltSyncConfig().get() != nullptr ||
                                       !isNullFilter(firstFolderPair->getAltFilterConfig());

    m_bpButtonAltCompCfg ->Show(showLocalCfgFirstPair);
    m_bpButtonAltSyncCfg ->Show(showLocalCfgFirstPair);
    m_bpButtonLocalFilter->Show(showLocalCfgFirstPair);

    //update controls
    const int maxAddFolderPairsVisible = 2;

    int pairHeight = sbSizerMainPair->GetSize().GetHeight(); //respect height of main pair
    if (!additionalFolderPairs.empty())
        pairHeight += std::min<double>(maxAddFolderPairsVisible + 0.5, additionalFolderPairs.size()) * //have 0.5 * height indicate that more folders are there
                      additionalFolderPairs[0]->GetSize().GetHeight();

    m_scrolledWindow6->SetMinSize(wxSize( -1, pairHeight));


    m_scrolledWindow6->Fit();     //adjust scrolled window size
    m_scrolledWindow6->Layout();  //adjust scrolled window size

    //bSizerAddFolderPairs->FitInside(m_scrolledWindow6);  //adjust scrolled window size

    m_panelOverview->Layout(); //adjust stuff inside scrolled window
    //m_panelOverview->InvalidateBestSize(); //needed for Fit() to work correctly!

    if (m_scrolledWindow6->GetSize().GetHeight() < pairHeight)
        Fit();                     //adapt dialog size
}


void BatchDialog::addFolderPair(const std::vector<zen::FolderPairEnh>& newPairs, bool addFront)
{
    wxWindowUpdateLocker dummy(m_panelOverview); //avoid display distortion

    if (!newPairs.empty())
    {
        //add folder pairs
        for (std::vector<zen::FolderPairEnh>::const_iterator i = newPairs.begin(); i != newPairs.end(); ++i)
        {
            DirectoryPairBatch* newPair = new DirectoryPairBatch(m_scrolledWindow6, *this); //owned by m_scrolledWindow6!

            //init dropdown history
            newPair->m_directoryLeft ->init(folderHistLeft_);
            newPair->m_directoryRight->init(folderHistRight_);

            if (addFront)
            {
                bSizerAddFolderPairs->Insert(0, newPair, 0, wxEXPAND, 5);
                additionalFolderPairs.insert(additionalFolderPairs.begin(), newPair);
            }
            else
            {
                bSizerAddFolderPairs->Add(newPair, 0, wxEXPAND, 5);
                additionalFolderPairs.push_back(newPair);
            }

            //register events
            newPair->m_bpButtonRemovePair->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(BatchDialog::OnRemoveFolderPair), nullptr, this );

            //set alternate configuration
            newPair->setValues(toWx(i->leftDirectory),
                               toWx(i->rightDirectory),
                               i->altCmpConfig,
                               i->altSyncConfig,
                               i->localFilter);
        }

        //after changing folder pairs window focus is lost: results in scrolled window scrolling to top each time window is shown: we don't want this
        m_bpButtonAddPair->SetFocus();
    }

    updateGuiForFolderPair();

    updateGui(); //mainly to update sync dir description text
}


void BatchDialog::removeAddFolderPair(int pos)
{
    wxWindowUpdateLocker dummy(m_panelOverview); //avoid display distortion

    if (0 <= pos && pos < static_cast<int>(additionalFolderPairs.size()))
    {
        //remove folder pairs from window
        DirectoryPairBatch* pairToDelete = additionalFolderPairs[pos];

        bSizerAddFolderPairs->Detach(pairToDelete); //Remove() does not work on Window*, so do it manually
        pairToDelete->Destroy();                 //
        additionalFolderPairs.erase(additionalFolderPairs.begin() + pos); //remove last element in vector

        //after changing folder pairs window focus is lost: results in scrolled window scrolling to top each time window is shown: we don't want this
        m_bpButtonRemovePair->SetFocus();
    }

    updateGuiForFolderPair();

    updateGui(); //mainly to update sync dir description text
}


void BatchDialog::clearAddFolderPairs()
{
    wxWindowUpdateLocker dummy(m_panelOverview); //avoid display distortion

    additionalFolderPairs.clear();
    bSizerAddFolderPairs->Clear(true);

    updateGuiForFolderPair();
}


void zen::showSyncBatchDlg(wxWindow* parent,
                           const wxString& referenceFile,
                           const XmlBatchConfig& batchCfg,
                           const std::shared_ptr<FolderHistory>& folderHistLeft,
                           const std::shared_ptr<FolderHistory>& folderHistRight,
                           std::vector<std::wstring>& execFinishedhistory,
                           size_t execFinishedhistoryMax)
{
    BatchDialog batchDlg(parent, referenceFile, batchCfg, folderHistLeft, folderHistRight, execFinishedhistory, execFinishedhistoryMax);
    batchDlg.ShowModal();
}
