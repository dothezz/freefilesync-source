#ifndef SMALLDIALOGS_H_INCLUDED
#define SMALLDIALOGS_H_INCLUDED

#include "mainDialog.h"

class MainDialog;

class AboutDlg : public AboutDlgGenerated
{
public:
    AboutDlg(MainDialog* window);
    ~AboutDlg();

private:
    void OnClose(wxCloseEvent& event);
    void OnOK(wxCommandEvent& event);
};


class HelpDlg : public HelpDlgGenerated
{
public:
    HelpDlg(MainDialog* window);
    ~HelpDlg();

private:
    void OnClose(wxCloseEvent& event);
    void OnOK(wxCommandEvent& event);
};


class FilterDlg : public FilterDlgGenerated
{
public:
    FilterDlg(MainDialog* window);
    ~FilterDlg();

    static const int okayButtonPressed = 25;

private:
    void OnDefault(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    MainDialog* mainDialog;
};


class DeleteDialog : public DeleteDialogGenerated
{
public:
    DeleteDialog(const wxString& headerText, const wxString& messageText, wxWindow* main);
    ~DeleteDialog();

    static const int okayButtonPressed   = 35;
    static const int cancelButtonPressed = 45;

private:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
};


class ErrorDlg : public ErrorDlgGenerated
{
public:
    ErrorDlg(const wxString messageText, bool& suppressErrormessages);
    ~ErrorDlg();

    static const int continueButtonPressed = 35;
    static const int retryButtonPressed    = 45;
    static const int abortButtonPressed    = 55;

private:
    void OnClose(wxCloseEvent& event);
    void OnContinue(wxCommandEvent& event);
    void OnRetry(wxCommandEvent& event);
    void OnAbort(wxCommandEvent& event);

    bool& suppressErrors;
};


class SyncStatus : public SyncStatusGenerated
{
public:
    SyncStatus(StatusUpdater* updater, wxWindow* parentWindow = 0);
    ~SyncStatus();

    void resetGauge(int totalObjectsToProcess, double totalDataToProcess);
    void incProgressIndicator_NoUpdate(int objectsProcessed, double dataProcessed);
    void setStatusText_NoUpdate(const wxString& text);
    void updateStatusDialogNow();

    void processHasFinished(const wxString& finalStatusText);  //essential to call this in StatusUpdater derived class destructor at the LATEST(!) to prevent access to currentStatusUpdater

private:
    void OnOkay(wxCommandEvent& event);
    void OnAbort(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    StatusUpdater* currentStatusUpdater;
    wxWindow* windowToDis;
    bool currentProcessIsRunning;

    //gauge variables
    double totalData;     //each data element represents one byte for proper progress indicator scaling
    double currentData;
    double scalingFactor;  //nr of elements has to be normalized to smaller nr. because of range of int limitation
    int currentObjects;    //each object represents a file or directory processed
    int totalObjects;

    wxString currentStatusText;
};


class CompareStatus : public CompareStatusGenerated
{
public:
    CompareStatus(wxWindow* parentWindow);
    ~CompareStatus();

    void resetMD5Gauge(int totalMD5ObjectsToProcess, double totalMD5DataToProcess);
    void incScannedFiles_NoUpdate(int number);
    void incProcessedMD5Data_NoUpdate(int objectsProcessed, double dataProcessed);
    void setStatusText_NoUpdate(const wxString& text);
    void updateStatusPanelNow();

private:
    //status variables
    unsigned int scannedFiles;
    wxString currentStatusText;

    //gauge variables
    double totalMD5Data;     //each data element represents one byte for proper progress indicator scaling
    double currentMD5Data;
    double scalingFactorMD5;  //nr of elements has to be normalized to smaller nr. because of range of int limitation
    int currentMD5Objects;    //each object represents a file or directory processed
    int totalMD5Objects;
};


#endif // SMALLDIALOGS_H_INCLUDED
