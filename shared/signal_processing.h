// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include <algorithm>
#include <limits>
#include <numeric>


namespace util
{
template <class T>
T abs(T value);

int round(double d); //little rounding function

template <class T>
bool isNull(T number);

//----------------------------------------------------------------------------------
//       smoothen data ranges through a window around each data point              |
//----------------------------------------------------------------------------------
template <class InputIterator, class OutputIterator>
void smoothen(InputIterator first, InputIterator last, OutputIterator result, size_t windowSize); //default implementation: averaging

template <class InputIterator, class OutputIterator, class DataProcessor>
void smoothen(InputIterator first, InputIterator last, OutputIterator result, size_t windowSize, DataProcessor proc);
/*
DataProcessor is an abstraction for data evaluation. A valid implementation needs to support three properties:
- add data entry at window front:   operator+=(ValueType value)
- remove data entry at window back: operator-=(ValueType value)
- evaluate smoothed middle value:   ValueType operator()(size_t windowSize)
*/
//----------------------------------------------------------------------------------






















//################# inline implementation #########################
template <class T>
inline
T abs(T value)
{
    return value < 0 ? -value : value;
}


template <class T>
inline
bool isNull(T value)
{
    return abs(number) <= std::numeric_limits<T>::epsilon(); //epsilon == 0 for integer types, therefore less-equal(!)
}


inline
int round(double d)
{
    return static_cast<int>(d < 0 ? d - .5 : d + .5);
}


template <class InputIterator, class OutputIterator, class DataProcessor>
inline
void smoothen(InputIterator first, InputIterator last, OutputIterator result, size_t windowSize, DataProcessor proc)
{
    windowSize = std::min(windowSize, static_cast<size_t>(last - first)); //std::distance() not used to enforce random access iterator requirement

    if (windowSize <= 1)
    {
        std::copy(first, last, result);
        return;
    }

    const size_t firstHalf = windowSize / 2;
    const size_t secondHalf = windowSize - firstHalf;

    //preparation
    for (InputIterator i = first; i != first + secondHalf; ++i)
        proc += *i;

    //beginning
    for (InputIterator i = first; i != first + firstHalf; ++i)
    {
        *result++ = proc(i - first + secondHalf);
        proc += *(i + secondHalf);
    }

    //main
    for (InputIterator i = first + firstHalf; i != last - secondHalf; ++i)
    {
        *result++ = proc(windowSize);
        proc += *(i + secondHalf);
        proc -= *(i - firstHalf);
    }

    //ending
    for (InputIterator i = last - secondHalf; i != last; ++i)
    {
        *result++ = proc(windowSize - (i - last + secondHalf));
        proc -= *(i - firstHalf);
    }
}


template <class ValueType>
class ProcessorAverage
{
public:
    ProcessorAverage() : valueAcc() {}

    //add front data entry
    ProcessorAverage& operator+=(ValueType value)
    {
        valueAcc += value;
        return *this;
    }

    //remove rear data entry
    ProcessorAverage& operator-=(ValueType value)
    {
        valueAcc -= value;
        return *this;
    }

    //evaluate smoothed value
    ValueType operator()(size_t windowSize) const
    {
        return valueAcc / windowSize;
    }

private:
    ValueType valueAcc; //accumulated values
};



template <class InputIterator, class OutputIterator>
inline
void smoothen(InputIterator first, InputIterator last, OutputIterator result, size_t windowSize)
{
    typedef typename std::iterator_traits<InputIterator>::value_type ValueType;
    smoothen(first, last, result, windowSize, ProcessorAverage<ValueType>());
}

}