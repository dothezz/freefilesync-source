#ifndef SYNCDIALOG_H_INCLUDED
#define SYNCDIALOG_H_INCLUDED

#include "guiGenerated.h"
#include "../library/processXml.h"
#include <memory>

class BatchFileDropEvent;
class BatchFolderPairPanel;

namespace FreeFileSync
{
    class DragDropOnDlg;
}


class SyncCfgDialog : public SyncCfgDlgGenerated
{
public:
    SyncCfgDialog(wxWindow* window,
                  const FreeFileSync::FolderComparison& folderCmpRef,
                  FreeFileSync::MainConfiguration& config,
                  bool& ignoreErrors);

    ~SyncCfgDialog();

    enum
    {
        BUTTON_OKAY = 10
    };

    static void updateConfigIcons(const FreeFileSync::CompareVariant compareVar,
                                  const FreeFileSync::SyncConfiguration& syncConfig,
                                  wxBitmapButton* buttonLeftOnly,
                                  wxBitmapButton* buttonRightOnly,
                                  wxBitmapButton* buttonLeftNewer,
                                  wxBitmapButton* buttonRightNewer,
                                  wxBitmapButton* buttonDifferent,
                                  wxStaticBitmap* bitmapLeftOnly,
                                  wxStaticBitmap* bitmapRightOnly,
                                  wxStaticBitmap* bitmapLeftNewer,
                                  wxStaticBitmap* bitmapRightNewer,
                                  wxStaticBitmap* bitmapDifferent);
    //some syntax relaxation
    void updateConfigIcons(const FreeFileSync::CompareVariant cmpVar, const FreeFileSync::SyncConfiguration& syncConfig);

private:
    void OnSyncLeftToRight( wxCommandEvent& event);
    void OnSyncUpdate(      wxCommandEvent& event);
    void OnSyncBothSides(   wxCommandEvent& event);

    void OnExLeftSideOnly(  wxCommandEvent& event);
    void OnExRightSideOnly( wxCommandEvent& event);
    void OnLeftNewer(       wxCommandEvent& event);
    void OnRightNewer(      wxCommandEvent& event);
    void OnDifferent(       wxCommandEvent& event);

    void OnClose(           wxCloseEvent&   event);
    void OnCancel(          wxCommandEvent& event);
    void OnApply(           wxCommandEvent& event);

    //error handling
    bool getErrorHandling();
    void setErrorHandling(bool ignoreErrors);
    void OnChangeErrorHandling(wxCommandEvent& event);

    //deletion handling
    FreeFileSync::DeletionPolicy getDeletionHandling();
    void setDeletionHandling(FreeFileSync::DeletionPolicy newValue);
    void OnChangeDeletionHandling(wxCommandEvent& event);

    //temporal copy of maindialog.cfg.syncConfiguration
    FreeFileSync::SyncConfiguration localSyncConfiguration;
    const FreeFileSync::FolderComparison& folderCmp;
    FreeFileSync::MainConfiguration& cfg;
    bool& m_ignoreErrors;

    std::auto_ptr<FreeFileSync::DragDropOnDlg> dragDropCustomDelFolder;
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

    virtual void OnExLeftSideOnly(     wxCommandEvent& event);
    virtual void OnExRightSideOnly(    wxCommandEvent& event);
    virtual void OnLeftNewer(          wxCommandEvent& event);
    virtual void OnRightNewer(         wxCommandEvent& event);
    virtual void OnDifferent(          wxCommandEvent& event);

    virtual void OnCheckFilter(        wxCommandEvent& event);
    virtual void OnCheckLogging(       wxCommandEvent& event);
    virtual void OnChangeCompareVar(   wxCommandEvent& event);
    virtual void OnClose(              wxCloseEvent&   event);
    virtual void OnCancel(             wxCommandEvent& event);
    virtual void OnSaveBatchJob(       wxCommandEvent& event);
    virtual void OnLoadBatchJob(       wxCommandEvent& event);
    virtual void OnAddFolderPair(      wxCommandEvent& event);
    virtual void OnRemoveFolderPair(   wxCommandEvent& event);
    virtual void OnRemoveTopFolderPair(wxCommandEvent& event);

    void addFolderPair(const std::vector<FreeFileSync::FolderPair>& newPairs, bool addFront = false);
    void removeAddFolderPair(const int pos);
    void clearAddFolderPairs();
    std::vector<FreeFileSync::FolderPair> getFolderPairs() const;

    FreeFileSync::CompareVariant getCurrentCompareVar();

    void updateConfigIcons(const FreeFileSync::CompareVariant cmpVar, const FreeFileSync::SyncConfiguration& syncConfig);

    void updateVisibleTabs();
    void showNotebookpage(wxWindow* page, const wxString& pageName, bool show);

    //error handling
    xmlAccess::OnError getSelectionHandleError();
    void setSelectionHandleError(const xmlAccess::OnError value);
    void OnChangeErrorHandling(wxCommandEvent& event);

    //deletion handling
    FreeFileSync::DeletionPolicy getDeletionHandling();
    void setDeletionHandling(FreeFileSync::DeletionPolicy newValue);
    void OnChangeDeletionHandling(wxCommandEvent& event);


    bool saveBatchFile(const wxString& filename);
    void loadBatchFile(const wxString& filename);
    void loadBatchCfg(const xmlAccess::XmlBatchConfig& batchCfg);


    FreeFileSync::SyncConfiguration localSyncConfiguration;
    std::vector<BatchFolderPairPanel*> additionalFolderPairs;

    //used when saving batch file
    wxString proposedBatchFileName;

    //add drag & drop support when selecting logfile directory
    std::auto_ptr<FreeFileSync::DragDropOnDlg> dragDropOnLogfileDir;

    //support for drag and drop on main pair
    std::auto_ptr<FreeFileSync::DragDropOnDlg> dragDropOnLeft;
    std::auto_ptr<FreeFileSync::DragDropOnDlg> dragDropOnRight;

    std::auto_ptr<FreeFileSync::DragDropOnDlg> dragDropCustomDelFolder;
};

#endif // SYNCDIALOG_H_INCLUDED
