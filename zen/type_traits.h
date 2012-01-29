// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License, Version 1.0. See accompanying file             *
// * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef TYPE_TRAITS_HEADER_3425628658765467
#define TYPE_TRAITS_HEADER_3425628658765467

namespace zen
{
//################# Built-in Types  ########################
//Example: "IsSignedInt<int>::result" evaluates to "true"

template <class T> struct IsUnsignedInt;
template <class T> struct IsSignedInt;
template <class T> struct IsFloat;

template <class T> struct IsInteger;    //IsSignedInt or IsUnsignedInt
template <class T> struct IsArithmetic; //IsInteger or IsFloat
//remaining non-arithmetic types: bool, char, wchar_t

//optional: specialize new types like:
//template <> struct IsUnsignedInt<UInt64> { enum { result = true }; };

//################# Class Members ########################

/*  Detect data or function members of a class by name: ZEN_INIT_DETECT_MEMBER + HasMember_
	!!! Note: this may ONLY be used for class types: fails to compile for non-class types !!!

	Example: 1. ZEN_INIT_DETECT_MEMBER(c_str);
	         2. HasMember_c_str<T>::result     -> use as boolean
*/

/*  Detect data or function members of a class by name and type: ZEN_INIT_DETECT_MEMBER2 + HasMember_

	Example: 1. ZEN_INIT_DETECT_MEMBER2(size, size_t (T::*)() const);
	         2. HasMember_size<T>::result     -> use as boolean
*/

/*  Detect member type of a class: ZEN_INIT_DETECT_MEMBER_TYPE + HasMemberType_

	Example: 1. ZEN_INIT_DETECT_MEMBER_TYPE(value_type);
	         2. HasMemberType_value_type<T>::result     -> use as boolean
*/




















//################ implementation ######################
#define ZEN_SPECIALIZE_TRAIT(X, Y) template <> struct X<Y> { enum { result = true }; };

template <class T>
struct IsUnsignedInt { enum { result = false }; };

ZEN_SPECIALIZE_TRAIT(IsUnsignedInt, unsigned char);
ZEN_SPECIALIZE_TRAIT(IsUnsignedInt, unsigned short int);
ZEN_SPECIALIZE_TRAIT(IsUnsignedInt, unsigned int);
ZEN_SPECIALIZE_TRAIT(IsUnsignedInt, unsigned long int);
ZEN_SPECIALIZE_TRAIT(IsUnsignedInt, unsigned long long int); //new with C++11 - same type as unsigned __int64 in VS2010
//------------------------------------------------------

template <class T>
struct IsSignedInt { enum { result = false }; };

ZEN_SPECIALIZE_TRAIT(IsSignedInt, signed char);
ZEN_SPECIALIZE_TRAIT(IsSignedInt, short int);
ZEN_SPECIALIZE_TRAIT(IsSignedInt, int);
ZEN_SPECIALIZE_TRAIT(IsSignedInt, long int);
ZEN_SPECIALIZE_TRAIT(IsSignedInt, long long int); //new with C++11 - same type as __int64 in VS2010
//------------------------------------------------------

template <class T>
struct IsFloat { enum { result = false }; };

ZEN_SPECIALIZE_TRAIT(IsFloat, float);
ZEN_SPECIALIZE_TRAIT(IsFloat, double);
ZEN_SPECIALIZE_TRAIT(IsFloat, long double);
//------------------------------------------------------

#undef ZEN_SPECIALIZE_TRAIT

template <class T>
struct IsInteger { enum { result = IsUnsignedInt<T>::result || IsSignedInt<T>::result }; };

template <class T>
struct IsArithmetic { enum { result = IsInteger<T>::result || IsFloat<T>::result }; };
//####################################################################

#define ZEN_INIT_DETECT_MEMBER(NAME) 		\
    \
    template<typename T>					\
    class HasMember_##NAME					\
    {										\
        typedef char Yes[1];				\
        typedef char No [2];				\
        \
        template <typename U, U t> class Helper {};		\
        struct Fallback { int NAME; };					\
        \
        template <class U>								\
        struct Helper2 : public U, public Fallback {};	\
        \
        template <class U> static  No& hasMember(Helper<int Fallback::*, &Helper2<U>::NAME>*);	\
        template <class U> static Yes& hasMember(...);											\
    public:																						\
        enum { result = sizeof(hasMember<T>(NULL)) == sizeof(Yes) };							\
    };
//####################################################################

#define ZEN_INIT_DETECT_MEMBER2(NAME, TYPE) 		\
    \
    template<typename U> 							\
    class HasMember_##NAME 							\
    { 												\
        typedef char Yes[1]; 						\
        typedef char No [2]; 						\
        \
        template <typename T, T t> class Helper {}; \
        \
        template <class T> static Yes& hasMember(Helper<TYPE, &T::NAME>*); 	\
        template <class T> static  No& hasMember(...); 					  	\
    public:																	\
        enum { result = sizeof(hasMember<U>(NULL)) == sizeof(Yes) };		\
    };
//####################################################################

#define ZEN_INIT_DETECT_MEMBER_TYPE(TYPENAME)		\
    \
    template<typename T>						\
    class HasMemberType_##TYPENAME				\
    {											\
        typedef char Yes[1];					\
        typedef char No [2];					\
        \
        template <typename U> class Helper {};	\
        \
        template <class U> static Yes& hasMemberType(Helper<typename U::TYPENAME>*); \
        template <class U> static  No& hasMemberType(...); 							 \
    public:          																 \
        enum { result = sizeof(hasMemberType<T>(NULL)) == sizeof(Yes) };			 \
    };
}

#endif //TYPE_TRAITS_HEADER_3425628658765467
