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
#include <wx/longlong.h>
#include <memory>
#include <sstream>

class wxStopWatch;


namespace common
{
//------------------------------------------------
//      FUNCTIONS
//------------------------------------------------
inline
int round(double d) //little rounding function
{
    return static_cast<int>(d < 0 ? d - .5 : d + .5);
}

template <class T>
inline
T abs(const T& d) //absolute value
{
    return d < 0 ? -d : d;
}


//formatted number conversion C++ ANSI/wide char versions
template <class CharType, class T>
std::basic_string<CharType> numberToString(const T& number); //convert number to string the C++ way

template <class T, class CharType>
T stringToNumber(const std::basic_string<CharType>& input); //convert number to string the C++ way

//formatted number conversion wxWidgets
template <class T> wxString numberToString(const T& number);
template <class T> T stringToNumber(const wxString& input);


size_t getDigitCount(size_t number); //count number of digits

//serialization: read/write numbers: int, long, unsigned int ... ect
template <class T> T readNumber(std::istream& stream);
template <class T> void writeNumber(std::ostream& stream, T number);

inline
wxLongLong convertToSigned(const wxULongLong number)
{
    return wxLongLong(number.GetHi(), number.GetLo());
}


//Note: the following lines are a performance optimization for deleting elements from a vector: linear runtime at most!
template <class T>
void removeRowsFromVector(const std::set<size_t>& rowsToRemove, std::vector<T>& grid);

//enhanced binary search template: returns an iterator
template <class ForwardIterator, class T, typename Compare>
ForwardIterator custom_binary_search(ForwardIterator first, ForwardIterator last, const T& value, Compare comp = std::less<T>());
}


//############################################################################
class wxFile;
class DebugLog
{
public:
    wxDEPRECATED(DebugLog(const wxString& filePrefix = wxString()));
    ~DebugLog();
    void write(const wxString& logText);

private:
    wxString assembleFileName();

    wxString logfileName;
    wxString prefix;
    int lineCount;
    wxFile* logFile; //logFile.close(); <- not needed
};
extern DebugLog logDebugInfo;
wxString getCodeLocation(const wxString file, const int line);

//small macro for writing debug information into a logfile
#define WRITE_DEBUG_LOG(x) logDebugInfo.write(getCodeLocation(__TFILE__, __LINE__) + x);
//speed alternative: wxLogDebug(wxT("text")) + DebugView





















//---------------Inline Implementation---------------------------------------------------
template <class CharType, class T>
inline
std::basic_string<CharType> common::numberToString(const T& number) //convert number to string the C++ way
{
    std::basic_ostringstream<CharType> ss;
    ss << number;
    return ss.str();
}


template <class T, class CharType>
inline
T common::stringToNumber(const std::basic_string<CharType>& input) //convert number to string the C++ way
{
    T number = 0;
    std::basic_istringstream<CharType>(input) >> number;
    return number;
}


template <class T>
inline
wxString common::numberToString(const T& number)
{
    return numberToString<wxChar, T>(number).c_str();
}


template <class T>
inline
T common::stringToNumber(const wxString& input)
{
    const std::basic_string<wxChar> inputConv(input.c_str());
    return stringToNumber<T, wxChar>(inputConv);
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


template <class T>
inline
T common::readNumber(std::istream& stream)
{
    T result = 0;
    stream.read(reinterpret_cast<char*>(&result), sizeof(T));
    return result;
}


template <class T>
inline
void common::writeNumber(std::ostream& stream, T number)
{
    stream.write(reinterpret_cast<const char*>(&number), sizeof(T));
}


#endif // GLOBALFUNCTIONS_H_INCLUDED
