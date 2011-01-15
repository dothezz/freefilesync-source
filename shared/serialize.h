// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef SERIALIZE_H_INCLUDED
#define SERIALIZE_H_INCLUDED

#include "zstring.h"
#include <wx/stream.h>
#include "file_error.h"
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>

namespace util
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
    T readNumberC() const; //throw FileError(), checked read operation

    Zstring readStringC() const; //throw FileError(), checked read operation

    typedef boost::shared_ptr<std::vector<char> > CharArray; //there's no guarantee std::string has a ref-counted implementation... so use this "thing"
    CharArray readArrayC() const; //throw FileError()

    void check() const;

    wxInputStream& getStream()
    {
        return stream_;
    }

private:
    wxInputStream& stream_;
    void throwReadError() const;  //throw FileError()
    const wxString& errorObjName_; //used for error text only
};


class WriteOutputStream //throw FileError()
{
protected:
    WriteOutputStream(const wxString& errorObjName, wxOutputStream& stream) : stream_(stream), errorObjName_(errorObjName) {}

    template <class T>
    void writeNumberC(T number) const; //throw FileError(), checked write operation

    void writeStringC(const Zstring& str) const; //throw FileError(), checked write operation

    void writeArrayC(const std::vector<char>& buffer) const; //throw FileError()

    void check() const;

    wxOutputStream& getStream()
    {
        return stream_;
    }

private:
    wxOutputStream& stream_;
    void throwWriteError() const;  //throw FileError()
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
Zstring readString(wxInputStream& stream)
{
    const boost::uint32_t strLength = readNumber<boost::uint32_t>(stream);
    if (strLength <= 1000)
    {
        Zchar buffer[1000];
        stream.Read(buffer, sizeof(Zchar) * strLength);
        return Zstring(buffer, strLength);
    }
    else
    {
        boost::scoped_array<Zchar> buffer(new Zchar[strLength]);
        stream.Read(buffer.get(), sizeof(Zchar) * strLength);
        return Zstring(buffer.get(), strLength);
    }
}


inline
void writeString(wxOutputStream& stream, const Zstring& str)
{
    writeNumber<boost::uint32_t>(stream, static_cast<boost::uint32_t>(str.length()));
    stream.Write(str.c_str(), sizeof(Zchar) * str.length());
}


inline
void ReadInputStream::check() const
{
    if (stream_.GetLastError() != wxSTREAM_NO_ERROR)
        throwReadError();
}


template <class T>
inline
T ReadInputStream::readNumberC() const //checked read operation
{
    T output = readNumber<T>(stream_);
    check();
    return output;
}


inline
Zstring ReadInputStream::readStringC() const //checked read operation
{
    Zstring output = readString(stream_);
    check();
    return output;
}


template <class T>
inline
void WriteOutputStream::writeNumberC(T number) const //checked write operation
{
    writeNumber<T>(stream_, number);
    check();
}


inline
void WriteOutputStream::writeStringC(const Zstring& str) const //checked write operation
{
    writeString(stream_, str);
    check();
}



inline
void WriteOutputStream::check() const
{
    if (stream_.GetLastError() != wxSTREAM_NO_ERROR)
        throwWriteError();
}


}

#endif // SERIALIZE_H_INCLUDED
