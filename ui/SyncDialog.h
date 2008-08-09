#ifndef SYNCDIALOG_H_INCLUDED
#define SYNCDIALOG_H_INCLUDED

#include "GUI_Generated.h"
#include "MainDialog.h"
#include "..\FreeFileSync.h"

class MainDialog;

class SyncDialog: public SyncDialogGenerated
{
public:
    SyncDialog(MainDialog* window);
    ~SyncDialog();

private:
    void updateConfigIcons();

    void OnSyncLeftToRight( wxCommandEvent& event );
    void OnSyncBothSides( wxCommandEvent& event );
    void OnSyncCostum( wxCommandEvent& event );

    void OnExLeftSideOnly( wxCommandEvent& event );
    void OnExRightSideOnly( wxCommandEvent& event );
    void OnLeftNewer( wxCommandEvent& event );
    void OnRightNewer( wxCommandEvent& event );
    void OnDifferent( wxCommandEvent& event );

    void OnStartSync(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnBack(wxCommandEvent& event);

    MainDialog* mainDialog;
};

#endif // SYNCDIALOG_H_INCLUDED
