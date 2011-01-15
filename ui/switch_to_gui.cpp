// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "switch_to_gui.h"
#include "main_dlg.h"

using ffs3::SwitchToGui;


SwitchToGui::SwitchToGui(const xmlAccess::XmlBatchConfig& batchCfg,
                         xmlAccess::XmlGlobalSettings& globalSettings) : //prepare
    globalSettings_(globalSettings),
    guiCfg(xmlAccess::convertBatchToGui(batchCfg)) {}


void SwitchToGui::execute() const //throw()
{
    try
    {
        MainDialog* frame = new MainDialog(guiCfg, globalSettings_, true);
        frame->Show();
    }
    catch(...) {}
}
