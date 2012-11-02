// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef ERRORLOGGING_H_INCLUDED
#define ERRORLOGGING_H_INCLUDED

#include <cassert>
#include <algorithm>
#include <vector>
#include <string>
#include "time.h"
#include "i18n.h"
#include "string_base.h"

namespace zen
{
enum MessageType
{
    TYPE_INFO        = 0x1,
    TYPE_WARNING     = 0x2,
    TYPE_ERROR       = 0x4,
    TYPE_FATAL_ERROR = 0x8,
};

typedef Zbase<wchar_t> MsgString; //std::wstring may employ small string optimization: we cannot accept bloating the "logEntries" memory block below (think 1 million entries)

struct LogEntry
{
    time_t      time;
    MessageType type;
    MsgString   message; 
};

MsgString formatMessage(const LogEntry& msg);


class ErrorLog
{
public:
	template <class String>
    void logMsg(const String& message, MessageType type);

    int getItemCount(int typeFilter = TYPE_INFO | TYPE_WARNING | TYPE_ERROR | TYPE_FATAL_ERROR) const;

    const std::vector<LogEntry>& getEntries() const { return logEntries; }

private:
    std::vector<LogEntry> logEntries; //list of non-resolved errors and warnings
};













//######################## implementation ##########################
template <class String> inline
void ErrorLog::logMsg(const String& message, zen::MessageType type)
{
	const LogEntry newEntry = { std::time(nullptr), type, copyStringTo<MsgString>(message) };
    logEntries.push_back(newEntry);
}


inline
int ErrorLog::getItemCount(int typeFilter) const
{
    return static_cast<int>(std::count_if(logEntries.begin(), logEntries.end(), [&](const LogEntry& e) { return e.type & typeFilter; }));
}


namespace
{
MsgString formatMessageImpl(const LogEntry& entry) //internal linkage
{
    auto getTypeName = [&]() -> std::wstring
    {
        switch (entry.type)
        {
            case TYPE_INFO:
                return _("Info");
            case TYPE_WARNING:
                return _("Warning");
            case TYPE_ERROR:
                return _("Error");
            case TYPE_FATAL_ERROR:
                return _("Fatal Error");
        }
		assert(false);
        return std::wstring();
    };

    MsgString formattedText = L"[" + formatTime<MsgString>(FORMAT_TIME, localTime(entry.time)) + L"] " + copyStringTo<MsgString>(getTypeName()) + L": ";
    const size_t prefixLen = formattedText.size();

    for (auto iter = entry.message.begin(); iter != entry.message.end(); )
        if (*iter == L'\n')
        {
            formattedText += L'\n';

            MsgString blanks;
            blanks.resize(prefixLen, L' ');
            formattedText += blanks;

            do //skip duplicate newlines
            {
                ++iter;
            }
            while (iter != entry.message.end() && *iter == L'\n');
        }
        else
            formattedText += *iter++;

    return formattedText;
}
}

inline 
MsgString formatMessage(const LogEntry& entry) { return formatMessageImpl(entry); }
}

#endif //ERRORLOGGING_H_INCLUDED
