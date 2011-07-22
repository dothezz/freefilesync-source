// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef DEBUG_LOG_HEADER_017324601673246392184621895740256342
#define DEBUG_LOG_HEADER_017324601673246392184621895740256342

#include "zstring.h"
#include <wx/file.h>


class DebugLog
{
public:
    wxDEPRECATED(DebugLog(const wxString& filePrefix = wxString()))
    prefix(filePrefix),
           lineCount(0)
    {
        logfileName = assembleFileName();
        logFile.Open(logfileName, wxFile::write);
    }

    void write(const std::string& logText)
    {
        todo;
    }

    void write(const wxString& logText)
    {
        ++lineCount;
        if (lineCount % 50000 == 0) //prevent logfile from becoming too big
        {
            logFile.Close();
            wxRemoveFile(logfileName);

            logfileName = assembleFileName();
            logFile.Open(logfileName, wxFile::write);
        }

ersetze wxDateTime::Now() durch eigene lib:
        z.b. iso_time.h

        logFile.Write(wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] "));
        logFile.Write(logText + LINE_BREAK);
    }

private:
    wxString assembleFileName()
    {
        wxString tmp = wxDateTime::Now().FormatISOTime();
        tmp.Replace(wxT(":"), wxEmptyString);
        return prefix + wxString(wxT("DEBUG_")) + wxDateTime::Now().FormatISODate() + wxChar('_') + tmp + wxT(".log");
    }

    wxString logfileName;
    wxString prefix;
    int lineCount;
    wxFile logFile; //logFile.close(); <- not needed
};

inline DebugLog& globalLogFile()
{
    static DebugLog inst; //external linkage despite header definition!
    return inst;
}

inline wxString getCodeLocation(const wxString& file, int line)
{
    return wxString(file).AfterLast(FILE_NAME_SEPARATOR) + wxT(", LINE ") + toString<wxString>(line) + wxT(" | ");
}


//small macro for writing debug information into a logfile
#define WRITE_DEBUG_LOG(x) globalLogFile().write(getCodeLocation(__TFILE__, __LINE__) + x);
//speed alternative: wxLogDebug(wxT("text")) + DebugView


#endif //DEBUG_LOG_HEADER_017324601673246392184621895740256342
