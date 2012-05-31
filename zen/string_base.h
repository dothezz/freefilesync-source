// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef Z_BASE_H_INCLUDED
#define Z_BASE_H_INCLUDED

#include <algorithm>
#include <cassert>
#include <cstdint>
#include "string_tools.h"
#include <boost/detail/atomic_count.hpp>

//Zbase - a policy based string class optimizing performance and genericity


namespace zen
{
/*
Allocator Policy:
-----------------
    void* allocate(size_t size) //throw std::bad_alloc
    void deallocate(void* ptr)
    size_t calcCapacity(size_t length)
*/
class AllocatorOptimalSpeed //exponential growth + min size
{
public:
    //::operator new/ ::operator delete show same performance characterisics like malloc()/free()!
    static void* allocate(size_t size) { return ::operator new(size); } //throw std::bad_alloc
    static void  deallocate(void* ptr) { ::operator delete(ptr); }
    static size_t calcCapacity(size_t length) { return std::max<size_t>(16, length + length / 2); } //any growth rate should not exceed golden ratio: 1.618033989
};


class AllocatorOptimalMemory //no wasted memory, but more reallocations required when manipulating string
{
public:
    static void* allocate(size_t size) { return ::operator new(size); } //throw std::bad_alloc
    static void  deallocate(void* ptr) { ::operator delete(ptr); }
    static size_t calcCapacity(size_t length) { return length; }
};

/*
Storage Policy:
---------------
template <typename Char, //Character Type
         class AP>       //Allocator Policy

    Char* create(size_t size)
    Char* create(size_t size, size_t minCapacity)
    Char* clone(Char* ptr)
    void destroy(Char* ptr)
    bool canWrite(const Char* ptr, size_t minCapacity) //needs to be checked before writing to "ptr"
    size_t length(const Char* ptr)
    void setLength(Char* ptr, size_t newLength)
*/

template <typename Char, //Character Type
         class    AP>   //Allocator Policy
class StorageDeepCopy : public AP
{
protected:
    ~StorageDeepCopy() {}

    static Char* create(size_t size) { return create(size, size); }
    static Char* create(size_t size, size_t minCapacity)
    {
        const size_t newCapacity = AP::calcCapacity(minCapacity);
        assert(newCapacity >= minCapacity);
        assert(minCapacity >= size);

        Descriptor* const newDescr = static_cast<Descriptor*>(AP::allocate(sizeof(Descriptor) + (newCapacity + 1) * sizeof(Char)));

        newDescr->length   = size;
        newDescr->capacity = newCapacity;

        return reinterpret_cast<Char*>(newDescr + 1); //alignment note: "newDescr + 1" is Descriptor-aligned, which is larger than alignment for Char-array! => no problem!
    }

    static Char* clone(Char* ptr)
    {
        Char* newData = create(length(ptr));
        std::copy(ptr, ptr + length(ptr) + 1, newData);
        return newData;
    }

    static void destroy(Char* ptr) { AP::deallocate(descr(ptr)); }

    //this needs to be checked before writing to "ptr"
    static bool canWrite(const Char* ptr, size_t minCapacity) { return minCapacity <= descr(ptr)->capacity; }
    static size_t length(const Char* ptr) { return descr(ptr)->length; }

    static void setLength(Char* ptr, size_t newLength)
    {
        assert(canWrite(ptr, newLength));
        descr(ptr)->length = newLength;
    }

private:
    struct Descriptor
    {
        std::uint32_t length;
        std::uint32_t capacity; //allocated size without null-termination
    };

    static       Descriptor* descr(      Char* ptr) { return reinterpret_cast<      Descriptor*>(ptr) - 1; }
    static const Descriptor* descr(const Char* ptr) { return reinterpret_cast<const Descriptor*>(ptr) - 1; }
};


template <typename Char, //Character Type
         class AP>      //Allocator Policy
class StorageRefCountThreadSafe : public AP
{
protected:
    ~StorageRefCountThreadSafe() {}

    static Char* create(size_t size) { return create(size, size); }
    static Char* create(size_t size, size_t minCapacity)
    {
        const size_t newCapacity = AP::calcCapacity(minCapacity);
        assert(newCapacity >= minCapacity);
        assert(minCapacity >= size);

        Descriptor* const newDescr = static_cast<Descriptor*>(AP::allocate(sizeof(Descriptor) + (newCapacity + 1) * sizeof(Char)));
        new (newDescr) Descriptor(1, size, newCapacity);

        return reinterpret_cast<Char*>(newDescr + 1);
    }

    static Char* clone(Char* ptr)
    {
        assert(descr(ptr)->refCount > 0);
        ++descr(ptr)->refCount;
        return ptr;
    }

    static void destroy(Char* ptr)
    {
        if (--descr(ptr)->refCount == 0)
        {
            descr(ptr)->~Descriptor();
            AP::deallocate(descr(ptr));
        }
    }

    static bool canWrite(const Char* ptr, size_t minCapacity) //needs to be checked before writing to "ptr"
    {
        assert(descr(ptr)->refCount > 0);
        return descr(ptr)->refCount == 1 && minCapacity <= descr(ptr)->capacity;
    }

    static size_t length(const Char* ptr) { return descr(ptr)->length; }

    static void setLength(Char* ptr, size_t newLength)
    {
        assert(canWrite(ptr, newLength));
        descr(ptr)->length = static_cast<std::uint32_t>(newLength);
    }

private:
    struct Descriptor
    {
        Descriptor(long rc, size_t len, size_t cap) :
            refCount(rc),
            length(static_cast<std::uint32_t>(len)),
            capacity(static_cast<std::uint32_t>(cap)) {}

        boost::detail::atomic_count refCount; //practically no perf loss: ~0.2%! (FFS comparison)
        //replace by #include <atomic> std::atomic_int when finally getting rid of VS2010
        std::uint32_t length;
        std::uint32_t capacity; //allocated size without null-termination
    };

    static       Descriptor* descr(      Char* ptr) { return reinterpret_cast<      Descriptor*>(ptr) - 1; }
    static const Descriptor* descr(const Char* ptr) { return reinterpret_cast<const Descriptor*>(ptr) - 1; }
};
//################################################################################################################################################################


//perf note: interstingly StorageDeepCopy and StorageRefCountThreadSafe show same performance in FFS comparison

template <class Char,  							                       //Character Type
         template <class, class> class SP = StorageRefCountThreadSafe, //Storage Policy
         class AP = AllocatorOptimalSpeed>				               //Allocator Policy
class Zbase : public SP<Char, AP>
{
public:
    Zbase();
    Zbase(const Char* source); //implicit conversion from a C-string
    Zbase(const Char* source, size_t length);
    Zbase(const Zbase& source);
    Zbase(Zbase&& tmp);
    explicit Zbase(Char source); //dangerous if implicit: Char buffer[]; return buffer[0]; ups... forgot &, but no error
    //allow explicit construction from different string type, prevent ambiguity via SFINAE
    template <class S> explicit Zbase(const S& other, typename S::value_type = 0);
    ~Zbase();

    //operator const Char* () const; //NO implicit conversion to a C-string!! Many problems... one of them: if we forget to provide operator overloads, it'll just work with a Char*...

    //STL accessors
    typedef       Char*       iterator;
    typedef const Char* const_iterator;
    typedef       Char&       reference;
    typedef const Char& const_reference;
    typedef       Char value_type;
    const Char* begin() const;
    const Char* end()   const;
    Char*       begin();
    Char*       end();

    //std::string functions
    size_t length() const;
    size_t size  () const { return length(); }
    const Char* c_str() const { return rawStr; }; //C-string format with 0-termination
    const Char* data()  const { return rawStr; }; //internal representation, 0-termination not guaranteed
    const Char operator[](size_t pos) const;
    bool empty() const { return length() == 0; }
    void clear();
    size_t find (const Zbase& str, size_t pos = 0)    const; //
    size_t find (const Char* str,  size_t pos = 0)    const; //
    size_t find (Char  ch,         size_t pos = 0)    const; //returns "npos" if not found
    size_t rfind(Char  ch,         size_t pos = npos) const; //
    size_t rfind(const Char* str,  size_t pos = npos) const; //
    Zbase& replace(size_t pos1, size_t n1, const Zbase& str);
    void reserve(size_t minCapacity);
    Zbase& assign(const Char* source, size_t len);
    Zbase& append(const Char* source, size_t len);
    void resize(size_t newSize, Char fillChar = 0);
    void swap(Zbase& other);
    void push_back(Char val) { operator+=(val); } //STL access

    Zbase& operator=(Zbase source);
    Zbase& operator=(const Char* source);
    Zbase& operator=(Char source);
    Zbase& operator+=(const Zbase& other);
    Zbase& operator+=(const Char* other);
    Zbase& operator+=(Char ch);

    static const size_t	npos = static_cast<size_t>(-1);

private:
    Zbase(int);             //
    Zbase& operator=(int);  //detect usage errors
    Zbase& operator+=(int); //
    void push_back(int);    //

    Char* rawStr;
};

template <class Char, template <class, class> class SP, class AP>        bool operator==(const Zbase<Char, SP, AP>& lhs, const Zbase<Char, SP, AP>& rhs);
template <class Char, template <class, class> class SP, class AP>        bool operator==(const Zbase<Char, SP, AP>& lhs, const Char*                rhs);
template <class Char, template <class, class> class SP, class AP> inline bool operator==(const Char*                lhs, const Zbase<Char, SP, AP>& rhs) { return operator==(rhs, lhs); }

template <class Char, template <class, class> class SP, class AP> inline bool operator!=(const Zbase<Char, SP, AP>& lhs, const Zbase<Char, SP, AP>& rhs) { return !operator==(lhs, rhs); }
template <class Char, template <class, class> class SP, class AP> inline bool operator!=(const Zbase<Char, SP, AP>& lhs, const Char*                rhs) { return !operator==(lhs, rhs); }
template <class Char, template <class, class> class SP, class AP> inline bool operator!=(const Char*                lhs, const Zbase<Char, SP, AP>& rhs) { return !operator==(lhs, rhs); }

template <class Char, template <class, class> class SP, class AP> bool operator<(const Zbase<Char, SP, AP>& lhs, const Zbase<Char, SP, AP>& rhs);
template <class Char, template <class, class> class SP, class AP> bool operator<(const Zbase<Char, SP, AP>& lhs, const Char*                rhs);
template <class Char, template <class, class> class SP, class AP> bool operator<(const Char*                lhs, const Zbase<Char, SP, AP>& rhs);

//rvalue references: unified first argument!
template <class Char, template <class, class> class SP, class AP> inline Zbase<Char, SP, AP> operator+(Zbase<Char, SP, AP> lhs, const Zbase<Char, SP, AP>& rhs) { return lhs += rhs; }
template <class Char, template <class, class> class SP, class AP> inline Zbase<Char, SP, AP> operator+(Zbase<Char, SP, AP> lhs, const Char*                rhs) { return lhs += rhs; }
template <class Char, template <class, class> class SP, class AP> inline Zbase<Char, SP, AP> operator+(Zbase<Char, SP, AP> lhs,       Char                 rhs) { return lhs += rhs; }
template <class Char, template <class, class> class SP, class AP> inline Zbase<Char, SP, AP> operator+(      Char          lhs, const Zbase<Char, SP, AP>& rhs) { return Zbase<Char, SP, AP>(lhs) += rhs; }
template <class Char, template <class, class> class SP, class AP> inline Zbase<Char, SP, AP> operator+(const Char*         lhs, const Zbase<Char, SP, AP>& rhs) { return Zbase<Char, SP, AP>(lhs) += rhs; }






















//################################# implementation ########################################
template <class Char, template <class, class> class SP, class AP> inline
Zbase<Char, SP, AP>::Zbase()
{
    //resist the temptation to avoid this allocation by referening a static global: NO performance advantage, MT issues!
    rawStr    = this->create(0);
    rawStr[0] = 0;
}


template <class Char, template <class, class> class SP, class AP> inline
Zbase<Char, SP, AP>::Zbase(Char source)
{
    rawStr    = this->create(1);
    rawStr[0] = source;
    rawStr[1] = 0;
}


template <class Char, template <class, class> class SP, class AP> inline
Zbase<Char, SP, AP>::Zbase(const Char* source)
{
    const size_t sourceLen = strLength(source);
    rawStr = this->create(sourceLen);
    std::copy(source, source + sourceLen + 1, rawStr); //include null-termination
}


template <class Char, template <class, class> class SP, class AP> inline
Zbase<Char, SP, AP>::Zbase(const Char* source, size_t sourceLen)
{
    rawStr = this->create(sourceLen);
    std::copy(source, source + sourceLen, rawStr);
    rawStr[sourceLen] = 0;
}


template <class Char, template <class, class> class SP, class AP> inline
Zbase<Char, SP, AP>::Zbase(const Zbase<Char, SP, AP>& source)
{
    rawStr = this->clone(source.rawStr);
}


template <class Char, template <class, class> class SP, class AP> inline
Zbase<Char, SP, AP>::Zbase(Zbase<Char, SP, AP>&& tmp)
{
    //rawStr = this->clone(tmp.rawStr); NO! do not increment ref-count of a potentially unshared string! We'd lose optimization opportunity of reusing it!
    //instead create a dummy string and swap:
    if (this->canWrite(tmp.rawStr, 0)) //perf: this check saves about 4%
    {
        rawStr    = this->create(0); //no perf issue! see comment in default constructor
        rawStr[0] = 0;
        swap(tmp);
    }
    else //shared representation: yet another "add ref" won't hurt
        rawStr = this->clone(tmp.rawStr);
}


template <class Char, template <class, class> class SP, class AP>
template <class S> inline
Zbase<Char, SP, AP>::Zbase(const S& other, typename S::value_type)
{
    const size_t sourceLen = other.size();
    rawStr = this->create(sourceLen);
    std::copy(other.c_str(), other.c_str() + sourceLen, rawStr);
    rawStr[sourceLen] = 0;
}


template <class Char, template <class, class> class SP, class AP> inline
Zbase<Char, SP, AP>::~Zbase()
{
    this->destroy(rawStr);
}


template <class Char, template <class, class> class SP, class AP> inline
size_t Zbase<Char, SP, AP>::find(const Zbase& str, size_t pos) const
{
    assert(pos <= length());
    const Char* thisEnd = end(); //respect embedded 0
    const Char* iter = std::search(begin() + pos, thisEnd,
                                   str.begin(), str.end());
    return iter == thisEnd ? npos : iter - begin();
}


template <class Char, template <class, class> class SP, class AP> inline
size_t Zbase<Char, SP, AP>::find(const Char* str, size_t pos) const
{
    assert(pos <= length());
    const Char* thisEnd = end(); //respect embedded 0
    const Char* iter = std::search(begin() + pos, thisEnd,
                                   str, str + strLength(str));
    return iter == thisEnd ? npos : iter - begin();
}


template <class Char, template <class, class> class SP, class AP> inline
size_t Zbase<Char, SP, AP>::find(Char ch, size_t pos) const
{
    assert(pos <= length());
    const Char* thisEnd = end();
    const Char* iter = std::find(begin() + pos, thisEnd, ch); //respect embedded 0
    return iter == thisEnd ? npos : iter - begin();
}


template <class Char, template <class, class> class SP, class AP> inline
size_t Zbase<Char, SP, AP>::rfind(Char ch, size_t pos) const
{
    assert(pos == npos || pos <= length());

    const Char* currEnd = pos == npos ? end() : begin() + std::min(pos + 1, length());

    const Char* iter = find_last(begin(), currEnd, ch);
    return iter == currEnd ? npos : iter - begin();
}


template <class Char, template <class, class> class SP, class AP> inline
size_t Zbase<Char, SP, AP>::rfind(const Char* str, size_t pos) const
{
    assert(pos == npos || pos <= length());

    const size_t strLen = strLength(str);
    const Char* currEnd = pos == npos ? end() : begin() + std::min(pos + strLen, length());

    const Char* iter = search_last(begin(), currEnd,
                                   str, str + strLen);
    return iter == currEnd ? npos : iter - begin();
}


template <class Char, template <class, class> class SP, class AP>
Zbase<Char, SP, AP>& Zbase<Char, SP, AP>::replace(size_t pos1, size_t n1, const Zbase& str)
{
    assert(str.data() < rawStr || rawStr + length() < str.data()); //str mustn't point to data in this string
    assert(pos1 + n1 <= length());

    const size_t n2 = str.length();

    const size_t oldLen = length();
    if (oldLen == 0)
        return *this = str;

    const size_t newLen = oldLen - n1 + n2;

    if (this->canWrite(rawStr, newLen))
    {
        if (n1 < n2) //move remainder right -> std::copy_backward
        {
            std::copy_backward(rawStr + pos1 + n1, rawStr + oldLen + 1, rawStr + newLen + 1); //include null-termination
            this->setLength(rawStr, newLen);
        }
        else if (n1 > n2) //shift left -> std::copy
        {
            std::copy(rawStr + pos1 + n1, rawStr + oldLen + 1, rawStr + pos1 + n2); //include null-termination
            this->setLength(rawStr, newLen);
        }

        std::copy(str.data(), str.data() + n2, rawStr + pos1);
    }
    else
    {
        //copy directly into new string
        Char* const newStr = this->create(newLen);

        std::copy(rawStr, rawStr + pos1, newStr);
        std::copy(str.data(), str.data() + n2, newStr + pos1);
        std::copy(rawStr + pos1 + n1, rawStr + oldLen + 1, newStr + pos1 + n2); //include null-termination

        this->destroy(rawStr);
        rawStr = newStr;
    }
    return *this;
}


template <class Char, template <class, class> class SP, class AP> inline
void Zbase<Char, SP, AP>::resize(size_t newSize, Char fillChar)
{
    if (this->canWrite(rawStr, newSize))
    {
        if (length() < newSize)
            std::fill(rawStr + length(), rawStr + newSize, fillChar);
        rawStr[newSize] = 0;
        this->setLength(rawStr, newSize); //keep after call to length()
    }
    else
    {
        Char* newStr = this->create(newSize);
        newStr[newSize] = 0;

        if (length() < newSize)
        {
            std::copy(rawStr, rawStr + length(), newStr);
            std::fill(newStr + length(), newStr + newSize, fillChar);
        }
        else
            std::copy(rawStr, rawStr + newSize, newStr);

        this->destroy(rawStr);
        rawStr = newStr;
    }
}


template <class Char, template <class, class> class SP, class AP> inline
bool operator==(const Zbase<Char, SP, AP>& lhs, const Zbase<Char, SP, AP>& rhs)
{
    return lhs.length() == rhs.length() && std::equal(lhs.begin(), lhs.end(), rhs.begin()); //respect embedded 0
}


template <class Char, template <class, class> class SP, class AP> inline
bool operator==(const Zbase<Char, SP, AP>& lhs, const Char* rhs)
{
    return lhs.length() == strLength(rhs) && std::equal(lhs.begin(), lhs.end(), rhs); //respect embedded 0
}


template <class Char, template <class, class> class SP, class AP> inline
bool operator<(const Zbase<Char, SP, AP>& lhs, const Zbase<Char, SP, AP>& rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), //respect embedded 0
                                        rhs.begin(), rhs.end());
}


template <class Char, template <class, class> class SP, class AP> inline
bool operator<(const Zbase<Char, SP, AP>& lhs, const Char* rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), //respect embedded 0
                                        rhs, rhs + strLength(rhs));
}


template <class Char, template <class, class> class SP, class AP> inline
bool operator<(const Char* lhs, const Zbase<Char, SP, AP>& rhs)
{
    return std::lexicographical_compare(lhs, lhs + strLength(lhs), //respect embedded 0
                                        rhs.begin(), rhs.end());
}


template <class Char, template <class, class> class SP, class AP> inline
size_t Zbase<Char, SP, AP>::length() const
{
    return SP<Char, AP>::length(rawStr);
}


template <class Char, template <class, class> class SP, class AP> inline
const Char Zbase<Char, SP, AP>::operator[](size_t pos) const
{
    assert(pos < length());
    return rawStr[pos];
}


template <class Char, template <class, class> class SP, class AP> inline
const Char* Zbase<Char, SP, AP>::begin() const
{
    return rawStr;
}


template <class Char, template <class, class> class SP, class AP> inline
const Char* Zbase<Char, SP, AP>::end() const
{
    return rawStr + length();
}


template <class Char, template <class, class> class SP, class AP> inline
Char* Zbase<Char, SP, AP>::begin()
{
    reserve(length()); //make unshared!
    return rawStr;
}


template <class Char, template <class, class> class SP, class AP> inline
Char* Zbase<Char, SP, AP>::end()
{
    return begin() + length();
}


template <class Char, template <class, class> class SP, class AP> inline
void Zbase<Char, SP, AP>::clear()
{
    if (!empty())
    {
        if (this->canWrite(rawStr, 0))
        {
            rawStr[0] = 0;        //keep allocated memory
            this->setLength(rawStr, 0); //
        }
        else
            *this = Zbase();
    }
}


template <class Char, template <class, class> class SP, class AP> inline
void Zbase<Char, SP, AP>::swap(Zbase<Char, SP, AP>& other)
{
    std::swap(rawStr, other.rawStr);
}


template <class Char, template <class, class> class SP, class AP> inline
void Zbase<Char, SP, AP>::reserve(size_t minCapacity) //make unshared and check capacity
{
    if (!this->canWrite(rawStr, minCapacity))
    {
        //allocate a new string
        Char* newStr = this->create(length(), std::max(minCapacity, length())); //reserve() must NEVER shrink the string: logical const!
        std::copy(rawStr, rawStr + length() + 1, newStr); //include 0-termination

        this->destroy(rawStr);
        rawStr = newStr;
    }
}


template <class Char, template <class, class> class SP, class AP> inline
Zbase<Char, SP, AP>& Zbase<Char, SP, AP>::assign(const Char* source, size_t len)
{
    if (this->canWrite(rawStr, len))
    {
        std::copy(source, source + len, rawStr);
        rawStr[len] = 0; //include null-termination
        this->setLength(rawStr, len);
    }
    else
        *this = Zbase(source, len);

    return *this;
}


template <class Char, template <class, class> class SP, class AP> inline
Zbase<Char, SP, AP>& Zbase<Char, SP, AP>::append(const Char* source, size_t len)
{
    const size_t thisLen = length();
    reserve(thisLen + len); //make unshared and check capacity

    std::copy(source, source + len, rawStr + thisLen);
    rawStr[thisLen + len] = 0;
    this->setLength(rawStr, thisLen + len);
    return *this;
}


template <class Char, template <class, class> class SP, class AP> inline
Zbase<Char, SP, AP>& Zbase<Char, SP, AP>::operator=(Zbase<Char, SP, AP> other) //unifying assignment: no need for r-value reference optimization!
{
    swap(other);
    return *this;
}


template <class Char, template <class, class> class SP, class AP> inline
Zbase<Char, SP, AP>& Zbase<Char, SP, AP>::operator=(const Char* source)
{
    return assign(source, strLength(source));
}


template <class Char, template <class, class> class SP, class AP> inline
Zbase<Char, SP, AP>& Zbase<Char, SP, AP>::operator=(Char ch)
{
    return assign(&ch, 1);
}


template <class Char, template <class, class> class SP, class AP> inline
Zbase<Char, SP, AP>& Zbase<Char, SP, AP>::operator+=(const Zbase<Char, SP, AP>& other)
{
    return append(other.c_str(), other.length());
}


template <class Char, template <class, class> class SP, class AP> inline
Zbase<Char, SP, AP>& Zbase<Char, SP, AP>::operator+=(const Char* other)
{
    return append(other, strLength(other));
}


template <class Char, template <class, class> class SP, class AP> inline
Zbase<Char, SP, AP>& Zbase<Char, SP, AP>::operator+=(Char ch)
{
    return append(&ch, 1);
}
}

#endif //Z_BASE_H_INCLUDED
