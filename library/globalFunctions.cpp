#include "globalFunctions.h"
#include "resources.h"

inline
int globalFunctions::round(const double d)
{
    return static_cast<int>(d<0?d-.5:d+.5);
}


string globalFunctions::numberToString(const unsigned int number)
{
    char result[100];
    sprintf(result, "%u", number);
    return string(result);
}


string globalFunctions::numberToString(const int number)
{
    char result[100];
    sprintf(result, "%d", number);
    return string(result);
}


string globalFunctions::numberToString(const float number)
{
    char result[100];
    sprintf(result, "%f", number);
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
        throw RuntimeException(_("Error when converting wxString to long"));
}


inline
double globalFunctions::wxStringToDouble(const wxString& number)
{
    double result = 0;
    if (number.ToDouble(&result))
        return result;
    else
        throw RuntimeException(_("Error when converting wxString to double"));
}


wxString& globalFunctions::includeNumberSeparator(wxString& number)
{
    for (int i = number.size() - 3; i > 0; i-= 3)
        number.insert(i, GlobalResources::thousandsSeparator);
    return number;
}


int globalFunctions::readInt(ifstream& stream)
{
    int result = 0;
    char* buffer = reinterpret_cast<char*>(&result);
    stream.read(buffer, sizeof(int));
    return result;
}


void globalFunctions::writeInt(ofstream& stream, const int number)
{
    const char* buffer = reinterpret_cast<const char*>(&number);
    stream.write(buffer, sizeof(int));
}


int globalFunctions::readInt(wxInputStream& stream)
{
    int result = 0;
    char* buffer = reinterpret_cast<char*>(&result);
    stream.Read(buffer, sizeof(int));
    return result;
}


void globalFunctions::writeInt(wxOutputStream& stream, const int number)
{
    const char* buffer = reinterpret_cast<const char*>(&number);
    stream.Write(buffer, sizeof(int));
}
