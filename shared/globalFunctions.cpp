#include "globalFunctions.h"
#include <wx/msgdlg.h>
#include <wx/file.h>
#include <fstream>
#include <wx/stream.h>
#include <wx/stopwatch.h>
#include <cmath>
#include "systemConstants.h"


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


int globalFunctions::stringToInt(const std::string& number)
{
    return atoi(number.c_str());
}


long globalFunctions::stringToLong(const std::string& number)
{
    return atol(number.c_str());
}


double globalFunctions::stringToDouble(const std::string& number)
{
    return atof(number.c_str());
}


int globalFunctions::wxStringToInt(const wxString& number)
{
    long result = 0;
    if (number.ToLong(&result))
        return result;
    else
        return 0; //don't throw exceptions here: wxEmptyString shall be interpreted as 0
}


double globalFunctions::wxStringToDouble(const wxString& number)
{
    double result = 0;
    if (number.ToDouble(&result))
        return result;
    else
        return 0; //don't throw exceptions here: wxEmptyString shall be interpreted as 0
}


unsigned int globalFunctions::getDigitCount(const unsigned int number) //count number of digits
{
    return number == 0 ? 1 : static_cast<unsigned int>(::log10(static_cast<double>(number))) + 1;
}


int globalFunctions::readInt(std::ifstream& stream)
{
    int result = 0;
    char* buffer = reinterpret_cast<char*>(&result);
    stream.read(buffer, sizeof(int));
    return result;
}


void globalFunctions::writeInt(std::ofstream& stream, const int number)
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


//############################################################################
Performance::Performance() :
        resultWasShown(false),
        timer(new wxStopWatch)
{
    timer->Start();
}


Performance::~Performance()
{
    if (!resultWasShown)
        showResult();
}


void Performance::showResult()
{
    resultWasShown = true;
    wxMessageBox(globalFunctions::numberToWxString(unsigned(timer->Time())) + wxT(" ms"));
    timer->Start(); //reset timer
}


//############################################################################
DebugLog::DebugLog() :
        lineCount(0),
        logFile(NULL)
{
    logFile = new wxFile;
    logfileName = assembleFileName();
    logFile->Open(logfileName.c_str(), wxFile::write);
}


DebugLog::~DebugLog()
{
    delete logFile; //automatically closes file handle
}


wxString DebugLog::assembleFileName()
{
    wxString tmp = wxDateTime::Now().FormatISOTime();
    tmp.Replace(wxT(":"), wxEmptyString);
    return wxString(wxT("DEBUG_")) + wxDateTime::Now().FormatISODate() + wxChar('_') + tmp + wxT(".log");
}


void DebugLog::write(const wxString& logText)
{
    ++lineCount;
    if (lineCount % 50000 == 0) //prevent logfile from becoming too big
    {
        logFile->Close();
        wxRemoveFile(logfileName);

        logfileName = assembleFileName();
        logFile->Open(logfileName.c_str(), wxFile::write);
    }

    logFile->Write(wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] "));
    logFile->Write(logText + globalFunctions::LINE_BREAK);
}

//DebugLog logDebugInfo;


wxString getCodeLocation(const wxString file, const int line)
{
    return wxString(file).AfterLast(globalFunctions::FILE_NAME_SEPARATOR) + wxT(", LINE ") + globalFunctions::numberToWxString(line) + wxT(" | ");
}
