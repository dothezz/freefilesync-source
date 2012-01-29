// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef SYNCCONFIG_H_INCLUDED
#define SYNCCONFIG_H_INCLUDED

#include "../lib/process_xml.h"

namespace zen
{
struct ReturnSyncConfig
{
    enum ButtonPressed
    {
        BUTTON_CANCEL,
        BUTTON_OKAY = 1
    };
};

struct ExecWhenFinishedCfg
{
    std::wstring* command;              //*must* be bound!
    std::vector<std::wstring>* history; //
    size_t historyMax;
};


ReturnSyncConfig::ButtonPressed showSyncConfigDlg(CompareVariant compareVar,
                                                  SyncConfig&    syncCfg,
                                                  xmlAccess::OnGuiError* handleError,     //
                                                  ExecWhenFinishedCfg* execWhenFinished); //optional input parameter
}

#endif // SYNCCONFIG_H_INCLUDED
