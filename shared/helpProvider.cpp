// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "helpProvider.h"
#include <wx/help.h>
#include "standardPaths.h"
#include <wx/image.h>

namespace
{
class HelpProvider
{
public:
    HelpProvider()
    {
        controller.Initialize(FreeFileSync::getResourceDir() +
#ifdef FFS_WIN
                              wxT("FreeFileSync.chm"));
#elif defined FFS_LINUX
                              wxT("Help/FreeFileSync.hhp"));

        wxImage::AddHandler(new wxJPEGHandler); //ownership passed; display .jpg files correctly in Linux
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

void FreeFileSync::displayHelpEntry(const wxString& section)
{
    static HelpProvider provider;
    provider.showHelp(section);
}
