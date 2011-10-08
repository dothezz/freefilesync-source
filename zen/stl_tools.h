// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef STL_TOOLS_HEADER_84567184321434
#define STL_TOOLS_HEADER_84567184321434

//no need to drag in any STL includes :)

namespace zen
{
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


// binary search returning an iterator
template <class ForwardIterator, class T, typename Compare> inline
ForwardIterator custom_binary_search(ForwardIterator first, ForwardIterator last, const T& value, Compare comp)
{
    first = std::lower_bound(first, last, value, comp);
    if (first != last && !comp(value, *first))
        return first;
    else
        return last;
}
}


#endif //STL_TOOLS_HEADER_84567184321434