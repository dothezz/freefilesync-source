#ifndef GLOBALFUNCTIONS_H_INCLUDED
#define GLOBALFUNCTIONS_H_INCLUDED

#include <string>
#include <algorithm>
#include <wx/string.h>
#include <stdexcept> //for std::runtime_error

using namespace std;

extern const wxChar FileNameSeparator;
extern const wxChar* FloatingPointSeparator;
extern const wxChar* NumberSeparator;

namespace GlobalFunctions
{
    int round(double d); //little rounding function

    int          abs(const int d);          //absolute value
    unsigned int abs(const unsigned int d); //absolute value
    float        abs(const float d);        //absolute value
    double       abs(const double d);       //absolute value

    string numberToString(const unsigned int number); //Convert number to string
    string numberToString(const int number);          //Convert number to string
    string numberToString(const float number);        //Convert number to string

    wxString numberToWxString(const unsigned int number); //Convert number to wxString
    wxString numberToWxString(const int number);          //Convert number to wxString
    wxString numberToWxString(const float number);        //Convert number to wxString

    int    stringToInt(   const string& number); //Convert String to number
    double stringToDouble(const string& number); //Convert String to number

    int    wxStringToInt(   const wxString& number); //Convert wxString to number
    double wxStringToDouble(const wxString& number); //Convert wxString to number

    wxString& includeNumberSeparator(wxString& number);
}
#endif // GLOBALFUNCTIONS_H_INCLUDED
