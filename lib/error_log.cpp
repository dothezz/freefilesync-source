// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "error_log.h"
#include <zen/time.h>
#include <zen/i18n.h>
#include <algorithm>

using namespace zen;


void ErrorLogging::logMsg(const wxString& message, zen::MessageType type)
{
    Entry newEntry;
    newEntry.type    = type;
    newEntry.time    = std::time(NULL);
    newEntry.message = message;

    messages.push_back(newEntry);

    ++statistics[type];
}


int ErrorLogging::typeCount(int typeFilter) const
{
    int count = 0;

    if (typeFilter & TYPE_INFO)
        count += statistics[TYPE_INFO];
    if (typeFilter & TYPE_WARNING)
        count += statistics[TYPE_WARNING];
    if (typeFilter & TYPE_ERROR)
        count += statistics[TYPE_ERROR];
    if (typeFilter & TYPE_FATAL_ERROR)
        count += statistics[TYPE_FATAL_ERROR];

    return count;
}


std::vector<wxString> ErrorLogging::getFormattedMessages(int typeFilter) const
{
    std::vector<wxString> output;

    std::for_each(messages.begin(), messages.end(),
                  [&](const Entry& entry)
    {
        if (entry.type & typeFilter)
            output.push_back(formatMessage(entry));
    });

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

    const wxString prefix = L"[" + formatTime<wxString>(FORMAT_TIME, localTime(msg.time)) + L"] " + typeName + L": ";

    wxString formattedText = prefix;
    for (auto iter = msg.message.begin(); iter != msg.message.end(); )
        if (*iter == L'\n')
        {
            formattedText += L'\n';

            wxString blanks;
            blanks.resize(prefix.size(), L' ');
            formattedText += blanks;

            do //remove duplicate newlines
            {
                ++iter;
            }
            while (iter != msg.message.end() && *iter == L'\n');
        }
        else
            formattedText += *iter++;

    return formattedText;
}
