// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef ERRORLOGGING_H_INCLUDED
#define ERRORLOGGING_H_INCLUDED

#include <wx/string.h>
#include <vector>
#include <map>


namespace zen
{
enum MessageType
{
    TYPE_INFO        = 1,
    TYPE_WARNING     = 2,
    TYPE_ERROR       = 4,
    TYPE_FATAL_ERROR = 8,
};

class ErrorLogging
{
public:
    void logMsg(const wxString& message, MessageType type);

    int typeCount(int types = TYPE_INFO | TYPE_WARNING | TYPE_ERROR | TYPE_FATAL_ERROR) const;

    std::vector<wxString> getFormattedMessages(int types = TYPE_INFO | TYPE_WARNING | TYPE_ERROR | TYPE_FATAL_ERROR) const;

private:
    struct Entry
    {
        MessageType type;
        time_t      time;
        wxString message;
    };

    static wxString formatMessage(const Entry& msg);

    std::vector<Entry> messages; //list of non-resolved errors and warnings

    mutable std::map<MessageType, int> statistics;
};
}

#endif // ERRORLOGGING_H_INCLUDED
