// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "error_log.h"
#include <wx/datetime.h>
#include <wx/intl.h>


using ffs3::ErrorLogging;


void ErrorLogging::logInfo(const wxString& infoMessage)
{
    const wxString prefix = wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] ") + _("Info") + wxT(": ");
    formattedMessages.push_back(assembleMessage(prefix, infoMessage));
}


void ErrorLogging::logWarning(const wxString& warningMessage)
{
    const wxString prefix = wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] ") + _("Warning") + wxT(": ");
    formattedMessages.push_back(assembleMessage(prefix, warningMessage));
}


void ErrorLogging::logError(const wxString& errorMessage)
{
    ++errorCount;

    const wxString prefix = wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] ") + _("Error") + wxT(": ");
    formattedMessages.push_back(assembleMessage(prefix, errorMessage));
}


void ErrorLogging::logFatalError(const wxString& errorMessage)
{
    ++errorCount;

    const wxString prefix = wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] ") + _("Fatal Error") + wxT(": ");
    formattedMessages.push_back(assembleMessage(prefix, errorMessage));
}


wxString ErrorLogging::assembleMessage(const wxString& prefix, const wxString& message)
{
    const size_t prefixLength = prefix.size();
    wxString formattedText = prefix;
    for (wxString::const_iterator i = message.begin(); i != message.end(); ++i)
        if (*i == wxChar('\n'))
        {
            formattedText += wxString(wxChar('\n')).Pad(prefixLength, wxChar(' '), true);
            while (*++i == wxChar('\n')) //remove duplicate newlines
                ;
            --i;
        }
        else
            formattedText += *i;

    return formattedText;
}
