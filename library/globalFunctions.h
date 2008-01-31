#ifndef GLOBALFUNCTIONS_H_INCLUDED
#define GLOBALFUNCTIONS_H_INCLUDED

#include <string>
#include <algorithm>
#include <vector>
#include <set>
#include <wx/string.h>
#include <fstream>
#include <wx/stream.h>
#include <wx/stopwatch.h>

using namespace std;


namespace globalFunctions
{
    int round(double d); //little rounding function

    template <class T>
    T abs(const T& d)    //absolute value
    {
        return(d<0?-d:d);
    }

    string numberToString(const unsigned int number); //convert number to string
    string numberToString(const int number);          //convert number to string
    string numberToString(const float number);        //convert number to string

    wxString numberToWxString(const unsigned int number); //convert number to wxString
    wxString numberToWxString(const int number);          //convert number to wxString
    wxString numberToWxString(const float number);        //convert number to wxString

    int    stringToInt(   const string& number); //convert String to number
    double stringToDouble(const string& number); //convert String to number

    int    wxStringToInt(   const wxString& number); //convert wxString to number
    double wxStringToDouble(const wxString& number); //convert wxString to number

    wxString& includeNumberSeparator(wxString& number);

    int readInt(ifstream& stream);  //read int from file stream
    void writeInt(ofstream& stream, const int number);  //write int to filestream

    int readInt(wxInputStream& stream);  //read int from file stream
    void writeInt(wxOutputStream& stream, const int number);  //write int to filestream
}


//############################################################################
class Performance
{
public:
    wxDEPRECATED(Performance()); //generates compiler warnings as a reminder to remove code after measurements
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
void removeRowsFromVector(vector<T>& grid, const set<int>& rowsToRemove)
{
    vector<T> temp;
    int rowToSkip = -1; //keep it an INT!

    set<int>::iterator rowToSkipIndex = rowsToRemove.begin();

    if (rowToSkipIndex != rowsToRemove.end())
        rowToSkip = *rowToSkipIndex;

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


#endif // GLOBALFUNCTIONS_H_INCLUDED
