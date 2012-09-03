// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License, Version 1.0. See accompanying file             *
// * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef STRING_TRAITS_HEADER_813274321443234
#define STRING_TRAITS_HEADER_813274321443234

#include "type_tools.h"
#include "assert_static.h"


//uniform access to string-like types, both classes and character arrays
namespace zen
{
/*
IsStringLike<>::value:
    IsStringLike<const wchar_t*>::value; //equals "true"
	IsStringLike<const int*>    ::value; //equals "false"

GetCharType<>::Type:
	GetCharType<std::wstring>::Type  //equals wchar_t
	GetCharType<wchar_t[5]>  ::Type  //equals wchar_t

strBegin():
	std::wstring str(L"dummy");
	char array[] = "dummy";
	strBegin(str);   //returns str.c_str()
	strBegin(array); //returns array

strLength():
	strLength(str);   //equals str.length()
	strLength(array); //equals cStringLength(array)
*/

//reference a sub-string or a char* as an intermediate string class when the length is already known
template <class Char>
class StringProxy
{
public:
    StringProxy(const Char* cstr, size_t len              ) : cstr_(cstr),  length_(len) {}
    StringProxy(const Char* cstrBegin, const Char* cstrEnd) : cstr_(cstrBegin),  length_(cstrEnd - cstrBegin) {}

    const Char* c_str() const { return cstr_; }
    size_t length() const { return length_; }

private:
    const Char* cstr_;
    size_t length_;
};













//---------------------- implementation ----------------------
namespace implementation
{
template<class S, class Char> //test if result of S::c_str() can convert to const Char*
class HasConversion
{
    typedef char Yes[1];
    typedef char No [2];

    static Yes& hasConversion(const Char*);
    static  No& hasConversion(...);

    static S& createInstance();

public:
    enum { value = sizeof(hasConversion(createInstance().c_str())) == sizeof(Yes) };
};


template <class S, bool isStringClass>  struct GetCharTypeImpl : ResultType<NullType> {};
template <class S>
struct GetCharTypeImpl<S, true> :
        ResultType<
        typename SelectIf<HasConversion<S, wchar_t>::value, wchar_t,
        typename SelectIf<HasConversion<S, char   >::value, char, NullType>::Type
        >::Type>
{
    //typedef typename S::value_type Type;
    /*DON'T use S::value_type:
        1. support Glib::ustring: value_type is "unsigned int" but c_str() returns "const char*"
        2. wxString, wxWidgets v2.9, has some questionable string design: wxString::c_str() returns a proxy (wxCStrData) which
           is implicitly convertible to *both* "const char*" and "const wchar_t*" while wxString::value_type is a wrapper around an unsigned int
    */
};

template <> struct GetCharTypeImpl<char,    false> : ResultType<char   > {};
template <> struct GetCharTypeImpl<wchar_t, false> : ResultType<wchar_t> {};

ZEN_INIT_DETECT_MEMBER_TYPE(value_type);
ZEN_INIT_DETECT_MEMBER(c_str);  //we don't know the exact declaration of the member attribute and it may be in a base class!
ZEN_INIT_DETECT_MEMBER(length); //

template <class S>
class StringTraits
{
    typedef typename RemoveRef    <S           >::Type NonRefType;
    typedef typename RemoveConst  <NonRefType  >::Type NonConstType;
    typedef typename RemoveArray  <NonConstType>::Type NonArrayType;
    typedef typename RemovePointer<NonArrayType>::Type NonPtrType;
    typedef typename RemoveConst  <NonPtrType  >::Type UndecoratedType; //handle "const char* const"

public:
    enum
    {
        isStringClass = HasMemberType_value_type<NonConstType>::value &&
                        HasMember_c_str         <NonConstType>::value &&
                        HasMember_length        <NonConstType>::value
    };

    typedef typename GetCharTypeImpl<UndecoratedType, isStringClass>::Type CharType;

    enum
    {
        isStringLike = IsSameType<CharType, char>::value ||
        IsSameType<CharType, wchar_t>::value
    };
};
}

template <class T>
struct IsStringLike : StaticBool<implementation::StringTraits<T>::isStringLike> {};

template <class T>
struct GetCharType : ResultType<typename implementation::StringTraits<T>::CharType> {};


namespace implementation
{
template <class C> inline
size_t cStringLength(const C* str) //strlen()
{
    assert_static((IsSameType<C, char>::value || IsSameType<C, wchar_t>::value));
    size_t len = 0;
    while (*str++ != 0)
        ++len;
    return len;
}
}


template <class S> inline
const typename GetCharType<S>::Type* strBegin(const S& str, typename EnableIf<implementation::StringTraits<S>::isStringClass>::Type* = nullptr) //SFINAE: T must be a "string"
{
    return str.c_str();
}

inline const char*    strBegin(const char*    str) { return str; }
inline const wchar_t* strBegin(const wchar_t* str) { return str; }
inline const char*    strBegin(const char&    ch)  { return &ch; }
inline const wchar_t* strBegin(const wchar_t& ch)  { return &ch; }


template <class S> inline
size_t strLength(const S& str, typename EnableIf<implementation::StringTraits<S>::isStringClass>::Type* = nullptr) //SFINAE: T must be a "string"
{
    return str.length();
}

inline size_t strLength(const char*    str) { return implementation::cStringLength(str); }
inline size_t strLength(const wchar_t* str) { return implementation::cStringLength(str); }
inline size_t strLength(char)            { return 1; }
inline size_t strLength(wchar_t)         { return 1; }
}

#endif //STRING_TRAITS_HEADER_813274321443234
