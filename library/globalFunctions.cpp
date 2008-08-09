#include "globalFunctions.h"
#include <wx/intl.h>

inline
int GlobalFunctions::round(const double d)
{
    return static_cast<int>(d<0?d-.5:d+.5);
}

inline
int GlobalFunctions::abs(const int d)
{
    return(d<0?-d:d);
}

inline
unsigned int GlobalFunctions::abs(const unsigned int d)
{
    return(d<0?-d:d);
}

inline
float GlobalFunctions::abs(const float d)
{
    return(d<0?-d:d);
};

inline
double GlobalFunctions::abs(const double d)
{
    return(d<0?-d:d);
}

string GlobalFunctions::numberToString(const unsigned int number)
{
    char result[100];
    sprintf( result, "%u", number);
    return string(result);
}

string GlobalFunctions::numberToString(const int number)
{
    char result[100];
    sprintf( result, "%d", number);
    return string(result);
}

string GlobalFunctions::numberToString(const float number)
{
    char result[100];
    sprintf( result, "%f", number);
    return string(result);
}

inline
int GlobalFunctions::StringToInt(const string& number)
{
    return atoi(number.c_str());
}

inline
double GlobalFunctions::StringToDouble(const string& number)
{
    return atof(number.c_str());
}

string& GlobalFunctions::includeNumberSeparator(string& number)
{
    const char* NumberSeparator = _(",");

    for (int i = number.size() - 3; i > 0; i-= 3)
        number.insert(i, NumberSeparator);
    return number;
}
