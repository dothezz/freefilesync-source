// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef BATCHCONFIG_H_INCLUDED
#define BATCHCONFIG_H_INCLUDED

#include "gui_generated.h"
#include "../library/process_xml.h"


namespace ffs3
{
class DragDropOnDlg;
}

class BatchFolderPairPanel;
class FirstBatchFolderPairCfg;


class BatchDialog: public BatchDlgGenerated
{
    friend class BatchFileDropEvent;
    template <class GuiPanel>
    friend class FolderPairCallback;

public:
    BatchDialog(wxWindow* window, const xmlAccess::XmlBatchConfig& batchCfg);
    BatchDialog(wxWindow* window, const wxString& filename);
    ~BatchDialog();

    enum
    {
        BATCH_FILE_SAVED = 15
    };

private:
    void init();

    virtual void OnCmpSettings(        wxCommandEvent& event);
    virtual void OnSyncSettings(       wxCommandEvent& event);
    virtual void OnConfigureFilter(    wxCommandEvent& event);

    virtual void OnHelp(               wxCommandEvent& event);

    void OnGlobalFilterOpenContext(wxCommandEvent& event);
    void OnGlobalFilterRemConfirm(wxCommandEvent& event);
    virtual void OnCheckSilent(        wxCommandEvent& event);
    virtual void OnClose(              wxCloseEvent&   event);
    virtual void OnCancel(             wxCommandEvent& event);
    virtual void OnSaveBatchJob(       wxCommandEvent& event);
    virtual void OnLoadBatchJob(       wxCommandEvent& event);
    virtual void OnAddFolderPair(      wxCommandEvent& event);
    virtual void OnRemoveFolderPair(   wxCommandEvent& event);
    virtual void OnRemoveTopFolderPair(wxCommandEvent& event);

    void addFolderPair(const std::vector<ffs3::FolderPairEnh>& newPairs, bool addFront = false);
    void removeAddFolderPair(const int pos);
    void clearAddFolderPairs();

    void updateGuiForFolderPair();

    void updateGui(); //re-evaluate gui after config changes

    void updateVisibleTabs();
    void showNotebookpage(wxWindow* page, const wxString& pageName, bool show);

    //error handling
    xmlAccess::OnError getSelectionHandleError() const;
    void setSelectionHandleError(const xmlAccess::OnError value);
    void OnChangeErrorHandling(wxCommandEvent& event);
    void updateToolTipErrorHandling(const xmlAccess::OnError value);

    bool saveBatchFile(const wxString& filename);
    void loadBatchFile(const wxString& filename);
    void loadBatchCfg(const xmlAccess::XmlBatchConfig& batchCfg);

    xmlAccess::XmlBatchConfig getCurrentConfiguration() const;

    boost::shared_ptr<FirstBatchFolderPairCfg> firstFolderPair; //always bound!!!
    std::vector<BatchFolderPairPanel*> additionalFolderPairs;

    //used when saving batch file
    wxString proposedBatchFileName;

    xmlAccess::XmlBatchConfig localBatchCfg;

    std::auto_ptr<wxMenu> contextMenu;

    //add drag & drop support when selecting logfile directory
    std::auto_ptr<ffs3::DragDropOnDlg> dragDropOnLogfileDir;
};

#endif // BATCHCONFIG_H_INCLUDED
