// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef SYNCCONFIG_H_INCLUDED
#define SYNCCONFIG_H_INCLUDED

#include <memory>
#include "guiGenerated.h"
#include "../structures.h"


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
    FreeFileSync::SyncConfiguration currentSyncConfig;

    //changing data
    FreeFileSync::SyncConfiguration& refSyncConfiguration;
    FreeFileSync::DeletionPolicy& refHandleDeletion;
    wxString& refCustomDeletionDirectory;
    bool* refIgnoreErrors;

    std::auto_ptr<FreeFileSync::DragDropOnDlg> dragDropCustomDelFolder;
};

#endif // SYNCCONFIG_H_INCLUDED
