#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <wx/intl.h>
#include <wx/bitmap.h>
#include <vector>

class Translation;


namespace FreeFileSync
{
    struct LocInfoLine
    {
        int languageID;
        wxString languageName;
        std::string languageFile;
        wxString translatorName;
        wxBitmap* languageFlag;
    };


    class LocalizationInfo
    {
    public:
        static const std::vector<LocInfoLine>& getMapping();

    private:
        LocalizationInfo();

        std::vector<LocInfoLine> locMapping;
    };


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

    private:
        Translation* translationDB;
        int currentLanguage;
    };
}

#endif // MISC_H_INCLUDED
