#ifndef SYNCDIALOG_H_INCLUDED
#define SYNCDIALOG_H_INCLUDED

#include "guiGenerated.h"
#include "../library/processXml.h"
#include <memory>

class BatchFileDropEvent;
class BatchFolderPairPanel;
class FirstBatchFolderPairCfg;

namespace FreeFileSync
{
class DragDropOnDlg;
}


class SyncCfgDialog : public SyncCfgDlgGenerated
{
public:
    SyncCfgDialog(wxWindow* window,
                  const FreeFileSync::CompareVariant compareVar,
                  FreeFileSync::SyncConfiguration& syncConfiguration,
                  FreeFileSync::DeletionPolicy& handleDeletion,
                  wxString& customDeletionDirectory,
                  bool* ignoreErrors); //optional input parameter

    ~SyncCfgDialog();

    enum
    {
        BUTTON_APPLY = 10
    };

    static void updateConfigIcons(const FreeFileSync::CompareVariant compareVar,
                                  const FreeFileSync::SyncConfiguration& syncConfig,
                                  wxBitmapButton* buttonLeftOnly,
                                  wxBitmapButton* buttonRightOnly,
                                  wxBitmapButton* buttonLeftNewer,
                                  wxBitmapButton* buttonRightNewer,
                                  wxBitmapButton* buttonDifferent,
                                  wxBitmapButton* buttonConflict,
                                  wxStaticBitmap* bitmapLeftOnly,
                                  wxStaticBitmap* bitmapRightOnly,
                                  wxStaticBitmap* bitmapLeftNewer,
                                  wxStaticBitmap* bitmapRightNewer,
                                  wxStaticBitmap* bitmapDifferent,
                                  wxStaticBitmap* bitmapConflict,
                                  wxSizer*        syncDirections);
    //some syntax relaxation
    void updateConfigIcons(const FreeFileSync::CompareVariant cmpVar, const FreeFileSync::SyncConfiguration& syncConfig);

private:
    virtual void OnSyncAutomatic(   wxCommandEvent& event);
    virtual void OnSyncLeftToRight( wxCommandEvent& event);
    virtual void OnSyncUpdate(      wxCommandEvent& event);
    virtual void OnSyncBothSides(   wxCommandEvent& event);

    virtual void OnExLeftSideOnly(  wxCommandEvent& event);
    virtual void OnExRightSideOnly( wxCommandEvent& event);
    virtual void OnLeftNewer(       wxCommandEvent& event);
    virtual void OnRightNewer(      wxCommandEvent& event);
    virtual void OnDifferent(       wxCommandEvent& event);
    virtual void OnConflict(        wxCommandEvent& event);

    virtual void OnClose(           wxCloseEvent&   event);
    virtual void OnCancel(          wxCommandEvent& event);
    virtual void OnApply(           wxCommandEvent& event);

    //set tooltip
    void updateToolTipErrorHandling(bool ignoreErrors);

    //error handling
    bool getErrorHandling();
    void setErrorHandling(bool ignoreErrors);
    void OnChangeErrorHandling(wxCommandEvent& event);

    //deletion handling
    FreeFileSync::DeletionPolicy getDeletionHandling();
    void setDeletionHandling(FreeFileSync::DeletionPolicy newValue);
    void OnChangeDeletionHandling(wxCommandEvent& event);

    const FreeFileSync::CompareVariant cmpVariant;

    //temporal copy of maindialog.cfg.syncConfiguration
    FreeFileSync::SyncConfiguration localSyncConfiguration;

    //changing data
    FreeFileSync::SyncConfiguration& refSyncConfiguration;
    FreeFileSync::DeletionPolicy& refHandleDeletion;
    wxString& refCustomDeletionDirectory;
    bool* refIgnoreErrors;

    std::auto_ptr<FreeFileSync::DragDropOnDlg> dragDropCustomDelFolder;
};


class BatchDialog: public BatchDlgGenerated
{
    friend class BatchFileDropEvent;
    template <class GuiPanel>
    friend class FolderPairCallback;

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
    virtual void OnConflict(           wxCommandEvent& event);

    virtual void OnCheckAutomatic(     wxCommandEvent& event);
    virtual void OnCheckFilter(        wxCommandEvent& event);
    virtual void OnCheckSilent(        wxCommandEvent& event);
    virtual void OnChangeCompareVar(   wxCommandEvent& event);
    virtual void OnClose(              wxCloseEvent&   event);
    virtual void OnCancel(             wxCommandEvent& event);
    virtual void OnSaveBatchJob(       wxCommandEvent& event);
    virtual void OnLoadBatchJob(       wxCommandEvent& event);
    virtual void OnAddFolderPair(      wxCommandEvent& event);
    virtual void OnRemoveFolderPair(   wxCommandEvent& event);
    virtual void OnRemoveTopFolderPair(wxCommandEvent& event);

    void addFolderPair(const std::vector<FreeFileSync::FolderPairEnh>& newPairs, bool addFront = false);
    void removeAddFolderPair(const int pos);
    void clearAddFolderPairs();

void updateGuiForFolderPair();

    FreeFileSync::CompareVariant getCurrentCompareVar() const;

    void updateConfigIcons(const FreeFileSync::CompareVariant cmpVar, const FreeFileSync::SyncConfiguration& syncConfig);

    void updateVisibleTabs();
    void showNotebookpage(wxWindow* page, const wxString& pageName, bool show);

    //error handling
    xmlAccess::OnError getSelectionHandleError() const;
    void setSelectionHandleError(const xmlAccess::OnError value);
    void OnChangeErrorHandling(wxCommandEvent& event);

    //set tooltip
    void updateToolTipErrorHandling(const xmlAccess::OnError value);

    //deletion handling
    FreeFileSync::DeletionPolicy getDeletionHandling() const;
    void setDeletionHandling(FreeFileSync::DeletionPolicy newValue);
    void OnChangeDeletionHandling(wxCommandEvent& event);


    bool saveBatchFile(const wxString& filename);
    void loadBatchFile(const wxString& filename);
    void loadBatchCfg(const xmlAccess::XmlBatchConfig& batchCfg);

    xmlAccess::XmlBatchConfig getCurrentConfiguration() const;

    FreeFileSync::SyncConfiguration localSyncConfiguration;

    boost::shared_ptr<FirstBatchFolderPairCfg> firstFolderPair; //always bound!!!
    std::vector<BatchFolderPairPanel*> additionalFolderPairs;

    //used when saving batch file
    wxString proposedBatchFileName;

    //add drag & drop support when selecting logfile directory
    std::auto_ptr<FreeFileSync::DragDropOnDlg> dragDropOnLogfileDir;
    std::auto_ptr<FreeFileSync::DragDropOnDlg> dragDropCustomDelFolder;
};

#endif // SYNCDIALOG_H_INCLUDED
