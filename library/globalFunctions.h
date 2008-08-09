#ifndef GLOBALFUNCTIONS_H_INCLUDED
#define GLOBALFUNCTIONS_H_INCLUDED

#include <string>
#include <algorithm>

using namespace std;

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

    int    StringToInt(   const string& number); //Convert String to number
    double StringToDouble(const string& number); //Convert String to number

    string& includeNumberSeparator(string& number);
}
#endif // GLOBALFUNCTIONS_H_INCLUDED
