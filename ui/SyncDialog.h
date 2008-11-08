#ifndef SYNCDIALOG_H_INCLUDED
#define SYNCDIALOG_H_INCLUDED

#include "../FreeFileSync.h"
#include "guiGenerated.h"

class SyncDialog: public SyncDlgGenerated
{
public:
    SyncDialog(wxWindow* window,
               const FileCompareResult& gridDataRef,
               Configuration& config,
               bool synchronizationEnabled);

    ~SyncDialog();

    static const int StartSynchronizationProcess = 15;

    static void updateConfigIcons(wxBitmapButton* button1,
                                  wxBitmapButton* button2,
                                  wxBitmapButton* button3,
                                  wxBitmapButton* button4,
                                  wxBitmapButton* button5,
                                  const SyncConfiguration& syncConfig);

    static void adjustToolTips(wxStaticBitmap* bitmap, const CompareVariant var);

private:
    void calculatePreview();

    void OnSyncLeftToRight( wxCommandEvent& event);
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
    SyncConfiguration localSyncConfiguration;
    const FileCompareResult& gridData;
    Configuration& cfg;
};


class BatchDialog: public BatchDlgGenerated
{
public:
    BatchDialog(wxWindow* window,
                const Configuration& config,
                const wxString& leftDir,
                const wxString& rightDir);

    ~BatchDialog();

    static const int batchFileCreated = 15;

private:
    void OnExLeftSideOnly(  wxCommandEvent& event);
    void OnExRightSideOnly( wxCommandEvent& event);
    void OnLeftNewer(       wxCommandEvent& event);
    void OnRightNewer(      wxCommandEvent& event);
    void OnDifferent(       wxCommandEvent& event);

    void OnFilterButton(    wxCommandEvent& event);
    void OnSelectRecycleBin(wxCommandEvent& event);
    void OnChangeCompareVar(wxCommandEvent& event);

    void OnClose(           wxCloseEvent&   event);
    void OnCancel(          wxCommandEvent& event);
    void OnCreateBatchJob(  wxCommandEvent& event);

    void updateFilterButton();
    wxString getCommandlineArguments();
    bool createBatchFile(const wxString& filename);

    SyncConfiguration localSyncConfiguration;
    bool filterIsActive;
};

#endif // SYNCDIALOG_H_INCLUDED
