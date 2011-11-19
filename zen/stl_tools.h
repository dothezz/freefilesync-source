// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License, Version 1.0. See accompanying file             *
// * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.       *
// * Copyright (C) 2011 ZenJu (zhnmju123 AT gmx.de)                         *
// **************************************************************************

#ifndef STL_TOOLS_HEADER_84567184321434
#define STL_TOOLS_HEADER_84567184321434

//no need to drag in any STL includes


//enhancements for <algorithm>
namespace zen
{
//idomatic remove selected elements from container
template <class V, class Predicate>
void vector_remove_if(V& vec, Predicate p);

template <class S, class Predicate>
void set_remove_if(S& set, Predicate p);

template <class M, class Predicate>
void map_remove_if(M& map, Predicate p);

//binary search returning an iterator
template <class ForwardIterator, class T, typename Compare>
ForwardIterator binary_search(ForwardIterator first, ForwardIterator last, const T& value, Compare comp);

template <class BidirectionalIterator, class T>
BidirectionalIterator find_last(BidirectionalIterator first, BidirectionalIterator last, const T& value);

//replacement for std::find_end taking advantage of bidirectional iterators (and giving the algorithm a reasonable name)
template <class BidirectionalIterator1, class BidirectionalIterator2>
BidirectionalIterator1 search_last(BidirectionalIterator1 first1, BidirectionalIterator1 last1,
                                   BidirectionalIterator2 first2, BidirectionalIterator2 last2);


























//######################## implementation ########################

template <class V, class Predicate> inline
void vector_remove_if(V& vec, Predicate p)
{
    vec.erase(std::remove_if(vec.begin(), vec.end(), p), vec.end());
}


template <class S, class Predicate> inline
void set_remove_if(S& set, Predicate p)
{
    for (auto iter = set.begin(); iter != set.end();)
        if (p(*iter))
            set.erase(iter++);
        else
            ++iter;
}


template <class M, class Predicate> inline
void map_remove_if(M& map, Predicate p) { set_remove_if(map, p); }


template <class ForwardIterator, class T, typename Compare> inline
ForwardIterator binary_search(ForwardIterator first, ForwardIterator last, const T& value, Compare comp)
{
    first = std::lower_bound(first, last, value, comp);
    if (first != last && !comp(value, *first))
        return first;
    else
        return last;
}


template <class BidirectionalIterator, class T> inline
BidirectionalIterator find_last(const BidirectionalIterator first, BidirectionalIterator last, const T& value)
{
    //reverse iteration: 1. check 2. decrement 3. evaluate
    const BidirectionalIterator iterNotFound = last;
    for (;;) //VS 2010 doesn't like "while (true)"
    {
        if (last == first)
            return iterNotFound;
        --last;

        if (*last == value)
            return last;
    }
}


template <class BidirectionalIterator1, class BidirectionalIterator2> inline
BidirectionalIterator1 search_last(const BidirectionalIterator1 first1, BidirectionalIterator1 last1,
                                   const BidirectionalIterator2 first2, BidirectionalIterator2 last2)
{
    const BidirectionalIterator1 iterNotFound = last1;

    //reverse iteration: 1. check 2. decrement 3. evaluate
    for (;;)
    {
        BidirectionalIterator1 it1 = last1;
        BidirectionalIterator2 it2 = last2;

        for (;;)
        {
            if (it2 == first2) return it1;
            if (it1 == first1) return iterNotFound;

            --it1;
            --it2;

            if (*it1 != *it2) break;
        }
        --last1;
    }
}
}

#endif //STL_TOOLS_HEADER_84567184321434
