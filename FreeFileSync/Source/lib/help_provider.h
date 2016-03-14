// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef HELP_PROVIDER_H_85930427583421563126
#define HELP_PROVIDER_H_85930427583421563126

#if 1
namespace zen
{
inline void displayHelpEntry(const wxString& topic, wxWindow* parent) { wxLaunchDefaultBrowser(L"http://www.freefilesync.org/manual.php?topic=" + topic); }
inline void uninitializeHelp() {}
}

#else
    #include <wx/html/helpctrl.h>

#include "ffs_paths.h"


namespace zen
{
void displayHelpEntry(wxWindow* parent);
void displayHelpEntry(const wxString& topic, wxWindow* parent);

void uninitializeHelp(); //clean up gracefully during app shutdown: leaving this up to static destruction crashes on Win 8.1!






//######################## implementation ########################
namespace impl
{
//finish wxWidgets' job:
class FfsHelpController
{
public:
    static FfsHelpController& getInstance()
    {
        static FfsHelpController inst;
        return inst;
    }

    void uninitialize() {}

    void openSection(const wxString& section, wxWindow* parent)
    {
        wxHtmlModalHelp dlg(parent, utfCvrtTo<wxString>(zen::getResourceDir()) + L"Help/FreeFileSync.hhp", section,
                            wxHF_DEFAULT_STYLE | wxHF_DIALOG | wxHF_MODAL | wxHF_MERGE_BOOKS);
        (void)dlg;
        //-> solves modal help craziness on OSX!
        //-> Suse Linux: avoids program hang on exit if user closed help parent dialog before the help dialog itself was closed (why is this even possible???)
        //               avoids ESC key not being recognized by help dialog (but by parent dialog instead)
    }
};
}


inline
void displayHelpEntry(const wxString& topic, wxWindow* parent)
{
    impl::FfsHelpController::getInstance().openSection(L"html/" + topic + L".html", parent);
}


inline
void displayHelpEntry(wxWindow* parent)
{
    impl::FfsHelpController::getInstance().openSection(wxString(), parent);
}

inline
void uninitializeHelp()
{
    impl::FfsHelpController::getInstance().uninitialize();

}
}
#endif

#endif //HELP_PROVIDER_H_85930427583421563126
