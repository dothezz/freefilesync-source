// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <wx/string.h>
#include <vector>

namespace ffs3
{
struct LocInfoLine
{
    int languageID;
    wxString languageName;
    wxString languageFile;
    wxString translatorName;
    wxString languageFlag;
};

class LocalizationInfo
{
public:
    static const std::vector<LocInfoLine>& get();

private:
    LocalizationInfo();
    LocalizationInfo(const LocalizationInfo&);
    LocalizationInfo& operator=(const LocalizationInfo&);

    std::vector<LocInfoLine> locMapping;
};


//language independent global variables: just use operating system's default setting!
wxString getThousandsSeparator();
wxString getDecimalPoint();

void setLanguage(int language);
int getLanguage();

int retrieveSystemLanguage();

wxString translate(const wxString& original); //translate into currently selected language

//translate plural forms: "%x day|%x days"
//returns "%x day" if n == 1; "%x days" else for english language
wxString translate(const wxString& original, int n);
}

//WXINTL_NO_GETTEXT_MACRO must be defined to deactivate wxWidgets underscore macro
#define _(s) ffs3::translate(wxT(s))
#define _P(s, n) ffs3::translate(wxT(s), n)

#endif // MISC_H_INCLUDED
