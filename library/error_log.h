// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef ERRORLOGGING_H_INCLUDED
#define ERRORLOGGING_H_INCLUDED

#include <wx/string.h>
#include <vector>
#include "../shared/zstring.h"


namespace ffs3
{
class ErrorLogging
{
public:
    ErrorLogging() : errorCount(0) {}

    void logInfo(      const wxString& infoMessage);
    void logWarning(   const wxString& warningMessage);
    void logError(     const wxString& errorMessage);
    void logFatalError(const wxString& errorMessage);

    int errorsTotal()
    {
        return errorCount;
    }

    typedef std::vector<wxString> MessageEntry;
    const MessageEntry& getFormattedMessages() const
    {
        return formattedMessages;
    }

private:
    wxString assembleMessage(const wxString& prefix, const wxString& message);

    MessageEntry formattedMessages; //list of non-resolved errors and warnings
    int errorCount;
};
}

#endif // ERRORLOGGING_H_INCLUDED
