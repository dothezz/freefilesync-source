// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License, Version 1.0. See accompanying file             *
// * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.       *
// * Copyright (C) 2011 ZenJu (zhnmju123 AT gmx.de)                         *
// **************************************************************************

#ifndef TYPE_TRAITS_HEADER_3425628658765467
#define TYPE_TRAITS_HEADER_3425628658765467

namespace zen
{
//Example: "IsSignedInt<int>::result" evaluates to "true"

template <class T> struct IsUnsignedInt;
template <class T> struct IsSignedInt;
template <class T> struct IsFloat;

template <class T> struct IsInteger;    //IsSignedInt or IsUnsignedInt
template <class T> struct IsArithmetic; //IsInteger or IsFloat
//remaining non-arithmetic types: bool, char, wchar_t

//optional: specialize new types like:
//template <> struct IsUnsignedInt<UInt64> { enum { result = true }; };























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
}

#endif //TYPE_TRAITS_HEADER_3425628658765467
