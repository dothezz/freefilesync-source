// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef I18_N_HEADER_3843489325045
#define I18_N_HEADER_3843489325045

#include <string>

namespace zen
{
//implement handler to enable program wide localizations
struct TranslationHandler
{
    virtual ~TranslationHandler() {}

    virtual std::wstring thousandsSeparator() = 0;
    virtual std::wstring translate(const std::wstring& text) = 0; //simple translation
    virtual std::wstring translate(const std::wstring& singular, const std::wstring& plural, int n) = 0;
};

void setTranslator(TranslationHandler* newHandler = NULL); //takes ownership
TranslationHandler* getTranslator();



inline std::wstring getThousandsSeparator() { return getTranslator() ? getTranslator()->thousandsSeparator() : L","; };

inline std::wstring translate(const std::wstring& text) { return getTranslator() ? getTranslator()->translate(text) : text; }

//translate plural forms: "%x day" "%x days"
//returns "%x day" if n == 1; "%x days" else for english language
inline std::wstring translate(const std::wstring& singular, const std::wstring& plural, int n) { return getTranslator() ? getTranslator()->translate(singular, plural, n) : n == 1 ? singular : plural; }

template <class T> inline
std::wstring translate(const std::wstring& singular, const std::wstring& plural, T n)
{
    return translate(singular, plural, static_cast<int>(n % 1000000));
}
}

#define CONCAT_HELPER(txt1, txt2) txt1 ## txt2

#ifndef WXINTL_NO_GETTEXT_MACRO
#error WXINTL_NO_GETTEXT_MACRO must be defined to deactivate wxWidgets underscore macro
#endif

#define _(s) zen::translate(CONCAT_HELPER(L, s))
#define _P(s, p, n) zen::translate(CONCAT_HELPER(L, s), CONCAT_HELPER(L, p), n)

#endif //I18_N_HEADER_3843489325045
