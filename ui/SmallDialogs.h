#ifndef SMALLDIALOGS_H_INCLUDED
#define SMALLDIALOGS_H_INCLUDED

#include "../FreeFileSync.h"
#include "guiGenerated.h"
#include <wx/stopwatch.h>

class AboutDlg : public AboutDlgGenerated
{
public:
    AboutDlg(wxWindow* window);
    ~AboutDlg();

private:
    void OnClose(wxCloseEvent& event);
    void OnOK(wxCommandEvent& event);
};


class HelpDlg : public HelpDlgGenerated
{
public:
    HelpDlg(wxWindow* window);
    ~HelpDlg();

private:
    void OnClose(wxCloseEvent& event);
    void OnOK(wxCommandEvent& event);
};


class FilterDlg : public FilterDlgGenerated
{
public:
    FilterDlg(wxWindow* window, wxString& filterIncl, wxString& filterExcl);
    ~FilterDlg();

    static const int okayButtonPressed = 25;

private:
    void OnHelp(wxCommandEvent& event);
    void OnDefault(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    wxString& includeFilter;
    wxString& excludeFilter;
};


class DeleteDialog : public DeleteDlgGenerated
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
    ErrorDlg(const wxString messageText, bool& continueError);
    ~ErrorDlg();

    static const int continueButtonPressed = 35;
    static const int retryButtonPressed    = 45;
    static const int abortButtonPressed    = 55;

private:
    void OnClose(wxCloseEvent& event);
    void OnContinue(wxCommandEvent& event);
    void OnRetry(wxCommandEvent& event);
    void OnAbort(wxCommandEvent& event);

    bool& continueOnError;
};


class SyncStatus : public SyncStatusDlgGenerated
{
public:
    SyncStatus(StatusUpdater* updater, wxWindow* parentWindow = NULL);
    ~SyncStatus();

    enum SyncStatusID
    {
        ABORTED,
        FINISHED_WITH_SUCCESS,
        FINISHED_WITH_ERROR,
        PAUSE,
        SCANNING,
        COMPARING,
        SYNCHRONIZING
    };

    void resetGauge(int totalObjectsToProcess, double totalDataToProcess);
    void incProgressIndicator_NoUpdate(int objectsProcessed, double dataProcessed);
    void setStatusText_NoUpdate(const wxString& text);
    void updateStatusDialogNow();

    void setCurrentStatus(SyncStatusID id);
    void processHasFinished(SyncStatusID id);  //essential to call this in StatusUpdater derived class destructor at the LATEST(!) to prevent access to currentStatusUpdater

private:
    void OnOkay(wxCommandEvent& event);
    void OnPause(wxCommandEvent& event);
    void OnAbort(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    wxStopWatch timeElapsed;

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
    bool processPaused;
};

/*
class RemainingTime
{
public:
    RemainingTime();
    ~RemainingTime();
    wxLongLong getRemainingTime(double processedDataSinceLastCall, int remainingFiles, double remainingData); //returns the remaining time in milliseconds

private:
    double n;
    double m;
    double X;
    double F;
    double p;
    double q;
    double r;
    double s;
    double z_1;
    double z_2;
    wxLongLong lastExec;

    vector<double> x; //dummy: DELETE asap!
    vector<double> f;
};
*/

class CompareStatus : public CompareStatusGenerated
{
public:
    CompareStatus(wxWindow* parentWindow);
    ~CompareStatus();

    void init(); //initialize all status values

    void switchToCompareBytewise(int totalCmpObjectsToProcess, double totalCmpDataToProcess);
    void incScannedObjects_NoUpdate(int number);
    void incProcessedCmpData_NoUpdate(int objectsProcessed, double dataProcessed);
    void setStatusText_NoUpdate(const wxString& text);
    void updateStatusPanelNow();

private:
    //status variables
    unsigned int scannedObjects;
    wxString currentStatusText;

    wxStopWatch timeElapsed;

    //gauge variables
    double scalingFactorCmp;  //nr of elements has to be normalized to smaller nr. because of range of int limitation
    double totalCmpData;     //each data element represents one byte for proper progress indicator scaling
    double processedCmpData;
    int totalCmpObjects;
    int processedCmpObjects;    //each object represents a file or directory processed
    /*
        //remaining time
        RemainingTime calcTimeLeft;
        wxLongLong timeRemaining;          //time in milliseconds
        wxLongLong timeRemainingTimeStamp; //time in milliseconds
    */
};


#endif // SMALLDIALOGS_H_INCLUDED
