// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef Z_BASE_H_INCLUDED
#define Z_BASE_H_INCLUDED

#include <algorithm>
#include <cassert>
#include "string_tools.h"
#include <boost/detail/atomic_count.hpp>

//Zbase - a policy based string class


/*
Allocator Policy:
-----------------
    void* allocate(size_t size) //throw (std::bad_alloc)
    void deallocate(void* ptr)
    size_t calcCapacity(size_t length)
*/
class AllocatorOptimalSpeed //exponential growth + min size
{
public:
    //::operator new/ ::operator delete show same performance characterisics like malloc()/free()!
    static void* allocate(size_t size) { return ::operator new(size); } //throw (std::bad_alloc)
    static void  deallocate(void* ptr) { ::operator delete(ptr); }
    static size_t calcCapacity(size_t length) { return std::max<size_t>(16, length + length / 2); }
};


class AllocatorOptimalMemory //no wasted memory, but more reallocations required when manipulating string
{
public:
    static void* allocate(size_t size) { return ::operator new(size); } //throw (std::bad_alloc)
    static void  deallocate(void* ptr) { ::operator delete(ptr); }
    static size_t calcCapacity(size_t length) { return length; }
};

/*
Storage Policy:
---------------
template <typename T, //Character Type
         class AP>    //Allocator Policy

    T* create(size_t size)
    T* create(size_t size, size_t minCapacity)
    T* clone(T* ptr)
    void destroy(T* ptr)
    bool canWrite(const T* ptr, size_t minCapacity) //needs to be checked before writing to "ptr"
    size_t length(const T* ptr)
    void setLength(T* ptr, size_t newLength)
*/

template <typename T, //Character Type
         class AP>   //Allocator Policy
class StorageDeepCopy : public AP
{
protected:
    ~StorageDeepCopy() {}

    static T* create(size_t size) { return create(size, size); }
    static T* create(size_t size, size_t minCapacity)
    {
        const size_t newCapacity = AP::calcCapacity(minCapacity);
        assert(newCapacity >= minCapacity);
        assert(minCapacity >= size);

        Descriptor* const newDescr = static_cast<Descriptor*>(AP::allocate(sizeof(Descriptor) + (newCapacity + 1) * sizeof(T)));

        newDescr->length   = size;
        newDescr->capacity = newCapacity;

        return reinterpret_cast<T*>(newDescr + 1);
    }

    static T* clone(T* ptr)
    {
        T* newData = create(length(ptr));
        std::copy(ptr, ptr + length(ptr) + 1, newData);
        return newData;
    }

    static void destroy(T* ptr) { AP::deallocate(descr(ptr)); }

    //this needs to be checked before writing to "ptr"
    static bool canWrite(const T* ptr, size_t minCapacity) { return minCapacity <= descr(ptr)->capacity; }
    static size_t length(const T* ptr) { return descr(ptr)->length; }

    static void setLength(T* ptr, size_t newLength)
    {
        assert(canWrite(ptr, newLength));
        descr(ptr)->length = newLength;
    }

private:
    struct Descriptor
    {
        size_t length;
        size_t capacity; //allocated size without null-termination
    };

    static       Descriptor* descr(      T* ptr) { return reinterpret_cast<      Descriptor*>(ptr) - 1; }
    static const Descriptor* descr(const T* ptr) { return reinterpret_cast<const Descriptor*>(ptr) - 1; }
};


template <typename T, //Character Type
         class AP>   //Allocator Policy
class StorageRefCountThreadSafe : public AP
{
protected:
    ~StorageRefCountThreadSafe() {}

    static T* create(size_t size)
    {
        return create(size, size);
    }

    static T* create(size_t size, size_t minCapacity)
    {
        const size_t newCapacity = AP::calcCapacity(minCapacity);
        assert(newCapacity >= minCapacity);
        assert(minCapacity >= size);

        Descriptor* const newDescr = static_cast<Descriptor*>(AP::allocate(sizeof(Descriptor) + (newCapacity + 1) * sizeof(T)));
        new (newDescr) Descriptor(1, size, newCapacity);

        return reinterpret_cast<T*>(newDescr + 1);
    }

    static T* clone(T* ptr)
    {
        assert(descr(ptr)->refCount > 0);
        ++descr(ptr)->refCount;
        return ptr;
    }

    static void destroy(T* ptr)
    {
        if (--descr(ptr)->refCount == 0)
        {
            descr(ptr)->~Descriptor();
            AP::deallocate(descr(ptr));
        }
    }

    static bool canWrite(const T* ptr, size_t minCapacity) //needs to be checked before writing to "ptr"
    {
        assert(descr(ptr)->refCount > 0);
        return descr(ptr)->refCount == 1 && minCapacity <= descr(ptr)->capacity;
    }

    static size_t length(const T* ptr)
    {
        return descr(ptr)->length;
    }

    static void setLength(T* ptr, size_t newLength)
    {
        assert(canWrite(ptr, newLength));
        descr(ptr)->length = newLength;
    }

private:
    struct Descriptor
    {
        Descriptor(long rc, size_t len, size_t cap) : refCount(rc), length(len), capacity(cap) {}

        boost::detail::atomic_count refCount; //practically no perf loss: ~0.2%! (FFS comparison)
        size_t length;
        size_t capacity; //allocated size without null-termination
    };

    static       Descriptor* descr(      T* ptr) { return reinterpret_cast<      Descriptor*>(ptr) - 1; }
    static const Descriptor* descr(const T* ptr) { return reinterpret_cast<const Descriptor*>(ptr) - 1; }
};
//################################################################################################################################################################


//perf note: interstingly StorageDeepCopy and StorageRefCountThreadSafe show same performance in FFS comparison

template <class T,									                  //Character Type
         template <class, class> class SP = StorageRefCountThreadSafe, //Storage Policy
         class AP = AllocatorOptimalSpeed>				              //Allocator Policy
class Zbase : public SP<T, AP>
{
public:
    Zbase();
    Zbase(const T* source); //implicit conversion from a C-string
    Zbase(const T* source, size_t length);
    Zbase(const Zbase& source);
    Zbase(Zbase&& tmp);
    explicit Zbase(T source); //dangerous if implicit: T buffer[]; Zbase name = buffer; ups...
    //allow explicit construction from different string type, prevent ambiguity via SFINAE
    template <class S> explicit Zbase(const S& other, typename S::value_type = 0);
    ~Zbase();

    //operator const T* () const; //NO implicit conversion to a C-string!! Many problems... one of them: if we forget to provide operator overloads, it'll just work with a T*...

    //STL accessors
    typedef       T*       iterator;
    typedef const T* const_iterator;
    typedef       T&       reference;
    typedef const T& const_reference;
    typedef       T value_type;
    const T* begin() const;
    const T* end()   const;
    T*       begin();
    T*       end();

    //std::string functions
    size_t length() const;
    size_t size() const;
    const T* c_str() const; //C-string format with NULL-termination
    const T* data()  const; //internal representation, NULL-termination not guaranteed
    const T operator[](size_t pos) const;
    bool empty() const;
    void clear();
    size_t find(const Zbase& str, size_t pos = 0)    const; //
    size_t find(const T* str,     size_t pos = 0)    const; //returns "npos" if not found
    size_t find(T ch,             size_t pos = 0)    const; //
    size_t rfind(T ch,            size_t pos = npos) const; //
    size_t rfind(const T* str,    size_t pos = npos) const; //
    Zbase& replace(size_t pos1, size_t n1, const Zbase& str);
    void reserve(size_t minCapacity);
    Zbase& assign(const T* source, size_t len);
    void resize(size_t newSize, T fillChar = 0);
    void swap(Zbase& other);
    void push_back(T val); //STL access

    Zbase& operator=(const Zbase& source);
    Zbase& operator=(Zbase&& tmp);
    Zbase& operator=(const T* source);
    Zbase& operator=(T source);
    Zbase& operator+=(const Zbase& other);
    Zbase& operator+=(const T* other);
    Zbase& operator+=(T ch);

    static const size_t	npos = static_cast<size_t>(-1);

private:
    Zbase(int);            //detect usage errors
    Zbase& operator=(int); //

    T* rawStr;
};

template <class T, template <class, class> class SP, class AP> bool operator==(const Zbase<T, SP, AP>& lhs, const Zbase<T, SP, AP>& rhs);
template <class T, template <class, class> class SP, class AP> bool operator==(const Zbase<T, SP, AP>& lhs, const T*                rhs);
template <class T, template <class, class> class SP, class AP> bool operator==(const T*                lhs, const Zbase<T, SP, AP>& rhs);

template <class T, template <class, class> class SP, class AP> bool operator!=(const Zbase<T, SP, AP>& lhs, const Zbase<T, SP, AP>& rhs);
template <class T, template <class, class> class SP, class AP> bool operator!=(const Zbase<T, SP, AP>& lhs, const T*                rhs);
template <class T, template <class, class> class SP, class AP> bool operator!=(const T*                lhs, const Zbase<T, SP, AP>& rhs);

template <class T, template <class, class> class SP, class AP> bool operator< (const Zbase<T, SP, AP>& lhs, const Zbase<T, SP, AP>& rhs);
template <class T, template <class, class> class SP, class AP> bool operator< (const Zbase<T, SP, AP>& lhs, const T*                rhs);
template <class T, template <class, class> class SP, class AP> bool operator< (const T*                lhs, const Zbase<T, SP, AP>& rhs);

template <class T, template <class, class> class SP, class AP> const Zbase<T, SP, AP> operator+(const Zbase<T, SP, AP>& lhs, const Zbase<T, SP, AP>& rhs);
template <class T, template <class, class> class SP, class AP> const Zbase<T, SP, AP> operator+(const Zbase<T, SP, AP>& lhs, const T*                rhs);
template <class T, template <class, class> class SP, class AP> const Zbase<T, SP, AP> operator+(const T*                lhs, const Zbase<T, SP, AP>& rhs);
template <class T, template <class, class> class SP, class AP> const Zbase<T, SP, AP> operator+(      T                 lhs, const Zbase<T, SP, AP>& rhs);
template <class T, template <class, class> class SP, class AP> const Zbase<T, SP, AP> operator+(const Zbase<T, SP, AP>& lhs,       T                 rhs);





































//################################# inline implementation ########################################
template <class T, template <class, class> class SP, class AP> inline
Zbase<T, SP, AP>::Zbase()
{
    //resist the temptation to avoid this allocation by referening a static global: NO performance advantage, MT issues!
    rawStr    = this->create(0);
    rawStr[0] = 0;
}


template <class T, template <class, class> class SP, class AP> inline
Zbase<T, SP, AP>::Zbase(T source)
{
    rawStr    = this->create(1);
    rawStr[0] = source;
    rawStr[1] = 0;
}


template <class T, template <class, class> class SP, class AP> inline
Zbase<T, SP, AP>::Zbase(const T* source)
{
    const size_t sourceLen = zen::strLength(source);
    rawStr = this->create(sourceLen);
    std::copy(source, source + sourceLen + 1, rawStr); //include null-termination
}


template <class T, template <class, class> class SP, class AP> inline
Zbase<T, SP, AP>::Zbase(const T* source, size_t sourceLen)
{
    rawStr = this->create(sourceLen);
    std::copy(source, source + sourceLen, rawStr);
    rawStr[sourceLen] = 0;
}


template <class T, template <class, class> class SP, class AP> inline
Zbase<T, SP, AP>::Zbase(const Zbase<T, SP, AP>& source)
{
    rawStr = this->clone(source.rawStr);
}


template <class T, template <class, class> class SP, class AP> inline
Zbase<T, SP, AP>::Zbase(Zbase<T, SP, AP>&& tmp)
{
    rawStr = this->clone(tmp.rawStr); //for a ref-counting string there probably isn't a faster way, even with r-value references
}


template <class T, template <class, class> class SP, class AP>
template <class S> inline
Zbase<T, SP, AP>::Zbase(const S& other, typename S::value_type)
{
    const size_t sourceLen = other.size();
    rawStr = this->create(sourceLen);
    std::copy(other.c_str(), other.c_str() + sourceLen, rawStr);
    rawStr[sourceLen] = 0;
}


template <class T, template <class, class> class SP, class AP> inline
Zbase<T, SP, AP>::~Zbase()
{
    this->destroy(rawStr);
}


template <class T, template <class, class> class SP, class AP> inline
size_t Zbase<T, SP, AP>::find(const Zbase& str, size_t pos) const
{
    assert(pos <= length());
    const T* thisEnd = end(); //respect embedded 0
    const T* iter = std::search(begin() + pos, thisEnd,
                                str.begin(), str.end());
    return iter == thisEnd ? npos : iter - begin();
}


template <class T, template <class, class> class SP, class AP> inline
size_t Zbase<T, SP, AP>::find(const T* str, size_t pos) const
{
    assert(pos <= length());
    const T* thisEnd = end(); //respect embedded 0
    const T* iter = std::search(begin() + pos, thisEnd,
                                str, str + zen::strLength(str));
    return iter == thisEnd ? npos : iter - begin();
}


template <class T, template <class, class> class SP, class AP> inline
size_t Zbase<T, SP, AP>::find(T ch, size_t pos) const
{
    assert(pos <= length());
    const T* thisEnd = end();
    const T* iter = std::find(begin() + pos, thisEnd, ch); //respect embedded 0
    return iter == thisEnd ? npos : iter - begin();
}


template <class T, template <class, class> class SP, class AP> inline
size_t Zbase<T, SP, AP>::rfind(T ch, size_t pos) const
{
    assert(pos == npos || pos <= length());

    const size_t thisLen = length();
    if (thisLen == 0) return npos;
    pos = std::min(thisLen - 1, pos); //handle "npos" and "pos == length()" implicitly

    while (rawStr[pos] != ch) //pos points to last char of the string
    {
        if (pos == 0)
            return npos;
        --pos;
    }
    return pos;
}


template <class T, template <class, class> class SP, class AP> inline
size_t Zbase<T, SP, AP>::rfind(const T* str, size_t pos) const
{
    assert(pos == npos || pos <= length());

    const size_t strLen = zen::strLength(str);
    const T* currEnd = pos == npos ? end() : begin() + std::min(pos + strLen, length());

    const T* iter = std::find_end(begin(), currEnd,
                                  str, str + strLen);
    return iter == currEnd ? npos : iter - begin();
}


template <class T, template <class, class> class SP, class AP>
Zbase<T, SP, AP>& Zbase<T, SP, AP>::replace(size_t pos1, size_t n1, const Zbase& str)
{
    assert(str.data() < rawStr || rawStr + length() < str.data()); //str mustn't point to data in this string
    assert(pos1 + n1 <= length());

    const size_t n2 = str.length();

    const size_t oldLen = length();
    if (oldLen == 0)
        return *this = str;

    const size_t newLen = oldLen - n1 + n2;

    if (canWrite(rawStr, newLen))
    {
        if (n1 < n2) //move remainder right -> std::copy_backward
        {
            std::copy_backward(rawStr + pos1 + n1, rawStr + oldLen + 1, rawStr + newLen + 1); //include null-termination
            setLength(rawStr, newLen);
        }
        else if (n1 > n2) //shift left -> std::copy
        {
            std::copy(rawStr + pos1 + n1, rawStr + oldLen + 1, rawStr + pos1 + n2); //include null-termination
            setLength(rawStr, newLen);
        }

        std::copy(str.data(), str.data() + n2, rawStr + pos1);
    }
    else
    {
        //copy directly into new string
        T* const newStr = this->create(newLen);

        std::copy(rawStr, rawStr + pos1, newStr);
        std::copy(str.data(), str.data() + n2, newStr + pos1);
        std::copy(rawStr + pos1 + n1, rawStr + oldLen + 1, newStr + pos1 + n2); //include null-termination

        destroy(rawStr);
        rawStr = newStr;
    }
    return *this;
}


template <class T, template <class, class> class SP, class AP> inline
void Zbase<T, SP, AP>::resize(size_t newSize, T fillChar)
{
    if (canWrite(rawStr, newSize))
    {
        if (length() < newSize)
            std::fill(rawStr + length(), rawStr + newSize, fillChar);
        rawStr[newSize] = 0;
        setLength(rawStr, newSize); //keep after call to length()
    }
    else
    {
        T* newStr = this->create(newSize);
        newStr[newSize] = 0;

        if (length() < newSize)
        {
            std::copy(rawStr, rawStr + length(), newStr);
            std::fill(newStr + length(), newStr + newSize, fillChar);
        }
        else
            std::copy(rawStr, rawStr + newSize, newStr);

        destroy(rawStr);
        rawStr = newStr;
    }
}


template <class T, template <class, class> class SP, class AP> inline
bool operator==(const Zbase<T, SP, AP>& lhs, const Zbase<T, SP, AP>& rhs)
{
    return lhs.length() == rhs.length() && std::equal(lhs.begin(), lhs.end(), rhs.begin()); //respect embedded 0
}


template <class T, template <class, class> class SP, class AP> inline
bool operator==(const Zbase<T, SP, AP>& lhs, const T* rhs)
{
    return lhs.length() == zen::strLength(rhs) && std::equal(lhs.begin(), lhs.end(), rhs); //respect embedded 0
}


template <class T, template <class, class> class SP, class AP> inline
bool operator==(const T* lhs, const Zbase<T, SP, AP>& rhs)
{
    return operator==(rhs, lhs);
}


template <class T, template <class, class> class SP, class AP> inline
bool operator!=(const Zbase<T, SP, AP>& lhs, const Zbase<T, SP, AP>& rhs)
{
    return !operator==(lhs, rhs);
}


template <class T, template <class, class> class SP, class AP> inline
bool operator!=(const Zbase<T, SP, AP>& lhs, const T* rhs)
{
    return !operator==(lhs, rhs);
}


template <class T, template <class, class> class SP, class AP> inline
bool operator!=(const T* lhs, const Zbase<T, SP, AP>& rhs)
{
    return !operator==(lhs, rhs);
}


template <class T, template <class, class> class SP, class AP> inline
bool operator<(const Zbase<T, SP, AP>& lhs, const Zbase<T, SP, AP>& rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), //respect embedded 0
                                        rhs.begin(), rhs.end());
}


template <class T, template <class, class> class SP, class AP> inline
bool operator<(const Zbase<T, SP, AP>& lhs, const T* rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), //respect embedded 0
                                        rhs, rhs + zen::strLength(rhs));
}


template <class T, template <class, class> class SP, class AP> inline
bool operator<(const T* lhs, const Zbase<T, SP, AP>& rhs)
{
    return std::lexicographical_compare(lhs, lhs + zen::strLength(lhs), //respect embedded 0
                                        rhs.begin(), rhs.end());
}


template <class T, template <class, class> class SP, class AP> inline
size_t Zbase<T, SP, AP>::length() const
{
    return SP<T, AP>::length(rawStr);
}


template <class T, template <class, class> class SP, class AP> inline
size_t Zbase<T, SP, AP>::size() const
{
    return length();
}


template <class T, template <class, class> class SP, class AP> inline
const T* Zbase<T, SP, AP>::c_str() const
{
    return rawStr;
}


template <class T, template <class, class> class SP, class AP> inline
const T* Zbase<T, SP, AP>::data() const
{
    return rawStr;
}


template <class T, template <class, class> class SP, class AP> inline
const T Zbase<T, SP, AP>::operator[](size_t pos) const
{
    assert(pos < length());
    return rawStr[pos];
}


template <class T, template <class, class> class SP, class AP> inline
const T* Zbase<T, SP, AP>::begin() const
{
    return rawStr;
}


template <class T, template <class, class> class SP, class AP> inline
const T* Zbase<T, SP, AP>::end() const
{
    return rawStr + length();
}


template <class T, template <class, class> class SP, class AP> inline
T* Zbase<T, SP, AP>::begin()
{
    reserve(length());
    return rawStr;
}


template <class T, template <class, class> class SP, class AP> inline
T* Zbase<T, SP, AP>::end()
{
    return begin() + length();
}


template <class T, template <class, class> class SP, class AP> inline
void Zbase<T, SP, AP>::push_back(T val)
{
    operator+=(val);
}


template <class T, template <class, class> class SP, class AP> inline
bool Zbase<T, SP, AP>::empty() const
{
    return length() == 0;
}


template <class T, template <class, class> class SP, class AP> inline
void Zbase<T, SP, AP>::clear()
{
    if (!empty())
    {
        if (canWrite(rawStr, 0))
        {
            rawStr[0] = 0;        //keep allocated memory
            setLength(rawStr, 0); //
        }
        else
            *this = Zbase();
    }
}


template <class T, template <class, class> class SP, class AP> inline
const Zbase<T, SP, AP> operator+(const Zbase<T, SP, AP>& lhs, const Zbase<T, SP, AP>& rhs)
{
    return Zbase<T, SP, AP>(lhs) += rhs;
}


template <class T, template <class, class> class SP, class AP> inline
const Zbase<T, SP, AP> operator+(const Zbase<T, SP, AP>& lhs, const T* rhs)
{
    return Zbase<T, SP, AP>(lhs) += rhs;
}


template <class T, template <class, class> class SP, class AP> inline
const Zbase<T, SP, AP> operator+(const T* lhs, const Zbase<T, SP, AP>& rhs)
{
    return Zbase<T, SP, AP>(lhs) += rhs;
}


template <class T, template <class, class> class SP, class AP> inline
const Zbase<T, SP, AP> operator+(T lhs, const Zbase<T, SP, AP>& rhs)
{
    return (Zbase<T, SP, AP>() += lhs) += rhs;
}


template <class T, template <class, class> class SP, class AP> inline
const Zbase<T, SP, AP> operator+(const Zbase<T, SP, AP>& lhs, T rhs)
{
    return Zbase<T, SP, AP>(lhs) += rhs;
}


template <class T, template <class, class> class SP, class AP> inline
void Zbase<T, SP, AP>::swap(Zbase<T, SP, AP>& other)
{
    std::swap(rawStr, other.rawStr);
}


template <class T, template <class, class> class SP, class AP> inline
void Zbase<T, SP, AP>::reserve(size_t minCapacity) //make unshared and check capacity
{
    if (!canWrite(rawStr, minCapacity))
    {
        //allocate a new string
        T* newStr = create(length(), std::max(minCapacity, length())); //reserve() must NEVER shrink the string: logical const!
        std::copy(rawStr, rawStr + length() + 1, newStr); //include NULL-termination

        destroy(rawStr);
        rawStr = newStr;
    }
}


template <class T, template <class, class> class SP, class AP> inline
Zbase<T, SP, AP>& Zbase<T, SP, AP>::assign(const T* source, size_t len)
{
    if (canWrite(rawStr, len))
    {
        std::copy(source, source + len, rawStr);
        rawStr[len] = 0; //include null-termination
        setLength(rawStr, len);
    }
    else
        *this = Zbase(source, len);

    return *this;
}


template <class T, template <class, class> class SP, class AP> inline
Zbase<T, SP, AP>& Zbase<T, SP, AP>::operator=(const Zbase<T, SP, AP>& source)
{
    Zbase(source).swap(*this);
    return *this;
}


template <class T, template <class, class> class SP, class AP> inline
Zbase<T, SP, AP>& Zbase<T, SP, AP>::operator=(Zbase<T, SP, AP>&& tmp)
{
    swap(tmp);
    return *this;
}


template <class T, template <class, class> class SP, class AP> inline
Zbase<T, SP, AP>& Zbase<T, SP, AP>::operator=(const T* source)
{
    return assign(source, zen::strLength(source));
}


template <class T, template <class, class> class SP, class AP> inline
Zbase<T, SP, AP>& Zbase<T, SP, AP>::operator=(T source)
{
    if (canWrite(rawStr, 1))
    {
        rawStr[0] = source;
        rawStr[1] = 0; //include null-termination
        setLength(rawStr, 1);
    }
    else
        *this = Zbase(source);

    return *this;
}


template <class T, template <class, class> class SP, class AP> inline
Zbase<T, SP, AP>& Zbase<T, SP, AP>::operator+=(const Zbase<T, SP, AP>& other)
{
    const size_t thisLen  = length();
    const size_t otherLen = other.length();
    reserve(thisLen + otherLen); //make unshared and check capacity

    std::copy(other.rawStr, other.rawStr + otherLen + 1, rawStr + thisLen); //include null-termination
    setLength(rawStr, thisLen + otherLen);
    return *this;
}


template <class T, template <class, class> class SP, class AP> inline
Zbase<T, SP, AP>& Zbase<T, SP, AP>::operator+=(const T* other)
{
    const size_t thisLen  = length();
    const size_t otherLen = zen::strLength(other);
    reserve(thisLen + otherLen); //make unshared and check capacity

    std::copy(other, other + otherLen + 1, rawStr + thisLen); //include null-termination
    setLength(rawStr, thisLen + otherLen);
    return *this;
}


template <class T, template <class, class> class SP, class AP> inline
Zbase<T, SP, AP>& Zbase<T, SP, AP>::operator+=(T ch)
{
    const size_t thisLen = length();
    reserve(thisLen + 1); //make unshared and check capacity
    rawStr[thisLen] = ch;
    rawStr[thisLen + 1] = 0;
    setLength(rawStr, thisLen + 1);
    return *this;
}

#endif //Z_BASE_H_INCLUDED
