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

namespace zen
{
class ExistingTranslations
{
public:
    struct Entry
    {
        int languageID;
        wxString languageName;
        wxString languageFile;
        wxString translatorName;
        wxString languageFlag;
    };
    static const std::vector<Entry>& get();

private:
    ExistingTranslations();
    ExistingTranslations(const ExistingTranslations&);
    ExistingTranslations& operator=(const ExistingTranslations&);

    std::vector<Entry> locMapping;
};

void setLanguage(int language);
int getLanguage();
int retrieveSystemLanguage();
}

#endif // MISC_H_INCLUDED
