// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License, Version 1.0. See accompanying file             *
// * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.       *
// * Copyright (C) 2011 ZenJu (zhnmju123 AT gmx.de)                         *
// **************************************************************************

#ifndef STRING_TRAITS_HEADER_813274321443234
#define STRING_TRAITS_HEADER_813274321443234

#include "type_tools.h"
#include "type_traits.h"
#include "assert_static.h"


//uniform access to string-like types, both classes and character arrays
namespace zen
{
/*
IsStringLike<>::result:
    IsStringLike<const wchar_t*>::result; //equals "true"
	IsStringLike<const int*>    ::result; //equals "false"

GetCharType<>::Result:
	GetCharType<std::wstring>::Result  //equals wchar_t
	GetCharType<wchar_t[5]>  ::Result  //equals wchar_t

strBegin():
	std::wstring str(L"dummy");
	char array[] = "dummy";
	strBegin(str);   //returns str.c_str()
	strBegin(array); //returns array

strLength():
	strLength(str);   //equals str.length()
	strLength(array); //equals cStringLength(array)
*/

//wrap a sub-string or a char* as an intermediate string class when the length is already known
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
ZEN_INIT_DETECT_MEMBER(c_str)   //we don't know the exact declaration of the member attribute and it may be in a base class!
ZEN_INIT_DETECT_MEMBER(length)  //

template<typename T, bool isClassType>
struct HasStringMembers { enum { result = false }; };

template<typename T>
struct HasStringMembers<T, true> //Note: we can apply non-typed member-check on class types only!
{
    enum { result = HasMember_c_str <T>::result &&
                    HasMember_length<T>::result
         };
};


template<class S, class Char> //test if result of S::c_str() can convert to const Char*
class HasConversion
{
    typedef char Yes[1];
    typedef char No [2];

    static Yes& hasConversion(const Char*);
    static  No& hasConversion(...);

    static S createInstance();

public:
    enum { result = sizeof(hasConversion(createInstance().c_str())) == sizeof(Yes) };
};


template <class S, bool isStringClass>  struct GetCharTypeImpl { typedef EmptyType Result; };
template <class S>                      struct GetCharTypeImpl<S, true >
{
    //typedef typename S::value_type Result;
    /*DON'T use S::value_type:
        1. support Glib::ustring: value_type is "unsigned int" but c_str() returns "const char*"
        2. wxString, wxWidgets v2.9, has some questionable string design: wxString::c_str() returns a proxy (wxCStrData) which
           is implicitly convertible to *both* "const char*" and "const wchar_t*" while wxString::value_type is a wrapper around an unsigned int
    */
    typedef typename SelectIf<HasConversion<S, wchar_t>::result, wchar_t,
            typename SelectIf<HasConversion<S, char>::result, char, EmptyType>::Result
            >::Result Result;
};


template <> struct GetCharTypeImpl<char,    false> { typedef char    Result; };
template <> struct GetCharTypeImpl<wchar_t, false> { typedef wchar_t Result; };

ZEN_INIT_DETECT_MEMBER_TYPE(value_type);

template <class S>
class StringTraits
{
    typedef typename RemoveRef    <S           >::Result NonRefType;
    typedef typename RemoveConst  <NonRefType  >::Result NonConstType;
    typedef typename RemoveArray  <NonConstType>::Result NonArrayType;
    typedef typename RemovePointer<NonArrayType>::Result NonPtrType;
    typedef typename RemoveConst  <NonPtrType  >::Result UndecoratedType; //handle "const char* const"

public:
    enum
    {
        isStringClass = HasStringMembers<NonConstType, HasMemberType_value_type<NonConstType>::result>::result
    };

    typedef typename GetCharTypeImpl<UndecoratedType, isStringClass>::Result CharType;

    enum
    {
        isStringLike = IsSameType<CharType, char>::result ||
        IsSameType<CharType, wchar_t>::result
    };
};
}

template <class T>
struct IsStringLike { enum { result = implementation::StringTraits<T>::isStringLike }; };

template <class T>
struct GetCharType { typedef typename implementation::StringTraits<T>::CharType Result; };


namespace implementation
{
template <class C> inline
size_t cStringLength(const C* str) //strlen()
{
    assert_static((IsSameType<C, char>::result || IsSameType<C, wchar_t>::result));
    size_t len = 0;
    while (*str++ != 0)
        ++len;
    return len;
}
}


template <class S> inline
const typename GetCharType<S>::Result* strBegin(const S& str, typename EnableIf<implementation::StringTraits<S>::isStringClass>::Result* = NULL) //SFINAE: T must be a "string"
{
    return str.c_str();
}

inline const char*    strBegin(const char*    str) { return str; }
inline const wchar_t* strBegin(const wchar_t* str) { return str; }
inline const char*    strBegin(const char&    ch)  { return &ch; }
inline const wchar_t* strBegin(const wchar_t& ch)  { return &ch; }


template <class S> inline
size_t strLength(const S& str, typename EnableIf<implementation::StringTraits<S>::isStringClass>::Result* = NULL) //SFINAE: T must be a "string"
{
    return str.length();
}

inline size_t strLength(const char*    str) { return implementation::cStringLength(str); }
inline size_t strLength(const wchar_t* str) { return implementation::cStringLength(str); }
inline size_t strLength(char)            { return 1; }
inline size_t strLength(wchar_t)         { return 1; }
}

#endif //STRING_TRAITS_HEADER_813274321443234
