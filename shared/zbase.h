// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef Z_BASE_H_INCLUDED
#define Z_BASE_H_INCLUDED

#include <cstddef> //size_t
#include <cctype>  //isspace
#include <cwctype> //iswspace
#include <cassert>
#include <vector>
#include <sstream>
#include <algorithm>


/*
Allocator Policy:
-----------------
    void* allocate(size_t size) //throw (std::bad_alloc)
    void deallocate(void* ptr)
*/
class AllocatorFreeStore //same performance characterisics like malloc()/free()
{
public:
    static void* allocate(size_t size) //throw (std::bad_alloc)
    {
        return ::operator new(size);
    }

    static void deallocate(void* ptr)
    {
        ::operator delete(ptr);
    }
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
         class AP>    //Allocator Policy
class StorageDeepCopy : public AP
{
protected:
    ~StorageDeepCopy() {}

    static T* create(size_t size)
    {
        return create(size, size);
    }

    static T* create(size_t size, size_t minCapacity)
    {
        const size_t newCapacity = calcCapacity(minCapacity);
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

    static void destroy(T* ptr)
    {
        AP::deallocate(descr(ptr));
    }

    static bool canWrite(const T* ptr, size_t minCapacity) //needs to be checked before writing to "ptr"
    {
        return minCapacity <= descr(ptr)->capacity;
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
        size_t length;
        size_t capacity; //allocated size without null-termination
    };

    static Descriptor* descr(T* ptr)
    {
        return reinterpret_cast<Descriptor*>(ptr) - 1;
    }

    static const Descriptor* descr(const T* ptr)
    {
        return reinterpret_cast<const Descriptor*>(ptr) - 1;
    }

    static size_t calcCapacity(size_t length)
    {
        return (length + (19 - length % 16)); //allocate some additional length to speed up concatenation
    }
};


template <typename T, //Character Type
         class AP>    //Allocator Policy
class StorageRefCount : public AP
{
protected:
    ~StorageRefCount() {}

    static T* create(size_t size)
    {
        return create(size, size);
    }

    static T* create(size_t size, size_t minCapacity)
    {
        const size_t newCapacity = calcCapacity(minCapacity);
        assert(newCapacity >= minCapacity);
        assert(minCapacity >= size);

        Descriptor* const newDescr = static_cast<Descriptor*>(AP::allocate(sizeof(Descriptor) + (newCapacity + 1) * sizeof(T)));

        newDescr->refCount = 1;
        newDescr->length   = size;
        newDescr->capacity = newCapacity;

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
            AP::deallocate(descr(ptr));
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
        size_t refCount;
        size_t length;
        size_t capacity; //allocated size without null-termination
    };

    static Descriptor* descr(T* ptr)
    {
        return reinterpret_cast<Descriptor*>(ptr) - 1;
    }

    static const Descriptor* descr(const T* ptr)
    {
        return reinterpret_cast<const Descriptor*>(ptr) - 1;
    }

    static size_t calcCapacity(size_t length)
    {
        return (length + (19 - length % 16)); //allocate some additional length to speed up concatenation
    }
};


template <class T,									         //Character Type
         template <class, class> class SP = StorageRefCount, //Storage Policy
         class AP = AllocatorFreeStore>				         //Allocator Policy
class Zbase : public SP<T, AP>
{
public:
    Zbase();
    Zbase(T source);
    Zbase(const T* source);
    Zbase(const T* source, size_t length);
    Zbase(const Zbase& source);
    ~Zbase();

    operator const T*() const; //implicit conversion to C-string

    //STL accessors
    const T* begin() const;
    const T* end()   const;
    T*       begin();
    T*       end();

    //wxString-like functions
    bool StartsWith(const Zbase& prefix) const;
    bool StartsWith(const T* prefix) const;
    bool StartsWith(T prefix) const;
    bool EndsWith(const Zbase& postfix) const;
    bool EndsWith(const T* postfix) const;
    bool EndsWith(T postfix) const;
    Zbase& Truncate(size_t newLen);
    Zbase& Replace(const T* old, const T* replacement, bool replaceAll = true);
    Zbase AfterLast(  T ch) const;  //returns the whole string if "ch" not found
    Zbase BeforeLast( T ch) const;  //returns empty string if "ch" not found
    Zbase AfterFirst( T ch) const;  //returns empty string if "ch" not found
    Zbase BeforeFirst(T ch) const;  //returns the whole string if "ch" not found
    Zbase& Trim(bool fromLeft = true, bool fromRight = true);
    std::vector<Zbase> Split(T delimiter) const;

    //std::string functions
    size_t length() const;
    size_t size() const;
    const T* c_str() const; //C-string format with NULL-termination
    const T* data()  const; //internal representation, NULL-termination not guaranteed
    const T operator[](size_t pos) const;
    Zbase substr(size_t pos = 0, size_t len = npos) const;
    bool empty() const;
    void clear();
    size_t find(const Zbase& str, size_t pos = 0)    const; //
    size_t find(const T* str,     size_t pos = 0)    const; //returns "npos" if not found
    size_t find(T ch,             size_t pos = 0)    const; //
    size_t rfind(T ch,            size_t pos = npos) const; //
    Zbase& replace(size_t pos1, size_t n1, const T* str, size_t n2);
    void reserve(size_t minCapacity);
    Zbase& assign(const T* source, size_t len);
    void resize(size_t newSize, T fillChar = 0);
    void swap(Zbase& other);

    //number conversion
    template <class N> static Zbase fromNumber(N number);
    template <class N> N toNumber() const;

    Zbase& operator=(const Zbase& source);
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
namespace z_impl
{
//-------------C-string helper functions ---------------------------------------------------------
template <class T>
inline
size_t cStringLength(const T* input) //strlen()
{
    const T* iter = input;
    while (*iter != 0)
        ++iter;
    return iter - input;
}
}


//--------------------------------------------------------------------------------------------------
template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP>::Zbase()
{
    //resist the temptation to avoid this allocation by referening a static global: NO performance advantage, MT issues!
    rawStr    = this->create(0);
    rawStr[0] = 0;
}


template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP>::Zbase(T source)
{
    rawStr    = this->create(1);
    rawStr[0] = source;
    rawStr[1] = 0;
}


template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP>::Zbase(const T* source)
{
    const size_t sourceLen = z_impl::cStringLength(source);
    rawStr = this->create(sourceLen);
    std::copy(source, source + sourceLen + 1, rawStr); //include null-termination
}


template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP>::Zbase(const T* source, size_t sourceLen)
{
    rawStr = this->create(sourceLen);
    std::copy(source, source + sourceLen, rawStr);
    rawStr[sourceLen] = 0;
}


template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP>::Zbase(const Zbase<T, SP, AP>& source)
{
    rawStr = this->clone(source.rawStr);
}


template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP>::~Zbase()
{
    this->destroy(rawStr);
}


template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP>::operator const T*() const
{
    return rawStr;
}


// get all characters after the last occurence of ch
// (returns the whole string if ch not found)
template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP> Zbase<T, SP, AP>::AfterLast(T ch) const
{
    const size_t pos = rfind(ch, npos);
    if (pos != npos )
        return Zbase(rawStr + pos + 1, length() - pos - 1);
    else
        return *this;
}


// get all characters before the last occurence of ch
// (returns empty string if ch not found)
template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP> Zbase<T, SP, AP>::BeforeLast(T ch) const
{
    const size_t pos = rfind(ch, npos);
    if (pos != npos)
        return Zbase(rawStr, pos); //data is non-empty string in this context: else ch would not have been found!
    else
        return Zbase();
}


//returns empty string if ch not found
template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP> Zbase<T, SP, AP>::AfterFirst(T ch) const
{
    const size_t pos = find(ch, 0);
    if (pos != npos)
        return Zbase(rawStr + pos + 1, length() - pos - 1);
    else
        return Zbase();

}

//returns the whole string if ch not found
template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP> Zbase<T, SP, AP>::BeforeFirst(T ch) const
{
    const size_t pos = find(ch, 0);
    if (pos != npos)
        return Zbase(rawStr, pos); //data is non-empty string in this context: else ch would not have been found!
    else
        return *this;
}


template <class T, template <class, class> class SP, class AP>
inline
bool Zbase<T, SP, AP>::StartsWith(const T* prefix) const
{
    const size_t pfLength = z_impl::cStringLength(prefix);
    if (length() < pfLength)
        return false;

    return std::equal(rawStr, rawStr + pfLength,
                      prefix);
}


template <class T, template <class, class> class SP, class AP>
inline
bool Zbase<T, SP, AP>::StartsWith(T prefix) const
{
    return length() != 0 && operator[](0) == prefix;
}


template <class T, template <class, class> class SP, class AP>
inline
bool Zbase<T, SP, AP>::StartsWith(const Zbase<T, SP, AP>& prefix) const
{
    if (length() < prefix.length())
        return false;

    return std::equal(rawStr, rawStr + prefix.length(),
                      prefix.rawStr);
}


template <class T, template <class, class> class SP, class AP>
inline
bool Zbase<T, SP, AP>::EndsWith(const T* postfix) const
{
    const size_t pfLength  = z_impl::cStringLength(postfix);
    if (length() < pfLength)
        return false;

    const T* cmpBegin = rawStr + length() - pfLength;
    return std::equal(cmpBegin, cmpBegin + pfLength,
                      postfix);
}


template <class T, template <class, class> class SP, class AP>
inline
bool Zbase<T, SP, AP>::EndsWith(T postfix) const
{
    const size_t len = length();
    return len != 0 && operator[](len - 1) == postfix;
}


template <class T, template <class, class> class SP, class AP>
inline
bool Zbase<T, SP, AP>::EndsWith(const Zbase<T, SP, AP>& postfix) const
{
    if (length() < postfix.length())
        return false;

    const T* cmpBegin = rawStr + length() - postfix.length();
    return std::equal(cmpBegin, cmpBegin + postfix.length(),
                      postfix.rawStr);
}


template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP>& Zbase<T, SP, AP>::Truncate(size_t newLen)
{
    if (newLen < length())
    {
        if (canWrite(rawStr, newLen))
        {
            rawStr[newLen] = 0;
            setLength(rawStr, newLen);
        }
        else
            *this = Zbase(rawStr, newLen);
    }
    return *this;
}


template <class T, template <class, class> class SP, class AP>
inline
size_t Zbase<T, SP, AP>::find(const Zbase& str, size_t pos) const
{
    assert(pos <= length());
    const T* thisEnd = end(); //respect embedded 0
    const T* iter = std::search(begin() + pos, thisEnd,
                                str.begin(), str.end());
    return iter == thisEnd ? npos : iter - begin();
}


template <class T, template <class, class> class SP, class AP>
inline
size_t Zbase<T, SP, AP>::find(const T* str, size_t pos) const
{
    assert(pos <= length());
    const T* thisEnd = end(); //respect embedded 0
    const T* iter = std::search(begin() + pos, thisEnd,
                                str, str + z_impl::cStringLength(str));
    return iter == thisEnd ? npos : iter - begin();
}


template <class T, template <class, class> class SP, class AP>
inline
size_t Zbase<T, SP, AP>::find(T ch, size_t pos) const
{
    assert(pos <= length());
    const T* thisEnd = end();
    const T* iter = std::find(begin() + pos, thisEnd, ch); //respect embedded 0
    return iter == thisEnd ? npos : iter - begin();
}


template <class T, template <class, class> class SP, class AP>
inline
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


template <class T, template <class, class> class SP, class AP>
Zbase<T, SP, AP>& Zbase<T, SP, AP>::replace(size_t pos1, size_t n1, const T* str, size_t n2)
{
    assert(str < rawStr || rawStr + length() < str); //str mustn't point to data in this string
    assert(pos1 + n1 <= length());

    const size_t oldLen = length();
    if (oldLen == 0)
        return *this = Zbase(str, n2);

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

        std::copy(str, str + n2, rawStr + pos1);
    }
    else
    {
        //copy directly into new string
        T* const newStr = this->create(newLen);

        std::copy(rawStr, rawStr + pos1, newStr);
        std::copy(str, str + n2, newStr + pos1);
        std::copy(rawStr + pos1 + n1, rawStr + oldLen + 1, newStr + pos1 + n2); //include null-termination

        destroy(rawStr);
        rawStr = newStr;
    }
    return *this;
}


template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP>& Zbase<T, SP, AP>::Replace(const T* old, const T* replacement, bool replaceAll)
{
    const size_t oldLen         = z_impl::cStringLength(old);
    const size_t replacementLen = z_impl::cStringLength(replacement);

    size_t pos = 0;
    while ((pos = find(old, pos)) != npos)
    {
        replace(pos, oldLen, replacement, replacementLen);
        pos += replacementLen; //move past the string that was replaced

        if (!replaceAll)
            break;
    }
    return *this;
}


template <class T, template <class, class> class SP, class AP>
inline
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


template <class T, template <class, class> class SP, class AP>
inline
bool operator==(const Zbase<T, SP, AP>& lhs, const Zbase<T, SP, AP>& rhs)
{
    return lhs.length() == rhs.length() && std::equal(lhs.begin(), lhs.end(), rhs.begin()); //respect embedded 0
}


template <class T, template <class, class> class SP, class AP>
inline
bool operator==(const Zbase<T, SP, AP>& lhs, const T* rhs)
{
    return lhs.length() == z_impl::cStringLength(rhs) && std::equal(lhs.begin(), lhs.end(), rhs); //respect embedded 0
}


template <class T, template <class, class> class SP, class AP>
inline
bool operator==(const T* lhs, const Zbase<T, SP, AP>& rhs)
{
    return operator==(rhs, lhs);
}


template <class T, template <class, class> class SP, class AP>
inline
bool operator!=(const Zbase<T, SP, AP>& lhs, const Zbase<T, SP, AP>& rhs)
{
    return !operator==(lhs, rhs);
}


template <class T, template <class, class> class SP, class AP>
inline
bool operator!=(const Zbase<T, SP, AP>& lhs, const T* rhs)
{
    return !operator==(lhs, rhs);
}


template <class T, template <class, class> class SP, class AP>
inline
bool operator!=(const T* lhs, const Zbase<T, SP, AP>& rhs)
{
    return !operator==(lhs, rhs);
}


template <class T, template <class, class> class SP, class AP>
inline
bool operator<(const Zbase<T, SP, AP>& lhs, const Zbase<T, SP, AP>& rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), //respect embedded 0
                                        rhs.begin(), rhs.end());
}


template <class T, template <class, class> class SP, class AP>
inline
bool operator<(const Zbase<T, SP, AP>& lhs, const T* rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), //respect embedded 0
                                        rhs, rhs + z_impl::cStringLength(rhs));
}


template <class T, template <class, class> class SP, class AP>
inline
bool operator<(const T* lhs, const Zbase<T, SP, AP>& rhs)
{
    return std::lexicographical_compare(lhs, lhs + z_impl::cStringLength(lhs), //respect embedded 0
                                        rhs.begin(), rhs.end());
}


template <class T, template <class, class> class SP, class AP>
inline
size_t Zbase<T, SP, AP>::length() const
{
    return SP<T, AP>::length(rawStr);
}


template <class T, template <class, class> class SP, class AP>
inline
size_t Zbase<T, SP, AP>::size() const
{
    return length();
}


template <class T, template <class, class> class SP, class AP>
inline
const T* Zbase<T, SP, AP>::c_str() const
{
    return rawStr;
}


template <class T, template <class, class> class SP, class AP>
inline
const T* Zbase<T, SP, AP>::data() const
{
    return rawStr;
}


template <class T, template <class, class> class SP, class AP>
inline
const T Zbase<T, SP, AP>::operator[](size_t pos) const
{
    assert(pos < length());
    return rawStr[pos];
}


template <class T, template <class, class> class SP, class AP>
inline
const T* Zbase<T, SP, AP>::begin() const
{
    return rawStr;
}


template <class T, template <class, class> class SP, class AP>
inline
const T* Zbase<T, SP, AP>::end() const
{
    return rawStr + length();
}


template <class T, template <class, class> class SP, class AP>
inline
T* Zbase<T, SP, AP>::begin()
{
    reserve(length());
    return rawStr;
}


template <class T, template <class, class> class SP, class AP>
inline
T* Zbase<T, SP, AP>::end()
{
    return begin() + length();
}


template <class T, template <class, class> class SP, class AP>
inline
bool Zbase<T, SP, AP>::empty() const
{
    return length() == 0;
}


template <class T, template <class, class> class SP, class AP>
inline
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


template <class T, template <class, class> class SP, class AP>
inline
const Zbase<T, SP, AP> operator+(const Zbase<T, SP, AP>& lhs, const Zbase<T, SP, AP>& rhs)
{
    return Zbase<T, SP, AP>(lhs) += rhs;
}


template <class T, template <class, class> class SP, class AP>
inline
const Zbase<T, SP, AP> operator+(const Zbase<T, SP, AP>& lhs, const T* rhs)
{
    return Zbase<T, SP, AP>(lhs) += rhs;
}


template <class T, template <class, class> class SP, class AP>
inline
const Zbase<T, SP, AP> operator+(const T* lhs, const Zbase<T, SP, AP>& rhs)
{
    return Zbase<T, SP, AP>(lhs) += rhs;
}


template <class T, template <class, class> class SP, class AP>
inline
const Zbase<T, SP, AP> operator+(T lhs, const Zbase<T, SP, AP>& rhs)
{
    return Zbase<T, SP, AP>(lhs) += rhs;
}


template <class T, template <class, class> class SP, class AP>
inline
const Zbase<T, SP, AP> operator+(const Zbase<T, SP, AP>& lhs, T rhs)
{
    return Zbase<T, SP, AP>(lhs) += rhs;
}


template <class T, template <class, class> class SP, class AP>
inline
void Zbase<T, SP, AP>::swap(Zbase<T, SP, AP>& other)
{
    std::swap(rawStr, other.rawStr);
}


template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP> Zbase<T, SP, AP>::substr(size_t pos, size_t len) const
{
    assert(pos + (len == npos ? 0 : len) <= length());
    return Zbase(rawStr + pos, len == npos ? length() - pos : len);
}


template <class T, template <class, class> class SP, class AP>
inline
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


template <class T, template <class, class> class SP, class AP>
inline
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


template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP>& Zbase<T, SP, AP>::operator=(const Zbase<T, SP, AP>& source)
{
    Zbase(source).swap(*this);
    return *this;
}


template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP>& Zbase<T, SP, AP>::operator=(const T* source)
{
    return assign(source, z_impl::cStringLength(source));
}


template <class T, template <class, class> class SP, class AP>
inline
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


template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP>& Zbase<T, SP, AP>::operator+=(const Zbase<T, SP, AP>& other)
{
    const size_t thisLen  = length();
    const size_t otherLen = other.length();
    reserve(thisLen + otherLen); //make unshared and check capacity

    std::copy(other.rawStr, other.rawStr + otherLen + 1, rawStr + thisLen); //include null-termination
    setLength(rawStr, thisLen + otherLen);
    return *this;
}


template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP>& Zbase<T, SP, AP>::operator+=(const T* other)
{
    const size_t thisLen  = length();
    const size_t otherLen = z_impl::cStringLength(other);
    reserve(thisLen + otherLen); //make unshared and check capacity

    std::copy(other, other + otherLen + 1, rawStr + thisLen); //include null-termination
    setLength(rawStr, thisLen + otherLen);
    return *this;
}


template <class T, template <class, class> class SP, class AP>
inline
Zbase<T, SP, AP>& Zbase<T, SP, AP>::operator+=(T ch)
{
    const size_t thisLen = length();
    reserve(thisLen + 1); //make unshared and check capacity
    rawStr[thisLen] = ch;
    rawStr[thisLen + 1] = 0;
    setLength(rawStr, thisLen + 1);
    return *this;
}


template <class T, template <class, class> class SP, class AP>
template <class N>
inline
Zbase<T, SP, AP> Zbase<T, SP, AP>::fromNumber(N number)
{
    std::basic_ostringstream<T> ss;
    ss << number;
    return Zbase<T, SP, AP>(ss.str().c_str());
}


template <class T, template <class, class> class SP, class AP>
template <class N>
inline
N Zbase<T, SP, AP>::toNumber() const
{
    std::basic_istringstream<T> ss(std::basic_string<T>(rawStr));
    T number = 0;
    ss >> number;
    return number;
}


namespace z_impl
{
template <class T>
bool cStringWhiteSpace(T ch);

template <>
inline
bool cStringWhiteSpace(char ch)
{
    return std::isspace(static_cast<unsigned char>(ch)) != 0; //some compilers (e.g. VC++ 6.0) return true for a call to isspace('\xEA'); but no issue with newer versions of MSVC
}


template <>
inline
bool cStringWhiteSpace(wchar_t ch)
{
    return std::iswspace(ch) != 0; //some compilers (e.g. VC++ 6.0) return true for a call to isspace('\xEA'); but no issue with newer versions of MSVC
}
}


template <class T, template <class, class> class SP, class AP>
Zbase<T, SP, AP>& Zbase<T, SP, AP>::Trim(bool fromLeft, bool fromRight)
{
    assert(fromLeft || fromRight);

    const T* newBegin = rawStr;
    const T* newEnd   = rawStr + length();

    if (fromRight)
        while (newBegin != newEnd && z_impl::cStringWhiteSpace(newEnd[-1]))
            --newEnd;

    if (fromLeft)
        while (newBegin != newEnd && z_impl::cStringWhiteSpace(*newBegin))
            ++newBegin;

    const size_t newLength = newEnd - newBegin;
    if (newLength != length())
    {
        if (canWrite(rawStr, newLength))
        {
            std::copy(newBegin, newBegin + newLength, rawStr); //shift left => std::copy, shift right std::copy_backward
            rawStr[newLength] = 0;
            setLength(rawStr, newLength);
        }
        else
            *this = Zbase(newBegin, newLength);
    }
    return *this;
}


template <class T, template <class, class> class SP, class AP>
std::vector<Zbase<T, SP, AP> > Zbase<T, SP, AP>::Split(T delimiter) const
{
    std::vector<Zbase> output;

    const size_t thisLen = length();
    size_t startPos = 0;
    for (;;) //make MSVC happy
    {
        size_t endPos = find(delimiter, startPos);
        if (endPos == npos)
            endPos = thisLen;

        if (startPos != endPos) //do not add empty strings
        {
            Zbase newEntry = substr(startPos, endPos - startPos);
            newEntry.Trim();  //remove whitespace characters

            if (!newEntry.empty())
                output.push_back(newEntry);
        }
        if (endPos == thisLen)
            break;

        startPos = endPos + 1; //skip delimiter
    }

    return output;
}


#endif //Z_BASE_H_INCLUDED
