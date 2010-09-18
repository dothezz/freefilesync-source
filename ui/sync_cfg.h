// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef SYNCCONFIG_H_INCLUDED
#define SYNCCONFIG_H_INCLUDED

#include <memory>
#include "gui_generated.h"
#include "../structures.h"


namespace ffs3
{
class DirectoryName;
}


class SyncCfgDialog : public SyncCfgDlgGenerated
{
public:
    SyncCfgDialog(wxWindow* window,
                  const ffs3::CompareVariant compareVar,
                  ffs3::SyncConfiguration& syncConfiguration,
                  ffs3::DeletionPolicy& handleDeletion,
                  wxString& customDeletionDirectory,
                  bool* ignoreErrors); //optional input parameter

    ~SyncCfgDialog();

    enum
    {
        BUTTON_APPLY = 10
    };

    static void updateConfigIcons(const ffs3::CompareVariant compareVar,
                                  const ffs3::SyncConfiguration& syncConfig,
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
    void updateConfigIcons(const ffs3::CompareVariant cmpVar, const ffs3::SyncConfiguration& syncConfig);

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
    ffs3::DeletionPolicy getDeletionHandling();
    void setDeletionHandling(ffs3::DeletionPolicy newValue);
    void OnChangeDeletionHandling(wxCommandEvent& event);

    const ffs3::CompareVariant cmpVariant;

    //temporal copy of maindialog.cfg.syncConfiguration
    ffs3::SyncConfiguration currentSyncConfig;

    //changing data
    ffs3::SyncConfiguration& refSyncConfiguration;
    ffs3::DeletionPolicy& refHandleDeletion;
    wxString& refCustomDeletionDirectory;
    bool* refIgnoreErrors;

    std::auto_ptr<ffs3::DirectoryName> customDelFolder;
};

#endif // SYNCCONFIG_H_INCLUDED
