// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
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


//wxInputStream proxy throwing FileError on error
class CheckedReader
{
public:
    CheckedReader(wxInputStream& stream, const Zstring& errorObjName) : stream_(stream), errorObjName_(errorObjName) {}

    template <class T>
    T readNumberC() const; //throw FileError, checked read operation

    template <class S>
    S readStringC() const; //throw FileError, checked read operation

private:
    void check() const;

    wxInputStream& stream_;
    const Zstring& errorObjName_; //used for error text only
};


//wxOutputStream proxy throwing FileError on error
class CheckedWriter
{
public:
    CheckedWriter(wxOutputStream& stream, const Zstring& errorObjName) : stream_(stream), errorObjName_(errorObjName) {}

    template <class T>
    void writeNumberC(T number) const; //throw FileError, checked write operation

    template <class S>
    void writeStringC(const S& str) const; //throw FileError, checked write operation

private:
    void check() const;

    wxOutputStream& stream_;
    const Zstring& errorObjName_; //used for error text only!
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
    output.resize(strLength); //throw std::bad_alloc
    stream.Read(&*output.begin(), sizeof(CharType) * strLength);
    return output;
}


template <class S> inline
void writeString(wxOutputStream& stream, const S& str)
{
    writePOD(stream, static_cast<std::uint32_t>(str.length()));
    stream.Write(str.c_str(), sizeof(typename S::value_type) * str.length());
}


inline
void CheckedReader::check() const
{
    if (stream_.GetLastError() != wxSTREAM_NO_ERROR)
        throw zen::FileError(_("Error reading from synchronization database:") + L" \n" + L"\"" +  errorObjName_ + L"\"");
}


template <class T>
inline
T CheckedReader::readNumberC() const //checked read operation
{
    T output = readPOD<T>(stream_);
    check();
    return output;
}


template <class S> inline
S CheckedReader::readStringC() const //checked read operation
{
    S output;
    try
    {
        output = readString<S>(stream_); //throw std::bad_alloc
        check();
        if (stream_.LastRead() != output.length() * sizeof(typename S::value_type)) //some additional check
            throw FileError(_("Error reading from synchronization database:") + L" \n" + L"\"" +  errorObjName_ + L"\"");
    }
    catch (std::exception&)
    {
        throw FileError(_("Error reading from synchronization database:") + L" \n" + L"\"" +  errorObjName_ + L"\"");
    }
    return output;
}


template <class T> inline
void CheckedWriter::writeNumberC(T number) const //checked write operation
{
    writePOD<T>(stream_, number);
    check();
}


template <class S> inline
void CheckedWriter::writeStringC(const S& str) const //checked write operation
{
    writeString(stream_, str);
    check();
    if (stream_.LastWrite() != str.length() * sizeof(typename S::value_type)) //some additional check
        throw FileError(_("Error writing to synchronization database:") + L" \n" + L"\"" + errorObjName_ + L"\"");
}


inline
void CheckedWriter::check() const
{
    if (stream_.GetLastError() != wxSTREAM_NO_ERROR)
        throw FileError(_("Error writing to synchronization database:") + L" \n" + L"\"" + errorObjName_ + L"\"");
}

}

#endif //SERIALIZE_H_INCLUDED
