#include "globalFunctions.h"
#include <wx/intl.h>

#ifdef FFS_WIN
const wxChar FileNameSeparator = '\\';
#endif  // FFS_WIN

#ifdef FFS_LINUX
const wxChar FileNameSeparator = '/';
#endif  // FFS_LINUX

const wxChar* FloatingPointSeparator = _(".");
const wxChar* NumberSeparator = _(",");


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

wxString GlobalFunctions::numberToWxString(const unsigned int number)
{
    return wxString::Format(wxT("%u"), number);
}

wxString GlobalFunctions::numberToWxString(const int number)
{
    return wxString::Format(wxT("%i"), number);
}

wxString GlobalFunctions::numberToWxString(const float number)
{
    return wxString::Format(wxT("%f"), number);
}

inline
int GlobalFunctions::stringToInt(const string& number)
{
    return atoi(number.c_str());
}

inline
double GlobalFunctions::stringToDouble(const string& number)
{
    return atof(number.c_str());
}

inline
int GlobalFunctions::wxStringToInt(const wxString& number)
{
    long result = 0;
    if (number.ToLong(&result))
        return result;
    else
        throw std::runtime_error("Error when converting number to long");
}

inline
double GlobalFunctions::wxStringToDouble(const wxString& number)
{
    double result = 0;
    if (number.ToDouble(&result))
        return result;
    else
        throw std::runtime_error("Error when converting number to double");
}

wxString& GlobalFunctions::includeNumberSeparator(wxString& number)
{
    for (int i = number.size() - 3; i > 0; i-= 3)
        number.insert(i, NumberSeparator);
    return number;
}
