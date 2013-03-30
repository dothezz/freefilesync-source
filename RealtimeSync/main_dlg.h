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
    MainDialog(wxDialog* dlg, const wxString& cfgFileName);
    ~MainDialog();

private:
    void loadConfig(const wxString& filename);

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
    void setLastUsedConfig(const wxString& filename);

    void layoutAsync(); //call Layout() asynchronously

    void addFolder(const wxString& dirname, bool addFront = false);
    void addFolder(const std::vector<wxString>& newFolders, bool addFront = false);
    void removeAddFolder(int pos); //keep it an int, allow negative values!
    void clearAddFolders();

    static const wxString& lastConfigFileName();

    std::unique_ptr<zen::DirectoryName<wxTextCtrl>> dirNameFirst;
    std::vector<DirectoryPanel*> dirNamesExtra; //additional pairs to the standard pair

    wxString currentConfigFileName;
};

#endif // REALTIMESYNCMAIN_H
