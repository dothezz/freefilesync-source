// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef SERIALIZE_H_INCLUDED
#define SERIALIZE_H_INCLUDED

#include <cstdint>
#include <zen/string_base.h>
#include <zen/file_io.h>

#ifdef FFS_WIN
warn_static("get rid of wx, then move to zen")
#endif

#include <wx/stream.h>


namespace zen
{
//high-performance unformatted serialization (avoiding wxMemoryOutputStream/wxMemoryInputStream inefficiencies)

/*
--------------------------
|Binary Container Concept|
--------------------------
binary container for data storage: must support "basic" std::vector interface (e.g. std::vector<char>, std::string, Zbase<char>)
*/

//binary container reference implementations
typedef Zbase<char> Utf8String; //ref-counted + COW text stream + guaranteed performance: exponential growth
class BinaryStream;             //ref-counted       byte stream + guaranteed performance: exponential growth -> no COW, but 12% faster than Utf8String (due to no null-termination?)

class BinaryStream //essentially a std::vector<char> with ref-counted semantics
{
public:
    BinaryStream() : buffer(std::make_shared<std::vector<char>>()) {}

    typedef std::vector<char>::value_type value_type;
    typedef std::vector<char>::iterator iterator;
    typedef std::vector<char>::const_iterator const_iterator;

    iterator begin() { return buffer->begin(); }
    iterator end  () { return buffer->end  (); }

    const_iterator begin() const { return buffer->begin(); }
    const_iterator end  () const { return buffer->end  (); }

    void resize(size_t len) { buffer->resize(len); }
    size_t size() const { return buffer->size(); }
    bool empty() const { return buffer->empty(); }

    inline friend bool operator==(const BinaryStream& lhs, const BinaryStream& rhs) { return *lhs.buffer == *rhs.buffer; }

private:
    std::shared_ptr<std::vector<char>> buffer; //always bound!
    //perf: shared_ptr indirection irrelevant: less than 1% slower!
};

//----------------------------------------------------------------------
//functions based on binary container abstraction
template <class BinContainer>         void saveBinStream(const Zstring& filename, const BinContainer& cont); //throw FileError
template <class BinContainer> BinContainer loadBinStream(const Zstring& filename); //throw FileError, ErrorNotExisting


/*
-----------------------------
|Binary Input Stream Concept|
-----------------------------
struct BinInputStream
{
    const void* requestRead(size_t len); //expect external read of len bytes
};

------------------------------
|Binary Output Stream Concept|
------------------------------
struct BinOutputStream
{
    void* requestWrite(size_t len); //expect external write of len bytes
};
*/

//binary input/output stream reference implementation
class UnexpectedEndOfStreamError {};

struct BinStreamIn //throw UnexpectedEndOfStreamError
{
    BinStreamIn(const BinaryStream& cont) : buffer(cont), pos(0) {} //this better be cheap!

    const void* requestRead(size_t len) //throw UnexpectedEndOfStreamError
    {
        if (pos + len > buffer.size())
            throw UnexpectedEndOfStreamError();
        size_t oldPos = pos;
        pos += len;
        return &*buffer.begin() + oldPos;
    }

private:
    const BinaryStream buffer;
    size_t pos;
};

struct BinStreamOut
{
    void* requestWrite(size_t len)
    {
        size_t oldSize = buffer.size();
        buffer.resize(buffer.size() + len);
        return &*buffer.begin() + oldSize;
    }

    BinaryStream get() { return buffer; }

private:
    BinaryStream buffer;
};

//----------------------------------------------------------------------
//functions based on binary stream abstraction
template <class N, class BinOutputStream> void writeNumber   (BinOutputStream& stream, const N& num);                 //
template <class C, class BinOutputStream> void writeContainer(BinOutputStream& stream, const C& str);                 //throw ()
template <         class BinOutputStream> void writeArray    (BinOutputStream& stream, const void* data, size_t len); //

//----------------------------------------------------------------------

template <class N, class BinInputStream> N    readNumber   (BinInputStream& stream); //
template <class C, class BinInputStream> C    readContainer(BinInputStream& stream); //throw UnexpectedEndOfStreamError (corrupted data)
template <         class BinInputStream> void readArray    (BinInputStream& stream, void* data, size_t len); //























//-----------------------implementation-------------------------------
template <class BinContainer> inline
void saveBinStream(const Zstring& filename, const BinContainer& cont) //throw FileError
{
    assert_static(sizeof(typename BinContainer::value_type) == 1); //expect: bytes (until further)

    FileOutput fileOut(filename, zen::FileOutput::ACC_OVERWRITE); //throw FileError
    if (!cont.empty())
        fileOut.write(&*cont.begin(), cont.size()); //throw FileError
}


template <class BinContainer> inline
BinContainer loadBinStream(const Zstring& filename) //throw FileError, ErrorNotExisting
{
    assert_static(sizeof(typename BinContainer::value_type) == 1); //expect: bytes (until further)

    FileInput fileIn(filename); //throw FileError, ErrorNotExisting

    BinContainer contOut;
    const size_t blockSize = 64 * 1024;
    do
    {
        contOut.resize(contOut.size() + blockSize);

        const size_t bytesRead = fileIn.read(&*contOut.begin() + contOut.size() - blockSize, blockSize); //throw FileError
        if (bytesRead < blockSize)
            contOut.resize(contOut.size() - (blockSize - bytesRead)); //caveat: unsigned arithmetics
    }
    while (!fileIn.eof());

    return contOut;
}


template <class BinOutputStream> inline
void writeArray(BinOutputStream& stream, const void* data, size_t len)
{
    std::copy(static_cast<const char*>(data),
              static_cast<const char*>(data) + len,
              static_cast<      char*>(stream.requestWrite(len)));
}


template <class N, class BinOutputStream> inline
void writeNumber(BinOutputStream& stream, const N& num)
{
    assert_static((IsArithmetic<N>::value || IsSameType<N, bool>::value));
    writeArray(stream, &num, sizeof(N));
}


template <class C, class BinOutputStream> inline
void writeContainer(BinOutputStream& stream, const C& cont) //don't even consider UTF8 conversions here! "string" is expected to handle arbitrary binary data!
{
    const auto len = cont.size();
    writeNumber(stream, static_cast<std::uint32_t>(len));
    if (len > 0)
        writeArray(stream, &*cont.begin(), sizeof(typename C::value_type) * len); //don't use c_str(), but access uniformly via STL interface
}


template <class BinInputStream> inline
void readArray(BinInputStream& stream, void* data, size_t len)
{
    const char* const src = static_cast<const char*>(stream.requestRead(len)); //expect external write of len bytes
    std::copy(src, src + len, static_cast<char*>(data));
}


template <class N, class BinInputStream> inline
N readNumber(BinInputStream& stream)
{
    assert_static((IsArithmetic<N>::value || IsSameType<N, bool>::value));
    N num = 0;
    readArray(stream, &num, sizeof(N));
    return num;
}


template <class C, class BinInputStream> inline
C readContainer(BinInputStream& stream)
{
    C cont;
    auto strLength = readNumber<std::uint32_t>(stream);
    if (strLength > 0)
        try
        {
            cont.resize(strLength); //throw std::bad_alloc
            readArray(stream, &*cont.begin(), sizeof(typename C::value_type) * strLength);
        }
        catch (std::bad_alloc&) //most likely this is due to data corruption!
        {
            throw UnexpectedEndOfStreamError();
        }
    return cont;
}


















































#ifdef FFS_WIN
warn_static("get rid of wx, then move to zen")
#endif



//unchecked, unformatted serialization
template <class T> T    readPOD (wxInputStream&  stream);
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
    if (stream_.LastRead() != str.size() * sizeof(typename S::value_type)) //some additional check
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
    //warn_static("buggy check if length 0!")
    if (stream_.LastWrite() != str.length() * sizeof(typename S::value_type)) //some additional check
        throwException();
}
}

#endif //SERIALIZE_H_INCLUDED
