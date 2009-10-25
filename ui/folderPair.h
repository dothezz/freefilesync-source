#ifndef FOLDERPAIR_H_INCLUDED
#define FOLDERPAIR_H_INCLUDED

#include "../structures.h"
#include "../shared/dragAndDrop.h"
#include "../library/resources.h"
#include "smallDialogs.h"
#include "settingsDialog.h"


namespace FreeFileSync
{
//basic functionality for changing alternate folder pair configuration: adaptable to generated gui class

template <class GuiPanel>
class FolderPairPanelBasic : public GuiPanel
{
    using GuiPanel::m_bpButtonAltSyncCfg;
    using GuiPanel::m_bpButtonAltFilter;

public:
    FolderPairPanelBasic(wxWindow* parent) :
        GuiPanel(parent),
        dragDropOnLeft(new DragDropOnDlg( GuiPanel::m_panelLeft,  GuiPanel::m_dirPickerLeft,  GuiPanel::m_directoryLeft)),
        dragDropOnRight(new DragDropOnDlg(GuiPanel::m_panelRight, GuiPanel::m_dirPickerRight, GuiPanel::m_directoryRight))
    {
        //register events for removal of alternate configuration
        m_bpButtonAltFilter->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(FolderPairPanelBasic::OnAltFilterCfgRemove), NULL, this);
        m_bpButtonAltSyncCfg->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(FolderPairPanelBasic::OnAltSyncCfgRemove), NULL, this);

        m_bpButtonAltSyncCfg->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderPairPanelBasic::OnAltSyncCfg), NULL, this);
        m_bpButtonAltFilter-> Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderPairPanelBasic::OnAltFilterCfg), NULL, this);

        GuiPanel::m_bpButtonRemovePair->SetBitmapLabel(*GlobalResources::getInstance().bitmapRemoveFolderPair);
    }

    typedef boost::shared_ptr<const FreeFileSync::AlternateSyncConfig> AltSyncCfgPtr;
    typedef boost::shared_ptr<const FreeFileSync::AlternateFilter>     AltFilterPtr;

    AltSyncCfgPtr getAltSyncConfig() const
    {
        return altSyncConfig;
    }

    AltFilterPtr getAltFilterConfig() const
    {
        return altFilter;
    }

    void setValues(AltSyncCfgPtr syncCfg,  AltFilterPtr filter)
    {
        altSyncConfig = syncCfg;
        altFilter     = filter;

        updateAltButtonColor();
    }


protected:
    virtual void OnAltFilterCfgRemoveConfirm(wxCommandEvent& event)
    {
        altFilter.reset();
        updateAltButtonColor();
    }

    virtual void OnAltSyncCfgRemoveConfirm(wxCommandEvent& event)
    {
        altSyncConfig.reset();
        updateAltButtonColor();
    }


private:
    void updateAltButtonColor()
    {
        if (altSyncConfig.get())
            m_bpButtonAltSyncCfg->SetBitmapLabel(*GlobalResources::getInstance().bitmapSyncCfgSmall);
        else
            m_bpButtonAltSyncCfg->SetBitmapLabel(*GlobalResources::getInstance().bitmapSyncCfgSmallGrey);

        if (altFilter.get())
            m_bpButtonAltFilter->SetBitmapLabel(*GlobalResources::getInstance().bitmapFilterSmall);
        else
            m_bpButtonAltFilter->SetBitmapLabel(*GlobalResources::getInstance().bitmapFilterSmallGrey);

        //set tooltips
        if (altSyncConfig.get())
            m_bpButtonAltSyncCfg->SetToolTip(wxString(_("Select alternate synchronization settings")) + wxT("  \n") +
                                             wxT("(") + altSyncConfig->syncConfiguration.getVariantName() + wxT(")"));
        else
            m_bpButtonAltSyncCfg->SetToolTip(_("Select alternate synchronization settings"));

        m_bpButtonAltFilter->SetToolTip(_("Select alternate filter settings"));
    }

    void OnAltFilterCfgRemove(wxCommandEvent& event)
    {
        contextMenu.reset(new wxMenu); //re-create context menu
        contextMenu->Append(wxID_ANY, _("Remove alternate settings"));
        contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(FolderPairPanelBasic::OnAltFilterCfgRemoveConfirm), NULL, this);
        GuiPanel::PopupMenu(contextMenu.get()); //show context menu
    }

    void OnAltSyncCfgRemove(wxCommandEvent& event)
    {
        contextMenu.reset(new wxMenu); //re-create context menu
        contextMenu->Append(wxID_ANY, _("Remove alternate settings"));
        contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(FolderPairPanelBasic::OnAltSyncCfgRemoveConfirm), NULL, this);
        GuiPanel::PopupMenu(contextMenu.get()); //show context menu
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
        SyncCfgDialog* syncDlg = new SyncCfgDialog(getParentWindow(),
                mainCfg.compareVar,
                altSyncCfg.syncConfiguration,
                altSyncCfg.handleDeletion,
                altSyncCfg.customDeletionDirectory,
                NULL);
        if (syncDlg->ShowModal() == SyncCfgDialog::BUTTON_APPLY)
        {
            altSyncConfig.reset(new AlternateSyncConfig(altSyncCfg));
            updateAltButtonColor();

            OnAltSyncCfgChange();
        }
    }

    virtual void OnAltFilterCfgChange(bool defaultValueSet) {};

    void OnAltFilterCfg(wxCommandEvent& event)
    {
        const MainConfiguration mainCfg = getMainConfig();
        const AlternateFilter filterMain(mainCfg.includeFilter, mainCfg.excludeFilter);

        AlternateFilter altFilt = altFilter.get() ? *altFilter : filterMain;
        FilterDlg* filterDlg = new FilterDlg(getParentWindow(), altFilt.includeFilter, altFilt.excludeFilter);
        if (filterDlg->ShowModal() == FilterDlg::BUTTON_APPLY)
        {
            altFilter.reset(new AlternateFilter(altFilt));
            updateAltButtonColor();

            if (    altFilt.includeFilter == defaultIncludeFilter() &&
                    altFilt.excludeFilter == defaultExcludeFilter()) //default
                OnAltFilterCfgChange(true);
            else
                OnAltFilterCfgChange(false);
        }
    }

    //alternate configuration attached to it
    AltSyncCfgPtr altSyncConfig; //optional
    AltFilterPtr  altFilter;     //optional

    std::auto_ptr<wxMenu> contextMenu;

    //support for drag and drop
    std::auto_ptr<DragDropOnDlg> dragDropOnLeft;
    std::auto_ptr<DragDropOnDlg> dragDropOnRight;
};
}


#endif // FOLDERPAIR_H_INCLUDED

