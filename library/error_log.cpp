// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "error_log.h"
#include <wx/datetime.h>
#include "../shared/i18n.h"


using zen::ErrorLogging;


void ErrorLogging::logMsg(const wxString& message, zen::MessageType type)
{
    Entry newEntry;
    newEntry.type    = type;
    newEntry.time    = wxDateTime::GetTimeNow();
    newEntry.message = message;

    messages.push_back(newEntry);

    ++statistics[type];
}


int ErrorLogging::typeCount(int types) const
{
    int count = 0;

    if (types & TYPE_INFO)
        count += statistics[TYPE_INFO];
    if (types & TYPE_WARNING)
        count += statistics[TYPE_WARNING];
    if (types & TYPE_ERROR)
        count += statistics[TYPE_ERROR];
    if (types & TYPE_FATAL_ERROR)
        count += statistics[TYPE_FATAL_ERROR];

    return count;
}


std::vector<wxString> ErrorLogging::getFormattedMessages(int types) const
{
    std::vector<wxString> output;

    for (std::vector<Entry>::const_iterator i = messages.begin(); i != messages.end(); ++i)
        if (i->type & types)
            output.push_back(formatMessage(*i));

    return output;
}


wxString ErrorLogging::formatMessage(const Entry& msg)
{
    wxString typeName;
    switch (msg.type)
    {
        case TYPE_INFO:
            typeName = _("Info");
            break;
        case TYPE_WARNING:
            typeName = _("Warning");
            break;
        case TYPE_ERROR:
            typeName = _("Error");
            break;
        case TYPE_FATAL_ERROR:
            typeName = _("Fatal Error");
            break;
    }

    const wxString prefix = wxString(L"[") + wxDateTime(msg.time).FormatTime() + L"] " + typeName + L": ";

    wxString formattedText = prefix;
    for (auto i = msg.message.begin(); i != msg.message.end(); ++i)
        if (*i == wxChar('\n'))
        {
            formattedText += L'\n';

            wxString blanks;
            blanks.resize(prefix.size(), L' ');
            formattedText += blanks;

            while (*++i == L'\n') //remove duplicate newlines
                ;
            --i;
        }
        else
            formattedText += *i;

    return formattedText;
}
