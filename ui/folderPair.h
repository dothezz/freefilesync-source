#ifndef FOLDERPAIR_H_INCLUDED
#define FOLDERPAIR_H_INCLUDED

#include "../structures.h"
#include "../shared/dragAndDrop.h"
#include "../library/resources.h"
#include "smallDialogs.h"
#include "syncDialog.h"


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
                dragDropOnLeft(new DragDropOnDlg(GuiPanel::m_panelLeft, GuiPanel::m_dirPickerLeft, GuiPanel::m_directoryLeft)),
                dragDropOnRight(new DragDropOnDlg(GuiPanel::m_panelRight, GuiPanel::m_dirPickerRight, GuiPanel::m_directoryRight))
        {
            //register events for removal of alternate configuration
            m_bpButtonAltFilter->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(FolderPairPanelBasic::OnAltFilterCfgRemove), NULL, this);
            m_bpButtonAltSyncCfg->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(FolderPairPanelBasic::OnAltSyncCfgRemove), NULL, this);

            m_bpButtonAltSyncCfg->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderPairPanelBasic::OnAltSyncCfg), NULL, this);
            m_bpButtonAltFilter-> Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(FolderPairPanelBasic::OnAltFilterCfg), NULL, this);

            GuiPanel::m_bpButtonRemovePair->SetBitmapLabel(*GlobalResources::getInstance().bitmapRemoveFolderPair);
        }

        //alternate configuration attached to it
        boost::shared_ptr<const FreeFileSync::AlternateSyncConfig> altSyncConfig; //optional
        boost::shared_ptr<const FreeFileSync::AlternateFilter>     altFilter;     //optional


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
            const MainConfiguration& mainCfg = getMainConfig();

            AlternateSyncConfig altSyncCfg = altSyncConfig.get() ?
                                             *altSyncConfig :
                                             AlternateSyncConfig(mainCfg.syncConfiguration,
                                                                 mainCfg.handleDeletion,
                                                                 mainCfg.customDeletionDirectory);
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
            const MainConfiguration& mainCfg = getMainConfig();

            AlternateFilter altFilt = altFilter.get() ?
                                      *altFilter :
                                      AlternateFilter(mainCfg.includeFilter, mainCfg.excludeFilter);

            FilterDlg* filterDlg = new FilterDlg(getParentWindow(), altFilt.includeFilter, altFilt.excludeFilter);
            if (filterDlg->ShowModal() == FilterDlg::BUTTON_APPLY)
            {
                altFilter.reset(new AlternateFilter(altFilt));
                updateAltButtonColor();

                if (altFilt.includeFilter == wxT("*") && altFilt.excludeFilter.empty()) //default
                    OnAltFilterCfgChange(true);
                else
                    OnAltFilterCfgChange(false);
            }
        }

        std::auto_ptr<wxMenu> contextMenu;

        //support for drag and drop
        std::auto_ptr<DragDropOnDlg> dragDropOnLeft;
        std::auto_ptr<DragDropOnDlg> dragDropOnRight;
    };
}


#endif // FOLDERPAIR_H_INCLUDED
