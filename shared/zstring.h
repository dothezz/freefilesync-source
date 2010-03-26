// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef ZSTRING_H_INCLUDED
#define ZSTRING_H_INCLUDED

#include <cstring> //size_t, memcpy(), memcmp()
#include <cstdlib> //malloc(), free()
#include <assert.h>
#include <vector>
#include <sstream>

#ifdef __WXDEBUG__
#include <set>
#include <wx/thread.h>
#endif


#ifdef ZSTRING_CHAR
typedef char DefaultChar; //use char strings
#define DefaultStr(x) x   //
#elif defined ZSTRING_WIDE_CHAR
typedef wchar_t DefaultChar; //use wide character strings
#define DefaultStr(x) L ## x //
#endif


class Zstring
{
public:
    Zstring();
    Zstring(const DefaultChar* source);                //string is copied: O(length)
    Zstring(const DefaultChar* source, size_t length); //string is copied: O(length)
    Zstring(const Zstring& source);                    //reference-counting => O(1)
    ~Zstring();

    operator const DefaultChar*() const;               //implicit conversion to C string

    //Compare filenames: Windows does NOT distinguish between upper/lower-case, while Linux DOES
    int cmpFileName(const Zstring& other) const;
    int cmpFileName(const DefaultChar* other) const;

    //wxWidgets-like functions
    bool StartsWith(const DefaultChar* begin) const;
    bool StartsWith(DefaultChar begin) const;
    bool StartsWith(const Zstring& begin) const;
    bool EndsWith(const DefaultChar* end) const;
    bool EndsWith(const DefaultChar end) const;
    bool EndsWith(const Zstring& end) const;
    Zstring& Truncate(size_t newLen);
#ifdef FFS_WIN
    Zstring& MakeUpper();
#endif

    Zstring& Replace(const DefaultChar* old, const DefaultChar* replacement, bool replaceAll = true);
    Zstring AfterLast(  DefaultChar ch) const;  //returns the whole string if ch not found
    Zstring BeforeLast( DefaultChar ch) const;  //returns empty string if ch not found
    Zstring AfterFirst( DefaultChar ch) const;  //returns empty string if ch not found
    Zstring BeforeFirst(DefaultChar ch) const;  //returns the whole string if ch not found
    size_t Find(DefaultChar ch, bool fromEnd) const; //returns npos if not found
    bool Matches(const DefaultChar* mask) const;
    static bool Matches(const DefaultChar* name, const DefaultChar* mask);
    Zstring& Trim(bool fromRight); //from right or left
    std::vector<Zstring> Tokenize(const DefaultChar delimiter) const;

    //std::string functions
    size_t length() const;
    const DefaultChar* c_str() const;
    Zstring substr(size_t pos = 0, size_t len = npos) const; //allocate new string
    bool empty() const;
    void clear();
    int compare(size_t pos1, size_t n1, const DefaultChar* other) const;
    size_t find(const DefaultChar* str, size_t pos = 0 ) const;
    size_t find(DefaultChar ch, size_t pos = 0) const;
    size_t rfind(DefaultChar ch, size_t pos = npos) const;
    Zstring& replace(size_t pos1, size_t n1, const DefaultChar* str, size_t n2);
    size_t size() const;
    void reserve(size_t minCapacity);
    Zstring& assign(const DefaultChar* source, size_t len);
    void resize(size_t newSize, DefaultChar fillChar = 0 );

    Zstring& operator=(const Zstring& source);
    Zstring& operator=(const DefaultChar* source);

    bool operator==(const Zstring&     other) const;
    bool operator==(const DefaultChar* other) const;
    bool operator< (const Zstring&     other) const;
    bool operator< (const DefaultChar* other) const;
    bool operator!=(const Zstring&     other) const;
    bool operator!=(const DefaultChar* other) const;

    const DefaultChar operator[](const size_t pos) const;

    Zstring& operator+=(const Zstring& other);
    Zstring& operator+=(const DefaultChar* other);
    Zstring& operator+=(DefaultChar ch);

    const Zstring operator+(const Zstring& string2) const;
    const Zstring operator+(const DefaultChar* string2) const;
    const Zstring operator+(const DefaultChar ch) const;

    static const size_t	npos = static_cast<size_t>(-1);

private:
    //detect usage errors
    Zstring(int);
    friend const Zstring operator+(const DefaultChar* lhs, const Zstring& rhs);
    friend const Zstring operator+(const DefaultChar ch, const Zstring& rhs);

    DefaultChar* data();

    void initAndCopy(const DefaultChar* source, size_t length);
    void incRef() const; //support for reference-counting
    void decRef();       //

    //helper methods
    static size_t defaultLength (const DefaultChar* input); //strlen()
    static int    defaultCompare(const DefaultChar* str1, const DefaultChar* str2); //strcmp()
    static int    defaultCompare(const DefaultChar* str1, const DefaultChar* str2, size_t count); //strncmp()
    static const DefaultChar* defaultStrFind(const DefaultChar* str1, DefaultChar ch); //strchr()
    static const DefaultChar* defaultStrFind(const DefaultChar* str1, const DefaultChar* str1End, const DefaultChar* str2); //"analog" to strstr()
    static bool matchesHelper(const DefaultChar* string, const DefaultChar* mask);

    struct StringDescriptor
    {
        mutable unsigned int refCount;
        size_t               length;
        size_t               capacity; //allocated length without null-termination
    };
    static StringDescriptor* allocate(const size_t newLength);

    StringDescriptor* descr;
};


template <class T>
Zstring numberToZstring(const T& number); //convert number to Zstring
























//#######################################################################################
//begin of implementation

//-------------standard helper functions ---------------------------------------------------------------
inline
size_t Zstring::defaultLength(const DefaultChar* input) //strlen()
{
    const DefaultChar* const startPos = input;
    while (*input != 0)
        ++input;

    return input - startPos;
}


inline
int Zstring::defaultCompare(const DefaultChar* str1, const DefaultChar* str2) //strcmp()
{
    while (*str1 == *str2)
    {
        if (*str1 == 0)
            return 0;
        ++str1;
        ++str2;
    }

    return *str1 - *str2;
}


inline
int Zstring::defaultCompare(const DefaultChar* str1, const DefaultChar* str2, size_t count) //strncmp()
{
    while (count-- != 0)
    {
        if (*str1 != *str2)
            return *str1 - *str2;

        if (*str1 == 0)
            return 0;
        ++str1;
        ++str2;
    }

    return 0;
}


inline
const DefaultChar* Zstring::defaultStrFind(const DefaultChar* str1, DefaultChar ch) //strchr()
{
    while (*str1 != ch) //ch is allowed to be 0 by contract! must return end of string in this case
    {
        if (*str1 == 0)
            return NULL;

        ++str1;
    }

    return str1;
}


inline
const DefaultChar* Zstring::defaultStrFind(const DefaultChar* str1, const DefaultChar* str1End, const DefaultChar* str2) //"analog" to strstr()
{
    const size_t str2Len = defaultLength(str2);

    str1End -= str2Len; //no need to process the "last chunk" of str1
    ++str1End;          //

    while(str1 < str1End) //don't use !=; str1End may be smaller than str1!
    {
        if(::memcmp(str1, str2, str2Len * sizeof(DefaultChar)) == 0)
            return str1;
        ++str1;
    }
    return NULL;
}
//--------------------------------------------------------------------------------------------------


#ifdef __WXDEBUG__
class AllocationCount //small test for memory leaks in Zstring
{
public:
    void inc(const DefaultChar* object)
    {
        wxCriticalSectionLocker dummy(lockActStrings);
        activeStrings.insert(object);
    }

    void dec(const DefaultChar* object)
    {
        wxCriticalSectionLocker dummy(lockActStrings);
        activeStrings.erase(object);
    }

    static AllocationCount& getInstance();

private:
    AllocationCount() {}
    AllocationCount(const AllocationCount&);
    ~AllocationCount();

    wxCriticalSection lockActStrings;
    std::set<const DefaultChar*> activeStrings;
};
#endif


inline
size_t getCapacityToAllocate(const size_t length)
{
    return (length + (19 - length % 16)); //allocate some additional length to speed up concatenation
}


inline
Zstring::StringDescriptor* Zstring::allocate(const size_t newLength)
{
    //allocate and set data for new string
    const size_t newCapacity = getCapacityToAllocate(newLength);
    assert(newCapacity);

    StringDescriptor* const newDescr = static_cast<StringDescriptor*>(::malloc(sizeof(StringDescriptor) + (newCapacity + 1) * sizeof(DefaultChar))); //use C-memory functions because of realloc()
    if (newDescr == NULL)
        throw std::bad_alloc();

    newDescr->refCount = 1;
    newDescr->length   = newLength;
    newDescr->capacity = newCapacity;

#ifdef __WXDEBUG__
    AllocationCount::getInstance().inc(reinterpret_cast<DefaultChar*>(newDescr + 1)); //test Zstring for memory leaks
#endif
    return newDescr;
}


inline
Zstring::Zstring()
{
    //static (dummy) empty Zstring
#ifdef ZSTRING_CHAR
    static Zstring emptyString("");
#elif defined ZSTRING_WIDE_CHAR
    static Zstring emptyString(L"");
#endif

    emptyString.incRef();
    descr = emptyString.descr;
}


inline

Zstring::Zstring(const DefaultChar* source)
{
    initAndCopy(source, defaultLength(source));
}


inline
Zstring::Zstring(const DefaultChar* source, size_t sourceLen)
{
    initAndCopy(source, sourceLen);
}


inline
Zstring::Zstring(const Zstring& source)
{
    descr = source.descr;
    incRef(); //reference counting!
}


inline
Zstring::~Zstring()
{
    decRef();
}


inline
void Zstring::initAndCopy(const DefaultChar* source, size_t sourceLen)
{
    assert(source);
    descr = allocate(sourceLen);
    ::memcpy(data(), source, sourceLen * sizeof(DefaultChar));
    data()[sourceLen] = 0;
}


inline
void Zstring::incRef() const
{
    assert(descr);
    ++descr->refCount;
}


inline
void Zstring::decRef()
{
    assert(descr && descr->refCount >= 1); //descr points to the begin of the allocated memory block
    if (--descr->refCount == 0)
    {
#ifdef __WXDEBUG__
        AllocationCount::getInstance().dec(c_str()); //test Zstring for memory leaks
#endif
        ::free(descr); //beginning of whole memory block
        descr = NULL;
    }
}


inline
Zstring::operator const DefaultChar*() const
{
    return c_str();
}


inline
Zstring& Zstring::operator=(const Zstring& source)
{
    source.incRef(); //implicitly handle case "this == &source" and avoid this check
    decRef();        //
    descr = source.descr;

    return *this;
}


inline
size_t Zstring::Find(DefaultChar ch, bool fromEnd) const
{
    return fromEnd ?
           rfind(ch, npos) :
           find(ch, 0);
}


// get all characters after the last occurence of ch
// (returns the whole string if ch not found)
inline
Zstring Zstring::AfterLast(DefaultChar ch) const
{
    const size_t pos = rfind(ch, npos);
    if (pos != npos )
        return Zstring(c_str() + pos + 1, length() - pos - 1);
    else
        return *this;
}


// get all characters before the last occurence of ch
// (returns empty string if ch not found)
inline
Zstring Zstring::BeforeLast(DefaultChar ch) const
{
    const size_t pos = rfind(ch, npos);
    if (pos != npos)
        return Zstring(c_str(), pos); //data is non-empty string in this context: else ch would not have been found!
    else
        return Zstring();
}


//returns empty string if ch not found
inline
Zstring Zstring::AfterFirst(DefaultChar ch) const
{
    const size_t pos = find(ch, 0);
    if (pos != npos)
        return Zstring(c_str() + pos + 1, length() - pos - 1);
    else
        return Zstring();

}

//returns the whole string if ch not found
inline
Zstring Zstring::BeforeFirst(DefaultChar ch) const
{
    const size_t pos = find(ch, 0);
    if (pos != npos)
        return Zstring(c_str(), pos); //data is non-empty string in this context: else ch would not have been found!
    else
        return *this;
}


inline
bool Zstring::StartsWith(const DefaultChar* begin) const
{
    const size_t beginLength = defaultLength(begin);
    if (length() < beginLength)
        return false;
    return compare(0, beginLength, begin) == 0;
}


inline
bool Zstring::StartsWith(DefaultChar begin) const
{
    const size_t len = length();
    return len && (this->operator[](0) == begin);
}


inline
bool Zstring::StartsWith(const Zstring& begin) const
{
    const size_t beginLength = begin.length();
    if (length() < beginLength)
        return false;
    return compare(0, beginLength, begin.c_str()) == 0;
}


inline
bool Zstring::EndsWith(const DefaultChar* end) const
{
    const size_t thisLength = length();
    const size_t endLength  = defaultLength(end);
    if (thisLength < endLength)
        return false;
    return compare(thisLength - endLength, endLength, end) == 0;
}


inline
bool Zstring::EndsWith(const DefaultChar end) const
{
    const size_t len = length();
    return len && (this->operator[](len - 1) == end);
}


inline
bool Zstring::EndsWith(const Zstring& end) const
{
    const size_t thisLength = length();
    const size_t endLength  = end.length();
    if (thisLength < endLength)
        return false;
    return compare(thisLength - endLength, endLength, end.c_str()) == 0;
}


inline
Zstring& Zstring::Truncate(size_t newLen)
{
    if (newLen < length())
    {
        if (descr->refCount > 1) //allocate new string
            return *this = Zstring(c_str(), newLen);
        else //overwrite this string
        {
            descr->length  = newLen;
            data()[newLen] = 0;
        }
    }

    return *this;
}


inline
size_t Zstring::find(const DefaultChar* str, size_t pos) const
{
    assert(pos <= length());
    const DefaultChar* const found = defaultStrFind(c_str() + pos, c_str() + length(),  str);
    return found == NULL ? npos : found - c_str();
}


inline
size_t Zstring::find(DefaultChar ch, size_t pos) const
{
    assert(pos <= length());
    const DefaultChar* thisStr = c_str();
    const DefaultChar* found = defaultStrFind(thisStr + pos, ch);
    return found == NULL ? npos : found - thisStr;
}


inline
bool Zstring::operator==(const Zstring& other) const
{
    return length() != other.length() ? false : defaultCompare(c_str(), other.c_str()) == 0; //memcmp() offers no better performance here...
}


inline
bool Zstring::operator==(const DefaultChar* other) const
{
    return defaultCompare(c_str(), other) == 0; //overload using strcmp(char*, char*) should be fastest!
}


inline
bool Zstring::operator<(const Zstring& other) const
{
    return defaultCompare(c_str(), other.c_str()) < 0;
}


inline
bool Zstring::operator<(const DefaultChar* other) const
{
    return defaultCompare(c_str(), other) < 0; //overload using strcmp(char*, char*) should be fastest!
}


inline
bool Zstring::operator!=(const Zstring& other) const
{
    return !(*this==other);
}


inline
bool Zstring::operator!=(const DefaultChar* other) const
{
    return !(*this==other);
}


inline
int Zstring::compare(size_t pos1, size_t n1, const DefaultChar* other) const
{
    assert(n1 <= length() - pos1);
    return defaultCompare(c_str() + pos1, other, n1);
}


inline
size_t Zstring::length() const
{
    return descr->length;
}


inline
size_t Zstring::size() const
{
    return descr->length;
}


inline
const DefaultChar* Zstring::c_str() const
{
    return reinterpret_cast<DefaultChar*>(descr + 1);
}


inline
DefaultChar* Zstring::data()
{
    return reinterpret_cast<DefaultChar*>(descr + 1);
}


inline
bool Zstring::empty() const
{
    return descr->length == 0;
}


inline
void Zstring::clear()
{
    *this = Zstring();
}


inline
const DefaultChar Zstring::operator[](const size_t pos) const
{
    assert(pos < length());
    return c_str()[pos];
}


inline
const Zstring Zstring::operator+(const Zstring& string2) const
{
    return Zstring(*this)+=string2;
}


inline
const Zstring Zstring::operator+(const DefaultChar* string2) const
{
    return Zstring(*this)+=string2;
}


inline
const Zstring Zstring::operator+(const DefaultChar ch) const
{
    return Zstring(*this)+=ch;
}


inline
void Zstring::resize(size_t newSize, DefaultChar fillChar)
{
    const size_t oldSize = length();
    if (oldSize < newSize)
    {
        reserve(newSize);    //make unshared and ensure capacity

        //fill up...
        DefaultChar* strPtr = data() + oldSize;
        const DefaultChar* const strEnd   = data() + newSize;
        while (strPtr != strEnd)
        {
            *strPtr = fillChar;
            ++strPtr;
        }

        data()[newSize] = 0;
        descr->length = newSize;
    }
    else if (oldSize > newSize)
    {
        if (descr->refCount > 1)
            *this = Zstring(c_str(), newSize); //no need to reserve() and copy the old string completely!
        else //overwrite this string
        {
            data()[newSize] = 0;
            descr->length  = newSize;
        }
    }
}


template <class T>
inline
Zstring numberToZstring(const T& number) //convert number to string the C++ way
{
#ifdef ZSTRING_CHAR
    std::stringstream ss;
#elif defined ZSTRING_WIDE_CHAR
    std::wstringstream ss;
#endif
    ss << number;
    return Zstring(ss.str().c_str(), ss.str().length());
}

#endif // ZSTRING_H_INCLUDED
