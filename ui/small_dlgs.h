// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef SMALLDIALOGS_H_INCLUDED
#define SMALLDIALOGS_H_INCLUDED

#include "../file_hierarchy.h"
#include "../lib/process_xml.h"
#include "../synchronization.h"

namespace zen
{
struct ReturnSmallDlg
{
    enum ButtonPressed
    {
        BUTTON_CANCEL,
        BUTTON_OKAY = 1
    };
};


void showAboutDialog();

ReturnSmallDlg::ButtonPressed showFilterDialog(bool isGlobalFilter, FilterConfig& filter);

ReturnSmallDlg::ButtonPressed showDeleteDialog(
    const std::vector<FileSystemObject*>& rowsOnLeft,
    const std::vector<FileSystemObject*>& rowsOnRight,
    bool& deleteOnBothSides,
    bool& useRecycleBin);

ReturnSmallDlg::ButtonPressed showSyncPreviewDlg(
    const wxString& variantName,
    const SyncStatistics& statistics,
    bool& dontShowAgain);

ReturnSmallDlg::ButtonPressed showCompareCfgDialog(CompConfig& cmpConfig);

ReturnSmallDlg::ButtonPressed showGlobalSettingsDlg(xmlAccess::XmlGlobalSettings& globalSettings);

ReturnSmallDlg::ButtonPressed showSelectTimespanDlg(Int64& timeFrom, Int64& timeTo);
}

#endif // SMALLDIALOGS_H_INCLUDED


