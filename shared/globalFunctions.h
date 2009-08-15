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

class wxInputStream;
class wxOutputStream;
class wxStopWatch;


namespace globalFunctions
{
//------------------------------------------------
//      GLOBALS
//------------------------------------------------
#ifdef FFS_WIN
    const wxChar FILE_NAME_SEPARATOR = '\\';
    static const wxChar* const LINE_BREAK = wxT("\r\n"); //internal linkage
#elif defined FFS_LINUX
    const wxChar FILE_NAME_SEPARATOR = '/';
    static const wxChar* const LINE_BREAK = wxT("\n");
#endif

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
    inline std::string numberToString(const T& number) //convert number to string the C++ way
    {
        std::stringstream ss;
        ss << number;
        return ss.str();
    }

    wxString numberToWxString(const unsigned int number); //convert number to wxString
    wxString numberToWxString(const int number);          //convert number to wxString
    wxString numberToWxString(const float number);        //convert number to wxString

    int    stringToInt(   const std::string& number); //convert String to number
    long   stringToLong(  const std::string& number); //convert String to number
    double stringToDouble(const std::string& number); //convert String to number

    int    wxStringToInt(   const wxString& number); //convert wxString to number
    double wxStringToDouble(const wxString& number); //convert wxString to number

    unsigned int getDigitCount(const unsigned int number); //count number of digits

    int readInt(std::ifstream& stream);  //read int from file stream
    void writeInt(std::ofstream& stream, const int number);  //write int to filestream

    int readInt(wxInputStream& stream);  //read int from file stream
    void writeInt(wxOutputStream& stream, const int number);  //write int to filestream

    inline
    wxLongLong convertToSigned(const wxULongLong number)
    {
        return wxLongLong(number.GetHi(), number.GetLo());
    }


    //Note: the following lines are a performance optimization for deleting elements from a vector. It is incredibly faster to create a new
//vector and leave specific elements out than to delete row by row and force recopying of most elements for each single deletion (linear vs quadratic runtime)
    template <class T>
    void removeRowsFromVector(const std::set<int>& rowsToRemove, std::vector<T>& grid)
    {
        if (rowsToRemove.size() > 0)
        {
            std::vector<T> temp;

            std::set<int>::const_iterator rowToSkipIndex = rowsToRemove.begin();
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


#endif // GLOBALFUNCTIONS_H_INCLUDED