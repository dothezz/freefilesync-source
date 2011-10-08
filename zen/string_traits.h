// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License, Version 1.0. See accompanying file             *
// * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.       *
// * Copyright (C) 2011 ZenJu (zhnmju123 AT gmx.de)                         *
// **************************************************************************

#ifndef STRING_TRAITS_HEADER_813274321443234
#define STRING_TRAITS_HEADER_813274321443234

#include "type_tools.h"
#include "assert_static.h"


//uniform access to string-like types: classes and character arrays
namespace zen
{
/*
strBegin():
	std::wstring str(L"dummy");
	char array[] = "dummy";
	const wchar_t* iter  = strBegin(str);   //returns str.c_str()
	const char*    iter2 = strBegin(array); //returns array

strLength():
	strLength(str);   //equals str.size()
	strLength(array); //equals cStringLength(array)

StringTraits<>::CharType:
	StringTraits<std::wstring>::CharType  //equals wchar_t
	StringTraits<wchar_t[5]>  ::CharType  //equals wchar_t

StringTraits<>::isStringLike:
    StringTraits<const wchar_t*>::isStringLike; //equals "true"
	StringTraits<const int*>    ::isStringLike; //equals "false"

StringTraits<>::isStringClass:
	StringTraits<std::wstring>::isStringClass  //equals "true"
	StringTraits<wchar_t[5]>  ::isStringClass  //equals "false"
*/













//---------------------- implementation ----------------------
namespace implementation
{
template<typename T>
class HasValueTypedef
{
    typedef char Yes[1];
    typedef char No [2];

    template <typename U> class HelperTp {};

    //detect presence of a member type called value_type
    template <class U> static Yes& hasMemberValueType(HelperTp<typename U::value_type>*);
    template <class U> static  No& hasMemberValueType(...);

public:
    enum { result = sizeof(hasMemberValueType<T>(NULL)) == sizeof(Yes)
         };
};


template<typename T, bool isClassType>
class HasStringMembers
{
public:
    enum { result = false };
};

template<typename T>
class HasStringMembers<T, true>
{
    typedef char Yes[1];
    typedef char No [2];

    //detect presence of member functions (without specific restriction on return type, within T or one of it's base classes)
    template <typename U, U t> class HelperFn {};

    struct Fallback
    {
        int c_str;
        int length;
    };

    template <class U>
    struct Helper2 : public U, public Fallback {}; //U must be a class-type!

    //we don't know the exact declaration of the member attribute (may be in base class), but we know what NOT to expect:
    template <class U> static  No& hasMemberCstr(HelperFn<int Fallback::*, &Helper2<U>::c_str>*);
    template <class U> static Yes& hasMemberCstr(...);

    template <class U> static  No& hasMemberLength(HelperFn<int Fallback::*, &Helper2<U>::length>*);
    template <class U> static Yes& hasMemberLength(...);
public:
    enum { result = sizeof(hasMemberCstr  <T>(NULL)) == sizeof(Yes) &&
                    sizeof(hasMemberLength<T>(NULL)) == sizeof(Yes)
         };
};

template <class S, bool isStringClass> struct StringTraits2 { typedef EmptyType Result; }; //"StringTraits2": fix some VS bug with namespace and partial template specialization

template <class S> struct StringTraits2<S,       true > { typedef typename S::value_type Result; };
template <>        struct StringTraits2<char,    false> { typedef char                   Result; };
template <>        struct StringTraits2<wchar_t, false> { typedef wchar_t                Result; };
}


template <class S>
struct StringTraits
{
private:
    typedef typename RemoveRef    <S           >::Result NonRefType;
    typedef typename RemoveConst  <NonRefType  >::Result NonConstType;
    typedef typename RemoveArray  <NonConstType>::Result NonArrayType;
    typedef typename RemovePointer<NonArrayType>::Result NonPtrType;
    typedef typename RemoveConst  <NonPtrType  >::Result UndecoratedType; //handle "const char* const"
public:
    enum
    {
        isStringClass = implementation::HasStringMembers<NonConstType, implementation::HasValueTypedef<NonConstType>::result>::result
    };

    typedef typename implementation::StringTraits2<UndecoratedType, isStringClass>::Result CharType;

    enum
    {
        isStringLike = IsSameType<CharType, char>::result || IsSameType<CharType, wchar_t>::result
    };
};


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
const typename StringTraits<S>::CharType* strBegin(const S& str, typename S::value_type dummy = 0) { return str.c_str(); } //SFINAE: T must be a "string"

template <class Char>
inline const typename StringTraits<Char>::CharType* strBegin(const Char* str) { return str; }
inline const char*    strBegin(const char& ch)    { return &ch; }
inline const wchar_t* strBegin(const wchar_t& ch) { return &ch; }


template <class S> inline
size_t strLength(const S& str, typename S::value_type dummy = 0) { return str.length(); } //SFINAE: T must be a "string"

template <class Char>
inline size_t strLength(const Char* str) { return implementation::cStringLength(str); }
inline size_t strLength(char)            { return 1; }
inline size_t strLength(wchar_t)         { return 1; }
}

#endif //STRING_TRAITS_HEADER_813274321443234
