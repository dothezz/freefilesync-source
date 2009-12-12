#include "serialize.h"
#include <wx/intl.h>

using namespace Utility;


void ReadInputStream::throwReadError()  //throw FileError()
{
    throw FreeFileSync::FileError(wxString(_("Error reading from synchronization database:")) + wxT(" \n") +
                                  wxT("\"") +  errorObjName_ + wxT("\""));
}


ReadInputStream::CharArray ReadInputStream::readArrayC()
{
    CharArray buffer(new std::vector<char>);
    const size_t byteCount = readNumberC<size_t>();
    if (byteCount > 0)
    {
        buffer->resize(byteCount);
        stream_.Read(&(*buffer)[0], byteCount);
        check();
        if (stream_.LastRead() != byteCount) //some additional check
            throwReadError();
    }
    return buffer;
}


//--------------------------------------------------------------------------------------------------------
void WriteOutputStream::throwWriteError()  //throw FileError()
{
    throw FreeFileSync::FileError(wxString(_("Error writing to synchronization database:")) + wxT(" \n") +
                                  wxT("\"") + errorObjName_ + wxT("\""));
}


void WriteOutputStream::writeArrayC(const std::vector<char>& buffer)
{
    writeNumberC<size_t>(buffer.size());
    if (buffer.size() > 0)
    {
        stream_.Write(&buffer[0], buffer.size());
        check();
        if (stream_.LastWrite() != buffer.size()) //some additional check
            throwWriteError();
    }
}


