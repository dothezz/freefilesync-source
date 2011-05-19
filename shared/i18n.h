// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef I18_N_HEADER_3843489325045
#define I18_N_HEADER_3843489325045

#include <wx/string.h>
#include <vector>

namespace zen
{
//implement handler to enable program wide localizations
struct TranslationHandler
{
    virtual ~TranslationHandler() {}

    virtual wxString thousandsSeparator() = 0;
    virtual wxString translate(const wxString& text) = 0; //simple translation
    virtual wxString translate(const wxString& singular, const wxString& plural, int n) = 0;
};

void setTranslator(TranslationHandler* newHandler = NULL); //takes ownership
TranslationHandler* getTranslator();



inline wxString getThousandsSeparator() { return getTranslator() ? getTranslator()->thousandsSeparator() : wxT(","); };

inline wxString translate(const wxString& text) { return getTranslator() ? getTranslator()->translate(text) : text; }

//translate plural forms: "%x day" "%x days"
//returns "%x day" if n == 1; "%x days" else for english language
inline wxString translate(const wxString& singular, const wxString& plural, int n) { return getTranslator() ? getTranslator()->translate(singular, plural, n) : n == 1 ? singular : plural; }

template <class T> inline
wxString translate(const wxString& singular, const wxString& plural, T n)
{
    return translate(singular, plural, static_cast<int>(n % 1000000));
}
}

#ifndef WXINTL_NO_GETTEXT_MACRO
#error WXINTL_NO_GETTEXT_MACRO must be defined to deactivate wxWidgets underscore macro
#endif

#define _(s) zen::translate(wxT(s))
#define _P(s, p, n) zen::translate(wxT(s), wxT(p), n)

#endif //I18_N_HEADER_3843489325045
