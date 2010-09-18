// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef REALTIMESYNCMAIN_H
#define REALTIMESYNCMAIN_H

#include "gui_generated.h"
#include <vector>
#include <memory>
#include "../shared/dir_name.h"

namespace xmlAccess
{
struct XmlRealConfig;
}


class DirectoryPanel : public FolderGenerated
{
public:
    DirectoryPanel(wxWindow* parent) :
        FolderGenerated(parent),
        dirName(this, m_dirPicker, m_txtCtrlDirectory) {}

    void setName(const Zstring& dirname)
    {
        dirName.setName(dirname);
    }

    Zstring getName() const
    {
        return dirName.getName();
    }

private:
    ffs3::DirectoryName dirName;
};



class MainDialog: public MainDlgGenerated
{
public:
    MainDialog(wxDialog *dlg, const wxString& cfgFilename);
    ~MainDialog();

    void loadConfig(const wxString& filename);

private:
    virtual void OnClose(           wxCloseEvent& event);
    virtual void OnQuit(            wxCommandEvent& event);
    virtual void OnShowHelp(        wxCommandEvent& event);
    virtual void OnMenuAbout(       wxCommandEvent& event);
    virtual void OnAddFolder(       wxCommandEvent& event);
    virtual void OnRemoveFolder(    wxCommandEvent& event);
    virtual void OnRemoveTopFolder( wxCommandEvent& event);
    virtual void OnKeyPressed(      wxKeyEvent& event);
    virtual void OnStart(           wxCommandEvent& event);
    virtual void OnSaveConfig(      wxCommandEvent& event);
    virtual void OnLoadConfig(      wxCommandEvent& event);

    void setConfiguration(const xmlAccess::XmlRealConfig& cfg);
    xmlAccess::XmlRealConfig getConfiguration();

    void layoutAsync(); //call Layout() asynchronously

    void addFolder(const wxString& dirname, bool addFront = false);
    void addFolder(const std::vector<wxString>& newFolders, bool addFront = false);
    void removeAddFolder(const int pos); //keep it an int, allow negative values!
    void clearAddFolders();

    static const wxString& lastConfigFileName();

    std::auto_ptr<ffs3::DirectoryName> dirNameFirst;
    std::vector<DirectoryPanel*> dirNamesExtra; //additional pairs to the standard pair
};

#endif // REALTIMESYNCMAIN_H
