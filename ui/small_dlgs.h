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

namespace ffs3
{
class SyncStatistics;


struct DefaultReturnCode
{
    enum Response
    {
        BUTTON_OKAY,
        BUTTON_CANCEL
    };
};

void showAboutDialog();

void showHelpDialog();

DefaultReturnCode::Response showFilterDialog(bool isGlobalFilter,
        Zstring& filterIncl,
        Zstring& filterExcl);

DefaultReturnCode::Response showDeleteDialog(
    const std::vector<FileSystemObject*>& rowsOnLeft,
    const std::vector<FileSystemObject*>& rowsOnRight,
    bool& deleteOnBothSides,
    bool& useRecycleBin,
    int& totalDeleteCount);

DefaultReturnCode::Response showCustomizeColsDlg(xmlAccess::ColumnAttributes& attr);

DefaultReturnCode::Response showSyncPreviewDlg(
    const wxString& variantName,
    const SyncStatistics& statistics,
    bool& dontShowAgain);

DefaultReturnCode::Response showCompareCfgDialog(
    const wxPoint& position,
    CompareVariant& cmpVar,
    SymLinkHandling& handleSymlinks);

DefaultReturnCode::Response showGlobalSettingsDlg(xmlAccess::XmlGlobalSettings& globalSettings);
}

#endif // SMALLDIALOGS_H_INCLUDED


