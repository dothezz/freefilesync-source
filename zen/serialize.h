// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef SERIALIZE_H_INCLUDED
#define SERIALIZE_H_INCLUDED

#include <cstdint>
#include <zen/string_base.h>
#include <zen/file_io.h>


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

class BinaryStream //essentially a std::vector<char> with ref-counted semantics, but no COW! => *almost* value type semantics, but not quite
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
        if (len == 0) return nullptr; //don't allow for possibility to access empty buffer
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
        if (len == 0) return nullptr; //don't allow for possibility to access empty buffer
        const size_t oldSize = buffer.size();
        buffer.resize(oldSize + len);
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
    const size_t blockSize = 128 * 1024;
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
void writeContainer(BinOutputStream& stream, const C& cont) //don't even consider UTF8 conversions here, we're handling arbitrary binary data!
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
}

#endif //SERIALIZE_H_INCLUDED
