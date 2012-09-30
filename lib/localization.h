// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <vector>
#include <zen/file_error.h>
#include <wx/string.h>

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

void setLanguage(int language); //throw FileError
int getLanguage();
int retrieveSystemLanguage();
}

#endif // MISC_H_INCLUDED
