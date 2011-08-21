// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef BATCHCONFIG_H_INCLUDED
#define BATCHCONFIG_H_INCLUDED

#include "../library/process_xml.h"


namespace zen
{
struct ReturnBatchConfig
{
    enum ButtonPressed
    {
        BUTTON_CANCEL,
        BATCH_FILE_SAVED = 1
    };
};

ReturnBatchConfig::ButtonPressed showSyncBatchDlg(const wxString& filename);
ReturnBatchConfig::ButtonPressed showSyncBatchDlg(const xmlAccess::XmlBatchConfig& batchCfg);
}


#endif // BATCHCONFIG_H_INCLUDED
