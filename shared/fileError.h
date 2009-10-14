#ifndef FILEERROR_H_INCLUDED
#define FILEERROR_H_INCLUDED

#include <wx/string.h>


namespace FreeFileSync
{
class FileError //Exception class used to notify file/directory copy/delete errors
{
public:
    FileError(const wxString& message) :
        errorMessage(message) {}

    const wxString& show() const
    {
        return errorMessage;
    }

private:
    wxString errorMessage;
};
}

#endif // FILEERROR_H_INCLUDED
