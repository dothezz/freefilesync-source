#ifndef GLOBALFUNCTIONS_H_INCLUDED
#define GLOBALFUNCTIONS_H_INCLUDED

#include <string>
#include <algorithm>
#include <wx/string.h>
#include <fstream>
#include <wx/stream.h>

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
