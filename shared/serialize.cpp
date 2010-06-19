// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "serialize.h"
#include <wx/intl.h>

using namespace Utility;


void ReadInputStream::throwReadError() const  //throw FileError()
{
    throw FreeFileSync::FileError(wxString(_("Error reading from synchronization database:")) + wxT(" \n") +
                                  wxT("\"") +  errorObjName_ + wxT("\""));
}


ReadInputStream::CharArray ReadInputStream::readArrayC() const
{
    const size_t byteCount = readNumberC<size_t>();
    CharArray buffer(new std::vector<char>(byteCount));
    if (byteCount > 0)
    {
        stream_.Read(&(*buffer)[0], byteCount);
        check();
        if (stream_.LastRead() != byteCount) //some additional check
            throwReadError();
    }
    return buffer;
}


//--------------------------------------------------------------------------------------------------------
void WriteOutputStream::throwWriteError() const //throw FileError()
{
    throw FreeFileSync::FileError(wxString(_("Error writing to synchronization database:")) + wxT(" \n") +
                                  wxT("\"") + errorObjName_ + wxT("\""));
}


void WriteOutputStream::writeArrayC(const std::vector<char>& buffer) const
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


