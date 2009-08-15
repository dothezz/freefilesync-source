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

        std::auto_ptr<Translation> translationDB;
        int currentLanguage;
    };
}

#endif // MISC_H_INCLUDED
