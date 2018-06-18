// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef I18_N_HEADER_3843489325045
#define I18_N_HEADER_3843489325045

#include <string>
#include <memory>

//minimal layer enabling text translation - without platform/library dependencies!
#ifndef WXINTL_NO_GETTEXT_MACRO
#error WXINTL_NO_GETTEXT_MACRO must be defined to deactivate wxWidgets underscore macro
#endif

#define ZEN_CONCAT_SUB(X, Y) X ## Y
#define _(s) zen::implementation::translate(ZEN_CONCAT_SUB(L, s))
#define _P(s, p, n) zen::implementation::translate(ZEN_CONCAT_SUB(L, s), ZEN_CONCAT_SUB(L, p), n)


namespace zen
{
//implement handler to enable program wide localizations: implement THREAD-SAFE ACCESS!
struct TranslationHandler
{
    virtual ~TranslationHandler() {}

    virtual std::wstring translate(const std::wstring& text) = 0; //simple translation
    virtual std::wstring translate(const std::wstring& singular, const std::wstring& plural, int n) = 0;
};

void setTranslator(TranslationHandler* newHandler = nullptr); //takes ownership
TranslationHandler* getTranslator();





















//######################## implementation ##############################
namespace implementation
{
inline
std::wstring translate(const std::wstring& text)
{
    return getTranslator() ? getTranslator()->translate(text) : text;
}

//translate plural forms: "%x day" "%x days"
//returns "%x day" if n == 1; "%x days" else for english language
inline
std::wstring translate(const std::wstring& singular, const std::wstring& plural, int n)
{
    if (n < 0) n = -n;
    return getTranslator() ? getTranslator()->translate(singular, plural, n) : n == 1 ? singular : plural;
}

template <class T> inline
std::wstring translate(const std::wstring& singular, const std::wstring& plural, T n)
{
    return translate(singular, plural, static_cast<int>(n % 1000000));
}

inline
std::unique_ptr<TranslationHandler>& globalHandler()
{
    static std::unique_ptr<TranslationHandler> inst; //external linkage even in header!
    return inst;
}
}

inline
void setTranslator(TranslationHandler* newHandler) { implementation::globalHandler().reset(newHandler); } //takes ownership

inline
TranslationHandler* getTranslator() { return implementation::globalHandler().get(); }
}

#endif //I18_N_HEADER_3843489325045
