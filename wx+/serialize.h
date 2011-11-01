// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef SERIALIZE_H_INCLUDED
#define SERIALIZE_H_INCLUDED

#include <vector>
#include <cstdint>
#include <memory>
#include <wx/stream.h>
#include <zen/file_error.h>
#include <zen/file_io.h>


namespace zen
{
//unchecked, unformatted serialization
template <class T> T    readPOD (wxInputStream&  stream);
template <class T> void writePOD(wxOutputStream& stream, const T& pod);

template <class S> S    readString (wxInputStream&  stream);
template <class S> void writeString(wxOutputStream& stream, const S& str);


//############# wxWidgets stream adapter #############
// can be used as base classes (have virtual destructors)
class FileInputStream : public wxInputStream
{
public:
    FileInputStream(const Zstring& filename) : //throw FileError
        fileObj(filename) {}

private:
    virtual size_t OnSysRead(void* buffer, size_t bufsize)
    {
        return fileObj.read(buffer, bufsize); //throw FileError
    }

    zen::FileInput fileObj;
};


class FileOutputStream : public wxOutputStream
{
public:
    FileOutputStream(const Zstring& filename) : //throw FileError
        fileObj(filename, zen::FileOutput::ACC_OVERWRITE) {}

private:
    virtual size_t OnSysWrite(const void* buffer, size_t bufsize)
    {
        fileObj.write(buffer, bufsize); //throw FileError
        return bufsize;
    }

    zen::FileOutput fileObj;
};



class ReadInputStream //throw FileError
{
protected:
    ReadInputStream(wxInputStream& stream, const Zstring& errorObjName) : stream_(stream), errorObjName_(errorObjName) {}

    template <class T>
    T readNumberC() const; //throw FileError, checked read operation

    template <class S>
    S readStringC() const; //throw FileError, checked read operation

    typedef std::shared_ptr<std::vector<char> > CharArray; //there's no guarantee std::string has a ref-counted implementation... so use this "thing"
    CharArray readArrayC() const; //throw FileError

    void check() const;

    wxInputStream& getStream() { return stream_; }

private:
    wxInputStream& stream_;
    const Zstring& errorObjName_; //used for error text only
};


class WriteOutputStream //throw FileError
{
protected:
    WriteOutputStream(const Zstring& errorObjName, wxOutputStream& stream) : stream_(stream), errorObjName_(errorObjName) {}

    template <class T>
    void writeNumberC(T number) const; //throw FileError, checked write operation

    template <class S>
    void writeStringC(const S& str) const; //throw FileError, checked write operation

    void writeArrayC(const std::vector<char>& buffer) const; //throw FileError

    void check() const;

    wxOutputStream& getStream() { return stream_; }

private:
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
    typedef typename S::value_type CharType;

    const auto strLength = readPOD<std::uint32_t>(stream);
    if (strLength <= 1000)
    {
        CharType buffer[1000];
        stream.Read(buffer, sizeof(CharType) * strLength);
        return S(buffer, strLength);
    }
    else
    {
        std::vector<CharType> buffer(strLength); //throw std::bad_alloc
        stream.Read(&buffer[0], sizeof(CharType) * strLength);
        return S(&buffer[0], strLength);
    }
}


template <class S> inline
void writeString(wxOutputStream& stream, const S& str)
{
    writePOD(stream, static_cast<std::uint32_t>(str.length()));
    stream.Write(str.c_str(), sizeof(typename S::value_type) * str.length());
}


inline
void ReadInputStream::check() const
{
    if (stream_.GetLastError() != wxSTREAM_NO_ERROR)
        throw zen::FileError(_("Error reading from synchronization database:") + L" \n" + L"\"" +  errorObjName_ + L"\"");
}


template <class T>
inline
T ReadInputStream::readNumberC() const //checked read operation
{
    T output = readPOD<T>(stream_);
    check();
    return output;
}


template <class S> inline
S ReadInputStream::readStringC() const //checked read operation
{
    S output;
    try
    {
        output = readString<S>(stream_); //throw (std::bad_alloc)
        check();
    }
    catch (std::exception&)
    {
        throw FileError(_("Error reading from synchronization database:") + L" \n" + L"\"" +  errorObjName_ + L"\"");
    }
    return output;
}


inline
ReadInputStream::CharArray ReadInputStream::readArrayC() const
{
    const std::uint32_t byteCount = readNumberC<std::uint32_t>();
    CharArray buffer(new std::vector<char>(byteCount));
    if (byteCount > 0)
    {
        stream_.Read(&(*buffer)[0], byteCount);
        check();
        if (stream_.LastRead() != byteCount) //some additional check
            throw FileError(_("Error reading from synchronization database:") + L" \n" + L"\"" +  errorObjName_ + L"\"");
    }
    return buffer;
}


template <class T> inline
void WriteOutputStream::writeNumberC(T number) const //checked write operation
{
    writePOD<T>(stream_, number);
    check();
}


template <class S> inline
void WriteOutputStream::writeStringC(const S& str) const //checked write operation
{
    writeString(stream_, str);
    check();
}


inline
void WriteOutputStream::writeArrayC(const std::vector<char>& buffer) const
{
    writeNumberC<std::uint32_t>(static_cast<std::uint32_t>(buffer.size()));
    if (buffer.size() > 0)
    {
        stream_.Write(&buffer[0], buffer.size());
        check();
        if (stream_.LastWrite() != buffer.size()) //some additional check
            throw FileError(_("Error writing to synchronization database:") + L" \n" + L"\"" + errorObjName_ + L"\"");
    }
}


inline
void WriteOutputStream::check() const
{
    if (stream_.GetLastError() != wxSTREAM_NO_ERROR)
        throw FileError(_("Error writing to synchronization database:") + L" \n" + L"\"" + errorObjName_ + L"\"");
}

}

#endif //SERIALIZE_H_INCLUDED
