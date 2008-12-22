#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <wx/string.h>
#include <set>
#include <wx/intl.h>
#include <wx/panel.h>

using namespace std;

struct TranslationLine
{
    wxString original;
    wxString translation;

    bool operator>(const TranslationLine& b ) const
    {
        return (original > b.original);
    }
    bool operator<(const TranslationLine& b) const
    {
        return (original < b.original);
    }
    bool operator==(const TranslationLine& b) const
    {
        return (original == b.original);
    }
};
typedef set<TranslationLine> Translation;


class CustomLocale : public wxLocale
{
public:
    CustomLocale();
    ~CustomLocale() {}

    void setLanguage(const int language);

    int getLanguage()
    {
        return currentLanguage;
    }

    const wxChar* GetString(const wxChar* szOrigString, const wxChar* szDomain = NULL) const;

    static const string FfsLanguageDat;

private:
    Translation translationDB;
    int currentLanguage;
};


#endif // MISC_H_INCLUDED
