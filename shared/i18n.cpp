// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "i18n.h"
#include <fstream>
#include <map>
#include <wx/ffile.h>
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include "../shared/standard_paths.h"
#include "../shared/string_conv.h"
#include "system_constants.h"
#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include <list>

using ffs3::LocalizationInfo;


namespace
{
//will receive their proper value in CustomLocale::CustomLocale()
wxString THOUSANDS_SEPARATOR = wxT(",");
wxString DECIMAL_POINT       = wxT(".");

typedef std::map<wxString, wxString> Translation;
Translation activeTranslation; //map original text |-> translation

int activeLanguage = wxLANGUAGE_ENGLISH;
}


wxString ffs3::getThousandsSeparator()
{
    return THOUSANDS_SEPARATOR;
}


wxString ffs3::getDecimalPoint()
{
    return DECIMAL_POINT;
}


const std::vector<ffs3::LocInfoLine>& LocalizationInfo::get()
{
    static LocalizationInfo instance;
    return instance.locMapping;
}


LocalizationInfo::LocalizationInfo()
{
    ffs3::LocInfoLine newEntry;

    newEntry.languageID     = wxLANGUAGE_CZECH;
    newEntry.languageName   = wxT("Čeština");
    newEntry.languageFile   = wxT("czech.lng");
    newEntry.translatorName = wxT("ViCi");
    newEntry.languageFlag   = wxT("czechRep.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_GERMAN;
    newEntry.languageName   = wxT("Deutsch");
    newEntry.languageFile   = wxT("german.lng");
    newEntry.translatorName = wxT("ZenJu");
    newEntry.languageFlag   = wxT("germany.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_ENGLISH_UK;
    newEntry.languageName   = wxT("English (UK)");
    newEntry.languageFile   = wxT("english_uk.lng");
    newEntry.translatorName = wxT("Robert Readman");
    newEntry.languageFlag   = wxT("england.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_ENGLISH;
    newEntry.languageName   = wxT("English (US)");
    newEntry.languageFile   = wxT("");
    newEntry.translatorName = wxT("ZenJu");
    newEntry.languageFlag   = wxT("usa.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_SPANISH;
    newEntry.languageName   = wxT("Español");
    newEntry.languageFile   = wxT("spanish.lng");
    newEntry.translatorName = wxT("Alexis Martínez");
    newEntry.languageFlag   = wxT("spain.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_GREEK;
    newEntry.languageName   = wxT("Ελληνικά");
    newEntry.languageFile   = wxT("greek.lng");
    newEntry.translatorName = wxT("Γιώργος Γιαγλής");
    newEntry.languageFlag   = wxT("greece.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_FRENCH;
    newEntry.languageName   = wxT("Français");
    newEntry.languageFile   = wxT("french.lng");
    newEntry.translatorName = wxT("Jean-François Hartmann");
    newEntry.languageFlag   = wxT("france.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_ITALIAN;
    newEntry.languageName   = wxT("Italiano");
    newEntry.languageFile   = wxT("italian.lng");
    newEntry.translatorName = wxT("Emmo");
    newEntry.languageFlag   = wxT("italy.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_HUNGARIAN;
    newEntry.languageName   = wxT("Magyar");
    newEntry.languageFile   = wxT("hungarian.lng");
    newEntry.translatorName = wxT("Demon");
    newEntry.languageFlag   = wxT("hungary.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_DUTCH;
    newEntry.languageName   = wxT("Nederlands");
    newEntry.languageFile   = wxT("dutch.lng");
    newEntry.translatorName = wxT("Dion van Lieshout");
    newEntry.languageFlag   = wxT("holland.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_POLISH;
    newEntry.languageName   = wxT("Polski");
    newEntry.languageFile   = wxT("polish.lng");
    newEntry.translatorName = wxT("Wojtek Pietruszewski");
    newEntry.languageFlag   = wxT("poland.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_PORTUGUESE;
    newEntry.languageName   = wxT("Português");
    newEntry.languageFile   = wxT("portuguese.lng");
    newEntry.translatorName = wxT("QuestMark");
    newEntry.languageFlag   = wxT("portugal.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_PORTUGUESE_BRAZILIAN;
    newEntry.languageName   = wxT("Português do Brasil");
    newEntry.languageFile   = wxT("portuguese_br.lng");
    newEntry.translatorName = wxT("Edison Aranha");
    newEntry.languageFlag   = wxT("brazil.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_RUSSIAN;
    newEntry.languageName   = wxT("Pусский");
    newEntry.languageFile   = wxT("russian.lng");
    newEntry.translatorName = wxT("Fayzullin T.N. aka Svobodniy");
    newEntry.languageFlag   = wxT("russia.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_ROMANIAN;
    newEntry.languageName   = wxT("Română");
    newEntry.languageFile   = wxT("romanian.lng");
    newEntry.translatorName = wxT("Alexandru Bogdan Munteanu");
    newEntry.languageFlag   = wxT("romania.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_SLOVENIAN;
    newEntry.languageName   = wxT("Slovenščina");
    newEntry.languageFile   = wxT("slovenian.lng");
    newEntry.translatorName = wxT("Matej Badalic");
    newEntry.languageFlag   = wxT("slovakia.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_FINNISH;
    newEntry.languageName   = wxT("Suomi");
    newEntry.languageFile   = wxT("finnish.lng");
    newEntry.translatorName = wxT("Nalle Juslén");
    newEntry.languageFlag   = wxT("finland.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_SWEDISH;
    newEntry.languageName   = wxT("Svenska");
    newEntry.languageFile   = wxT("swedish.lng");
    newEntry.translatorName = wxT("Åke Engelbrektson");
    newEntry.languageFlag   = wxT("sweden.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_TURKISH;
    newEntry.languageName   = wxT("Türkçe");
    newEntry.languageFile   = wxT("turkish.lng");
    newEntry.translatorName = wxT("Kaya Zeren");
    newEntry.languageFlag   = wxT("turkey.png");
    locMapping.push_back(newEntry);

    //    newEntry.languageID     = wxLANGUAGE_HEBREW;
    //    newEntry.languageName   = wxT("עִבְרִית");
    //    newEntry.languageFile   = wxT("hebrew.lng");
    //    newEntry.translatorName = wxT("Moshe Olshevsky");
    //    newEntry.languageFlag   = wxT("isreal.png");
    //    locMapping.push_back(newEntry);

    //    newEntry.languageID     = wxLANGUAGE_ARABIC;
    //    newEntry.languageName   = wxT("العربية");
    //    newEntry.languageFile   = wxT("arabic.lng");
    //    newEntry.translatorName = wxT("Yousef Shamshoum");
    //    newEntry.languageFlag   = wxT("arabic-language.png");
    //    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_JAPANESE;
    newEntry.languageName   = wxT("日本語");
    newEntry.languageFile   = wxT("japanese.lng");
    newEntry.translatorName = wxT("Tilt");
    newEntry.languageFlag   = wxT("japan.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_CHINESE_TRADITIONAL;
    newEntry.languageName   = wxT("正體中文");
    newEntry.languageFile   = wxT("chinese_traditional.lng");
    newEntry.translatorName = wxT("Carlos");
    newEntry.languageFlag   = wxT("taiwan.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_CHINESE_SIMPLIFIED;
    newEntry.languageName   = wxT("简体中文");
    newEntry.languageFile   = wxT("chinese_simple.lng");
    newEntry.translatorName = wxT("CyberCowBoy");
    newEntry.languageFlag   = wxT("china.png");
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_KOREAN;
    newEntry.languageName   = wxT("한국어");
    newEntry.languageFile   = wxT("korean.lng");
    newEntry.translatorName = wxT("Simon Park");
    newEntry.languageFlag   = wxT("south_korea.png");
    locMapping.push_back(newEntry);
}


namespace
{
int mapLanguageDialect(int language)
{
    switch (language) //map language dialects
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


inline
void exchangeEscapeChars(wxString& data)
{
    wxString output;

    const wxChar* input = data.c_str();

    wxChar value;
    while ((value = *input) != wxChar(0))
    {
        //read backslash
        if (value == wxChar('\\'))
        {
            //read next character
            ++input;
            if ((value = *input) == wxChar(0))
                break;

            switch (value)
            {
                case wxChar('\\'):
                    output += wxChar('\\');
                    break;
                case wxChar('n'):
                    output += wxChar('\n');
                    break;
                case wxChar('t'):
                    output += wxChar('\t');
                    break;
                case wxChar('\"'):
                    output += wxChar('\"');
                    break;
                default:
                    output += value;
            }
        }
        else
            output += value;

        ++input;
    }
    data = output;
}


//workaround to get a FILE* from a unicode filename in a portable way
class UnicodeFileReader
{
public:
    UnicodeFileReader(const wxString& filename) :
        inputFile(NULL)
    {
        wxFFile dummyFile(filename, wxT("rb"));
        if (dummyFile.IsOpened())
        {
            inputFile = dummyFile.fp();
            dummyFile.Detach();
        }
    }

    ~UnicodeFileReader()
    {
        if (inputFile != NULL)
            fclose(inputFile);
    }

    bool isOkay()
    {
        return inputFile != NULL;
    }

    bool getNextLine(wxString& line)
    {
        std::string output;

        while (true)
        {
            const int c = fgetc(inputFile);
            if (c == EOF)
                return false;
            else if (c == 0xD)
            {
                //Delimiter:
                //----------
                //Linux: 0xA        \n
                //Mac:   0xD        \r
                //Win:   0xD 0xA    \r\n    <- language files are in Windows format

                fgetc(inputFile);  //discard the 0xA character

                line = wxString::FromUTF8(output.c_str(), output.length());
                return true;
            }
            output += static_cast<char>(c);
        }
    }

private:
    FILE* inputFile;
};


void loadTranslation(const wxString& filename, Translation& trans) //empty translation on error
{
    trans.clear();

    UnicodeFileReader langFile(ffs3::getResourceDir() +  wxT("Languages") + ffs3::zToWx(common::FILE_NAME_SEPARATOR) + filename);
    if (langFile.isOkay())
    {
        int rowNumber = 0;
        wxString original;
        wxString tmpString;
        while (langFile.getNextLine(tmpString))
        {
            exchangeEscapeChars(tmpString);

            if (rowNumber++ % 2 == 0)
                original = tmpString;
            else
            {
                const wxString& translation = tmpString;

                if (!original.empty() && !translation.empty())
                    trans.insert(std::make_pair(original, translation));
            }
        }
    }
}
}


void ffs3::setLanguage(int language)
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
            DECIMAL_POINT       = wxString::FromUTF8(localInfo->decimal_point);

            // why not working?
            // THOUSANDS_SEPARATOR = std::use_facet<std::numpunct<wchar_t> >(std::locale("")).thousands_sep();
            // DECIMAL_POINT       = std::use_facet<std::numpunct<wchar_t> >(std::locale("")).decimal_point();
        }
    private:
        wxLocale loc; //required for RTL language support (and nothing else)
    } dummy;

    activeLanguage = language;

    //default: english
    wxString languageFile;

    //(try to) retrieve language filename
    const int mappedLanguage = mapLanguageDialect(language);
    for (std::vector<LocInfoLine>::const_iterator i = LocalizationInfo::get().begin(); i != LocalizationInfo::get().end(); ++i)
        if (i->languageID == mappedLanguage)
        {
            languageFile = i->languageFile;
            break;
        }

    //load language file into buffer
    activeTranslation.clear();
    if (!languageFile.empty())
    {
        loadTranslation(languageFile, activeTranslation); //empty translation on error
        if (activeTranslation.empty())
        {
            wxMessageBox(wxString(_("Error reading file:")) + wxT(" \"") + languageFile + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
            activeLanguage = wxLANGUAGE_ENGLISH; //reset to english language to show this error just once
        }
    }
    else
        ;   //if languageFile is empty texts will be english per default
}


int ffs3::getLanguage()
{
    return activeLanguage;
}


int ffs3::retrieveSystemLanguage()
{
    return wxLocale::GetSystemLanguage();
}




/*
typedef Zbase<wchar_t> Wstring;

const Wstring TK_TERNARY_QUEST = L"?";
const Wstring TK_TERNARY_COLON = L":";
const Wstring TK_OR            = L"||";
const Wstring TK_AND           = L"&&";
const Wstring TK_EQUAL         = L"==";
const Wstring TK_NOT_EQUAL     = L"!=";
const Wstring TK_LESS          = L"<";
const Wstring TK_LESS_EQUAL    = L"<=";
const Wstring TK_GREATER       = L">";
const Wstring TK_GREATER_EQUAL = L">=";
const Wstring TK_MODULUS       = L"%";
const Wstring TK_N             = L"n";
const Wstring TK_BRACKET_LEFT  = L"(";
const Wstring TK_BRACKET_RIGHT = L")";
const Wstring TK_FORM_COUNT    = L"nplurals=";
const Wstring TK_PHRASE_BEGIN  = L"plural=";


template <class T>
struct Expr
{
    typedef T ValueType;
    virtual ValueType eval() const = 0;
};


template <class In, class Out, class BiOp>
struct GenericBiExp : public Out
{
    GenericBiExp(const In& lhs, const In& rhs, BiOp biop) : lhs_(lhs), rhs_(rhs), biop_(biop) {}
    virtual typename Out::ValueType eval() const { return biop_(lhs_.eval(), rhs_.eval()); }
    const In& lhs_;
    const In& rhs_;
    BiOp biop_;
};

template <class Out, class In, class BiOp>
inline
GenericBiExp<In, Out, BiOp> makeBiExp(const In& lhs, const In& rhs, BiOp biop)
{
    return GenericBiExp<In, Out, BiOp>(lhs, rhs, biop);
}

template <class Out>
struct TernaryExp : public Out
{
    TernaryExp(const Expr<bool>& ifExp, const Out& thenExp, const Out& elseExp) : ifExp_(ifExp), thenExp_(thenExp), elseExp_(elseExp) {}
    virtual typename Out::ValueType eval() const { return ifExp_.eval() ? thenExp_.eval() : elseExp_.eval(); }
    const Expr<bool>& ifExp_;
    const Out& thenExp_;
    const Out& elseExp_;
};

struct LiteralNumberEx : public Expr<int>
{
    LiteralNumberEx(int n) : n_(n) {}
    virtual ValueType eval() const { return n_; }
    int n_;
};

struct NumberN : public Expr<int>
{
    NumberN(int& n) : n_(n) {}
    virtual ValueType eval() const { return n_; }
    int& n_;
};


class PluralForm
{
public:
    struct ParserError {};

    PluralForm(const Wstring& phrase) //.po format,e.g.: nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)
    {
        Wstring tmp;
        std::remove_copy_if(phrase.begin(), phrase.end(), std::back_inserter(tmp), std::iswspace);

        size_t pos = tmp.find(TK_FORM_COUNT);
        if (pos == Wstring::npos)
            throw ParserError();

        count = tmp.substr(pos + TK_FORM_COUNT.size()).toNumber<int>();

        pos = tmp.find(TK_PHRASE_BEGIN);
        if (pos == Wstring::npos)
            throw ParserError();

        expr = &parseNumber(tmp.substr(pos + TK_PHRASE_BEGIN.size()));
    }

    int formCount() const { return count; }

    int getForm(int n) const { n_ = n ; return expr->eval(); }

private:
    const Expr<bool>& parseBool(const Wstring& phrase)
    {
        Wstring part1, part2;
        if (trySplit(phrase, TK_OR, part1, part2))
            return manageObj(makeBiExp<Expr<bool> >(parseBool(part1), parseBool(part2), std::logical_or<bool>()));
        else if (trySplit(phrase, TK_AND, part1, part2))
            return manageObj(makeBiExp<Expr<bool> >(parseBool(part1), parseBool(part2), std::logical_and<bool>()));
        else if (trySplit(phrase, TK_EQUAL, part1, part2))
            return manageObj(makeBiExp<Expr<bool> >(parseNumber(part1), parseNumber(part2), std::equal_to<int>()));
        else if (trySplit(phrase, TK_NOT_EQUAL, part1, part2))
            return manageObj(makeBiExp<Expr<bool> >(parseNumber(part1), parseNumber(part2), std::not_equal_to<int>()));
        else if (trySplit(phrase, TK_LESS, part1, part2))
            return manageObj(makeBiExp<Expr<bool> >(parseNumber(part1), parseNumber(part2), std::less<int>()));
        else if (trySplit(phrase, TK_LESS_EQUAL, part1, part2))
            return manageObj(makeBiExp<Expr<bool> >(parseNumber(part1), parseNumber(part2), std::less_equal<int>()));
        else if (trySplit(phrase, TK_GREATER, part1, part2))
            return manageObj(makeBiExp<Expr<bool> >(parseNumber(part1), parseNumber(part2), std::greater<int>()));
        else if (trySplit(phrase, TK_GREATER_EQUAL, part1, part2))
            return manageObj(makeBiExp<Expr<bool> >(parseNumber(part1), parseNumber(part2), std::greater_equal<int>()));
        else
            throw ParserError();
    }

    const Expr<int>& parseNumber(const Wstring& phrase)
    {
        Wstring part1, part2;


        if (trySplit(phrase, TK_TERNARY_QUEST, part1, part2, TO_LEFT))
        {
            Wstring part3;
            if (!trySplit(Wstring(part2), TK_TERNARY_COLON, part2, part3, TO_LEFT))
                throw ParserError();
            return manageObj(TernaryExp<Expr<int> >(parseBool(part1), parseNumber(part2), parseNumber(part3)));
        }
        else if (trySplit(phrase, TK_MODULUS, part1, part2))
            return manageObj(makeBiExp<Expr<int> >(parseNumber(part1), parseNumber(part2), std::modulus<int>()));
        else if (phrase == TK_N)
            return manageObj(NumberN(n_));
        else //expect literal number
        {
            if (std::find_if(phrase.begin(), phrase.end(), std::not1(std::ptr_fun(std::iswdigit))) != phrase.end())
                throw ParserError();
            return manageObj(LiteralNumberEx(phrase.toNumber<int>()));
        }
    }

    enum Associativity
    {
        TO_LEFT,
        TO_RIGHT
    };
    static bool trySplit(const Wstring& phrase, const Wstring& delimiter, Wstring& part1, Wstring& part2, Associativity assoc = TO_RIGHT)
    {
        size_t pos = assoc == TO_LEFT ?
                     phrase.find(delimiter) : //reverse [!]
                     phrase.rfind(delimiter);
        if (pos == Wstring::npos)
            return false;

        part1 = phrase.substr(0, pos);
        part2 = phrase.substr(pos + delimiter.size());
        return true;
    }

    template <class T>
    const T& manageObj(const T& obj)
    {
        dump.push_back(obj);
        return *boost::any_cast<T>(&dump.back());
    }

    int count;
    const Expr<int>* expr;
    mutable int n_;

    std::list<boost::any> dump; //manage polymorphc object lifetimes
};


    PluralForm dummy(L"nplurals=3; plural=n == 1 ? 0 : n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 10 || n % 100 > 20) ? 1 : 2");
    int ddf = dummy.formCount();
    int kddf = dummy.getForm(0);
*/



wxString ffs3::translate(const wxString& original) //translate into currently selected language
{
    //look for translation in buffer table
    const Translation::const_iterator i = activeTranslation.find(original);
    if (i != activeTranslation.end())
        return i->second.c_str();

    //fallback
    return original;
}
