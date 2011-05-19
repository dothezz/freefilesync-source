// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef FOLDERPAIR_H_INCLUDED
#define FOLDERPAIR_H_INCLUDED

#include "../structures.h"
#include "../shared/dir_name.h"
#include "../library/resources.h"
#include "small_dlgs.h"
#include "sync_cfg.h"
#include <wx/event.h>
#include <wx/menu.h>
#include "../shared/util.h"
#include "../shared/string_conv.h"
#include "../library/norm_filter.h"

namespace zen
{
//basic functionality for handling alternate folder pair configuration: change sync-cfg/filter cfg, right-click context menu, button icons...

template <class GuiPanel>
class FolderPairPanelBasic : private wxEvtHandler
{
public:
    typedef boost::shared_ptr<const zen::AlternateSyncConfig> AltSyncCfgPtr;

    AltSyncCfgPtr getAltSyncConfig() const
    {
        return altSyncConfig;
    }

    FilterConfig getAltFilterConfig() const
    {
        return localFilter;
    }

    void setConfig(AltSyncCfgPtr syncCfg, const FilterConfig& filter)
    {
        altSyncConfig = syncCfg;
        localFilter   = filter;
        refreshButtons();
    }


    void refreshButtons()
    {
        if (altSyncConfig.get())
        {
            basicPanel_.m_bpButtonAltSyncCfg->SetBitmapLabel(GlobalResources::instance().getImage(wxT("syncConfigSmall")));
            basicPanel_.m_bpButtonAltSyncCfg->SetToolTip(wxString(_("Select alternate synchronization settings")) +  wxT(" ") + common::LINE_BREAK +
                                                         wxT("(") + getVariantName(altSyncConfig->syncConfiguration.var) + wxT(")"));
        }
        else
        {
            basicPanel_.m_bpButtonAltSyncCfg->SetBitmapLabel(GlobalResources::instance().getImage(wxT("syncConfigSmallGrey")));
            basicPanel_.m_bpButtonAltSyncCfg->SetToolTip(_("Select alternate synchronization settings"));
        }

        //test for Null-filter
        if (isNullFilter(localFilter))
        {
            basicPanel_.m_bpButtonLocalFilter->SetBitmapLabel(GlobalResources::instance().getImage(wxT("filterSmallGrey")));
            basicPanel_.m_bpButtonLocalFilter->SetToolTip(_("No filter selected"));
        }
        else
        {
            basicPanel_.m_bpButtonLocalFilter->SetBitmapLabel(GlobalResources::instance().getImage(wxT("filterSmall")));
            basicPanel_.m_bpButtonLocalFilter->SetToolTip(_("Filter is active"));
        }
    }

protected:
    FolderPairPanelBasic(GuiPanel& basicPanel) : //takes reference on basic panel to be enhanced
        basicPanel_(basicPanel)
    {
        //register events for removal of alternate configuration
        basicPanel_.m_bpButtonAltSyncCfg->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(FolderPairPanelBasic::OnAltSyncCfgRemove), NULL, this);
        basicPanel_.m_bpButtonLocalFilter->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(FolderPairPanelBasic::OnLocalFilterCfgRemove), NULL, this);

        basicPanel_.m_bpButtonAltSyncCfg-> Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderPairPanelBasic::OnAltSyncCfg), NULL, this);
        basicPanel_.m_bpButtonLocalFilter->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderPairPanelBasic::OnLocalFilterCfg), NULL, this);

        basicPanel_.m_bpButtonRemovePair->SetBitmapLabel(GlobalResources::instance().getImage(wxT("removeFolderPair")));
    }

    virtual void OnLocalFilterCfgRemoveConfirm(wxCommandEvent& event)
    {
        localFilter = FilterConfig();
        refreshButtons();
    }

    virtual void OnAltSyncCfgRemoveConfirm(wxCommandEvent& event)
    {
        altSyncConfig.reset();
        refreshButtons();
    }


private:
    void OnLocalFilterCfgRemove(wxCommandEvent& event)
    {
        const int menuId = 1234;
        contextMenu.reset(new wxMenu); //re-create context menu
        contextMenu->Append(menuId, _("Clear filter settings"));
        contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(FolderPairPanelBasic::OnLocalFilterCfgRemoveConfirm), NULL, this);

        if (isNullFilter(localFilter))
            contextMenu->Enable(menuId, false); //disable menu item, if clicking wouldn't make sense anyway

        basicPanel_.PopupMenu(contextMenu.get()); //show context menu
    }

    void OnAltSyncCfgRemove(wxCommandEvent& event)
    {
        const int menuId = 1234;
        contextMenu.reset(new wxMenu); //re-create context menu
        contextMenu->Append(menuId, _("Remove alternate settings"));
        contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(FolderPairPanelBasic::OnAltSyncCfgRemoveConfirm), NULL, this);

        if (!altSyncConfig.get())
            contextMenu->Enable(menuId, false); //disable menu item, if clicking wouldn't make sense anyway

        basicPanel_.PopupMenu(contextMenu.get()); //show context menu
    }

    virtual MainConfiguration getMainConfig() const = 0;
    virtual wxWindow* getParentWindow() = 0;

    virtual void OnAltSyncCfgChange() {};

    void OnAltSyncCfg(wxCommandEvent& event)
    {
        const MainConfiguration mainCfg = getMainConfig();
        const AlternateSyncConfig syncConfigMain(mainCfg.syncConfiguration,
                                                 mainCfg.handleDeletion,
                                                 mainCfg.customDeletionDirectory);

        AlternateSyncConfig altSyncCfg = altSyncConfig.get() ? *altSyncConfig : syncConfigMain;


        if (showSyncConfigDlg(mainCfg.compareVar,
                              altSyncCfg.syncConfiguration,
                              altSyncCfg.handleDeletion,
                              altSyncCfg.customDeletionDirectory,
                              NULL) == ReturnSyncConfig::BUTTON_OKAY) //optional input parameter
        {
            altSyncConfig.reset(new AlternateSyncConfig(altSyncCfg));
            refreshButtons();

            OnAltSyncCfgChange();
        }
    }

    virtual void OnLocalFilterCfgChange() {};

    void OnLocalFilterCfg(wxCommandEvent& event)
    {
        FilterConfig localFiltTmp = localFilter;

        if (showFilterDialog(false, //is local filter dialog
                             localFiltTmp) == ReturnSmallDlg::BUTTON_OKAY)
        {
            localFilter = localFiltTmp;
            refreshButtons();

            OnLocalFilterCfgChange();
        }
    }

    GuiPanel& basicPanel_; //panel to be enhanced by this template

    //alternate configuration attached to it
    AltSyncCfgPtr altSyncConfig; //optional: present if non-NULL
    FilterConfig  localFilter;

    std::auto_ptr<wxMenu> contextMenu;
};
}


#endif // FOLDERPAIR_H_INCLUDED


