// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "batch_config.h"
#include <wx/wupdlock.h>
#include <wx+/mouse_move_dlg.h>
#include <wx+/std_button_order.h>
#include <wx+/font_size.h>
#include "gui_generated.h"
#include "dir_name.h"
#include "../ui/exec_finished_box.h"
#include "../lib/help_provider.h"
#include "../lib/resources.h"

using namespace zen;
using namespace xmlAccess;


namespace
{
enum ButtonPressed
{
    BUTTON_CANCEL,
    BUTTON_SAVE_AS
};


class BatchDialog : public BatchDlgGenerated
{
public:
    BatchDialog(wxWindow* parent,
                XmlBatchConfig& batchCfg, //in/out
                std::vector<std::wstring>& onCompletionHistory,
                size_t onCompletionHistoryMax);

private:
    virtual void OnHelp        (wxCommandEvent& event) { displayHelpEntry(L"html/Schedule a Batch Job.html", this); }
    virtual void OnClose       (wxCloseEvent&   event) { EndModal(BUTTON_CANCEL); }
    virtual void OnCancel      (wxCommandEvent& event) { EndModal(BUTTON_CANCEL); }
    virtual void OnSaveBatchJob(wxCommandEvent& event);
    virtual void OnErrorPopup (wxCommandEvent& event) { localBatchCfg.handleError = ON_ERROR_POPUP;  updateGui(); }
    virtual void OnErrorIgnore(wxCommandEvent& event) { localBatchCfg.handleError = ON_ERROR_IGNORE; updateGui(); }
    virtual void OnErrorAbort (wxCommandEvent& event) { localBatchCfg.handleError = ON_ERROR_ABORT;  updateGui(); }

    virtual void OnToggleGenerateLogfile(wxCommandEvent& event) { updateGui(); }
    virtual void OnToggleLogfilesLimit  (wxCommandEvent& event) { updateGui(); }

    void updateGui(); //re-evaluate gui after config changes

    void setConfig(const XmlBatchConfig& batchCfg);
    XmlBatchConfig getConfig() const;

    XmlBatchConfig& batchCfgOutRef; //output only!
    XmlBatchConfig localBatchCfg; //a mixture of settings some of which have OWNERSHIP WITHIN GUI CONTROLS! use getConfig() to resolve

    std::unique_ptr<DirectoryName<FolderHistoryBox>> logfileDir; //always bound, solve circular compile-time dependency
};

//###################################################################################################################################

BatchDialog::BatchDialog(wxWindow* parent,
                         XmlBatchConfig& batchCfg,
                         std::vector<std::wstring>& onCompletionHistory,
                         size_t onCompletionHistoryMax) :
    BatchDlgGenerated(parent),
    batchCfgOutRef(batchCfg)
{
#ifdef ZEN_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif
    setStandardButtonOrder(*bSizerStdButtons, StdButtons().setAffirmative(m_buttonSaveAs).setCancel(m_buttonCancel));

    wxWindowUpdateLocker dummy(this); //avoid display distortion
    setRelativeFontSize(*m_staticTextHeader, 1.25);

    m_staticTextDescr->SetLabel(replaceCpy(m_staticTextDescr->GetLabel(), L"%x", L"FreeFileSync.exe <" + _("job name") + L">.ffs_batch"));

    m_comboBoxExecFinished->initHistory(onCompletionHistory, onCompletionHistoryMax);

    m_bpButtonHelp  ->SetBitmapLabel(getResourceImage(L"help"));
    m_bitmapBatchJob->SetBitmap     (getResourceImage(L"batch"));

    logfileDir = make_unique<DirectoryName<FolderHistoryBox>>(*this, *m_buttonSelectLogfileDir, *m_logfileDir);

    setConfig(batchCfg);

    Fit(); //child-element widths have changed: image was set
    Layout();

    m_buttonSaveAs->SetFocus();
}


void BatchDialog::updateGui() //re-evaluate gui after config changes
{
    XmlBatchConfig cfg = getConfig(); //resolve parameter ownership: some on GUI controls, others member variables

    m_panelLogfile        ->Enable(m_checkBoxGenerateLogfile->GetValue()); //enabled status is *not* directly dependent from resolved config! (but transitively)
    m_spinCtrlLogfileLimit->Enable(m_checkBoxGenerateLogfile->GetValue() && m_checkBoxLogfilesLimit->GetValue());

    m_toggleBtnErrorIgnore->SetValue(false);
    m_toggleBtnErrorPopup ->SetValue(false);
    m_toggleBtnErrorAbort ->SetValue(false);
    switch (cfg.handleError) //*not* owned by GUI controls
    {
        case ON_ERROR_IGNORE:
            m_toggleBtnErrorIgnore->SetValue(true);
            break;
        case ON_ERROR_POPUP:
            m_toggleBtnErrorPopup->SetValue(true);
            break;
        case ON_ERROR_ABORT:
            m_toggleBtnErrorAbort->SetValue(true);
            break;
    }
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

    updateGui(); //re-evaluate gui after config changes
}


XmlBatchConfig BatchDialog::getConfig() const
{
    XmlBatchConfig batchCfg = localBatchCfg;

    //load parameters with ownership within GIU controls...

    //load structure with batch settings "batchCfg"
    batchCfg.showProgress     = m_checkBoxShowProgress->GetValue();
    batchCfg.logFileDirectory = utfCvrtTo<Zstring>(logfileDir->getName());
    batchCfg.mainCfg.onCompletion = m_comboBoxExecFinished->getValue();
    //get single parameter "logfiles limit" from all three checkboxes and spin ctrl:
    batchCfg.logfilesCountLimit = m_checkBoxGenerateLogfile->GetValue() ? (m_checkBoxLogfilesLimit->GetValue() ? m_spinCtrlLogfileLimit->GetValue() : -1) : 0;

    return batchCfg;
}


void BatchDialog::OnSaveBatchJob(wxCommandEvent& event)
{
    batchCfgOutRef = getConfig();
    m_comboBoxExecFinished->addItemHistory(); //a good place to commit current "on completion" history item
    EndModal(BUTTON_SAVE_AS);
}
}


bool zen::customizeBatchConfig(wxWindow* parent,
                               xmlAccess::XmlBatchConfig& batchCfg, //in/out
                               std::vector<std::wstring>& execFinishedhistory,
                               size_t execFinishedhistoryMax)
{
    BatchDialog batchDlg(parent, batchCfg, execFinishedhistory, execFinishedhistoryMax);
    return static_cast<ButtonPressed>(batchDlg.ShowModal()) == BUTTON_SAVE_AS;
}
