// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "localization.h"
#include <fstream>
#include <map>
#include <list>
#include <iterator>
#include <wx/ffile.h>
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include "system_constants.h"
#include "parse_plural.h"
#include "parse_lng.h"
#include "util.h"
#include "string_tools.h"
#include "file_traverser.h"
#include "../shared/standard_paths.h"
#include "../shared/string_conv.h"
#include "i18n.h"

using namespace zen;


namespace
{
//global objects
wxString THOUSANDS_SEPARATOR = wxT(",");


class FFSLocale : public TranslationHandler
{
public:
    FFSLocale(const wxString& filename, wxLanguage languageId); //throw (lngfile::ParsingError, PluralForm::ParsingError)

    wxLanguage langId() const { return langId_; }

    virtual wxString thousandsSeparator() { return THOUSANDS_SEPARATOR; };

    virtual wxString translate(const wxString& text)
    {
        //look for translation in buffer table
        const Translation::const_iterator iter = transMapping.find(text);
        if (iter != transMapping.end())
            return iter->second;

        return text; //fallback
    }

    virtual wxString translate(const wxString& singular, const wxString& plural, int n)
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
    typedef std::map<wxString, wxString> Translation;
    typedef std::map<std::pair<wxString, wxString>, std::vector<wxString> > TranslationPlural;

    Translation       transMapping; //map original text |-> translation
    TranslationPlural transMappingPl;
    std::auto_ptr<PluralForm> pluralParser;
    wxLanguage langId_;
};



std::string getFileStream(const wxString& filename) //return empty string on error throw()
{
    std::string inputStream;

    //workaround to get a FILE* from a unicode filename in a portable way
    wxFFile langFile(filename, wxT("rb"));
    if (langFile.IsOpened())
    {
        FILE* fpInput = langFile.fp();

        std::vector<char> buffer(50 * 1024);
        size_t bytesRead = 0;
        do
        {
            bytesRead = ::fread(&buffer[0], 1, buffer.size(), fpInput);
            inputStream.append(&buffer[0], bytesRead);
        }
        while (bytesRead == buffer.size());
    }
    return inputStream;
}


FFSLocale::FFSLocale(const wxString& filename, wxLanguage languageId) : langId_(languageId) //throw (lngfile::ParsingError, PluralForm::ParsingError)
{
    static class StaticInit
    {
    public:
        StaticInit() : loc(wxLANGUAGE_DEFAULT)  //wxLocale: we need deferred initialization, sigh...
        {
            //::setlocale (LC_ALL, ""); -> implicitly called by wxLocale
            const lconv* localInfo = ::localeconv();

            //actually these two parameters are language dependent, but we take system setting to handle all kinds of language derivations
            THOUSANDS_SEPARATOR = wxString::FromUTF8(localInfo->thousands_sep);

            // why not working?
            // THOUSANDS_SEPARATOR = std::use_facet<std::numpunct<wchar_t> >(std::locale("")).thousands_sep();
            // DECIMAL_POINT       = std::use_facet<std::numpunct<wchar_t> >(std::locale("")).decimal_point();
        }
    private:
        wxLocale loc; //required for RTL language support (and nothing else)
    } dummy;


    const std::string inputStream = getFileStream(filename);
    if (inputStream.empty())
        throw lngfile::ParsingError(0, 0);

    lngfile::TransHeader          header;
    lngfile::TranslationMap       transInput;
    lngfile::TranslationPluralMap transPluralInput;
    lngfile::parseLng(inputStream, header, transInput, transPluralInput); //throw ParsingError

    for (lngfile::TranslationMap::const_iterator i = transInput.begin(); i != transInput.end(); ++i)
    {
        const wxString original    = wxString::FromUTF8(i->first.c_str());
        const wxString translation = wxString::FromUTF8(i->second.c_str());
        assert(!translation.empty());
        transMapping.insert(std::make_pair(original , translation));
    }

    for (lngfile::TranslationPluralMap::const_iterator i = transPluralInput.begin(); i != transPluralInput.end(); ++i)
    {
        const wxString singular = wxString::FromUTF8(i->first.first.c_str());
        const wxString plural   = wxString::FromUTF8(i->first.second.c_str());
        const lngfile::PluralForms& plForms = i->second;

        std::vector<wxString> plFormsWide;
        for (lngfile::PluralForms::const_iterator j = plForms.begin(); j != plForms.end(); ++j)
            plFormsWide.push_back(wxString::FromUTF8(j->c_str()));

        assert(!plFormsWide.empty());

        transMappingPl.insert(std::make_pair(std::make_pair(singular, plural), plFormsWide));
    }

    pluralParser.reset(new PluralForm(header.pluralDefinition.c_str())); //throw PluralForm::ParsingError
}
}


class FindLngfiles : public zen::TraverseCallback
{
public:
    FindLngfiles(std::vector<Zstring>& lngFiles) : lngFiles_(lngFiles) {}

    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details)
    {
        if (Zstring(shortName).EndsWith(Zstr(".lng")))
            lngFiles_.push_back(fullName);
    }

    virtual void onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details) {}
    virtual ReturnValDir onDir(const Zchar* shortName, const Zstring& fullName) { return Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_IGNORE>(); }
    virtual void onError(const wxString& errorText) {} //errors are not really critical in this context

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

    traverseFolder(wxToZ(zen::getResourceDir() +  wxT("Languages")), //throw();
                   false, //don't follow symlinks
                   traverseCallback);

    for (std::vector<Zstring>::const_iterator i = lngFiles.begin(); i != lngFiles.end(); ++i)
    {
        const std::string stream = getFileStream(zToWx(*i));
        if (!stream.empty())
            try
            {
                lngfile::TransHeader lngHeader;
                lngfile::parseHeader(stream, lngHeader); //throw ParsingError

                const wxLanguageInfo* locInfo = wxLocale::FindLanguageInfo(wxString::FromUTF8(lngHeader.localeName.c_str()));
                if (locInfo)
                {
                    ExistingTranslations::Entry newEntry;
                    newEntry.languageID     = locInfo->Language;
                    newEntry.languageName   = wxString::FromUTF8(lngHeader.languageName.c_str());
                    newEntry.languageFile   = zToWx(*i);
                    newEntry.translatorName = wxString::FromUTF8(lngHeader.translatorName.c_str());
                    newEntry.languageFlag   = wxString::FromUTF8(lngHeader.flagFile.c_str());
                    locMapping.push_back(newEntry);
                }
            }
            catch (lngfile::ParsingError&) {}
    }

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


void zen::setLanguage(int language)
{
    //(try to) retrieve language file
    wxString languageFile;
    for (std::vector<ExistingTranslations::Entry>::const_iterator i = ExistingTranslations::get().begin(); i != ExistingTranslations::get().end(); ++i)
        if (i->languageID == language)
        {
            languageFile = i->languageFile;
            break;
        }


    //reset to english language; in case of error show error message just once
    zen::setTranslator();

    //load language file into buffer
    if (!languageFile.empty()) //if languageFile is empty texts will be english per default
    {
        try
        {
            zen::setTranslator(new FFSLocale(languageFile, static_cast<wxLanguage>(language))); //throw (lngfile::ParsingError, PluralForm::ParsingError)
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
