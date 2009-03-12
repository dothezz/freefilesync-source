#ifndef SMALLDIALOGS_H_INCLUDED
#define SMALLDIALOGS_H_INCLUDED

#include "../FreeFileSync.h"
#include "../library/statusHandler.h"
#include "../library/processXml.h"
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

    enum
    {
        BUTTON_OKAY
    };

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
    DeleteDialog(wxWindow* main,
                 const FileCompareResult& grid,
                 const std::set<int>& rowsOnLeft,
                 const std::set<int>& rowsOnRight,
                 bool& deleteOnBothSides,
                 bool& useRecycleBin);

    ~DeleteDialog() {}

    enum
    {
        BUTTON_OKAY,
        BUTTON_CANCEL
    };

private:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnDelOnBothSides(wxCommandEvent& event);
    void OnUseRecycler(wxCommandEvent& event);

    void updateTexts();

    const FileCompareResult& mainGrid;
    const std::set<int>& rowsToDeleteOnLeft;
    const std::set<int>& rowsToDeleteOnRight;
    bool& m_deleteOnBothSides;
    bool& m_useRecycleBin;
};


class ErrorDlg : public ErrorDlgGenerated
{
public:
    ErrorDlg(wxWindow* parentWindow, const int activeButtons, const wxString messageText, bool& ignoreNextErrors);
    ~ErrorDlg();

    enum
    {
        BUTTON_IGNORE = 1,
        BUTTON_RETRY  = 2,
        BUTTON_ABORT  = 4
    };

private:
    void OnClose(wxCloseEvent& event);
    void OnIgnore(wxCommandEvent& event);
    void OnRetry(wxCommandEvent& event);
    void OnAbort(wxCommandEvent& event);

    bool& ignoreErrors;
};


class WarningDlg : public WarningDlgGenerated
{
public:
    WarningDlg(wxWindow* parentWindow, int activeButtons, const wxString messageText, bool& dontShowAgain);
    ~WarningDlg();

    enum
    {
        BUTTON_IGNORE  = 1,
        BUTTON_ABORT   = 2
    };

private:
    void OnClose(wxCloseEvent& event);
    void OnIgnore(wxCommandEvent& event);
    void OnResolve(wxCommandEvent& event);
    void OnAbort(wxCommandEvent& event);
    void OnOkay(wxCommandEvent& event);

    bool& dontShowAgain;
};


class CustomizeColsDlg : public CustomizeColsDlgGenerated
{
public:
    CustomizeColsDlg(wxWindow* window, xmlAccess::ColumnAttributes& attr);
    ~CustomizeColsDlg() {}

    enum
    {
        BUTTON_OKAY = 10
    };

private:
    void OnOkay(wxCommandEvent& event);
    void OnDefault(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    void OnMoveUp(wxCommandEvent& event);
    void OnMoveDown(wxCommandEvent& event);

    xmlAccess::ColumnAttributes& output;
};


class GlobalSettingsDlg : public GlobalSettingsDlgGenerated
{
public:
    GlobalSettingsDlg(wxWindow* window, xmlAccess::XmlGlobalSettings& globalSettings);
    ~GlobalSettingsDlg() {}

    enum
    {
        BUTTON_OKAY = 10
    };

private:
    void OnOkay(wxCommandEvent& event);
    void OnResetWarnings(wxCommandEvent& event);
    void OnDefault(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    xmlAccess::XmlGlobalSettings& settings;
};


class SyncStatus : public SyncStatusDlgGenerated
{
public:
    SyncStatus(StatusHandler* updater, wxWindow* parentWindow = NULL);
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
    void setStatusText_NoUpdate(const Zstring& text);
    void updateStatusDialogNow();

    void setCurrentStatus(SyncStatusID id);
    void processHasFinished(SyncStatusID id);  //essential to call this in StatusUpdater derived class destructor at the LATEST(!) to prevent access to currentStatusUpdater

private:
    void OnOkay(wxCommandEvent& event);
    void OnPause(wxCommandEvent& event);
    void OnAbort(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    wxStopWatch timeElapsed;

    StatusHandler* currentStatusHandler;
    wxWindow* windowToDis;
    bool currentProcessIsRunning;

    //gauge variables
    double totalData;     //each data element represents one byte for proper progress indicator scaling
    double currentData;
    double scalingFactor;  //nr of elements has to be normalized to smaller nr. because of range of int limitation
    int currentObjects;    //each object represents a file or directory processed
    int totalObjects;

    Zstring currentStatusText;
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
    void setStatusText_NoUpdate(const Zstring& text);
    void updateStatusPanelNow();

private:
    //status variables
    unsigned int scannedObjects;
    Zstring currentStatusText;

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
