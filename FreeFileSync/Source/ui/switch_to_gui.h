// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef SWITCHTOGUI_H_INCLUDED
#define SWITCHTOGUI_H_INCLUDED

#include "../lib/process_xml.h"
#include "main_dlg.h" //in "application.cpp" we have this dependency anyway!

namespace zen
{
//switch from FreeFileSync Batch to GUI modus: opens a new FreeFileSync GUI session asynchronously
class SwitchToGui
{
public:
    SwitchToGui(const Zstring& referenceFile,
                const xmlAccess::XmlBatchConfig& batchCfg,
                xmlAccess::XmlGlobalSettings& globalSettings) :
        guiCfg(xmlAccess::convertBatchToGui(batchCfg)),
        globalSettings_(globalSettings)
    {
        referenceFiles.push_back(referenceFile);
    }

    void execute() const
    {
        MainDialog::create(guiCfg, referenceFiles, &globalSettings_, true); //new toplevel window
    }

private:
    std::vector<Zstring> referenceFiles;
    const xmlAccess::XmlGuiConfig guiCfg;
    xmlAccess::XmlGlobalSettings& globalSettings_;
};
}

#endif // SWITCHTOGUI_H_INCLUDED
