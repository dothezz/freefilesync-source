// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License, Version 1.0. See accompanying file             *
// * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.       *
// * Copyright (C) 2011 ZenJu (zhnmju123 AT gmx.de)                         *
// **************************************************************************

#ifndef TYPE_TOOLS_HEADER_45237590734254545
#define TYPE_TOOLS_HEADER_45237590734254545

namespace zen
{
//########## Strawman Classes ##########################
struct EmptyType {};
struct NullType {};

//########## Type Mapping ##############################
template <int n>
struct Int2Type {};
//------------------------------------------------------
template <class T>
struct Type2Type {};

//########## Control Structures ########################
template <bool flag, class T, class U>
struct Select
{
    typedef T Result;
};
template <class T, class U>
struct Select<false, T, U>
{
    typedef U Result;
};
//------------------------------------------------------
template <class T, class U>
struct IsSameType
{
    enum { result = false };
};

template <class T>
struct IsSameType<T, T>
{
    enum { result = true };
};

//########## Type Cleanup ##############################
template <class T>
struct RemoveRef { typedef T Result; };

template <class T>
struct RemoveRef<T&> { typedef T Result; };
//------------------------------------------------------
template <class T>
struct RemoveConst { typedef T Result; };

template <class T>
struct RemoveConst<const T> { typedef T Result; };
//------------------------------------------------------
template <class T>
struct RemovePointer { typedef T Result; };

template <class T>
struct RemovePointer<T*> { typedef T Result; };
//------------------------------------------------------
template <class T>
struct RemoveArray { typedef T Result; };

template <class T, int N>
struct RemoveArray<T[N]> { typedef T Result; };
}

#endif //TYPE_TOOLS_HEADER_45237590734254545
