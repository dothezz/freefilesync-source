// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef GLOBALFUNCTIONS_H_INCLUDED
#define GLOBALFUNCTIONS_H_INCLUDED

#include <string>
#include <algorithm>
#include <vector>
#include <set>
#include <wx/string.h>


namespace common
{
//little rounding function
inline int round(double d) { return static_cast<int>(d < 0 ? d - 0.5 : d + 0.5); }

//absolute value
template <class T> inline T abs(const T& d) { return d < 0 ? -d : d; }

size_t getDigitCount(size_t number); //count number of digits

//Note: the following lines are a performance optimization for deleting elements from a vector: linear runtime at most!
template <class T>
void removeRowsFromVector(const std::set<size_t>& rowsToRemove, std::vector<T>& grid);

//enhanced binary search template: returns an iterator
template <class ForwardIterator, class T, typename Compare>
ForwardIterator custom_binary_search(ForwardIterator first, ForwardIterator last, const T& value, Compare comp = std::less<T>());
}


//############################################################################





















//---------------Inline Implementation---------------------------------------------------
inline
size_t common::getDigitCount(size_t number) //count number of digits
{
    return number == 0 ? 1 : static_cast<size_t>(::log10(static_cast<double>(number))) + 1;
}


//Note: the following lines are a performance optimization for deleting elements from a vector: linear runtime at most!
template <class T>
void common::removeRowsFromVector(const std::set<size_t>& rowsToRemove, std::vector<T>& grid)
{
    if (rowsToRemove.empty())
        return;

    std::set<size_t>::const_iterator rowToSkipIndex = rowsToRemove.begin();
    size_t rowToSkip = *rowToSkipIndex;

    if (rowToSkip >= grid.size())
        return;

    typename std::vector<T>::iterator insertPos = grid.begin() + rowToSkip;

    for (size_t i = rowToSkip; i < grid.size(); ++i)
    {
        if (i != rowToSkip)
        {
            *insertPos = grid[i];
            ++insertPos;
        }
        else
        {
            ++rowToSkipIndex;
            if (rowToSkipIndex != rowsToRemove.end())
                rowToSkip = *rowToSkipIndex;
        }
    }
    grid.erase(insertPos, grid.end());
}


//enhanced binary search template: returns an iterator
template <class ForwardIterator, class T, typename Compare>
inline
ForwardIterator common::custom_binary_search(ForwardIterator first, ForwardIterator last, const T& value, Compare comp)
{
    first = std::lower_bound(first, last, value, comp);
    if (first != last && !comp(value, *first))
        return first;
    else
        return last;
}

#endif // GLOBALFUNCTIONS_H_INCLUDED
