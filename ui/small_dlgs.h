// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef SMALLDIALOGS_H_INCLUDED
#define SMALLDIALOGS_H_INCLUDED

#include "../file_hierarchy.h"
#include "../library/process_xml.h"
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

void showHelpDialog();

ReturnSmallDlg::ButtonPressed showFilterDialog(bool isGlobalFilter, FilterConfig& filter);

ReturnSmallDlg::ButtonPressed showDeleteDialog(
    const std::vector<FileSystemObject*>& rowsOnLeft,
    const std::vector<FileSystemObject*>& rowsOnRight,
    bool& deleteOnBothSides,
    bool& useRecycleBin);

ReturnSmallDlg::ButtonPressed showCustomizeColsDlg(xmlAccess::ColumnAttributes& attr);

ReturnSmallDlg::ButtonPressed showSyncPreviewDlg(
    const wxString& variantName,
    const SyncStatistics& statistics,
    bool& dontShowAgain);

ReturnSmallDlg::ButtonPressed showCompareCfgDialog(
    CompareVariant& cmpVar,
    SymLinkHandling& handleSymlinks);

ReturnSmallDlg::ButtonPressed showGlobalSettingsDlg(xmlAccess::XmlGlobalSettings& globalSettings);
}

#endif // SMALLDIALOGS_H_INCLUDED


