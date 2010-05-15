// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "switchToGui.h"
#include "mainDialog.h"

using FreeFileSync::SwitchToGui;


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
