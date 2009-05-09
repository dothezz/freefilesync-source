#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <wx/intl.h>

class Translation;


class CustomLocale : public wxLocale
{
public:
    CustomLocale();
    ~CustomLocale();

    void setLanguage(const int language);

    int getLanguage() const
    {
        return currentLanguage;
    }

    const wxChar* GetString(const wxChar* szOrigString, const wxChar* szDomain = NULL) const;

    static const std::string FfsLanguageDat;

private:
    Translation* translationDB;
    int currentLanguage;
};


#endif // MISC_H_INCLUDED
