// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef PARSE_LNG_HEADER_INCLUDED
#define PARSE_LNG_HEADER_INCLUDED

#include <algorithm>
#include <cctype>
#include <functional>
//#include <list>
#include <memory>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <zen/utf.h>
#include <zen/string_tools.h>
#include "parse_plural.h"
//#include <zen/perf.h>

namespace lngfile
{
//singular forms
typedef std::map <std::string, std::string> TranslationMap;  //orig |-> translation

//plural forms
typedef std::pair<std::string, std::string> SingularPluralPair; //1 house| n houses
typedef std::vector<std::string> PluralForms; //1 dom | 2 domy | 5 domów
typedef std::map<SingularPluralPair, PluralForms> TranslationPluralMap; //(sing/plu) |-> pluralforms

struct TransHeader
{
    TransHeader() : pluralCount(0) {}
    std::string languageName;     //display name: "English (UK)"
    std::string translatorName;   //"Zenju"
    std::string localeName;       //ISO 639 language code + ISO 3166 country code, e.g. "en_GB", or "en_US"
    std::string flagFile;         //"england.png"
    int pluralCount;              //2
    std::string pluralDefinition; //"n == 1 ? 0 : 1"
};


struct ParsingError
{
    ParsingError(size_t rowNo, size_t colNo) : row(rowNo), col(colNo) {}
    size_t row; //starting with 0
    size_t col; //
};
void parseLng(const std::string& fileStream, TransHeader& header, TranslationMap& out, TranslationPluralMap& pluralOut); //throw ParsingError
void parseHeader(const std::string& fileStream, TransHeader& header); //throw ParsingError

class TranslationList; //unordered list of unique translation items
std::string generateLng(const TranslationList& in, const TransHeader& header);



















//--------------------------- implementation ---------------------------
class TranslationList //unordered list of unique translation items
{
public:
    void addItem(const std::string& orig, const std::string& trans)
    {
        if (!transUnique.insert(orig).second) return;
        sequence.push_back(std::make_shared<RegularItem>(std::make_pair(orig, trans)));
    }
    void addPluralItem(const SingularPluralPair& orig, const PluralForms& trans)
    {
        if (!pluralUnique.insert(orig).second) return;
        sequence.push_back(std::make_shared<PluralItem>(std::make_pair(orig, trans)));
    }

    bool untranslatedTextExists() const
    {
        for (auto it = sequence.begin(); it != sequence.end(); ++it)
            if (const TranslationList::RegularItem* regular = dynamic_cast<const TranslationList::RegularItem*>(it->get()))
            {
                if (regular->value.second.empty())
                    return true;
            }
            else if (const TranslationList::PluralItem*  plural  = dynamic_cast<const TranslationList::PluralItem* >(it->get()))
                if (plural->value.second.empty())
                    return true;
        return false;
    }

private:
    friend std::string generateLng(const TranslationList& in, const TransHeader& header);

    struct Item { virtual ~Item() {} };
    struct RegularItem : public Item { RegularItem(const TranslationMap      ::value_type& val) : value(val) {} TranslationMap      ::value_type value; };
    struct PluralItem  : public Item { PluralItem (const TranslationPluralMap::value_type& val) : value(val) {} TranslationPluralMap::value_type value; };

    std::vector<std::shared_ptr<Item>> sequence; //ordered list of translation elements

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
        TokenMap::const_iterator it = asList().find(t);
        return it != asList().end() ? it->second : std::string();
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
        tokens.insert(std::make_pair(Token::TK_PLURAL_COUNT_BEGIN, "<plural forms>"));
        tokens.insert(std::make_pair(Token::TK_PLURAL_COUNT_END,   "</plural forms>"));
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


class Scanner
{
public:
    Scanner(const std::string& fileStream) : stream(fileStream), pos(stream.begin())
    {
        if (zen::startsWith(stream, zen::BYTE_ORDER_MARK_UTF8))
            pos += zen::strLength(zen::BYTE_ORDER_MARK_UTF8);
    }

    Token nextToken()
    {
        //skip whitespace
        pos = std::find_if(pos, stream.end(), [](char c) { return !zen::isWhiteSpace(c); });

        if (pos == stream.end())
            return Token(Token::TK_END);

        for (auto it = KnownTokens::asList().begin(); it != KnownTokens::asList().end(); ++it)
            if (startsWith(it->second))
            {
                pos += it->second.size();
                return Token(it->first);
            }

        //rest must be "text"
        std::string::const_iterator itBegin = pos;
        while (pos != stream.end() && !startsWithKnownTag())
            pos = std::find(pos + 1, stream.end(), '<');

        std::string text(itBegin, pos);

        normalize(text); //remove whitespace from end ect.

        if (text.empty() && pos == stream.end())
            return Token(Token::TK_END);

        Token out(Token::TK_TEXT);
        out.text = text;
        return out;
    }

    size_t posRow() const //current row beginning with 0
    {
        //count line endings
        const size_t crSum = std::count(stream.begin(), pos, '\r'); //carriage returns
        const size_t nlSum = std::count(stream.begin(), pos, '\n'); //new lines
        assert(crSum == 0 || nlSum == 0 || crSum == nlSum);
        return std::max(crSum, nlSum); //be compatible with Linux/Mac/Win
    }

    size_t posCol() const //current col beginning with 0
    {
        //seek beginning of line
        for (auto it = pos; it != stream.begin(); )
        {
            --it;
            if (*it == '\r' || *it == '\n')
                return pos - it - 1;
        }
        return pos - stream.begin();
    }

private:
    bool startsWithKnownTag() const
    {
        return std::any_of(KnownTokens::asList().begin(), KnownTokens::asList().end(),
        [&](const KnownTokens::TokenMap::value_type& p) { return startsWith(p.second); });
    }

    bool startsWith(const std::string& prefix) const
    {
        if (stream.end() - pos < static_cast<ptrdiff_t>(prefix.size()))
            return false;
        return std::equal(prefix.begin(), prefix.end(), pos);
    }

    static void normalize(std::string& text)
    {
        zen::trim(text); //remmove whitespace from end

        //Delimiter:
        //----------
        //Linux: 0xA        \n
        //Mac:   0xD        \r
        //Win:   0xD 0xA    \r\n    <- language files are in Windows format
        zen::replace(text, "\r\n", '\n'); //
        zen::replace(text, "\r",   '\n'); //ensure c-style line breaks
    }

    const std::string stream;
    std::string::const_iterator pos;
};


class LngParser
{
public:
    LngParser(const std::string& fileStream) : scn(fileStream), tk(scn.nextToken()) {}

    void parse(TranslationMap& out, TranslationPluralMap& pluralOut, TransHeader& header)
    {
        parseHeader(header);

        try
        {
            parse_plural::PluralFormInfo pi(header.pluralDefinition, header.pluralCount);

            //items
            while (token().type != Token::TK_END)
                parseRegular(out, pluralOut, pi);
        }
        catch (const parse_plural::InvalidPluralForm&)
        {
            throw ParsingError(scn.posRow(), scn.posCol());
        }
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
        header.pluralCount = zen::stringTo<int>(tk.text);
        consumeToken(Token::TK_TEXT);
        consumeToken(Token::TK_PLURAL_COUNT_END);

        consumeToken(Token::TK_PLURAL_DEF_BEGIN);
        header.pluralDefinition = tk.text;
        consumeToken(Token::TK_TEXT);
        consumeToken(Token::TK_PLURAL_DEF_END);

        consumeToken(Token::TK_HEADER_END);
    }

private:
    void parseRegular(TranslationMap& out, TranslationPluralMap& pluralOut, const parse_plural::PluralFormInfo& pluralInfo)
    {
        consumeToken(Token::TK_SRC_BEGIN);

        if (token().type == Token::TK_PLURAL_BEGIN)
            return parsePlural(pluralOut, pluralInfo);

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

        validateTranslation(original, translation); //throw throw ParsingError
        out.insert(std::make_pair(original, translation));
    }

    void parsePlural(TranslationPluralMap& pluralOut, const parse_plural::PluralFormInfo& pluralInfo)
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

        consumeToken(Token::TK_TRG_END);

        const SingularPluralPair original(engSingular, engPlural);
        validateTranslation(original, pluralList, pluralInfo);
        pluralOut.insert(std::make_pair(original, pluralList));
    }

    void validateTranslation(const std::string& original, const std::string& translation) //throw ParsingError
    {
        if (original.empty())
            throw ParsingError(scn.posRow(), scn.posCol());

        if (!translation.empty())
        {
            //if original contains placeholder, so should translation!
            auto checkPlaceholder = [&](const std::string& placeholder)
            {
                if (zen::contains(original, placeholder) &&
                    !zen::contains(translation, placeholder))
                    throw ParsingError(scn.posRow(), scn.posCol());
            };
            checkPlaceholder("%x");
            checkPlaceholder("%y");
            checkPlaceholder("%z");
        }
    }

    void validateTranslation(const SingularPluralPair& original, const PluralForms& translation, const parse_plural::PluralFormInfo& pluralInfo) //throw ParsingError
    {
        //check the primary placeholder is existing at least for the second english text
        if (!zen::contains(original.second, "%x"))
            throw ParsingError(scn.posRow(), scn.posCol());

        if (!translation.empty())
        {
            //check for invalid number of plural forms
            if (pluralInfo.getCount() != static_cast<int>(translation.size()))
                throw ParsingError(scn.posRow(), scn.posCol());

            //ensure the placeholder is used when needed
            int pos = 0;
            for (auto it = translation.begin(); it != translation.end(); ++it, ++pos)
                if (!pluralInfo.isSingleNumberForm(pos) && !zen::contains(*it, "%x"))
                    throw ParsingError(scn.posRow(), scn.posCol());

            auto checkSecondaryPlaceholder = [&](const std::string& placeholder)
            {
                //make sure secondary placeholder is used in both source texts (or none)
                if (zen::contains(original.first,  placeholder) ||
                    zen::contains(original.second, placeholder))
                {
                    if (!zen::contains(original.first,  placeholder) ||
                        !zen::contains(original.second, placeholder))
                        throw ParsingError(scn.posRow(), scn.posCol());

                    //secondary placeholder is required for all plural forms
                    if (!std::all_of(translation.begin(), translation.end(), [&](const std::string& pform) { return zen::contains(pform, placeholder); }))
                    throw ParsingError(scn.posRow(), scn.posCol());
                }
            };

            checkSecondaryPlaceholder("%y");
            checkSecondaryPlaceholder("%z");
        }
    }

    void nextToken() { tk = scn.nextToken(); }
    const Token& token() const { return tk; }

    void consumeToken(Token::Type t) //throw ParsingError
    {
        expectToken(t); //throw ParsingError
        nextToken();
    }

    void expectToken(Token::Type t) //throw ParsingError
    {
        if (token().type != t)
            throw ParsingError(scn.posRow(), scn.posCol());
    }

    Scanner scn;
    Token tk;
};


inline
void parseLng(const std::string& fileStream, TransHeader& header, TranslationMap& out, TranslationPluralMap& pluralOut) //throw ParsingError
{
    out.clear();
    pluralOut.clear();

    LngParser(fileStream).parse(out, pluralOut, header);
}


inline
void parseHeader(const std::string& fileStream, TransHeader& header) //throw ParsingError
{
    LngParser(fileStream).parseHeader(header);
}


inline
void formatMultiLineText(std::string& text)
{
    assert(!zen::contains(text, "\r\n"));

    if (text.find('\n') != std::string::npos) //multiple lines
    {
        if (*text.begin() != '\n')
            text = '\n' + text;
        if (*text.rbegin() != '\n')
            text += '\n';
    }
}


std::string generateLng(const TranslationList& in, const TransHeader& header)
{
    std::string out;
    //header
    out += KnownTokens::text(Token::TK_HEADER_BEGIN) + '\n';

    out += '\t' + KnownTokens::text(Token::TK_LANG_NAME_BEGIN);
    out += header.languageName;
    out += KnownTokens::text(Token::TK_LANG_NAME_END) + '\n';

    out += '\t' + KnownTokens::text(Token::TK_TRANS_NAME_BEGIN);
    out += header.translatorName;
    out += KnownTokens::text(Token::TK_TRANS_NAME_END) + '\n';

    out += '\t' + KnownTokens::text(Token::TK_LOCALE_NAME_BEGIN);
    out += header.localeName;
    out += KnownTokens::text(Token::TK_LOCALE_NAME_END) + '\n';

    out += '\t' + KnownTokens::text(Token::TK_FLAG_FILE_BEGIN);
    out += header.flagFile;
    out += KnownTokens::text(Token::TK_FLAG_FILE_END) + '\n';

    out += '\t' + KnownTokens::text(Token::TK_PLURAL_COUNT_BEGIN);
    out += zen::numberTo<std::string>(header.pluralCount);
    out += KnownTokens::text(Token::TK_PLURAL_COUNT_END) + '\n';

    out += '\t' + KnownTokens::text(Token::TK_PLURAL_DEF_BEGIN);
    out += header.pluralDefinition;
    out += KnownTokens::text(Token::TK_PLURAL_DEF_END) + '\n';

    out += KnownTokens::text(Token::TK_HEADER_END) + '\n';

    out += '\n';


    //items
    for (auto it = in.sequence.begin(); it != in.sequence.end(); ++it)
    {
        const TranslationList::RegularItem* regular = dynamic_cast<const TranslationList::RegularItem*>(it->get());
        const TranslationList::PluralItem*  plural  = dynamic_cast<const TranslationList::PluralItem* >(it->get());

        if (regular)
        {
            std::string original    = regular->value.first;
            std::string translation = regular->value.second;

            formatMultiLineText(original);
            formatMultiLineText(translation);

            out += KnownTokens::text(Token::TK_SRC_BEGIN);
            out += original;
            out += KnownTokens::text(Token::TK_SRC_END) + '\n';

            out += KnownTokens::text(Token::TK_TRG_BEGIN);
            out += translation;
            out += KnownTokens::text(Token::TK_TRG_END) + '\n' + '\n';

        }
        else if (plural)
        {
            std::string engSingular  = plural->value.first.first;
            std::string engPlural    = plural->value.first.second;
            const PluralForms& forms = plural->value.second;

            formatMultiLineText(engSingular);
            formatMultiLineText(engPlural);

            out += KnownTokens::text(Token::TK_SRC_BEGIN) + '\n';
            out += KnownTokens::text(Token::TK_PLURAL_BEGIN);
            out += engSingular;
            out += KnownTokens::text(Token::TK_PLURAL_END) + '\n';
            out += KnownTokens::text(Token::TK_PLURAL_BEGIN);
            out += engPlural;
            out += KnownTokens::text(Token::TK_PLURAL_END) + '\n';
            out += KnownTokens::text(Token::TK_SRC_END) + '\n';

            out += KnownTokens::text(Token::TK_TRG_BEGIN);
            if (!forms.empty()) out += '\n';

            for (PluralForms::const_iterator j = forms.begin(); j != forms.end(); ++j)
            {
                std::string plForm = *j;
                formatMultiLineText(plForm);

                out += KnownTokens::text(Token::TK_PLURAL_BEGIN);
                out += plForm;
                out += KnownTokens::text(Token::TK_PLURAL_END) + '\n';
            }
            out += KnownTokens::text(Token::TK_TRG_END) + '\n' + '\n';
        }
        else
        {
            throw std::logic_error("that's what you get for brittle design ;)");
        }
    }
    assert(!zen::contains(out, "\r\n") && !zen::contains(out, "\r"));
    return zen::replaceCpy(out, '\n', "\r\n"); //back to win line endings
}
}

#endif //PARSE_LNG_HEADER_INCLUDED
