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
template <class T> T readPOD (wxInputStream&  stream);
template <class T> void readPOD (wxInputStream&  stream, T& pod);
template <class T> void writePOD(wxOutputStream& stream, const T& pod);

template <class S> S    readString (wxInputStream&  stream);
template <class S> void readString (wxInputStream&  stream, S& str);
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
public:
    virtual void throwException() const = 0;

protected:
    CheckedIo(wxStreamBase& stream) : stream_(stream) {}

    void check() const
    {
        if (stream_.GetLastError() != wxSTREAM_NO_ERROR)
            throwException();
    }

private:
    wxStreamBase& stream_;
};


//wxInputStream proxy throwing exception on error
class CheckedReader : public CheckedIo
{
public:
    CheckedReader(wxInputStream& stream) : CheckedIo(stream), stream_(stream) {}

    template <class T>
    T readPOD() const; //throw!

    template <class T>
    void readPOD(T& pod) const; //throw!

    template <class S>
    S readString() const; //throw!

    template <class S>
    void readString(S& str) const; //throw!

    void readArray(void* data, size_t len) const; //throw!

private:
    wxInputStream& stream_;
};


//wxOutputStream proxy throwing FileError on error
class CheckedWriter : public CheckedIo
{
public:
    CheckedWriter(wxOutputStream& stream) : CheckedIo(stream), stream_(stream) {}

    template <class T>
    void writePOD(const T& pod) const; //throw!

    template <class S>
    void writeString(const S& str) const; //throw!

    void writeArray(const void* data, size_t len) const; //throw!

private:
    wxOutputStream& stream_;
};
























//-----------------------implementation-------------------------------
template <class T> inline
T readPOD(wxInputStream& stream)
{
    T pod = 0;
    readPOD(stream, pod);
    return pod;
}


template <class T> inline
void readPOD(wxInputStream& stream, T& pod)
{
    stream.Read(reinterpret_cast<char*>(&pod), sizeof(T));
}


template <class T> inline
void writePOD(wxOutputStream& stream, const T& pod)
{
    stream.Write(reinterpret_cast<const char*>(&pod), sizeof(T));
}


template <class S> inline
S readString(wxInputStream& stream)
{
    S str;
    readString(stream, str);
    return str;
}


template <class S> inline
void readString(wxInputStream& stream, S& str)
{
    //don't even consider UTF8 conversions here! "string" is expected to handle arbitrary binary data!

    const auto strLength = readPOD<std::uint32_t>(stream);
    str.resize(strLength); //throw std::bad_alloc
    if (strLength > 0)
        stream.Read(&*str.begin(), sizeof(typename S::value_type) * strLength);
}


template <class S> inline
void writeString(wxOutputStream& stream, const S& str)
{
    const auto strLength = str.length();
    writePOD(stream, static_cast<std::uint32_t>(strLength));
    if (strLength > 0)
        stream.Write(&*str.begin(), sizeof(typename S::value_type) * strLength); //don't use c_str(), but access uniformly		 via STL interface
}


inline
void CheckedReader::readArray(void* data, size_t len) const //throw!
{
    stream_.Read(data, len);
    check();
}


template <class T> inline
T CheckedReader::readPOD() const //checked read operation
{
    T pod = 0;
    readPOD(pod);
    return pod;
}


template <class T> inline
void CheckedReader::readPOD(T& pod) const //checked read operation
{
    readArray(&pod, sizeof(T));
}


template <class S> inline
S CheckedReader::readString() const //checked read operation
{
    S str;
    readString(str);
    return str;
}


template <class S> inline
void CheckedReader::readString(S& str) const //checked read operation
{
    try
    {
        zen::readString<S>(stream_, str); //throw std::bad_alloc
    }
    catch (std::exception&) { throwException(); }
    check();
    if (stream_.LastRead() != str.length() * sizeof(typename S::value_type)) //some additional check
        throwException();
}


inline
void CheckedWriter::writeArray(const void* data, size_t len) const //throw!
{
    stream_.Write(data, len);
    check();
}


template <class T> inline
void CheckedWriter::writePOD(const T& pod) const //checked write opera
{
    writeArray(&pod, sizeof(T));
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
