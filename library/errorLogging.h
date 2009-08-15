#ifndef ERRORLOGGING_H_INCLUDED
#define ERRORLOGGING_H_INCLUDED

#include <wx/string.h>
#include <vector>

class Zstring;

namespace FreeFileSync
{
    class ErrorLogging
    {
    public:
        ErrorLogging() : errorCount(0) {}

        void logError(const wxString& errorMessage);
        void logWarning(const wxString& warningMessage);
        void logInfo(const wxString& infoMessage);

        int errorsTotal()
        {
            return errorCount;
        }

        const std::vector<wxString>& getFormattedMessages()
        {
            return formattedMessages;
        }

        size_t messageCount()
        {
            return formattedMessages.size();
        }

    private:
        wxString assembleMessage(const wxString& prefix, const wxString& message);

        std::vector<wxString> formattedMessages; //list of non-resolved errors and warnings
        int errorCount;
    };
}

#endif // ERRORLOGGING_H_INCLUDED
