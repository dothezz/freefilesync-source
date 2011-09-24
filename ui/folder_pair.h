// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

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
#include "../shared/custom_button.h"

namespace zen
{
//basic functionality for handling alternate folder pair configuration: change sync-cfg/filter cfg, right-click context menu, button icons...

template <class GuiPanel>
class FolderPairPanelBasic : private wxEvtHandler
{
public:
    typedef std::shared_ptr<const CompConfig> AltCompCfgPtr;
    typedef std::shared_ptr<const SyncConfig> AltSyncCfgPtr;

    AltCompCfgPtr getAltCompConfig() const { return altCompConfig; }
    AltSyncCfgPtr getAltSyncConfig() const { return altSyncConfig; }
    FilterConfig getAltFilterConfig() const { return localFilter; }

    void setConfig(AltCompCfgPtr compConfig, AltSyncCfgPtr syncCfg, const FilterConfig& filter)
    {
        altCompConfig = compConfig;
        altSyncConfig = syncCfg;
        localFilter   = filter;
        refreshButtons();
    }

    void refreshButtons()
    {
        if (altCompConfig.get())
        {
            setBitmapLabel(*basicPanel_.m_bpButtonAltCompCfg, GlobalResources::getImage(wxT("cmpConfigSmall")));
            basicPanel_.m_bpButtonAltCompCfg->SetToolTip(wxString(_("Select alternate comparison settings")) +  wxT(" \n") +
                                                         wxT("(") + getVariantName(altCompConfig->compareVar) + wxT(")"));
        }
        else
        {
            setBitmapLabel(*basicPanel_.m_bpButtonAltCompCfg, GlobalResources::getImage(wxT("cmpConfigSmallGrey")));
            basicPanel_.m_bpButtonAltCompCfg->SetToolTip(_("Select alternate comparison settings"));
        }

        if (altSyncConfig.get())
        {
            setBitmapLabel(*basicPanel_.m_bpButtonAltSyncCfg, GlobalResources::getImage(wxT("syncConfigSmall")));
            basicPanel_.m_bpButtonAltSyncCfg->SetToolTip(wxString(_("Select alternate synchronization settings")) +  wxT(" \n") +
                                                         wxT("(") + getVariantName(altSyncConfig->directionCfg.var) + wxT(")"));
        }
        else
        {
            setBitmapLabel(*basicPanel_.m_bpButtonAltSyncCfg, GlobalResources::getImage(wxT("syncConfigSmallGrey")));
            basicPanel_.m_bpButtonAltSyncCfg->SetToolTip(_("Select alternate synchronization settings"));
        }

        //test for Null-filter
        if (isNullFilter(localFilter))
        {
            setBitmapLabel(*basicPanel_.m_bpButtonLocalFilter, GlobalResources::getImage(wxT("filterSmallGrey")));
            basicPanel_.m_bpButtonLocalFilter->SetToolTip(_("No filter selected"));
        }
        else
        {
            setBitmapLabel(*basicPanel_.m_bpButtonLocalFilter, GlobalResources::getImage(wxT("filterSmall")));
            basicPanel_.m_bpButtonLocalFilter->SetToolTip(_("Filter is active"));
        }
    }

protected:
    FolderPairPanelBasic(GuiPanel& basicPanel) : //takes reference on basic panel to be enhanced
        basicPanel_(basicPanel)
    {
        //register events for removal of alternate configuration
        basicPanel_.m_bpButtonAltCompCfg ->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(FolderPairPanelBasic::OnAltCompCfgRemove    ), NULL, this);
        basicPanel_.m_bpButtonAltSyncCfg ->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(FolderPairPanelBasic::OnAltSyncCfgRemove    ), NULL, this);
        basicPanel_.m_bpButtonLocalFilter->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(FolderPairPanelBasic::OnLocalFilterCfgRemove), NULL, this);

        basicPanel_.m_bpButtonAltCompCfg-> Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderPairPanelBasic::OnAltCompCfg    ), NULL, this);
        basicPanel_.m_bpButtonAltSyncCfg-> Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderPairPanelBasic::OnAltSyncCfg    ), NULL, this);
        basicPanel_.m_bpButtonLocalFilter->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderPairPanelBasic::OnLocalFilterCfg), NULL, this);

        basicPanel_.m_bpButtonRemovePair->SetBitmapLabel(GlobalResources::getImage(wxT("removeFolderPair")));
    }

    virtual void OnAltCompCfgRemoveConfirm(wxCommandEvent& event)
    {
        altCompConfig.reset();
        refreshButtons();
    }

    virtual void OnAltSyncCfgRemoveConfirm(wxCommandEvent& event)
    {
        altSyncConfig.reset();
        refreshButtons();
    }

    virtual void OnLocalFilterCfgRemoveConfirm(wxCommandEvent& event)
    {
        localFilter = FilterConfig();
        refreshButtons();
    }

private:
    void OnAltCompCfgRemove(wxCommandEvent& event)
    {
        contextMenu.reset(new wxMenu); //re-create context menu

        wxMenuItem* itemRemove = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Remove alternate settings"));
        contextMenu->Append(itemRemove);
        contextMenu->Connect(itemRemove->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(FolderPairPanelBasic::OnAltCompCfgRemoveConfirm), NULL, this);

        if (!altCompConfig.get())
            contextMenu->Enable(itemRemove->GetId(), false); //disable menu item, if clicking wouldn't make sense anyway

        basicPanel_.PopupMenu(contextMenu.get()); //show context menu
    }

    void OnAltSyncCfgRemove(wxCommandEvent& event)
    {
        contextMenu.reset(new wxMenu); //re-create context menu

        wxMenuItem* itemRemove = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Remove alternate settings"));
        contextMenu->Append(itemRemove);
        contextMenu->Connect(itemRemove->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(FolderPairPanelBasic::OnAltSyncCfgRemoveConfirm), NULL, this);

        if (!altSyncConfig.get())
            contextMenu->Enable(itemRemove->GetId(), false); //disable menu item, if clicking wouldn't make sense anyway

        basicPanel_.PopupMenu(contextMenu.get()); //show context menu
    }

    void OnLocalFilterCfgRemove(wxCommandEvent& event)
    {
        contextMenu.reset(new wxMenu); //re-create context menu

        wxMenuItem* itemClear = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Clear filter settings"));
        contextMenu->Append(itemClear);
        contextMenu->Connect(itemClear->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(FolderPairPanelBasic::OnLocalFilterCfgRemoveConfirm), NULL, this);

        if (isNullFilter(localFilter))
            contextMenu->Enable(itemClear->GetId(), false); //disable menu item, if clicking wouldn't make sense anyway

        basicPanel_.PopupMenu(contextMenu.get()); //show context menu
    }


    virtual MainConfiguration getMainConfig() const = 0;
    virtual wxWindow* getParentWindow() = 0;

    virtual void OnAltCompCfgChange() = 0;
    virtual void OnAltSyncCfgChange() = 0;
    virtual void OnLocalFilterCfgChange() {};

    void OnAltCompCfg(wxCommandEvent& event)
    {
        const MainConfiguration mainCfg = getMainConfig();

        CompConfig cmpCfg = altCompConfig.get() ? *altCompConfig : mainCfg.cmpConfig;

        if (showCompareCfgDialog(cmpCfg) == ReturnSmallDlg::BUTTON_OKAY)
        {
            altCompConfig = std::make_shared<CompConfig>(cmpCfg);
            refreshButtons();

            OnAltCompCfgChange();
        }
    }

    void OnAltSyncCfg(wxCommandEvent& event)
    {
        const MainConfiguration mainCfg = getMainConfig();

        CompConfig cmpCfg  = altCompConfig.get() ? *altCompConfig : mainCfg.cmpConfig;
        SyncConfig syncCfg = altSyncConfig.get() ? *altSyncConfig : mainCfg.syncCfg;

        if (showSyncConfigDlg(cmpCfg.compareVar,
                              syncCfg,
                              NULL) == ReturnSyncConfig::BUTTON_OKAY) //optional input parameter
        {
            altSyncConfig = std::make_shared<SyncConfig>(syncCfg);
            refreshButtons();

            OnAltSyncCfgChange();
        }
    }

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
    AltCompCfgPtr altCompConfig; //optional: present if non-NULL
    AltSyncCfgPtr altSyncConfig; //
    FilterConfig  localFilter;

    std::unique_ptr<wxMenu> contextMenu;
};
}


#endif // FOLDERPAIR_H_INCLUDED


