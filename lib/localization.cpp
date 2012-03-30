// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "localization.h"
#include <fstream>
#include <map>
#include <list>
#include <iterator>
#include <wx/ffile.h>
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include "parse_plural.h"
#include "parse_lng.h"
#include <wx+/format_unit.h>
#include <zen/string_tools.h>
#include <zen/file_traverser.h>
#include "ffs_paths.h"
#include <wx+/string_conv.h>
#include <zenxml/io.h>
#include <zen/i18n.h>

using namespace zen;


namespace
{
//global objects

class FFSLocale : public TranslationHandler
{
public:
    FFSLocale(const wxString& filename, wxLanguage languageId); //throw lngfile::ParsingError, PluralForm::ParsingError

    wxLanguage langId() const { return langId_; }

    virtual std::wstring translate(const std::wstring& text)
    {
        //look for translation in buffer table
        const Translation::const_iterator iter = transMapping.find(text);
        if (iter != transMapping.end())
            return iter->second;

        return text; //fallback
    }

    virtual std::wstring translate(const std::wstring& singular, const std::wstring& plural, int n)
    {
        TranslationPlural::const_iterator iter = transMappingPl.find(std::make_pair(singular, plural));
        if (iter != transMappingPl.end())
        {
            const int formNo = pluralParser->getForm(n);
            if (0 <= formNo && formNo < static_cast<int>(iter->second.size()))
                return iter->second[formNo];
        }

        return n == 1 ? singular : plural; //fallback
    }

private:
    typedef std::map<std::wstring, std::wstring> Translation;
    typedef std::map<std::pair<std::wstring, std::wstring>, std::vector<std::wstring> > TranslationPlural;

    Translation       transMapping; //map original text |-> translation
    TranslationPlural transMappingPl;
    std::unique_ptr<PluralForm> pluralParser;
    wxLanguage langId_;
};



FFSLocale::FFSLocale(const wxString& filename, wxLanguage languageId) : langId_(languageId) //throw lngfile::ParsingError, PluralForm::ParsingError
{
    std::string inputStream;
    try
    {
        inputStream = loadStream(filename);; //throw XmlFileError
    }
    catch (...)
    {
        throw lngfile::ParsingError(0, 0);
    }

    lngfile::TransHeader          header;
    lngfile::TranslationMap       transInput;
    lngfile::TranslationPluralMap transPluralInput;
    lngfile::parseLng(inputStream, header, transInput, transPluralInput); //throw ParsingError

    for (lngfile::TranslationMap::const_iterator i = transInput.begin(); i != transInput.end(); ++i)
    {
        const std::wstring original    = utf8CvrtTo<std::wstring>(i->first);
        const std::wstring translation = utf8CvrtTo<std::wstring>(i->second);
        assert(!translation.empty());
        transMapping.insert(std::make_pair(original , translation));
    }

    for (lngfile::TranslationPluralMap::const_iterator i = transPluralInput.begin(); i != transPluralInput.end(); ++i)
    {
        const std::wstring singular = utf8CvrtTo<std::wstring>(i->first.first);
        const std::wstring plural   = utf8CvrtTo<std::wstring>(i->first.second);
        const lngfile::PluralForms& plForms = i->second;

        std::vector<std::wstring> plFormsWide;
        for (lngfile::PluralForms::const_iterator j = plForms.begin(); j != plForms.end(); ++j)
            plFormsWide.push_back(utf8CvrtTo<std::wstring>(*j));

        assert(!plFormsWide.empty());

        transMappingPl.insert(std::make_pair(std::make_pair(singular, plural), plFormsWide));
    }

    pluralParser.reset(new PluralForm(copyStringTo<Wstring>(header.pluralDefinition))); //throw PluralForm::ParsingError
}
}


class FindLngfiles : public zen::TraverseCallback
{
public:
    FindLngfiles(std::vector<Zstring>& lngFiles) : lngFiles_(lngFiles) {}

    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details)
    {
        if (endsWith(fullName, Zstr(".lng")))
            lngFiles_.push_back(fullName);
    }

    virtual void onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details) {}
    virtual std::shared_ptr<TraverseCallback> onDir(const Zchar* shortName, const Zstring& fullName) { return nullptr; }
    virtual HandleError onError(const std::wstring& errorText) { return TRAV_ERROR_IGNORE; } //errors are not really critical in this context

private:
    std::vector<Zstring>& lngFiles_;
};


struct LessTranslation : public std::binary_function<ExistingTranslations::Entry, ExistingTranslations::Entry, bool>
{
    bool operator()(const ExistingTranslations::Entry& lhs, const ExistingTranslations::Entry& rhs) const
    {
#ifdef FFS_WIN
        //use a more "natural" sort, that is ignore case and diacritics
        const int rv = ::CompareString(LOCALE_USER_DEFAULT,      //__in  LCID Locale,
                                       NORM_IGNORECASE,          //__in  DWORD dwCmpFlags,
                                       lhs.languageName.c_str(), //__in  LPCTSTR lpString1,
                                       static_cast<int>(lhs.languageName.size()), //__in  int cchCount1,
                                       rhs.languageName.c_str(), //__in  LPCTSTR lpString2,
                                       static_cast<int>(rhs.languageName.size())); //__in  int cchCount2
        if (rv == 0)
            throw std::runtime_error("Error comparing strings!");
        else
            return rv == CSTR_LESS_THAN; //convert to C-style string compare result
#else
        return lhs.languageName < rhs.languageName;
#endif
    }
};


ExistingTranslations::ExistingTranslations()
{
    {
        //default entry:
        ExistingTranslations::Entry newEntry;
        newEntry.languageID     = wxLANGUAGE_ENGLISH_US;
        newEntry.languageName   = wxT("English (US)");
        newEntry.languageFile   = wxT("");
        newEntry.translatorName = wxT("ZenJu");
        newEntry.languageFlag   = wxT("usa.png");
        locMapping.push_back(newEntry);
    }

    //search language files available
    std::vector<Zstring> lngFiles;
    FindLngfiles traverseCallback(lngFiles);

    traverseFolder(zen::getResourceDir() +  Zstr("Languages"), //throw();
                   false, //don't follow symlinks
                   traverseCallback);

    for (auto i = lngFiles.begin(); i != lngFiles.end(); ++i)
        try
        {
            std::string stream = loadStream(*i); //throw XmlFileError
            try
            {
                lngfile::TransHeader lngHeader;
                lngfile::parseHeader(stream, lngHeader); //throw ParsingError

                /*
                There is some buggy behavior in wxWidgets which maps "zh_TW" to simplified chinese.
                Fortunately locales can be also entered as description. I changed to "Chinese (Traditional)" which works fine.
                */
                if (const wxLanguageInfo* locInfo = wxLocale::FindLanguageInfo(utf8CvrtTo<wxString>(lngHeader.localeName)))
                {
                    ExistingTranslations::Entry newEntry;
                    newEntry.languageID     = locInfo->Language;
                    newEntry.languageName   = utf8CvrtTo<wxString>(lngHeader.languageName);
                    newEntry.languageFile   = toWx(*i);
                    newEntry.translatorName = utf8CvrtTo<wxString>(lngHeader.translatorName);
                    newEntry.languageFlag   = utf8CvrtTo<wxString>(lngHeader.flagFile);
                    locMapping.push_back(newEntry);
                }
            }
            catch (lngfile::ParsingError&) {} //better not show an error message here; scenario: batch jobs
        }
        catch (...) {}

    std::sort(locMapping.begin(), locMapping.end(), LessTranslation());
}


namespace
{
wxLanguage mapLanguageDialect(wxLanguage language)
{
    switch (static_cast<int>(language)) //map language dialects
    {
            //variants of wxLANGUAGE_GERMAN
        case wxLANGUAGE_GERMAN_AUSTRIAN:
        case wxLANGUAGE_GERMAN_BELGIUM:
        case wxLANGUAGE_GERMAN_LIECHTENSTEIN:
        case wxLANGUAGE_GERMAN_LUXEMBOURG:
        case wxLANGUAGE_GERMAN_SWISS:
            return wxLANGUAGE_GERMAN;

            //variants of wxLANGUAGE_FRENCH
        case wxLANGUAGE_FRENCH_BELGIAN:
        case wxLANGUAGE_FRENCH_CANADIAN:
        case wxLANGUAGE_FRENCH_LUXEMBOURG:
        case wxLANGUAGE_FRENCH_MONACO:
        case wxLANGUAGE_FRENCH_SWISS:
            return wxLANGUAGE_FRENCH;

            //variants of wxLANGUAGE_DUTCH
        case wxLANGUAGE_DUTCH_BELGIAN:
            return wxLANGUAGE_DUTCH;

            //variants of wxLANGUAGE_ITALIAN
        case wxLANGUAGE_ITALIAN_SWISS:
            return wxLANGUAGE_ITALIAN;

            //variants of wxLANGUAGE_CHINESE_SIMPLIFIED
        case wxLANGUAGE_CHINESE:
        case wxLANGUAGE_CHINESE_SINGAPORE:
            return wxLANGUAGE_CHINESE_SIMPLIFIED;

            //variants of wxLANGUAGE_CHINESE_TRADITIONAL
        case wxLANGUAGE_CHINESE_TAIWAN:
        case wxLANGUAGE_CHINESE_HONGKONG:
        case wxLANGUAGE_CHINESE_MACAU:
            return wxLANGUAGE_CHINESE_TRADITIONAL;

            //variants of wxLANGUAGE_RUSSIAN
        case wxLANGUAGE_RUSSIAN_UKRAINE:
            return wxLANGUAGE_RUSSIAN;

            //variants of wxLANGUAGE_SPANISH
        case wxLANGUAGE_SPANISH_ARGENTINA:
        case wxLANGUAGE_SPANISH_BOLIVIA:
        case wxLANGUAGE_SPANISH_CHILE:
        case wxLANGUAGE_SPANISH_COLOMBIA:
        case wxLANGUAGE_SPANISH_COSTA_RICA:
        case wxLANGUAGE_SPANISH_DOMINICAN_REPUBLIC:
        case wxLANGUAGE_SPANISH_ECUADOR:
        case wxLANGUAGE_SPANISH_EL_SALVADOR:
        case wxLANGUAGE_SPANISH_GUATEMALA:
        case wxLANGUAGE_SPANISH_HONDURAS:
        case wxLANGUAGE_SPANISH_MEXICAN:
        case wxLANGUAGE_SPANISH_MODERN:
        case wxLANGUAGE_SPANISH_NICARAGUA:
        case wxLANGUAGE_SPANISH_PANAMA:
        case wxLANGUAGE_SPANISH_PARAGUAY:
        case wxLANGUAGE_SPANISH_PERU:
        case wxLANGUAGE_SPANISH_PUERTO_RICO:
        case wxLANGUAGE_SPANISH_URUGUAY:
        case wxLANGUAGE_SPANISH_US:
        case wxLANGUAGE_SPANISH_VENEZUELA:
            return wxLANGUAGE_SPANISH;

            //variants of wxLANGUAGE_SWEDISH
        case wxLANGUAGE_SWEDISH_FINLAND:
            return wxLANGUAGE_SWEDISH;

            //case wxLANGUAGE_CZECH:
            //case wxLANGUAGE_DANISH:
            //case wxLANGUAGE_FINNISH:
            //case wxLANGUAGE_GREEK:
            //case wxLANGUAGE_JAPANESE:
            //case wxLANGUAGE_POLISH:
            //case wxLANGUAGE_SLOVENIAN:
            //case wxLANGUAGE_HUNGARIAN:
            //case wxLANGUAGE_PORTUGUESE:
            //case wxLANGUAGE_PORTUGUESE_BRAZILIAN:
            //case wxLANGUAGE_KOREAN:
            //case wxLANGUAGE_UKRAINIAN:
            //case wxLANGUAGE_CROATIAN:

            //variants of wxLANGUAGE_ARABIC
        case wxLANGUAGE_ARABIC_ALGERIA:
        case wxLANGUAGE_ARABIC_BAHRAIN:
        case wxLANGUAGE_ARABIC_EGYPT:
        case wxLANGUAGE_ARABIC_IRAQ:
        case wxLANGUAGE_ARABIC_JORDAN:
        case wxLANGUAGE_ARABIC_KUWAIT:
        case wxLANGUAGE_ARABIC_LEBANON:
        case wxLANGUAGE_ARABIC_LIBYA:
        case wxLANGUAGE_ARABIC_MOROCCO:
        case wxLANGUAGE_ARABIC_OMAN:
        case wxLANGUAGE_ARABIC_QATAR:
        case wxLANGUAGE_ARABIC_SAUDI_ARABIA:
        case wxLANGUAGE_ARABIC_SUDAN:
        case wxLANGUAGE_ARABIC_SYRIA:
        case wxLANGUAGE_ARABIC_TUNISIA:
        case wxLANGUAGE_ARABIC_UAE:
        case wxLANGUAGE_ARABIC_YEMEN:
            return wxLANGUAGE_ARABIC;

            //variants of wxLANGUAGE_ENGLISH_UK
        case wxLANGUAGE_ENGLISH_AUSTRALIA:
        case wxLANGUAGE_ENGLISH_NEW_ZEALAND:
        case wxLANGUAGE_ENGLISH_TRINIDAD:
        case wxLANGUAGE_ENGLISH_CARIBBEAN:
        case wxLANGUAGE_ENGLISH_JAMAICA:
        case wxLANGUAGE_ENGLISH_BELIZE:
        case wxLANGUAGE_ENGLISH_EIRE:
        case wxLANGUAGE_ENGLISH_SOUTH_AFRICA:
        case wxLANGUAGE_ENGLISH_ZIMBABWE:
        case wxLANGUAGE_ENGLISH_BOTSWANA:
        case wxLANGUAGE_ENGLISH_DENMARK:
            return wxLANGUAGE_ENGLISH_UK;

        default:
            return language;
    }
}
}


class CustomLocale
{
public:
    CustomLocale(int selectedLng)
    {
        const wxLanguageInfo* sysLngInfo = wxLocale::GetLanguageInfo(wxLocale::GetSystemLanguage());
        const wxLanguageInfo* selLngInfo = wxLocale::GetLanguageInfo(selectedLng);

        const bool sysLangIsRTL      = sysLngInfo ? sysLngInfo->LayoutDirection == wxLayout_RightToLeft : false;
        const bool selectedLangIsRTL = selLngInfo ? selLngInfo->LayoutDirection == wxLayout_RightToLeft : false;

        if (sysLangIsRTL == selectedLangIsRTL)
            loc.Init(wxLANGUAGE_DEFAULT); //use sys-lang to preserve sub-language specific rules (e.g. german swiss number punctation)
        else
            loc.Init(selectedLng);
    }
private:
    wxLocale loc; //required for RTL language support (and nothing else)
};


void zen::setLanguage(int language)
{
    //(try to) retrieve language file
    wxString languageFile;

    for (auto iter = ExistingTranslations::get().begin(); iter != ExistingTranslations::get().end(); ++iter)
        if (iter->languageID == language)
        {
            languageFile = iter->languageFile;
            break;
        }


    //handle RTL swapping: we need wxWidgets to do this
    static std::unique_ptr<CustomLocale> dummy;
    dummy.reset(); //avoid global locale lifetime overlap! wxWidgets cannot handle this and will crash!
    dummy.reset(new CustomLocale(languageFile.empty() ? wxLANGUAGE_ENGLISH : language));


    //reset to english language; in case of error show error message just once
    zen::setTranslator();

    //load language file into buffer
    if (!languageFile.empty()) //if languageFile is empty texts will be english per default
    {
        try
        {
            zen::setTranslator(new FFSLocale(languageFile, static_cast<wxLanguage>(language))); //throw lngfile::ParsingError, PluralForm::ParsingError
        }
        catch (lngfile::ParsingError& e)
        {
            wxMessageBox(wxString(_("Error reading file:")) + wxT(" \"") + languageFile + wxT("\"") + wxT("\n\n") +
                         wxT("Row: ")    + zen::toStringSep(e.row) + wxT("\n") +
                         wxT("Column: ") + zen::toStringSep(e.col) + wxT("\n"), _("Error"), wxOK | wxICON_ERROR);
        }
        catch (PluralForm::ParsingError&)
        {
            wxMessageBox(wxT("Invalid Plural Form"), _("Error"), wxOK | wxICON_ERROR);
        }
    }
}



int zen::getLanguage()
{
    FFSLocale* loc = dynamic_cast<FFSLocale*>(zen::getTranslator());
    return loc ? loc->langId() : wxLANGUAGE_ENGLISH_US;
}


int zen::retrieveSystemLanguage()
{
    return mapLanguageDialect(static_cast<wxLanguage>(wxLocale::GetSystemLanguage()));
}


const std::vector<ExistingTranslations::Entry>& ExistingTranslations::get()
{
    static ExistingTranslations instance;
    return instance.locMapping;
}
