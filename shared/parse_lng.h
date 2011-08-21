// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef PARSE_LNG_HEADER_INCLUDED
#define PARSE_LNG_HEADER_INCLUDED

#include <string>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <functional>
#include <cctype>
#include <list>
#include <stdexcept>


namespace lngfile
{
//singular forms
typedef std::map <std::string, std::string>
TranslationMap;  //orig |-> translation

//plural forms
typedef std::pair<std::string, std::string>
SingularPluralPair; //1 house| n houses

typedef std::vector<std::string>
PluralForms;        //1 dom | 2 domy | 5 domów

typedef std::map <SingularPluralPair, PluralForms>
TranslationPluralMap; //(sing/plu) |-> pluralforms

struct TransHeader
{
    std::string languageName;     //display name: "English (UK)"
    std::string translatorName;   //"ZenJu"
    std::string localeName;       //ISO 639 language code + ISO 3166 country code, e.g. "en_GB", or "en_US"
    std::string flagFile;         //"england.png"
    int pluralCount;
    std::string pluralDefinition; //"nplurals=2; plural= n == 1 ? 0 : 1"
};


struct ParsingError
{
    ParsingError(size_t rowNo, size_t colNo) : row(rowNo), col(colNo) {}
    size_t row;
    size_t col;
};
void parseLng(const std::string& fileStream, TransHeader& header, TranslationMap& out, TranslationPluralMap& pluralOut); //throw ParsingError
void parseHeader(const std::string& fileStream, TransHeader& header); //throw ParsingError

class TranslationList; //unordered list of unique translation items
void generateLng(const TranslationList& in, const TransHeader& header, std::string& fileStream);































//--------------------------- implementation ---------------------------
class TranslationList //unordered list of unique translation items
{
public:
    void addItem(const std::string& orig, const std::string& trans)
    {
        if (!transUnique.insert(orig).second) return;

        dump.push_back(RegularItem(std::make_pair(orig, trans)));
        sequence.push_back(&dump.back());
    }
    void addPluralItem(const SingularPluralPair& orig, const PluralForms& trans)
    {
        if (!pluralUnique.insert(orig).second) return;

        dumpPlural.push_back(PluralItem(std::make_pair(orig, trans)));
        sequence.push_back(&dumpPlural.back());
    }

    bool untranslatedTextExists() const
    {
        for (std::list<RegularItem>::const_iterator i = dump.begin(); i != dump.end(); ++i)
            if (i->value.second.empty())
                return true;
        for (std::list<PluralItem>::const_iterator i = dumpPlural.begin(); i != dumpPlural.end(); ++i)
            if (i->value.second.empty())
                return true;
        return false;
    }

private:
    friend void generateLng(const TranslationList& in, const TransHeader& header, std::string& fileStream);

    struct Item {virtual ~Item() {} };
    struct RegularItem : public Item { RegularItem(const TranslationMap      ::value_type& val) : value(val) {} TranslationMap      ::value_type value; };
    struct PluralItem  : public Item { PluralItem (const TranslationPluralMap::value_type& val) : value(val) {} TranslationPluralMap::value_type value; };

    std::vector<Item*> sequence;      //dynamic list of translation elements
    std::list<RegularItem> dump;      //manage memory
    std::list<PluralItem> dumpPlural; //manage memory

    std::set<TranslationMap      ::key_type> transUnique;  //check uniqueness
    std::set<TranslationPluralMap::key_type> pluralUnique; //
};


struct Token
{
    enum Type
    {
        //header information
        TK_HEADER_BEGIN,
        TK_HEADER_END,
        TK_LANG_NAME_BEGIN,
        TK_LANG_NAME_END,
        TK_TRANS_NAME_BEGIN,
        TK_TRANS_NAME_END,
        TK_LOCALE_NAME_BEGIN,
        TK_LOCALE_NAME_END,
        TK_FLAG_FILE_BEGIN,
        TK_FLAG_FILE_END,
        TK_PLURAL_COUNT_BEGIN,
        TK_PLURAL_COUNT_END,
        TK_PLURAL_DEF_BEGIN,
        TK_PLURAL_DEF_END,

        //item level
        TK_SRC_BEGIN,
        TK_SRC_END,
        TK_TRG_BEGIN,
        TK_TRG_END,
        TK_TEXT,
        TK_PLURAL_BEGIN,
        TK_PLURAL_END,
        TK_END
    };

    Token(Type t) : type(t) {}
    Type type;

    std::string text;
};


class KnownTokens
{
public:
    typedef std::map<Token::Type, std::string> TokenMap;

    static const TokenMap& asList()
    {
        static KnownTokens inst;
        return inst.tokens;
    }

    static std::string text(Token::Type t)
    {
        TokenMap::const_iterator iter = asList().find(t);
        return iter != asList().end() ? iter->second : std::string();
    }

private:
    KnownTokens()
    {
        //header information
        tokens.insert(std::make_pair(Token::TK_HEADER_BEGIN,      "<header>"));
        tokens.insert(std::make_pair(Token::TK_HEADER_END,        "</header>"));
        tokens.insert(std::make_pair(Token::TK_LANG_NAME_BEGIN,   "<language name>"));
        tokens.insert(std::make_pair(Token::TK_LANG_NAME_END,     "</language name>"));
        tokens.insert(std::make_pair(Token::TK_TRANS_NAME_BEGIN,  "<translator>"));
        tokens.insert(std::make_pair(Token::TK_TRANS_NAME_END,    "</translator>"));
        tokens.insert(std::make_pair(Token::TK_LOCALE_NAME_BEGIN, "<locale>"));
        tokens.insert(std::make_pair(Token::TK_LOCALE_NAME_END,   "</locale>"));
        tokens.insert(std::make_pair(Token::TK_FLAG_FILE_BEGIN,   "<flag file>"));
        tokens.insert(std::make_pair(Token::TK_FLAG_FILE_END,     "</flag file>"));
        tokens.insert(std::make_pair(Token::TK_PLURAL_COUNT_BEGIN,  "<plural forms>"));
        tokens.insert(std::make_pair(Token::TK_PLURAL_COUNT_END,    "</plural forms>"));
        tokens.insert(std::make_pair(Token::TK_PLURAL_DEF_BEGIN,  "<plural definition>"));
        tokens.insert(std::make_pair(Token::TK_PLURAL_DEF_END,    "</plural definition>"));

        //item level
        tokens.insert(std::make_pair(Token::TK_SRC_BEGIN,    "<source>"));
        tokens.insert(std::make_pair(Token::TK_SRC_END,      "</source>"));
        tokens.insert(std::make_pair(Token::TK_TRG_BEGIN,    "<target>"));
        tokens.insert(std::make_pair(Token::TK_TRG_END,      "</target>"));
        tokens.insert(std::make_pair(Token::TK_PLURAL_BEGIN, "<pluralform>"));
        tokens.insert(std::make_pair(Token::TK_PLURAL_END,   "</pluralform>"));
    }
    TokenMap tokens;
};

struct IsWhiteSpace : public std::unary_function<char, bool>
{
    bool operator()(char c) const
    {
        const unsigned char usc = c; //caveat 1: std::isspace() takes an int, but expects an unsigned char
        return usc < 128 &&          //caveat 2: some parts of UTF-8 chars are erroneously seen as whitespace, e.g. the a0 from "\xec\x8b\a0" (MSVC)
               std::isspace(usc) != 0; //[!]
    }
};

class Scanner
{
public:
    Scanner(const std::string& fileStream) : stream(fileStream), pos(stream.begin()) {}

    Token nextToken()
    {
        //skip whitespace
        pos = std::find_if(pos, stream.end(), std::not1(IsWhiteSpace()));

        if (pos == stream.end())
            return Token(Token::TK_END);

        for (KnownTokens::TokenMap::const_iterator i = KnownTokens::asList().begin(); i != KnownTokens::asList().end(); ++i)
            if (startsWith(i->second))
            {
                pos += i->second.size();
                return Token(i->first);
            }

        //rest must be "text"
        std::string::const_iterator textBegin = pos;
        while (pos != stream.end() && !startsWithKnownTag())
            pos = std::find(pos + 1, stream.end(), '<');

        std::string text(textBegin, pos);

        normalize(text); //remove whitespace from end ect.

        if (text.empty() && pos == stream.end())
            return Token(Token::TK_END);

        Token out(Token::TK_TEXT);
        out.text = text;
        return out;
    }

    std::pair<size_t, size_t> position() const //current (row/col) beginning with 1
    {
        //seek last line break
        std::string::const_iterator iter = pos;
        while (iter != stream.begin() && *iter != '\n')
            --iter;

        return std::make_pair(std::count(stream.begin(), pos, '\n') + 1, pos - iter);
    }

private:
    bool startsWithKnownTag() const
    {
        for (KnownTokens::TokenMap::const_iterator i = KnownTokens::asList().begin(); i != KnownTokens::asList().end(); ++i)
            if (startsWith(i->second))
                return true;
        return false;
    }

    bool startsWith(const std::string& prefix) const
    {
        if (stream.end() - pos < static_cast<int>(prefix.size()))
            return false;
        return std::equal(prefix.begin(), prefix.end(), pos);
    }

    static void normalize(std::string& text)
    {
        //remmove whitespace from end
        while (!text.empty() && IsWhiteSpace()(*text.rbegin()))
            text.resize(text.size() - 1);

        //ensure c-style line breaks

        //Delimiter:
        //----------
        //Linux: 0xA        \n
        //Mac:   0xD        \r
        //Win:   0xD 0xA    \r\n    <- language files are in Windows format
        if (text.find('\r') != std::string::npos)
        {
            std::string tmp;
            for (std::string::const_iterator i = text.begin(); i != text.end(); ++i)
                if(*i == '\r')
                {
                    std::string::const_iterator next = i + 1;
                    if (next != text.end() && *next == '\n')
                        ++i;
                    tmp += '\n';
                }
                else
                    tmp += *i;
            text = tmp;
        }
    }

    const std::string stream;
    std::string::const_iterator pos;
};

template <class C, class T>
inline
std::basic_string<C> numberToString(const T& number) //convert number to string the C++ way
{
    std::basic_ostringstream<C> ss;
    ss << number;
    return ss.str();
}

template <class T, class C>
inline
T stringToNumber(const std::basic_string<C>& str) //convert string to number the C++ way
{
    T number = 0;
    std::basic_istringstream<C>(str) >> number;
    return number;
}


class LngParser
{
public:
    LngParser(const std::string& fileStream) : scn(fileStream), tk(scn.nextToken()) {}

    void parse(TranslationMap& out, TranslationPluralMap& pluralOut, TransHeader& header)
    {
        //header
        parseHeader(header);

        //items
        while (token().type != Token::TK_END)
            parseRegular(out, pluralOut, header.pluralCount);
    }

    void parseHeader(TransHeader& header)
    {
        consumeToken(Token::TK_HEADER_BEGIN);

        consumeToken(Token::TK_LANG_NAME_BEGIN);
        header.languageName = tk.text;
        consumeToken(Token::TK_TEXT);
        consumeToken(Token::TK_LANG_NAME_END);

        consumeToken(Token::TK_TRANS_NAME_BEGIN);
        header.translatorName = tk.text;
        consumeToken(Token::TK_TEXT);
        consumeToken(Token::TK_TRANS_NAME_END);

        consumeToken(Token::TK_LOCALE_NAME_BEGIN);
        header.localeName = tk.text;
        consumeToken(Token::TK_TEXT);
        consumeToken(Token::TK_LOCALE_NAME_END);

        consumeToken(Token::TK_FLAG_FILE_BEGIN);
        header.flagFile = tk.text;
        consumeToken(Token::TK_TEXT);
        consumeToken(Token::TK_FLAG_FILE_END);

        consumeToken(Token::TK_PLURAL_COUNT_BEGIN);
        header.pluralCount = stringToNumber<int>(tk.text);
        consumeToken(Token::TK_TEXT);
        consumeToken(Token::TK_PLURAL_COUNT_END);

        consumeToken(Token::TK_PLURAL_DEF_BEGIN);
        header.pluralDefinition = tk.text;
        consumeToken(Token::TK_TEXT);
        consumeToken(Token::TK_PLURAL_DEF_END);

        consumeToken(Token::TK_HEADER_END);
    }

private:
    void parseRegular(TranslationMap& out, TranslationPluralMap& pluralOut, int formCount)
    {
        consumeToken(Token::TK_SRC_BEGIN);

        if (token().type == Token::TK_PLURAL_BEGIN)
            return parsePlural(pluralOut, formCount);

        std::string original = tk.text;
        consumeToken(Token::TK_TEXT);
        consumeToken(Token::TK_SRC_END);

        consumeToken(Token::TK_TRG_BEGIN);
        std::string translation;
        if (token().type == Token::TK_TEXT)
        {
            translation = token().text;
            nextToken();
        }
        consumeToken(Token::TK_TRG_END);

        if (!translation.empty()) //only add if translation is existing
            out.insert(std::make_pair(original, translation));
    }

    void parsePlural(TranslationPluralMap& pluralOut, int formCount)
    {
        //Token::TK_SRC_BEGIN already consumed

        consumeToken(Token::TK_PLURAL_BEGIN);
        std::string engSingular = tk.text;
        consumeToken(Token::TK_TEXT);
        consumeToken(Token::TK_PLURAL_END);

        consumeToken(Token::TK_PLURAL_BEGIN);
        std::string engPlural = tk.text;
        consumeToken(Token::TK_TEXT);
        consumeToken(Token::TK_PLURAL_END);

        consumeToken(Token::TK_SRC_END);

        consumeToken(Token::TK_TRG_BEGIN);

        PluralForms pluralList;
        while (token().type == Token::TK_PLURAL_BEGIN)
        {
            consumeToken(Token::TK_PLURAL_BEGIN);
            std::string pluralForm = tk.text;
            consumeToken(Token::TK_TEXT);
            consumeToken(Token::TK_PLURAL_END);
            pluralList.push_back(pluralForm);

        }

        if (!pluralList.empty()&& static_cast<int>(pluralList.size()) != formCount) //invalid number of plural forms
            throw ParsingError(scn.position().first, scn.position().second);

        consumeToken(Token::TK_TRG_END);

        if (!pluralList.empty()) //only add if translation is existing
            pluralOut.insert(std::make_pair(SingularPluralPair(engSingular, engPlural), pluralList));
    }


    void nextToken() { tk = scn.nextToken(); }
    const Token& token() const { return tk; }

    void consumeToken(Token::Type t)
    {
        if (token().type != t)
            throw ParsingError(scn.position().first, scn.position().second);
        nextToken();
    }

    Scanner scn;
    Token tk;
};


inline
void parseLng(const std::string& fileStream, TransHeader& header, TranslationMap& out, TranslationPluralMap& pluralOut) //throw ParsingError
{
    out.clear();
    pluralOut.clear();

    //skip UTF-8 Byte Ordering Mark
    const bool hasBOM = fileStream.size() >= 3 && fileStream.substr(0, 3) == "\xef\xbb\xbf";

    LngParser prs(hasBOM ? fileStream.substr(3) : fileStream);
    prs.parse(out, pluralOut, header);
}


inline
void parseHeader(const std::string& fileStream, TransHeader& header) //throw ParsingError
{
    //skip UTF-8 Byte Ordering Mark
    const bool hasBOM = fileStream.size() >= 3 && fileStream.substr(0, 3) == "\xef\xbb\xbf";

    LngParser prs(hasBOM ? fileStream.substr(3) : fileStream);
    prs.parseHeader(header);
}


inline
void formatMultiLineText(std::string& text)
{
    if (text.find('\n') != std::string::npos) //multiple lines
    {
        if (*text.begin() != '\n')
            text = '\n' + text;
        if (*text.rbegin() != '\n')
            text += '\n';
    }
}


const std::string LB = "\n";
const std::string TAB = "\t";


void generateLng(const TranslationList& in, const TransHeader& header, std::string& fileStream)
{
    //header
    fileStream += KnownTokens::text(Token::TK_HEADER_BEGIN) + LB;

    fileStream += TAB + KnownTokens::text(Token::TK_LANG_NAME_BEGIN);
    fileStream += header.languageName;
    fileStream += KnownTokens::text(Token::TK_LANG_NAME_END) + LB;

    fileStream += TAB + KnownTokens::text(Token::TK_TRANS_NAME_BEGIN);
    fileStream += header.translatorName;
    fileStream += KnownTokens::text(Token::TK_TRANS_NAME_END) + LB;

    fileStream += TAB + KnownTokens::text(Token::TK_LOCALE_NAME_BEGIN);
    fileStream += header.localeName;
    fileStream += KnownTokens::text(Token::TK_LOCALE_NAME_END) + LB;

    fileStream += TAB + KnownTokens::text(Token::TK_FLAG_FILE_BEGIN);
    fileStream += header.flagFile;
    fileStream += KnownTokens::text(Token::TK_FLAG_FILE_END) + LB;

    fileStream += TAB + KnownTokens::text(Token::TK_PLURAL_COUNT_BEGIN);
    fileStream += numberToString<char>(header.pluralCount);
    fileStream += KnownTokens::text(Token::TK_PLURAL_COUNT_END) + LB;

    fileStream += TAB + KnownTokens::text(Token::TK_PLURAL_DEF_BEGIN);
    fileStream += header.pluralDefinition;
    fileStream += KnownTokens::text(Token::TK_PLURAL_DEF_END) + LB;

    fileStream += KnownTokens::text(Token::TK_HEADER_END) + LB;

    fileStream += LB;


    //items
    for (std::vector<TranslationList::Item*>::const_iterator i = in.sequence.begin(); i != in.sequence.end(); ++i)
    {
        const TranslationList::RegularItem* regular = dynamic_cast<const TranslationList::RegularItem*>(*i);
        const TranslationList::PluralItem*  plural  = dynamic_cast<const TranslationList::PluralItem*>(*i);

        if (regular)
        {
            std::string original    = regular->value.first;
            std::string translation = regular->value.second;

            formatMultiLineText(original);
            formatMultiLineText(translation);

            fileStream += KnownTokens::text(Token::TK_SRC_BEGIN);
            fileStream += original;
            fileStream += KnownTokens::text(Token::TK_SRC_END) + LB;

            fileStream += KnownTokens::text(Token::TK_TRG_BEGIN);
            fileStream += translation;
            fileStream += KnownTokens::text(Token::TK_TRG_END) + LB;

        }
        else if (plural)
        {
            std::string engSingular  = plural->value.first.first;
            std::string engPlural    = plural->value.first.second;
            const PluralForms& forms = plural->value.second;

            formatMultiLineText(engSingular);
            formatMultiLineText(engPlural);

            fileStream += KnownTokens::text(Token::TK_SRC_BEGIN) + LB;
            fileStream += KnownTokens::text(Token::TK_PLURAL_BEGIN);
            fileStream += engSingular;
            fileStream += KnownTokens::text(Token::TK_PLURAL_END) + LB;
            fileStream += KnownTokens::text(Token::TK_PLURAL_BEGIN);
            fileStream += engPlural;
            fileStream += KnownTokens::text(Token::TK_PLURAL_END)+ LB;
            fileStream += KnownTokens::text(Token::TK_SRC_END) + LB;

            fileStream += KnownTokens::text(Token::TK_TRG_BEGIN);
            if (!forms.empty()) fileStream += LB;

            for (PluralForms::const_iterator j = forms.begin(); j != forms.end(); ++j)
            {
                std::string plForm = *j;
                formatMultiLineText(plForm);

                fileStream += KnownTokens::text(Token::TK_PLURAL_BEGIN);
                fileStream += plForm;
                fileStream += KnownTokens::text(Token::TK_PLURAL_END) + LB;
            }
            fileStream += KnownTokens::text(Token::TK_TRG_END) + LB;
        }
        else
        {
            throw std::logic_error("that's what you get for brittle design ;)");
        }
    }
}
}

#endif //PARSE_LNG_HEADER_INCLUDED
