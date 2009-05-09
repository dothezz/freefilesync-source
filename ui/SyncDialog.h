#ifndef SYNCDIALOG_H_INCLUDED
#define SYNCDIALOG_H_INCLUDED

#include "../structures.h"
#include "guiGenerated.h"
#include "../library/processXml.h"
#include <memory>

class BatchFileDropEvent;
class BatchFolderPairPanel;

namespace FreeFileSync
{
    class DragDropOnDlg;
}


class SyncDialog: public SyncDlgGenerated
{
public:
    SyncDialog(wxWindow* window,
               const FreeFileSync::FolderComparison& folderCmpRef,
               FreeFileSync::MainConfiguration& config,
               bool& ignoreErrors,
               bool synchronizationEnabled);

    ~SyncDialog();

    enum
    {
        BUTTON_START = 15
    };

    static void updateConfigIcons(wxBitmapButton* button1,
                                  wxBitmapButton* button2,
                                  wxBitmapButton* button3,
                                  wxBitmapButton* button4,
                                  wxBitmapButton* button5,
                                  const FreeFileSync::SyncConfiguration& syncConfig);

    static void adjustToolTips(wxStaticBitmap* bitmap, const FreeFileSync::CompareVariant var);

private:
    void calculatePreview();

    void OnSyncLeftToRight( wxCommandEvent& event);
    void OnSyncUpdate(      wxCommandEvent& event);
    void OnSyncBothSides(   wxCommandEvent& event);

    void OnExLeftSideOnly(  wxCommandEvent& event);
    void OnExRightSideOnly( wxCommandEvent& event);
    void OnLeftNewer(       wxCommandEvent& event);
    void OnRightNewer(      wxCommandEvent& event);
    void OnDifferent(       wxCommandEvent& event);

    void OnStartSync(       wxCommandEvent& event);
    void OnClose(           wxCloseEvent&   event);
    void OnBack(            wxCommandEvent& event);
    void OnCancel(          wxCommandEvent& event);

    void OnSelectRecycleBin(wxCommandEvent& event);

    //temporal copy of maindialog.cfg.syncConfiguration
    FreeFileSync::SyncConfiguration localSyncConfiguration;
    const FreeFileSync::FolderComparison& folderCmp;
    FreeFileSync::MainConfiguration& cfg;
    bool& m_ignoreErrors;
};


class BatchDialog: public BatchDlgGenerated
{
    friend class BatchFileDropEvent;

public:
    BatchDialog(wxWindow* window, const xmlAccess::XmlBatchConfig& batchCfg);
    BatchDialog(wxWindow* window, const wxString& filename);
    ~BatchDialog() {};

    enum
    {
        BATCH_FILE_SAVED = 15
    };

private:
    void init();

    void OnChangeErrorHandling(wxCommandEvent& event);

    void OnExLeftSideOnly(  wxCommandEvent& event);
    void OnExRightSideOnly( wxCommandEvent& event);
    void OnLeftNewer(       wxCommandEvent& event);
    void OnRightNewer(      wxCommandEvent& event);
    void OnDifferent(       wxCommandEvent& event);

    void OnCheckFilter(     wxCommandEvent& event);
    void OnCheckLogging(    wxCommandEvent& event);
    void OnSelectRecycleBin(wxCommandEvent& event);
    void OnChangeCompareVar(wxCommandEvent& event);

    void updateVisibleTabs();
    void showNotebookpage(wxWindow* page, const wxString& pageName, bool show);

    void OnClose(           wxCloseEvent&   event);
    void OnCancel(          wxCommandEvent& event);
    void OnSaveBatchJob(    wxCommandEvent& event);
    void OnLoadBatchJob(    wxCommandEvent& event);

    xmlAccess::OnError getSelectionHandleError();
    void setSelectionHandleError(const xmlAccess::OnError value);

    bool saveBatchFile(const wxString& filename);
    void loadBatchFile(const wxString& filename);
    void loadBatchCfg(const xmlAccess::XmlBatchConfig& batchCfg);

    FreeFileSync::SyncConfiguration localSyncConfiguration;
    std::vector<BatchFolderPairPanel*> localFolderPairs;

    //used when saving batch file
    wxString proposedBatchFileName;

    //add drag & drop support when selecting logfile directory
    std::auto_ptr<FreeFileSync::DragDropOnDlg> dragDropOnLogfileDir;
};

#endif // SYNCDIALOG_H_INCLUDED
