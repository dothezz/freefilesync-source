// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef I18_N_H_3843489325044253425456
#define I18_N_H_3843489325044253425456

#include <string>
#include <memory>
#include <cstdint>
#include "string_tools.h"
#include "format_unit.h"

//minimal layer enabling text translation - without platform/library dependencies!

#define ZEN_TRANS_CONCAT_SUB(X, Y) X ## Y
#define _(s) zen::implementation::translate(ZEN_TRANS_CONCAT_SUB(L, s))
#define _P(s, p, n) zen::implementation::translate(ZEN_TRANS_CONCAT_SUB(L, s), ZEN_TRANS_CONCAT_SUB(L, p), n)
//source and translation are required to use %x as number placeholder
//for plural form, which will be substituted automatically!!!

namespace zen
{
//implement handler to enable program-wide localizations:
struct TranslationHandler
{
    //THREAD-SAFETY: "const" member must model thread-safe access!
    TranslationHandler() {}
    virtual ~TranslationHandler() {}

    //C++11: std::wstring should be thread-safe like an int
    virtual std::wstring translate(const std::wstring& text) const = 0; //simple translation
    virtual std::wstring translate(const std::wstring& singular, const std::wstring& plural, std::int64_t n) const = 0;

private:
    TranslationHandler           (const TranslationHandler&) = delete;
    TranslationHandler& operator=(const TranslationHandler&) = delete;
};

void setTranslator(std::unique_ptr<const TranslationHandler>&& newHandler); //take ownership
std::shared_ptr<const TranslationHandler> getTranslator();













//######################## implementation ##############################
namespace implementation
{
inline
std::wstring translate(const std::wstring& text)
{
    if (std::shared_ptr<const TranslationHandler> t = getTranslator()) //std::shared_ptr => temporarily take (shared) ownership while using the interface!
        return t->translate(text);

    return text;
}


//translate plural forms: "%x day" "%x days"
//returns "1 day" if n == 1; "123 days" if n == 123 for english language
inline
std::wstring translate(const std::wstring& singular, const std::wstring& plural, std::int64_t n)
{
    assert(contains(plural, L"%x"));

    if (std::shared_ptr<const TranslationHandler> t = getTranslator())
    {
        std::wstring translation = t->translate(singular, plural, n);
        assert(!contains(translation, L"%x"));
        return translation;
    }

    return replaceCpy(std::abs(n) == 1 ? singular : plural, L"%x", toGuiString(n));
}


template <class T> inline
std::wstring translate(const std::wstring& singular, const std::wstring& plural, T n)
{
    static_assert(sizeof(n) <= sizeof(std::int64_t), "");
    return translate(singular, plural, static_cast<std::int64_t>(n));
}


inline
std::shared_ptr<const TranslationHandler>*& getTranslationInstance()
{
    //avoid static destruction order fiasco: there may be accesses to "getTranslator()" during process shutdown
	//e.g. show message in debug_minidump.cpp or some detached thread assembling an error message!
    //=> use POD instead of a plain std::shared_ptr<>!!!
    static std::shared_ptr<const TranslationHandler>* inst = nullptr; //external linkage even in header!
    return inst;
}


struct CleanUpTranslationHandler
{
    ~CleanUpTranslationHandler()
    {
        std::shared_ptr<const TranslationHandler>*& handler = getTranslationInstance();
        assert(!handler); //clean up at a better time rather than during static destruction! MT issues!
		auto oldHandler = handler;
        handler = nullptr; //getTranslator() may be called even after static objects of this translation unit are destroyed!
        delete oldHandler;
    }
};
}

//setTranslator/getTranslator() operating on a global are obviously racy for MT usage
//=> make them fast to cover the rare case of a language change and the not-so-rare case of language clean-up during shutdown
//=> can't synchronize with std::mutex which is non-POD and again leads to global destruction order fiasco
//=> return std::shared_ptr to let instance life time be handled by caller (MT!)

inline
void setTranslator(std::unique_ptr<const TranslationHandler>&& newHandler)
{
    static implementation::CleanUpTranslationHandler cuth; //external linkage even in header!

    std::shared_ptr<const TranslationHandler>*& handler = implementation::getTranslationInstance();
	auto oldHandler = handler;
	handler = nullptr;
    delete oldHandler;
	if (newHandler)
		handler = new std::shared_ptr<const TranslationHandler>(std::move(newHandler));
}


inline
std::shared_ptr<const TranslationHandler> getTranslator()
{
	std::shared_ptr<const TranslationHandler>*& handler = implementation::getTranslationInstance();
	if (handler)
		return *handler;
	return nullptr;
}
}

#endif //I18_N_H_3843489325044253425456
