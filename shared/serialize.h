// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef SERIALIZE_H_INCLUDED
#define SERIALIZE_H_INCLUDED

#include "zstring.h"
#include <wx/stream.h>
#include "fileError.h"
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>

namespace Utility
{
template <class T>
T readNumber(wxInputStream& stream);

template <class T>
void writeNumber(wxOutputStream& stream, T number);


Zstring readString(wxInputStream& stream);
void writeString(wxOutputStream& stream, const Zstring& str);


class ReadInputStream //throw FileError()
{
protected:
    ReadInputStream(wxInputStream& stream, const wxString& errorObjName) : stream_(stream), errorObjName_(errorObjName) {}

    template <class T>
    T readNumberC(); //throw FileError(), checked read operation

    Zstring readStringC(); //throw FileError(), checked read operation

    typedef boost::shared_ptr<std::vector<char> > CharArray;
    CharArray readArrayC(); //throw FileError()

    void check();

protected:
    wxInputStream& getStream()
    {
        return stream_;
    }

private:
    wxInputStream& stream_;
    void throwReadError();  //throw FileError()
    const wxString& errorObjName_; //used for error text only
};


class WriteOutputStream //throw FileError()
{
protected:
    WriteOutputStream(const wxString& errorObjName, wxOutputStream& stream) : stream_(stream), errorObjName_(errorObjName) {}

    template <class T>
    void writeNumberC(T number); //throw FileError(), checked write operation

    void writeStringC(const Zstring& str); //throw FileError(), checked write operation

    void writeArrayC(const std::vector<char>& buffer); //throw FileError()

    void check();

protected:
    wxOutputStream& getStream()
    {
        return stream_;
    }

private:
    wxOutputStream& stream_;
    void throwWriteError();  //throw FileError()
    const wxString& errorObjName_; //used for error text only!
};





























//---------------Inline Implementation---------------------------------------------------
template <class T>
inline
T readNumber(wxInputStream& stream)
{
    T result = 0;
    stream.Read(&result, sizeof(T));
    return result;
}


template <class T>
inline
void writeNumber(wxOutputStream& stream, T number)
{
    stream.Write(&number, sizeof(T));
}


inline
Zstring readString(wxInputStream& stream)  //read string from file stream
{
    const size_t strLength = readNumber<size_t>(stream);
    if (strLength <= 1000)
    {
        DefaultChar buffer[1000];
        stream.Read(buffer, sizeof(DefaultChar) * strLength);
        return Zstring(buffer, strLength);
    }
    else
    {
        boost::scoped_array<DefaultChar> buffer(new DefaultChar[strLength]);
        stream.Read(buffer.get(), sizeof(DefaultChar) * strLength);
        return Zstring(buffer.get(), strLength);
    }
}


inline
void writeString(wxOutputStream& stream, const Zstring& str)  //write string to filestream
{
    writeNumber<size_t>(stream, str.length());
    stream.Write(str.c_str(), sizeof(DefaultChar) * str.length());
}


inline
void ReadInputStream::check()
{
    if (stream_.GetLastError() != wxSTREAM_NO_ERROR)
        throwReadError();
}


template <class T>
inline
T ReadInputStream::readNumberC() //checked read operation
{
    T output = readNumber<T>(stream_);
    check();
    return output;
}


inline
Zstring ReadInputStream::readStringC() //checked read operation
{
    Zstring output = readString(stream_);
    check();
    return output;
}


template <class T>
inline
void WriteOutputStream::writeNumberC(T number) //checked write operation
{
    writeNumber<T>(stream_, number);
    check();
}


inline
void WriteOutputStream::writeStringC(const Zstring& str) //checked write operation
{
    writeString(stream_, str);
    check();
}



inline
void WriteOutputStream::check()
{
    if (stream_.GetLastError() != wxSTREAM_NO_ERROR)
        throwWriteError();
}


}

#endif // SERIALIZE_H_INCLUDED
