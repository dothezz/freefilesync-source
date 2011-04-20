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
#include <iterator>

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



//http://www.gnu.org/software/hello/manual/gettext/Plural-forms.html
//http://translate.sourceforge.net/wiki/l10n/pluralforms
/*
Plural forms parser: Grammar
----------------------------
expression:
    conditional-expression

conditional-expression:
    logical-or-expression
    logical-or-expression ? expression : expression

logical-or-expression:
    logical-and-expression
    logical-or-expression || logical-and-expression

logical-and-expression:
    equality-expression
    logical-and-expression && equality-expression

equality-expression:
    relational-expression
    relational-expression == relational-expression
    relational-expression != relational-expression

relational-expression:
    multiplicative-expression
    multiplicative-expression >  multiplicative-expression
    multiplicative-expression <  multiplicative-expression
    multiplicative-expression >= multiplicative-expression
    multiplicative-expression <= multiplicative-expression

multiplicative-expression:
    pm-expression
    multiplicative-expression % pm-expression

pm-expression:
    N
    Number
    ( Expression )
*/



//expression interface
struct Expression { virtual ~Expression() {} };

template <class T>
struct Expr : public Expression
{
    typedef T ValueType;
    virtual ValueType eval() const = 0;
};

//specific binary expression based on STL function objects
template <class StlOp>
struct BinaryExp : public Expr<typename StlOp::result_type>
{
    typedef const Expr<typename StlOp::first_argument_type> SourceExp;

    BinaryExp(const SourceExp& lhs, const SourceExp& rhs, StlOp biop) : lhs_(lhs), rhs_(rhs), biop_(biop) {}
    virtual typename StlOp::result_type eval() const { return biop_(lhs_.eval(), rhs_.eval()); }
    const SourceExp& lhs_;
    const SourceExp& rhs_;
    StlOp biop_;
};

template <class StlOp>
inline
BinaryExp<StlOp> makeBiExp(const Expression& lhs, const Expression& rhs, StlOp biop) //throw (std::bad_cast)
{
    return BinaryExp<StlOp>(dynamic_cast<const Expr<typename StlOp::first_argument_type >&>(lhs),
                            dynamic_cast<const Expr<typename StlOp::second_argument_type>&>(rhs), biop);
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
    virtual int eval() const { return n_; }
    int n_;
};

struct NumberN : public Expr<int>
{
    NumberN(int& n) : n_(n) {}
    virtual int eval() const { return n_; }
    int& n_;
};


typedef Zbase<wchar_t> Wstring;


class PluralForm
{
public:
    struct ParsingError {};

    PluralForm(const Wstring& phrase) : n_(0) //.po format,e.g.: nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)
    {
        Parser(phrase, //in
               expr,   //out
               n_,     //
               count,  //
               dump);  //
    }

    int formCount() const { return count; }

    int getForm(int n) const { n_ = n ; return expr->eval(); }

private:
    typedef std::list<boost::shared_ptr<Expression> > DumpList;

    struct Token
    {
        enum Type
        {
            TK_FORM_COUNT,
            TK_PHRASE_BEGIN,
            TK_ASSIGN,
            TK_SEMICOLON,
            TK_TERNARY_QUEST,
            TK_TERNARY_COLON,
            TK_OR,
            TK_AND,
            TK_EQUAL,
            TK_NOT_EQUAL,
            TK_LESS,
            TK_LESS_EQUAL,
            TK_GREATER,
            TK_GREATER_EQUAL,
            TK_MODULUS,
            TK_N,
            TK_NUMBER,
            TK_BRACKET_LEFT,
            TK_BRACKET_RIGHT,
            TK_END
        };

        Token(Type t) : type(t), number(0) {}

        Type type;
        int number; //if type == TK_NUMBER
    };

    class Scanner
    {
    public:
        Scanner(const Wstring& phrase) : phrase_(phrase)
        {
            tokens.push_back(std::make_pair(L"nplurals", Token::TK_FORM_COUNT));
            tokens.push_back(std::make_pair(L"plural"  , Token::TK_PHRASE_BEGIN));
            tokens.push_back(std::make_pair(L";" , Token::TK_SEMICOLON    ));
            tokens.push_back(std::make_pair(L"?" , Token::TK_TERNARY_QUEST));
            tokens.push_back(std::make_pair(L":" , Token::TK_TERNARY_COLON));
            tokens.push_back(std::make_pair(L"||", Token::TK_OR           ));
            tokens.push_back(std::make_pair(L"&&", Token::TK_AND          ));
            tokens.push_back(std::make_pair(L"==", Token::TK_EQUAL        ));
            tokens.push_back(std::make_pair(L"=" , Token::TK_ASSIGN       ));
            tokens.push_back(std::make_pair(L"!=", Token::TK_NOT_EQUAL    ));
            tokens.push_back(std::make_pair(L"<=", Token::TK_LESS_EQUAL   ));
            tokens.push_back(std::make_pair(L"<" , Token::TK_LESS         ));
            tokens.push_back(std::make_pair(L">=", Token::TK_GREATER_EQUAL));
            tokens.push_back(std::make_pair(L">" , Token::TK_GREATER      ));
            tokens.push_back(std::make_pair(L"%" , Token::TK_MODULUS      ));
            tokens.push_back(std::make_pair(L"n" , Token::TK_N            ));
            tokens.push_back(std::make_pair(L"N" , Token::TK_N            ));
            tokens.push_back(std::make_pair(L"(" , Token::TK_BRACKET_LEFT ));
            tokens.push_back(std::make_pair(L")" , Token::TK_BRACKET_RIGHT));
        }

        Token nextToken()
        {
            phrase_.Trim(true, false); //remove whitespace

            if (phrase_.empty()) return Token(Token::TK_END);

            for (TokenList::const_iterator i = tokens.begin(); i != tokens.end(); ++i)
                if (phrase_.StartsWith(i->first))
                {
                    phrase_ = phrase_.substr(i->first.size());
                    return Token(i->second);
                }

            Wstring::iterator digitEnd = std::find_if(phrase_.begin(), phrase_.end(), std::not1(std::ptr_fun(std::iswdigit)));
            int digitCount = digitEnd - phrase_.begin();
            if (digitCount != 0)
            {
                Token out(Token::TK_NUMBER);
                out.number = Wstring(phrase_.c_str(), digitCount).toNumber<int>();
                phrase_ = phrase_.substr(digitCount);
                return out;
            }

            throw ParsingError(); //unknown token
        }

    private:
        typedef std::vector<std::pair<Wstring, Token::Type> > TokenList;
        TokenList tokens;
        Wstring phrase_;
    };


    class Parser
    {
    public:
        Parser(const Wstring& phrase, //in
               const Expr<int>*& expr, int& n, int& count, PluralForm::DumpList& dump) ://out
            scn(phrase),
            tk(scn.nextToken()),
            n_(n),
            dump_(dump)
        {
            consumeToken(Token::TK_FORM_COUNT);
            consumeToken(Token::TK_ASSIGN);

            count = token().number;
            consumeToken(Token::TK_NUMBER);

            consumeToken(Token::TK_SEMICOLON);
            consumeToken(Token::TK_PHRASE_BEGIN);
            consumeToken(Token::TK_ASSIGN);

            try
            {
                const Expression& e = parse();
                expr = &dynamic_cast<const Expr<int>&>(e);
            }
            catch(std::bad_cast&) { throw ParsingError(); }

            consumeToken(Token::TK_END);
        }

    private:
        void nextToken() { tk = scn.nextToken(); }
        const Token& token() const { return tk; }

        void consumeToken(Token::Type t)
        {
            if (token().type != t)
                throw ParsingError();
            nextToken();
        }

        const Expression& parse() { return parseConditional(); };

        const Expression& parseConditional()
        {
            const Expression& e = parseLogicalOr();

            if (token().type == Token::TK_TERNARY_QUEST)
            {
                nextToken();
                const Expression& thenEx = parse(); //associativity: <-
                consumeToken(Token::TK_TERNARY_COLON);
                const Expression& elseEx = parse(); //

                return manageObj(TernaryExp<Expr<int> >(dynamic_cast<const Expr<bool>&>(e),
                                                        dynamic_cast<const Expr<int>&>(thenEx),
                                                        dynamic_cast<const Expr<int>&>(elseEx)));
            }
            return e;
        }

        const Expression& parseLogicalOr()
        {
            const Expression* e = &parseLogicalAnd();
            for (;;) //associativity: ->
                if (token().type == Token::TK_OR)
                {
                    nextToken();
                    const Expression& rhs = parseLogicalAnd();
                    e = &manageObj(makeBiExp(*e, rhs, std::logical_or<bool>()));
                }
                else break;
            return *e;
        }

        const Expression& parseLogicalAnd()
        {
            const Expression* e = &parseEquality();
            for (;;) //associativity: ->
                if (token().type == Token::TK_AND)
                {
                    nextToken();
                    const Expression& rhs = parseEquality();

                    e = &manageObj(makeBiExp(*e, rhs, std::logical_and<bool>()));
                }
                else break;
            return *e;
        }

        const Expression& parseEquality()
        {
            const Expression& e = parseRelational();

            Token::Type t = token().type;
            if (t == Token::TK_EQUAL || t == Token::TK_NOT_EQUAL) //associativity: n/a
            {
                nextToken();
                const Expression& rhs = parseRelational();

                if (t == Token::TK_EQUAL)
                    return manageObj(makeBiExp(e, rhs, std::equal_to<int>()));
                else
                    return manageObj(makeBiExp(e, rhs, std::not_equal_to<int>()));
            }
            return e;
        }

        const Expression& parseRelational()
        {
            const Expression& e = parseMultiplicative();

            Token::Type t = token().type;
            if (t == Token::TK_LESS      || //associativity: n/a
                t == Token::TK_LESS_EQUAL||
                t == Token::TK_GREATER   ||
                t == Token::TK_GREATER_EQUAL)
            {
                nextToken();
                const Expression& rhs = parseMultiplicative();

                if (t == Token::TK_LESS)          return manageObj(makeBiExp(e, rhs, std::less         <int>()));
                if (t == Token::TK_LESS_EQUAL)    return manageObj(makeBiExp(e, rhs, std::less_equal   <int>()));
                if (t == Token::TK_GREATER)       return manageObj(makeBiExp(e, rhs, std::greater      <int>()));
                if (t == Token::TK_GREATER_EQUAL) return manageObj(makeBiExp(e, rhs, std::greater_equal<int>()));
            }
            return e;
        }

        const Expression& parseMultiplicative()
        {
            const Expression* e = &parsePrimary();

            for (;;) //associativity: ->
                if (token().type == Token::TK_MODULUS)
                {
                    nextToken();
                    const Expression& rhs = parsePrimary();

                    //"compile-time" check: n % 0
                    const LiteralNumberEx* literal = dynamic_cast<const LiteralNumberEx*>(&rhs);
                    if (literal && literal->eval() == 0)
                        throw ParsingError();

                    e = &manageObj(makeBiExp(*e, rhs, std::modulus<int>()));
                }
                else break;
            return *e;
        }

        const Expression& parsePrimary()
        {
            if (token().type == Token::TK_N)
            {
                nextToken();
                return manageObj(NumberN(n_));
            }
            else if (token().type == Token::TK_NUMBER)
            {
                const int number = token().number;
                nextToken();
                return manageObj(LiteralNumberEx(number));
            }
            else if (token().type == Token::TK_BRACKET_LEFT)
            {
                nextToken();
                const Expression& e = parse();

                consumeToken(Token::TK_BRACKET_RIGHT);
                return e;
            }
            else
                throw ParsingError();
        }

        template <class T>
        const T& manageObj(const T& obj)
        {
            boost::shared_ptr<Expression> newEntry(new T(obj));
            dump_.push_back(newEntry);
            return static_cast<T&>(*dump_.back());
        }

        Scanner scn;
        Token tk;

        int& n_;
        DumpList& dump_; //manage polymorphc object lifetimes
    };

    const Expr<int>* expr;
    mutable int n_;
    int count;

    PluralForm::DumpList dump; //manage polymorphc object lifetimes
};


const wchar_t formPol[] = L"nplurals=3; plural=n == 1 ? 0 : n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 10 || n % 100 > 20) ? 1 : 2";
int tstPol(int n)
{
    return n == 1 ? 0 : n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 10 || n % 100 > 20) ? 1 : 2;
}

const wchar_t formRu[] = L"nplurals= 3; plural=n % 10 == 1 && n % 100 != 11 ? 0 : n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 10 || n % 100 > 20) ? 1 : 2";
int tstRu(int n)
{
    return n % 10 == 1 && n % 100 != 11 ? 0 : n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 10 || n % 100 > 20) ? 1 : 2 ;
}

const wchar_t formLit[] = L"nplurals =3; plural=n% 10 == 1&& n % 100 != 11 ? 0 : n % 100 != 12&& n % 10 == 2 ? 1 : 0";
int tstLit(int n)
{
    return n % 10 == 1&& n % 100 != 11 ? 0 : n % 100 != 12&& n % 10 == 2 ? 1 : 0;
}

const wchar_t formArab[] = L"nplurals = 6; plural = n == 0 ? 0 : n == 1 ? 1 : n == 2 ? 2 : (n % 100 >= 3 && n % 100 <= 10) ? 3 : (n % 100 >= 11 && n % 100 <= 99) || (n % 100 == 1) || (n % 100 ==2) ? 4 : 5";
int tstArab(int n)
{
    return n == 0 ? 0 : n == 1 ? 1 : n == 2 ? 2 : (n % 100 >= 3 && n % 100 <= 10) ? 3 : (n % 100 >= 11 && n % 100 <= 99) || (n % 100 == 1) || (n % 100 ==2) ? 4 : 5;
}

const wchar_t formGerm[] = L"nplurals=2; plural= n == 1 ? 0 : 1";
int tstGerm(int n)
{
    return n == 1 ? 0 : 1;
}

const wchar_t formFren[] = L"nplurals=2; plural= n <= 1 ? 0 : 1";
int tstFren(int n)
{
    return n <= 1 ? 0 : 1;
}

const wchar_t formJap[] = L"nplurals=1; plural=0";
int tstJap(int n)
{
    return 0;
}

const wchar_t formRom[] = L"nplurals=3; plural= n == 1 ? 0 : n == 0 || (n % 100 >= 1 && n % 100 <= 20) ? 1 : 2	";
int tstRom(int n)
{
    return n == 1 ? 0 : n == 0 || (n % 100 >= 1 && n % 100 <= 20) ? 1 : 2	;
}

const wchar_t formCze[] = L" nplurals=3; plural= n % 100 == 1 ? 0 : n % 100 >= 2 && n % 100 <= 4 ? 1 : 2";
int tstCze(int n)
{
    return n % 100 == 1 ? 0 : n % 100 >= 2 && n % 100 <= 4 ? 1 : 2;
}

const wchar_t formSlov[] = L" nplurals=4; plural=n%100==1 ? 0 : n%100==2 ? 1 : n%100==3 || n%100==4 ? 2 : 3 ";
int tstSlov(int n)
{
    return n % 100 == 1 ? 0 : n % 100 == 2 ? 1 : n % 100 == 3 || n % 100 == 4 ? 2 : 3;
}

void unitTest()
{
    typedef int (*TestFun)(int);
    typedef std::vector<std::pair<const wchar_t*, TestFun> > PhraseFunList;
    PhraseFunList phrases;
    phrases.push_back(std::make_pair(formPol,  &tstPol));
    phrases.push_back(std::make_pair(formRu,   &tstRu));
    phrases.push_back(std::make_pair(formLit,  &tstLit));
    phrases.push_back(std::make_pair(formArab, &tstArab));
    phrases.push_back(std::make_pair(formGerm, &tstGerm));
    phrases.push_back(std::make_pair(formFren, &tstFren));
    phrases.push_back(std::make_pair(formJap,  &tstJap));
    phrases.push_back(std::make_pair(formRom,  &tstRom));
    phrases.push_back(std::make_pair(formCze,  &tstCze));
    phrases.push_back(std::make_pair(formSlov, &tstSlov));

    for (PhraseFunList::const_iterator i = phrases.begin(); i != phrases.end(); ++i)
    {
        PluralForm pf(i->first);
        for (int j = 0; j < 10000000; ++j)
            assert((i->second)(j) == pf.getForm(j));
    }
}


wxString ffs3::translate(const wxString& original) //translate into currently selected language
{
	/*
    int ba = 3;

    unitTest();

#ifndef _MSC_VER
#warning 3434
#endif

	*/



    //look for translation in buffer table
    const Translation::const_iterator i = activeTranslation.find(original);
    if (i != activeTranslation.end())
        return i->second.c_str();

    //fallback
    return original;
}
