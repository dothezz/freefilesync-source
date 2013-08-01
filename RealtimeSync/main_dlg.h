// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef REALTIMESYNCMAIN_H
#define REALTIMESYNCMAIN_H

#include "gui_generated.h"
#include <vector>
#include <memory>
#include <zen/zstring.h>
#include <zen/async_task.h>
#include <wx+/file_drop.h>
#include "../ui/dir_name.h"

namespace xmlAccess
{
struct XmlRealConfig;
}
class DirectoryPanel;


class MainDialog: public MainDlgGenerated
{
public:
    MainDialog(wxDialog* dlg, const Zstring& cfgFileName);
    ~MainDialog();

private:
    void loadConfig(const Zstring& filename);

    virtual void OnClose          (wxCloseEvent& event)   { Destroy(); }
    virtual void OnShowHelp       (wxCommandEvent& event);
    virtual void OnMenuAbout      (wxCommandEvent& event);
    virtual void OnAddFolder      (wxCommandEvent& event);
    virtual void OnRemoveFolder   (wxCommandEvent& event);
    virtual void OnRemoveTopFolder(wxCommandEvent& event);
    virtual void OnKeyPressed     (wxKeyEvent& event);
    virtual void OnStart          (wxCommandEvent& event);
    virtual void OnConfigSave     (wxCommandEvent& event);
    virtual void OnConfigLoad     (wxCommandEvent& event);
    virtual void OnMenuQuit       (wxCommandEvent& event) { Close(); }
    void onFilesDropped(zen::FileDropEvent& event);

    void setConfiguration(const xmlAccess::XmlRealConfig& cfg);
    xmlAccess::XmlRealConfig getConfiguration();
    void setLastUsedConfig(const Zstring& filename);

    void layoutAsync(); //call Layout() asynchronously

    //void addFolder(const Zstring& dirname, bool addFront = false);
    void addFolder(const std::vector<Zstring>& newFolders, bool addFront = false);
    void removeAddFolder(size_t pos);
    void clearAddFolders();

    static const Zstring& lastConfigFileName();

    std::unique_ptr<zen::DirectoryName<wxTextCtrl>> dirNameFirst;
    std::vector<DirectoryPanel*> dirNamesExtra; //additional pairs to the standard pair

    Zstring currentConfigFileName;

    void onProcessAsyncTasks(wxEvent& event);

    template <class Fun, class Fun2>
    void processAsync(Fun doAsync, Fun2 evalOnGui) { asyncTasks.add(doAsync, evalOnGui); timerForAsyncTasks.Start(50); /*timer interval in [ms] */ }
    template <class Fun, class Fun2>
    void processAsync2(Fun doAsync, Fun2 evalOnGui) { asyncTasks.add2(doAsync, evalOnGui); timerForAsyncTasks.Start(50); /*timer interval in [ms] */ }

    //schedule and run long-running tasks asynchronously, but process results on GUI queue
    zen::AsyncTasks asyncTasks;
    wxTimer timerForAsyncTasks; //don't use wxWidgets idle handling => repeated idle requests/consumption hogs 100% cpu!
};

#endif // REALTIMESYNCMAIN_H
