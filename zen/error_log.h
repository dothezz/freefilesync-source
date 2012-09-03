// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef ERRORLOGGING_H_INCLUDED
#define ERRORLOGGING_H_INCLUDED

#include <algorithm>
#include <vector>
#include <string>
#include <zen/time.h>
#include <zen/i18n.h>

namespace zen
{
enum MessageType
{
    TYPE_INFO        = 0x1,
    TYPE_WARNING     = 0x2,
    TYPE_ERROR       = 0x4,
    TYPE_FATAL_ERROR = 0x8,
};

struct LogEntry
{
    time_t       time;
    MessageType  type;
    std::wstring message;
};

std::wstring formatMessage(const LogEntry& msg);


class ErrorLog
{
public:
    void logMsg(const std::wstring& message, MessageType type);

    int getItemCount(int typeFilter = TYPE_INFO | TYPE_WARNING | TYPE_ERROR | TYPE_FATAL_ERROR) const;

    const std::vector<LogEntry>& getEntries() const { return logEntries; }

private:
    std::vector<LogEntry> logEntries; //list of non-resolved errors and warnings
};



















//######################## implementation ##########################

inline
void ErrorLog::logMsg(const std::wstring& message, zen::MessageType type)
{
    const LogEntry newEntry = { std::time(nullptr), type, message };
    logEntries.push_back(newEntry);
}


inline
int ErrorLog::getItemCount(int typeFilter) const
{
    return static_cast<int>(std::count_if(logEntries.begin(), logEntries.end(), [&](const LogEntry& e) { return e.type & typeFilter; }));
}


namespace
{
std::wstring formatMessageImpl(const LogEntry& entry) //internal linkage
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
        return std::wstring();
    };

    std::wstring formattedText = L"[" + formatTime<std::wstring>(FORMAT_TIME, localTime(entry.time)) + L"] " + getTypeName() + L": ";
    const size_t prefixLen = formattedText.size();

    for (auto iter = entry.message.begin(); iter != entry.message.end(); )
        if (*iter == L'\n')
        {
            formattedText += L'\n';

            std::wstring blanks;
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

inline std::wstring formatMessage(const LogEntry& entry) { return formatMessageImpl(entry); }

}

#endif //ERRORLOGGING_H_INCLUDED
