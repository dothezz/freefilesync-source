#include "globalFunctions.h"
#include "../ui/resources.h"

inline
int globalFunctions::round(const double d)
{
    return static_cast<int>(d<0?d-.5:d+.5);
}


string globalFunctions::numberToString(const unsigned int number)
{
    char result[100];
    sprintf( result, "%u", number);
    return string(result);
}


string globalFunctions::numberToString(const int number)
{
    char result[100];
    sprintf( result, "%d", number);
    return string(result);
}


string globalFunctions::numberToString(const float number)
{
    char result[100];
    sprintf( result, "%f", number);
    return string(result);
}


wxString globalFunctions::numberToWxString(const unsigned int number)
{
    return wxString::Format(wxT("%u"), number);
}


wxString globalFunctions::numberToWxString(const int number)
{
    return wxString::Format(wxT("%i"), number);
}


wxString globalFunctions::numberToWxString(const float number)
{
    return wxString::Format(wxT("%f"), number);
}


inline
int globalFunctions::stringToInt(const string& number)
{
    return atoi(number.c_str());
}


inline
double globalFunctions::stringToDouble(const string& number)
{
    return atof(number.c_str());
}


inline
int globalFunctions::wxStringToInt(const wxString& number)
{
    long result = 0;
    if (number.ToLong(&result))
        return result;
    else
        throw std::runtime_error("Error when converting number to long");
}


inline
double globalFunctions::wxStringToDouble(const wxString& number)
{
    double result = 0;
    if (number.ToDouble(&result))
        return result;
    else
        throw std::runtime_error("Error when converting number to double");
}


wxString& globalFunctions::includeNumberSeparator(wxString& number)
{
    for (int i = number.size() - 3; i > 0; i-= 3)
        number.insert(i, GlobalResources::numberSeparator);
    return number;
}
