// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef BATCHCONFIG_H_INCLUDED
#define BATCHCONFIG_H_INCLUDED

#include "../lib/process_xml.h"
#include "folder_history_box.h"


namespace zen
{
void showSyncBatchDlg(wxWindow* parent,
                      const wxString& referenceFile,
                      const xmlAccess::XmlBatchConfig& batchCfg,
                      const std::shared_ptr<FolderHistory>& folderHistLeft,
                      const std::shared_ptr<FolderHistory>& folderHistRight,
                      std::vector<std::wstring>& execFinishedhistory,
                      size_t execFinishedhistoryMax);
}

#endif // BATCHCONFIG_H_INCLUDED
