// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef SERIALIZE_H_INCLUDED
#define SERIALIZE_H_INCLUDED

#include <cstdint>
#include <wx/stream.h>
#include <zen/file_io.h>


namespace zen
{
//unchecked, unformatted serialization
template <class T> T    readPOD (wxInputStream&  stream);
template <class T> void writePOD(wxOutputStream& stream, const T& pod);

template <class S> S    readString (wxInputStream&  stream);
template <class S> void writeString(wxOutputStream& stream, const S& str);


//############# wxWidgets stream adapter #############
class FileInputStream : public wxInputStream
{
public:
    FileInputStream(const Zstring& filename) : fileObj(filename) {} //throw FileError

private:
    virtual size_t OnSysRead(void* buffer, size_t bufsize) { return fileObj.read(buffer, bufsize); } //throw FileError

    zen::FileInput fileObj;
};


class FileOutputStream : public wxOutputStream
{
public:
    FileOutputStream(const Zstring& filename) : fileObj(filename, zen::FileOutput::ACC_OVERWRITE) {} //throw FileError

private:
    virtual size_t OnSysWrite(const void* buffer, size_t bufsize)
    {
        fileObj.write(buffer, bufsize); //throw FileError
        return bufsize;
    }

    zen::FileOutput fileObj;
};


class CheckedIo
{
protected:
    CheckedIo(wxStreamBase& stream) : stream_(stream) {}

    void check() const
    {
        if (stream_.GetLastError() != wxSTREAM_NO_ERROR)
            throwException();
    }
    virtual void throwException() const = 0;

private:
    wxStreamBase& stream_;
};


//wxInputStream proxy throwing exception on error
class CheckedReader : private CheckedIo
{
public:
    CheckedReader(wxInputStream& stream) : CheckedIo(stream), stream_(stream) {}

    template <class T>
    T readPOD() const; //throw!

    template <class S>
    S readString() const; //throw!

private:
    wxInputStream& stream_;
};


//wxOutputStream proxy throwing FileError on error
class CheckedWriter : private CheckedIo
{
public:
    CheckedWriter(wxOutputStream& stream) : CheckedIo(stream), stream_(stream) {}

    template <class T>
    void writePOD(T number) const; //throw!

    template <class S>
    void writeString(const S& str) const; //throw!

private:
    wxOutputStream& stream_;
};
























//-----------------------implementation-------------------------------
template <class T> inline
T readPOD(wxInputStream& stream)
{
    T pod = 0;
    stream.Read(reinterpret_cast<char*>(&pod), sizeof(T));
    return pod;
}


template <class T> inline
void writePOD(wxOutputStream& stream, const T& pod)
{
    stream.Write(reinterpret_cast<const char*>(&pod), sizeof(T));
}


template <class S> inline
S readString(wxInputStream& stream)
{
    //don't even consider UTF8 conversions here! "string" is expected to handle arbitrary binary data!

    typedef typename S::value_type CharType;

    const auto strLength = readPOD<std::uint32_t>(stream);
    S output;
    if (strLength > 0)
    {
        output.resize(strLength); //throw std::bad_alloc
        stream.Read(&*output.begin(), sizeof(CharType) * strLength);
    }
    return output;
}


template <class S> inline
void writeString(wxOutputStream& stream, const S& str)
{
    writePOD(stream, static_cast<std::uint32_t>(str.length()));
    stream.Write(str.c_str(), sizeof(typename S::value_type) * str.length());
}


template <class T>
inline
T CheckedReader::readPOD() const //checked read operation
{
    T output = zen::readPOD<T>(stream_);
    check();
    return output;
}


template <class S> inline
S CheckedReader::readString() const //checked read operation
{
    S output;
    try
    {
        output = zen::readString<S>(stream_); //throw std::bad_alloc
    }
    catch (std::exception&) { throwException(); }

    check();
    if (stream_.LastRead() != output.length() * sizeof(typename S::value_type)) //some additional check
        throwException();
    return output;
}


template <class T> inline
void CheckedWriter::writePOD(T number) const //checked write operation
{
    zen::writePOD<T>(stream_, number);
    check();
}


template <class S> inline
void CheckedWriter::writeString(const S& str) const //checked write operation
{
    zen::writeString(stream_, str);
    check();
    if (stream_.LastWrite() != str.length() * sizeof(typename S::value_type)) //some additional check
        throwException();
}
}

#endif //SERIALIZE_H_INCLUDED
