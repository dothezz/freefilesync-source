// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef PARSE_PLURAL_H_INCLUDED
#define PARSE_PLURAL_H_INCLUDED

#include <list>
#include <memory>
#include <zen/string_base.h>


//http://www.gnu.org/software/hello/manual/gettext/Plural-forms.html
//http://translate.sourceforge.net/wiki/l10n/pluralforms
/*
Grammar for Plural forms parser
-------------------------------
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


typedef zen::Zbase<char> Wstring;


class PluralForm
{
public:
    struct ParsingError {};

    //.po format,e.g.: (n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)
    PluralForm(const Wstring& phrase) : n_(0)
    {
        Parser(phrase, //in
               expr, n_, dump);  //out
    }

    int getForm(int n) const { n_ = n ; return expr->eval(); }

private:
    typedef std::list<std::shared_ptr<Expression> > DumpList;

    struct Token
    {
        enum Type
        {
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
        Scanner(const Wstring& phrase) : stream(phrase), pos(stream.begin())
        {
            tokens.push_back(std::make_pair("?" , Token::TK_TERNARY_QUEST));
            tokens.push_back(std::make_pair(":" , Token::TK_TERNARY_COLON));
            tokens.push_back(std::make_pair("||", Token::TK_OR           ));
            tokens.push_back(std::make_pair("&&", Token::TK_AND          ));
            tokens.push_back(std::make_pair("==", Token::TK_EQUAL        ));
            tokens.push_back(std::make_pair("!=", Token::TK_NOT_EQUAL    ));
            tokens.push_back(std::make_pair("<=", Token::TK_LESS_EQUAL   ));
            tokens.push_back(std::make_pair("<" , Token::TK_LESS         ));
            tokens.push_back(std::make_pair(">=", Token::TK_GREATER_EQUAL));
            tokens.push_back(std::make_pair(">" , Token::TK_GREATER      ));
            tokens.push_back(std::make_pair("%" , Token::TK_MODULUS      ));
            tokens.push_back(std::make_pair("n" , Token::TK_N            ));
            tokens.push_back(std::make_pair("N" , Token::TK_N            ));
            tokens.push_back(std::make_pair("(" , Token::TK_BRACKET_LEFT ));
            tokens.push_back(std::make_pair(")" , Token::TK_BRACKET_RIGHT));
        }

        Token nextToken()
        {
            //skip whitespace
            pos = std::find_if(pos, stream.end(), std::not1(std::ptr_fun(std::iswspace)));

            if (pos == stream.end()) return Token(Token::TK_END);

            for (TokenList::const_iterator i = tokens.begin(); i != tokens.end(); ++i)
                if (startsWith(i->first))
                {
                    pos += i->first.size();
                    return Token(i->second);
                }

            Wstring::const_iterator digitEnd = std::find_if(pos, stream.end(), std::not1(std::ptr_fun(std::iswdigit)));
            int digitCount = digitEnd - pos;
            if (digitCount != 0)
            {
                Token out(Token::TK_NUMBER);
                out.number = zen::toNumber<int>(Wstring(&*pos, digitCount));
                pos += digitCount;
                return out;
            }

            throw ParsingError(); //unknown token
        }

    private:
        bool startsWith(const Wstring& prefix) const
        {
            if (stream.end() - pos < static_cast<ptrdiff_t>(prefix.size()))
                return false;
            return std::equal(prefix.begin(), prefix.end(), pos);
        }

        typedef std::vector<std::pair<Wstring, Token::Type> > TokenList;
        TokenList tokens;

        const Wstring stream;
        Wstring::const_iterator pos;
    };


    class Parser
    {
    public:
        Parser(const Wstring& phrase, //in
               const Expr<int>*& expr, int& n, PluralForm::DumpList& dump) : //out
            scn(phrase),
            tk(scn.nextToken()),
            n_(n),
            dump_(dump)
        {
            try
            {
                const Expression& e = parse();
                expr = &dynamic_cast<const Expr<int>&>(e);
            }
            catch (std::bad_cast&) { throw ParsingError(); }

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

                if (t == Token::TK_EQUAL)     return manageObj(makeBiExp(e, rhs, std::equal_to    <int>()));
                if (t == Token::TK_NOT_EQUAL) return manageObj(makeBiExp(e, rhs, std::not_equal_to<int>()));
            }
            return e;
        }

        const Expression& parseRelational()
        {
            const Expression& e = parseMultiplicative();

            Token::Type t = token().type;
            if (t == Token::TK_LESS      || //associativity: n/a
                t == Token::TK_LESS_EQUAL ||
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
            std::shared_ptr<Expression> newEntry(new T(obj));
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

    PluralForm::DumpList dump; //manage polymorphc object lifetimes
};

#endif // PARSE_PLURAL_H_INCLUDED
