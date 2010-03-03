// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "globalFunctions.h"
#include <wx/msgdlg.h>
#include <wx/file.h>
#include <wx/stopwatch.h>
#include <cmath>
#include "systemConstants.h"


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


//############################################################################
Performance::Performance() :
    resultWasShown(false),
    timer(new wxStopWatch)
{
    timer->Start();
}


Performance::~Performance()
{
    //keep non-inline destructor for std::auto_ptr to work with forward declaration

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
