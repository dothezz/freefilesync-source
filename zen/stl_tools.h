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

template<class BidirectionalIterator, class T>
BidirectionalIterator find_last(BidirectionalIterator first, BidirectionalIterator last, const T& value);

//replacement for std::find_end taking advantage of bidirectional iterators (and giving the algorithm a reasonable name)
template<class BidirectionalIterator1, class BidirectionalIterator2>
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


template<class BidirectionalIterator, class T> inline
BidirectionalIterator find_last(const BidirectionalIterator first, BidirectionalIterator last, const T& value)
{
    const BidirectionalIterator iterNotFound = last;
    do
    {
        if (last == first)
            return iterNotFound;
        --last;
    }
    while (!(*last == value)); //loop over "last":  1. check 2. decrement 3. evaluate
    return last;
}


template<class BidirectionalIterator1, class BidirectionalIterator2> inline
BidirectionalIterator1 search_last(const BidirectionalIterator1 first1, BidirectionalIterator1 last1,
                                   const BidirectionalIterator2 first2, BidirectionalIterator2 last2)
{
    if (first1 == last1 ||
        first2 == last2)
        return last1;

    int diff = static_cast<int>(std::distance(first1, last1)) -
               static_cast<int>(std::distance(first2, last2));

    const BidirectionalIterator1 iterNotFound = last1;
    --last2;

    while (diff-- >= 0) //loop over "last1":  1. check 2. decrement 3. evaluate
    {
        --last1;

        BidirectionalIterator1 iter1 = last1;
        for (BidirectionalIterator2 iter2 = last2; *iter1 == *iter2; --iter1, --iter2)
            if (iter2 == first2)
                return iter1;
    }
    return iterNotFound;
}
}

#endif //STL_TOOLS_HEADER_84567184321434
