#ifndef SMALLDIALOGS_H_INCLUDED
#define SMALLDIALOGS_H_INCLUDED

#include "MainDialog.h"

class MainDialog;

class AboutDlg: public AboutDlgGenerated
{
public:
    AboutDlg(MainDialog* window);
    ~AboutDlg();

private:
    void OnClose(wxCloseEvent& event);
    void AboutDlg::OnOK(wxCommandEvent& event);

    MainDialog* mainDialog;
};


class HelpDlg: public HelpDlgGenerated
{
public:
    HelpDlg(MainDialog* window);
    ~HelpDlg();

private:
    void OnClose(wxCloseEvent& event);
    void OnOK(wxCommandEvent& event);
};


class FilterDlg: public FilterDlgGenerated
{
public:
    FilterDlg(MainDialog* window);
    ~FilterDlg();

private:
    void OnDefault(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    MainDialog* mainDialog;
};


class SyncErrorDlg: public SyncErrorDlgGenerated
{
public:
    SyncErrorDlg(MainDialog* window, wxString messageText, bool& suppressErrormessages);
    ~SyncErrorDlg();

private:
    void OnClose(wxCloseEvent& event);
    void OnContinue(wxCommandEvent& event);
    void OnAbort(wxCommandEvent& event);

    MainDialog* mainDialog;
    bool& suppressErrors;
};


#endif // SMALLDIALOGS_H_INCLUDED
