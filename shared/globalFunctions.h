// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
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
#include <fstream>

class wxStopWatch;


namespace globalFunctions
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
    return(d < 0 ? -d : d);
}

template <class T>
inline
std::string numberToString(const T& number) //convert number to string the C++ way
{
    std::stringstream ss;
    ss << number;
    return ss.str();
}


int    stringToInt(   const std::string& number); //convert String to number
long   stringToLong(  const std::string& number); //convert String to number
double stringToDouble(const std::string& number); //convert String to number

int    wxStringToInt(   const wxString& number); //convert wxString to number
double wxStringToDouble(const wxString& number); //convert wxString to number

unsigned int getDigitCount(const unsigned int number); //count number of digits

//read/write numbers: int, long, unsigned int ... ect
template <class T> T readNumber(std::ifstream& stream);
template <class T> void writeNumber(std::ofstream& stream, T number);

inline
wxLongLong convertToSigned(const wxULongLong number)
{
    return wxLongLong(number.GetHi(), number.GetLo());
}


//Note: the following lines are a performance optimization for deleting elements from a vector: linear runtime at most!
template <class T>
void removeRowsFromVector(const std::set<unsigned int>& rowsToRemove, std::vector<T>& grid)
{
    if (rowsToRemove.empty())
        return;

    std::set<unsigned int>::const_iterator rowToSkipIndex = rowsToRemove.begin();
    unsigned int rowToSkip = *rowToSkipIndex;

    if (rowToSkip >= grid.size())
        return;

    typename std::vector<T>::iterator insertPos = grid.begin() + rowToSkip;

    for (unsigned int i = rowToSkip; i < grid.size(); ++i)
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

//bubble sort using swap() instead of assignment: useful if assignment is very expensive
template <class VectorData, typename CompareFct>
void bubbleSwapSort(VectorData& folderCmp, CompareFct compare)
{
    for (int i = folderCmp.size() - 2; i >= 0; --i)
    {
        bool swapped = false;
        for (int j = 0; j <= i; ++j)
            if (compare(folderCmp[j + 1], folderCmp[j]))
            {
                folderCmp[j + 1].swap(folderCmp[j]);
                swapped = true;
            }

        if (!swapped)
            return;
    }
}

//enhanced binary search template: returns an iterator
template <class ForwardIterator, class T>
inline
ForwardIterator custom_binary_search(ForwardIterator first, ForwardIterator last, const T& value)
{
    first = lower_bound(first, last, value);
    if (first != last && !(value < *first))
        return first;
    else
        return last;
}

//enhanced binary search template: returns an iterator
template <class ForwardIterator, class T, typename Compare>
inline
ForwardIterator custom_binary_search(ForwardIterator first, ForwardIterator last, const T& value, Compare comp)
{
    first = lower_bound(first, last, value, comp);
    if (first != last && !comp(value, *first))
        return first;
    else
        return last;
}
}


//############################################################################
class Performance
{
public:
    wxDEPRECATED(Performance()); //generate compiler warnings as a reminder to remove code after measurements
    ~Performance();
    void showResult();

private:
    bool resultWasShown;
    std::auto_ptr<wxStopWatch> timer;
};

//two macros for quick performance measurements
#define PERF_START Performance a;
#define PERF_STOP  a.showResult();


//############################################################################
class wxFile;
class DebugLog
{
public:
    wxDEPRECATED(DebugLog());
    ~DebugLog();
    void write(const wxString& logText);

private:
    wxString assembleFileName();
    wxString logfileName;
    int lineCount;
    wxFile* logFile; //logFile.close(); <- not needed
};
extern DebugLog logDebugInfo;
wxString getCodeLocation(const wxString file, const int line);

//small macro for writing debug information into a logfile
#define WRITE_DEBUG_LOG(x) logDebugInfo.write(getCodeLocation(__TFILE__, __LINE__) + x);
//speed alternative: wxLogDebug(wxT("text")) + DebugView















//---------------Inline Implementation---------------------------------------------------
template <class T>
inline
T globalFunctions::readNumber(std::ifstream& stream)
{
    T result = 0;
    stream.read(reinterpret_cast<char*>(&result), sizeof(T));
    return result;
}


template <class T>
inline
void globalFunctions::writeNumber(std::ofstream& stream, T number)
{
    stream.write(reinterpret_cast<const char*>(&number), sizeof(T));
}


inline
int globalFunctions::stringToInt(const std::string& number)
{
    return atoi(number.c_str());
}


inline
long globalFunctions::stringToLong(const std::string& number)
{
    return atol(number.c_str());
}


inline
double globalFunctions::stringToDouble(const std::string& number)
{
    return atof(number.c_str());
}


#endif // GLOBALFUNCTIONS_H_INCLUDED
