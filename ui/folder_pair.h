// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef FOLDERPAIR_H_INCLUDED
#define FOLDERPAIR_H_INCLUDED

#include "../structures.h"
#include "dir_name.h"
#include "../lib/resources.h"
#include "small_dlgs.h"
#include "sync_cfg.h"
#include <wx/event.h>
#include <wx+/context_menu.h>
#include <wx/menu.h>
#include <wx+/string_conv.h>
#include "../lib/norm_filter.h"
#include <wx+/button.h>

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
            setImage(*basicPanel_.m_bpButtonAltCompCfg, GlobalResources::getImage(wxT("cmpConfigSmall")));
            basicPanel_.m_bpButtonAltCompCfg->SetToolTip(wxString(_("Select alternate comparison settings")) +  wxT(" \n") +
                                                         wxT("(") + getVariantName(altCompConfig->compareVar) + wxT(")"));
        }
        else
        {
            setImage(*basicPanel_.m_bpButtonAltCompCfg, GlobalResources::getImage(wxT("cmpConfigSmallGrey")));
            basicPanel_.m_bpButtonAltCompCfg->SetToolTip(_("Select alternate comparison settings"));
        }

        if (altSyncConfig.get())
        {
            setImage(*basicPanel_.m_bpButtonAltSyncCfg, GlobalResources::getImage(wxT("syncConfigSmall")));
            basicPanel_.m_bpButtonAltSyncCfg->SetToolTip(wxString(_("Select alternate synchronization settings")) +  wxT(" \n") +
                                                         wxT("(") + getVariantName(altSyncConfig->directionCfg.var) + wxT(")"));
        }
        else
        {
            setImage(*basicPanel_.m_bpButtonAltSyncCfg, GlobalResources::getImage(wxT("syncConfigSmallGrey")));
            basicPanel_.m_bpButtonAltSyncCfg->SetToolTip(_("Select alternate synchronization settings"));
        }

        //test for Null-filter
        if (isNullFilter(localFilter))
        {
            setImage(*basicPanel_.m_bpButtonLocalFilter, GlobalResources::getImage(wxT("filterSmallGrey")));
            basicPanel_.m_bpButtonLocalFilter->SetToolTip(_("No filter selected"));
        }
        else
        {
            setImage(*basicPanel_.m_bpButtonLocalFilter, GlobalResources::getImage(wxT("filterSmall")));
            basicPanel_.m_bpButtonLocalFilter->SetToolTip(_("Filter is active"));
        }
    }

protected:
    FolderPairPanelBasic(GuiPanel& basicPanel) : //takes reference on basic panel to be enhanced
        basicPanel_(basicPanel)
    {
        //register events for removal of alternate configuration
        basicPanel_.m_bpButtonAltCompCfg ->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(FolderPairPanelBasic::OnAltCompCfgContext    ), NULL, this);
        basicPanel_.m_bpButtonAltSyncCfg ->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(FolderPairPanelBasic::OnAltSyncCfgContext    ), NULL, this);
        basicPanel_.m_bpButtonLocalFilter->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(FolderPairPanelBasic::OnLocalFilterCfgContext), NULL, this);

        basicPanel_.m_bpButtonAltCompCfg-> Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderPairPanelBasic::OnAltCompCfg    ), NULL, this);
        basicPanel_.m_bpButtonAltSyncCfg-> Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderPairPanelBasic::OnAltSyncCfg    ), NULL, this);
        basicPanel_.m_bpButtonLocalFilter->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderPairPanelBasic::OnLocalFilterCfg), NULL, this);

        basicPanel_.m_bpButtonRemovePair->SetBitmapLabel(GlobalResources::getImage(L"removeFolderPair"));
    }

    virtual void removeAltCompCfg()
    {
        altCompConfig.reset();
        refreshButtons();
    }

    virtual void removeAltSyncCfg()
    {
        altSyncConfig.reset();
        refreshButtons();
    }

    virtual void removeLocalFilterCfg()
    {
        localFilter = FilterConfig();
        refreshButtons();
    }

private:
    void OnAltCompCfgContext(wxCommandEvent& event)
    {
        ContextMenu menu;
        menu.addItem(_("Remove alternate settings"), [this] { this->removeAltCompCfg(); }, NULL, altCompConfig.get() != NULL);
        menu.popup(basicPanel_);
    }

    void OnAltSyncCfgContext(wxCommandEvent& event)
    {
        ContextMenu menu;
        menu.addItem(_("Remove alternate settings"), [this] { this->removeAltSyncCfg(); }, NULL, altSyncConfig.get() != NULL);
        menu.popup(basicPanel_);
    }

    void OnLocalFilterCfgContext(wxCommandEvent& event)
    {
        ContextMenu menu;
        menu.addItem(_("Clear filter settings"), [this] { this->removeLocalFilterCfg(); }, NULL, !isNullFilter(localFilter));
        menu.popup(basicPanel_);
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
                              NULL,
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
};
}


#endif // FOLDERPAIR_H_INCLUDED


