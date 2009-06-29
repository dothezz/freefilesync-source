#ifndef FILEERROR_H_INCLUDED
#define FILEERROR_H_INCLUDED

#include "zstring.h"
#include "fileError.h"

namespace FreeFileSync
{
    class FileError //Exception class used to notify file/directory copy/delete errors
    {
    public:
        FileError(const Zstring& message) :
                errorMessage(message) {}

        const Zstring& show() const
        {
            return errorMessage;
        }

    private:
        Zstring errorMessage;
    };
}

#endif // FILEERROR_H_INCLUDED
