// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "help_provider.h"
#include <wx/help.h>
#include "standard_paths.h"
#include <wx/image.h>

namespace
{
class HelpProvider
{
public:
    HelpProvider()
    {
        controller.Initialize(zen::getResourceDir() +
#ifdef FFS_WIN
                              L"FreeFileSync.chm");
#elif defined FFS_LINUX
                              L"Help/FreeFileSync.hhp");
#endif
    }

    void showHelp(const wxString& section)
    {
        if (section.empty())
            controller.DisplayContents();
        else
            controller.DisplaySection(section);
    }

private:
    wxHelpController controller;
};
}

void zen::displayHelpEntry(const wxString& section)
{
    static HelpProvider provider;
    provider.showHelp(section);
}
