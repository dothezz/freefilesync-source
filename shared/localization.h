// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <wx/intl.h>
#include <vector>
#include <memory>

class Translation;


namespace FreeFileSync
{
//language dependent global variables: need to be initialized by CustomLocale on program startup and language switch

extern const wxChar* THOUSANDS_SEPARATOR;
extern const wxChar* DECIMAL_POINT;


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
    static const std::vector<LocInfoLine>& getMapping();

private:
    LocalizationInfo();
    LocalizationInfo(const LocalizationInfo&);
    LocalizationInfo& operator=(const LocalizationInfo&);

    std::vector<LocInfoLine> locMapping;
};


class CustomLocale : public wxLocale
{
public:
    static CustomLocale& getInstance();

    void setLanguage(const int language);

    int getLanguage() const
    {
        return currentLanguage;
    }

    virtual const wxChar* GetString(const wxChar* szOrigString, const wxChar* szDomain = NULL) const;

private:
    CustomLocale();
    ~CustomLocale(); //non-inline destructor for std::auto_ptr to work with forward declaration -> superfluous in this case: singleton pattern!

    std::auto_ptr<Translation> translationDB; //careful with forward-declarations and auto_ptr! save in this case, 'cause full class info available
    int currentLanguage;
};
}

#endif // MISC_H_INCLUDED
