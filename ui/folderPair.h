// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef FOLDERPAIR_H_INCLUDED
#define FOLDERPAIR_H_INCLUDED

#include "../structures.h"
#include "../shared/dragAndDrop.h"
#include "../library/resources.h"
#include "smallDialogs.h"
#include "settingsDialog.h"
#include <wx/event.h>


namespace FreeFileSync
{
//basic functionality for handling alternate folder pair configuration: change sync-cfg/filter cfg, right-click context menu, button icons...

template <class GuiPanel>
class FolderPairPanelBasic : private wxEvtHandler
{
public:
    typedef boost::shared_ptr<const FreeFileSync::AlternateSyncConfig> AltSyncCfgPtr;


    Zstring getLeftDir() const
    {
        return wxToZ(basicPanel_.m_directoryLeft->GetValue());
    }

    Zstring getRightDir() const
    {
        return wxToZ(basicPanel_.m_directoryRight->GetValue());
    }

    AltSyncCfgPtr getAltSyncConfig() const
    {
        return altSyncConfig;
    }

    FilterConfig getAltFilterConfig() const
    {
        return localFilter;
    }

    void setValues(const Zstring& leftDir,
                   const Zstring& rightDir,
                   AltSyncCfgPtr syncCfg,
                   const FilterConfig& filter)
    {
        altSyncConfig = syncCfg;
        localFilter   = filter;

        //insert directory names
        FreeFileSync::setDirectoryName(zToWx(leftDir),  basicPanel_.m_directoryLeft,  basicPanel_.m_dirPickerLeft);
        FreeFileSync::setDirectoryName(zToWx(rightDir), basicPanel_.m_directoryRight, basicPanel_.m_dirPickerRight);

        refreshButtons();
    }


    void refreshButtons()
    {
        if (altSyncConfig.get())
        {
            basicPanel_.m_bpButtonAltSyncCfg->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("syncConfigSmall")));
            basicPanel_.m_bpButtonAltSyncCfg->SetToolTip(wxString(_("Select alternate synchronization settings")) +  wxT(" ") + globalFunctions::LINE_BREAK +
                    wxT("(") + getVariantName(altSyncConfig->syncConfiguration) + wxT(")"));
        }
        else
        {
            basicPanel_.m_bpButtonAltSyncCfg->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("syncConfigSmallGrey")));
            basicPanel_.m_bpButtonAltSyncCfg->SetToolTip(_("Select alternate synchronization settings"));
        }


        if (getMainConfig().filterIsActive)
        {
            //test for Null-filter
            const bool isNullFilter = NameFilter(localFilter.includeFilter, localFilter.excludeFilter).isNull();
            if (isNullFilter)
            {
                basicPanel_.m_bpButtonLocalFilter->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("filterSmallGrey")));
                basicPanel_.m_bpButtonLocalFilter->SetToolTip(_("No filter selected"));
            }
            else
            {
                basicPanel_.m_bpButtonLocalFilter->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("filterSmall")));
                basicPanel_.m_bpButtonLocalFilter->SetToolTip(_("Filter has been selected"));
            }
        }
        else
        {
            basicPanel_.m_bpButtonLocalFilter->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("filterSmallGrey")));
            basicPanel_.m_bpButtonLocalFilter->SetToolTip(_("Filtering is deactivated"));
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

        basicPanel_.m_bpButtonRemovePair->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("removeFolderPair")));
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
        contextMenu->Append(menuId, _("Remove local filter settings"));
        contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(FolderPairPanelBasic::OnLocalFilterCfgRemoveConfirm), NULL, this);

        if (NameFilter(localFilter.includeFilter, localFilter.excludeFilter).isNull())
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

    virtual MainConfiguration getMainConfig()   const = 0;
    virtual wxWindow* getParentWindow() = 0;

    virtual void OnAltSyncCfgChange() {};

    void OnAltSyncCfg(wxCommandEvent& event)
    {
        const MainConfiguration mainCfg = getMainConfig();
        const AlternateSyncConfig syncConfigMain(mainCfg.syncConfiguration,
                mainCfg.handleDeletion,
                mainCfg.customDeletionDirectory);

        AlternateSyncConfig altSyncCfg = altSyncConfig.get() ? *altSyncConfig : syncConfigMain;
        SyncCfgDialog* syncDlg = new SyncCfgDialog(getParentWindow(),
                mainCfg.compareVar,
                altSyncCfg.syncConfiguration,
                altSyncCfg.handleDeletion,
                altSyncCfg.customDeletionDirectory,
                NULL);
        if (syncDlg->ShowModal() == SyncCfgDialog::BUTTON_APPLY)
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
                             localFiltTmp.includeFilter,
                             localFiltTmp.excludeFilter,
                             getMainConfig().filterIsActive) == DefaultReturnCode::BUTTON_OKAY)
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


