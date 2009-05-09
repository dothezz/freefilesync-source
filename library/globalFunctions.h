#ifndef GLOBALFUNCTIONS_H_INCLUDED
#define GLOBALFUNCTIONS_H_INCLUDED

#include <string>
#include <algorithm>
#include <vector>
#include <set>
#include <wx/string.h>
#include <wx/stream.h>
#include <wx/stopwatch.h>
#include <wx/longlong.h>

namespace globalFunctions
{
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

    std::string numberToString(const unsigned int number); //convert number to string
    std::string numberToString(const int number);          //convert number to string
    std::string numberToString(const long number);         //convert number to string
    std::string numberToString(const float number);        //convert number to string

    wxString numberToWxString(const unsigned int number); //convert number to wxString
    wxString numberToWxString(const int number);          //convert number to wxString
    wxString numberToWxString(const float number);        //convert number to wxString

    int    stringToInt(   const std::string& number); //convert String to number
    long   stringToLong(  const std::string& number); //convert String to number
    double stringToDouble(const std::string& number); //convert String to number

    int    wxStringToInt(   const wxString& number); //convert wxString to number
    double wxStringToDouble(const wxString& number); //convert wxString to number

    wxString includeNumberSeparator(const wxString& number);

    int readInt(std::ifstream& stream);  //read int from file stream
    void writeInt(std::ofstream& stream, const int number);  //write int to filestream

    int readInt(wxInputStream& stream);  //read int from file stream
    void writeInt(wxOutputStream& stream, const int number);  //write int to filestream

    inline
    wxLongLong convertToSigned(const wxULongLong number)
    {
        return wxLongLong(number.GetHi(), number.GetLo());
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
    wxStopWatch timer;
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


//############################################################################
class RuntimeException //Exception class used to notify of general runtime exceptions
{
public:
    RuntimeException(const wxString& txt) : errorMessage(txt) {}

    wxString show() const
    {

        return errorMessage;
    }

private:
    wxString errorMessage;
};


//Note: the following lines are a performance optimization for deleting elements from a vector. It is incredibly faster to create a new
//vector and leave specific elements out than to delete row by row and force recopying of most elements for each single deletion (linear vs quadratic runtime)
template <class T>
void removeRowsFromVector(std::vector<T>& grid, const std::set<int>& rowsToRemove)
{
    if (rowsToRemove.size() > 0)
    {
        std::vector<T> temp;

        std::set<int>::iterator rowToSkipIndex = rowsToRemove.begin();
        int rowToSkip = *rowToSkipIndex;

        for (int i = 0; i < int(grid.size()); ++i)
        {
            if (i != rowToSkip)
                temp.push_back(grid[i]);
            else
            {
                ++rowToSkipIndex;
                if (rowToSkipIndex != rowsToRemove.end())
                    rowToSkip = *rowToSkipIndex;
            }
        }
        grid.swap(temp);
    }
}


#endif // GLOBALFUNCTIONS_H_INCLUDED
