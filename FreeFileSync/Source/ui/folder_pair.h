// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef FOLDERPAIR_H_89341750847252345
#define FOLDERPAIR_H_89341750847252345

#include <wx/event.h>
#include <wx/menu.h>
#include <wx+/context_menu.h>
#include <wx+/bitmap_button.h>
#include <wx+/image_tools.h>
#include <wx+/image_resources.h>
#include "dir_name.h"
#include "small_dlgs.h"
#include "sync_cfg.h"
#include "../lib/norm_filter.h"
#include "../structures.h"

namespace zen
{
//basic functionality for handling alternate folder pair configuration: change sync-cfg/filter cfg, right-click context menu, button icons...

template <class GuiPanel>
class FolderPairPanelBasic : private wxEvtHandler
{
public:
    typedef std::shared_ptr<const CompConfig> AltCompCfgPtr;
    typedef std::shared_ptr<const SyncConfig> AltSyncCfgPtr;

    void setConfig(AltCompCfgPtr compConfig, AltSyncCfgPtr syncCfg, const FilterConfig& filter)
    {
        altCompConfig = compConfig;
        altSyncConfig = syncCfg;
        localFilter   = filter;
        refreshButtons();
    }

    AltCompCfgPtr getAltCompConfig  () const { return altCompConfig; }
    AltSyncCfgPtr getAltSyncConfig  () const { return altSyncConfig; }
    FilterConfig  getAltFilterConfig() const { return localFilter;   }


    FolderPairPanelBasic(GuiPanel& basicPanel) : //takes reference on basic panel to be enhanced
        basicPanel_(basicPanel)
    {
        //register events for removal of alternate configuration
        basicPanel_.m_bpButtonAltCompCfg ->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(FolderPairPanelBasic::OnAltCompCfgContext    ), nullptr, this);
        basicPanel_.m_bpButtonAltSyncCfg ->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(FolderPairPanelBasic::OnAltSyncCfgContext    ), nullptr, this);
        basicPanel_.m_bpButtonLocalFilter->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(FolderPairPanelBasic::OnLocalFilterCfgContext), nullptr, this);

        basicPanel_.m_bpButtonAltCompCfg-> Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderPairPanelBasic::OnAltCompCfg    ), nullptr, this);
        basicPanel_.m_bpButtonAltSyncCfg-> Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderPairPanelBasic::OnAltSyncCfg    ), nullptr, this);
        basicPanel_.m_bpButtonLocalFilter->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderPairPanelBasic::OnLocalFilterCfg), nullptr, this);

        basicPanel_.m_bpButtonRemovePair->SetBitmapLabel(getResourceImage(L"item_remove"));
    }

private:
    void refreshButtons()
    {
        if (altCompConfig.get())
        {
            setImage(*basicPanel_.m_bpButtonAltCompCfg, getResourceImage(L"cfg_compare_small"));
            basicPanel_.m_bpButtonAltCompCfg->SetToolTip(_("Local comparison settings") +  L" (" + getVariantName(altCompConfig->compareVar) + L")");
        }
        else
        {
            setImage(*basicPanel_.m_bpButtonAltCompCfg, greyScale(getResourceImage(L"cfg_compare_small")));
            basicPanel_.m_bpButtonAltCompCfg->SetToolTip(_("Local comparison settings"));
        }

        if (altSyncConfig.get())
        {
            setImage(*basicPanel_.m_bpButtonAltSyncCfg, getResourceImage(L"cfg_sync_small"));
            basicPanel_.m_bpButtonAltSyncCfg->SetToolTip(_("Local synchronization settings") +  L" (" + getVariantName(altSyncConfig->directionCfg.var) + L")");
        }
        else
        {
            setImage(*basicPanel_.m_bpButtonAltSyncCfg, greyScale(getResourceImage(L"cfg_sync_small")));
            basicPanel_.m_bpButtonAltSyncCfg->SetToolTip(_("Local synchronization settings"));
        }

        if (!isNullFilter(localFilter))
        {
            setImage(*basicPanel_.m_bpButtonLocalFilter, getResourceImage(L"filter_small"));
            basicPanel_.m_bpButtonLocalFilter->SetToolTip(_("Local filter") + L" (" + _("Active") + L")");
        }
        else
        {
            setImage(*basicPanel_.m_bpButtonLocalFilter, greyScale(getResourceImage(L"filter_small")));
            basicPanel_.m_bpButtonLocalFilter->SetToolTip(_("Local filter") + L" (" + _("None") + L")");
        }
    }

    void OnAltCompCfgContext(wxCommandEvent& event)
    {
        auto removeAltCompCfg = [&]
        {
            this->altCompConfig.reset(); //"this->" galore: workaround GCC compiler bugs
            this->refreshButtons();
            this->onAltCompCfgChange();
        };

        ContextMenu menu;
        menu.addItem(_("Remove local settings"), removeAltCompCfg, nullptr, altCompConfig.get() != nullptr);
        menu.popup(basicPanel_);
    }

    void OnAltSyncCfgContext(wxCommandEvent& event)
    {
        auto removeAltSyncCfg = [&]
        {
            this->altSyncConfig.reset();
            this->refreshButtons();
            this->onAltSyncCfgChange();
        };

        ContextMenu menu;
        menu.addItem(_("Remove local settings"), removeAltSyncCfg, nullptr, altSyncConfig.get() != nullptr);
        menu.popup(basicPanel_);
    }

    void OnLocalFilterCfgContext(wxCommandEvent& event)
    {
        auto removeLocalFilterCfg = [&]
        {
            this->localFilter = FilterConfig();
            this->refreshButtons();
            this->onLocalFilterCfgChange();
        };

        std::unique_ptr<FilterConfig>& filterCfgOnClipboard = getFilterCfgOnClipboardRef();

        auto copyFilter  = [&] { filterCfgOnClipboard = make_unique<FilterConfig>(this->localFilter); };
        auto pasteFilter = [&]
        {
            if (filterCfgOnClipboard)
            {
                this->localFilter = *filterCfgOnClipboard;
                this->refreshButtons();
                this->onLocalFilterCfgChange();
            }
        };

        ContextMenu menu;
        menu.addItem(_("Clear local filter"), removeLocalFilterCfg, nullptr, !isNullFilter(localFilter));
        menu.addSeparator();
        menu.addItem( _("Copy"),  copyFilter,  nullptr, !isNullFilter(localFilter));
        menu.addItem( _("Paste"), pasteFilter, nullptr, filterCfgOnClipboard.get() != nullptr);
        menu.popup(basicPanel_);
    }


    virtual MainConfiguration getMainConfig() const = 0;
    virtual wxWindow* getParentWindow() = 0;
    virtual std::unique_ptr<FilterConfig>& getFilterCfgOnClipboardRef() = 0;

    virtual void onAltCompCfgChange() = 0;
    virtual void onAltSyncCfgChange() = 0;
    virtual void onLocalFilterCfgChange() = 0;

    void OnAltCompCfg    (wxCommandEvent& event) { showConfigDialog(SyncConfigPanel::COMPARISON); }
    void OnLocalFilterCfg(wxCommandEvent& event) { showConfigDialog(SyncConfigPanel::FILTER    ); }
    void OnAltSyncCfg    (wxCommandEvent& event) { showConfigDialog(SyncConfigPanel::SYNC      ); }

    void showConfigDialog(SyncConfigPanel panelToShow)
    {
        const MainConfiguration mainCfg = getMainConfig();

        bool useAlternateCmpCfg  = altCompConfig.get() != nullptr;
        bool useAlternateSyncCfg = altSyncConfig.get() != nullptr;
        CompConfig cmpCfg  = altCompConfig.get() ? *altCompConfig : mainCfg.cmpConfig;
        SyncConfig syncCfg = altSyncConfig.get() ? *altSyncConfig : mainCfg.syncCfg;

        const bool useAlternateCmpCfgOld  = useAlternateCmpCfg;
        const bool useAlternateSyncCfgOld = useAlternateSyncCfg;
        const CompConfig   cmpCfgOld    = cmpCfg;
        const FilterConfig filterCfgOld = localFilter;
        const SyncConfig   syncCfgOld   = syncCfg;

        if (showSyncConfigDlg(getParentWindow(),
                              panelToShow,
                              &useAlternateCmpCfg,
                              cmpCfg,
                              localFilter,
                              &useAlternateSyncCfg,
                              syncCfg,
                              mainCfg.cmpConfig.compareVar,
                              nullptr,
                              _("Local Synchronization Settings")) == ReturnSyncConfig::BUTTON_OKAY)
        {
            if (!(cmpCfg == cmpCfgOld) || useAlternateCmpCfg != useAlternateCmpCfgOld)
            {
                altCompConfig = useAlternateCmpCfg ? std::make_shared<CompConfig>(cmpCfg) : nullptr;
                onAltCompCfgChange();
            }

            if (!(localFilter == filterCfgOld))
                onLocalFilterCfgChange();

            if (!(syncCfg == syncCfgOld) || useAlternateSyncCfg != useAlternateSyncCfgOld)
            {
                altSyncConfig = useAlternateSyncCfg ? std::make_shared<SyncConfig>(syncCfg) : nullptr;
                onAltSyncCfgChange();
            }

            refreshButtons();
        }
    }

    GuiPanel& basicPanel_; //panel to be enhanced by this template

    //alternate configuration attached to it
    AltCompCfgPtr altCompConfig; //optional
    AltSyncCfgPtr altSyncConfig; //
    FilterConfig  localFilter;
};
}


#endif //FOLDERPAIR_H_89341750847252345
