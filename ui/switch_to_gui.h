// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef SWITCHTOGUI_H_INCLUDED
#define SWITCHTOGUI_H_INCLUDED

#include "../library/process_xml.h"


namespace ffs3
{

//switch from FreeFileSync Batch to GUI modus: opens a new FreeFileSync GUI session asynchronously
class SwitchToGui
{
public:
    SwitchToGui(const xmlAccess::XmlBatchConfig& batchCfg, xmlAccess::XmlGlobalSettings& globalSettings); //prepare
    void execute() const; //throw()

private:
    xmlAccess::XmlGlobalSettings& globalSettings_;
    const xmlAccess::XmlGuiConfig guiCfg;
};
}

#endif // SWITCHTOGUI_H_INCLUDED
