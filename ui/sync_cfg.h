// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef SYNCCONFIG_H_INCLUDED
#define SYNCCONFIG_H_INCLUDED

#include "../library/process_xml.h"

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

ReturnSyncConfig::ButtonPressed showSyncConfigDlg(CompareVariant compareVar,
                                                  SyncConfig&    syncCfg,
                                                  xmlAccess::OnGuiError* handleError); //optional input parameter
}

#endif // SYNCCONFIG_H_INCLUDED
