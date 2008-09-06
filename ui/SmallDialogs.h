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

    static const int OkayButtonPressed = 25;

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

    static const int OkayButtonPressed   = 35;
    static const int CancelButtonPressed = 45;

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

    static const int ContinueButtonPressed = 35;
    static const int RetryButtonPressed    = 45;
    static const int AbortButtonPressed    = 55;

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
    SyncStatus(StatusUpdater* updater, double gaugeTotalElements, wxWindow* parentWindow = 0);
    ~SyncStatus();

    void resetGauge(double totalNrOfElements);
    void incProgressIndicator_NoUpdate(double number);
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
    double totalElements;     //each element represents one byte for proper progress indicator scaling
    double currentElements;
    double scalingFactor;   //nr of elements has to be normalized to smaller nr. because of range of int limitation

    wxString currentStatusText;
    unsigned int numberOfProcessedObjects;
};


#endif // SMALLDIALOGS_H_INCLUDED
