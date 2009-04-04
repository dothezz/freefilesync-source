/***************************************************************
 * Purpose:   High performance string class
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   Jan. 2009
 **************************************************************/

#ifndef ZSTRING_H_INCLUDED
#define ZSTRING_H_INCLUDED

#include <cstring>
#include <cctype>
#include <assert.h>
#include <new>
#include <cstdlib>


namespace FreeFileSync
{
#ifdef FFS_WIN
    //super fast case-insensitive string comparison: way faster than wxString::CmpNoCase()!!!
    int compareStringsWin32(const wchar_t* a, const wchar_t* b, const int aCount = -1, const int bCount = -1);
#endif  //FFS_WIN
}


#ifdef FFS_WIN
#define ZSTRING_WIDE_CHAR //use wide character strings

#elif defined FFS_LINUX
#define ZSTRING_CHAR //use char strings
#endif


#ifdef ZSTRING_CHAR
typedef char DefaultChar;
#elif defined ZSTRING_WIDE_CHAR
typedef wchar_t DefaultChar;
#endif

class Zsubstr;

class Zstring
{
public:
    Zstring();
    Zstring(const DefaultChar* source);                //string is copied: O(length)
    Zstring(const DefaultChar* source, size_t length); //string is copied: O(length)
    Zstring(const Zstring& source);                    //reference-counting => O(1)
    ~Zstring();

    operator const DefaultChar*() const;               //implicit conversion to C string

    //wxWidgets functions
    bool StartsWith(const DefaultChar* begin) const;
    bool StartsWith(const Zstring& begin) const;
    bool EndsWith(const DefaultChar* end) const;
    bool EndsWith(const Zstring& end) const;
#ifdef FFS_WIN
    int CmpNoCase(const DefaultChar* other) const;
    int CmpNoCase(const Zstring& other) const;
#endif
    int Cmp(const DefaultChar* other) const;
    int Cmp(const Zstring& other) const;
    size_t Replace(const DefaultChar* old, const DefaultChar* replacement, bool replaceAll = true);
    Zstring AfterLast(DefaultChar ch) const;
    Zstring BeforeLast(DefaultChar ch) const;
    size_t Find(DefaultChar ch, bool fromEnd) const;
    bool Matches(const DefaultChar* mask) const;
    Zstring& Trim(bool fromRight); //from right or left
    Zstring& MakeLower();

    //std::string functions
    size_t length() const;
    const DefaultChar* c_str() const;
    Zstring substr(size_t pos = 0, size_t len = npos) const; //allocate new string
    Zsubstr zsubstr(size_t pos = 0) const;                   //use ref-counting!
    bool empty() const;
    int compare(const DefaultChar* other) const;
    int compare(const Zstring& other) const;
    int compare(const size_t pos1, const size_t n1, const DefaultChar* other) const;
    size_t find(const DefaultChar* str, const size_t pos = 0 ) const;
    size_t find(const DefaultChar ch, const size_t pos = 0) const;
    size_t rfind(const DefaultChar ch, size_t pos = npos) const;
    Zstring& replace(size_t pos1, size_t n1, const DefaultChar* str, size_t n2);
    size_t size() const;

    Zstring& operator=(const Zstring& source);
    Zstring& operator=(const DefaultChar* source);

    bool operator==(const Zstring& other) const;
    bool operator==(const DefaultChar* other) const;
    bool operator!=(const Zstring& other) const;
    bool operator!=(const DefaultChar* other) const;

    DefaultChar operator[](const size_t pos) const;

    Zstring& operator+=(const Zstring& other);
    Zstring& operator+=(const DefaultChar* other);
    Zstring& operator+=(DefaultChar ch);

    Zstring operator+(const Zstring& string2) const;
    Zstring operator+(const DefaultChar* string2) const;
    Zstring operator+(const DefaultChar ch) const;

    static const size_t	npos = static_cast<size_t>(-1);

private:
    void initAndCopy(const DefaultChar* source, size_t length);
    void incRef() const;                               //support for reference-counting
    void decRef();                                     //
    void copyBeforeWrite(const size_t capacityNeeded); //and copy-on-write

    struct StringDescriptor
    {
        mutable unsigned int refCount;
        size_t               length;
        size_t               capacity; //allocated length without null-termination
    };
    static void allocate(const size_t newLength, Zstring::StringDescriptor*& newDescr, DefaultChar*& newData);

    StringDescriptor* descr;
    DefaultChar*      data;
};


class Zsubstr //ref-counted substring of a Zstring
{
public:
    Zsubstr();
    Zsubstr(const Zstring& ref, const size_t pos);

    const DefaultChar* c_str() const;
    size_t length() const;
    bool StartsWith(const Zstring& begin) const;
    size_t findFromEnd(const DefaultChar ch) const;

private:
    Zstring m_ref;
    const DefaultChar* m_data;
    size_t m_length;
};


//#######################################################################################
//begin of implementation

#ifdef ZSTRING_CHAR
inline
size_t defaultLength(const char* input)
{
    return strlen(input);
}

inline
int defaultCompare(const char* str1, const char* str2)
{
    return strcmp(str1, str2);
}

inline
int defaultCompare(const char* str1, const char* str2, const size_t count)
{
    return strncmp(str1, str2, count);
}

inline
char* defaultStrFind(const char* str1, const char* str2)
{
    return strstr(str1, str2);
}

inline
char* defaultStrFind(const char* str1, int ch)
{
    return strchr(str1, ch);
}

inline
bool defaultIsWhiteSpace(const char ch)
{
    // some compilers (e.g. VC++ 6.0) return true for a call to isspace('\xEA') => exclude char(128) to char(255)
    return ((unsigned char)ch < 128) && isspace((unsigned char)ch) != 0;
}

inline
char defaultToLower(const char ch)
{
    return tolower((unsigned char)ch); //caution: although tolower() has int as input parameter it expects unsigned chars!
}

#elif defined ZSTRING_WIDE_CHAR
inline
size_t defaultLength(const wchar_t* input)
{
    return wcslen(input);
}

inline
int defaultCompare(const wchar_t* str1, const wchar_t* str2)
{
    return wcscmp(str1, str2);
}

inline
int defaultCompare(const wchar_t* str1, const wchar_t* str2, const size_t count)
{
    return wcsncmp(str1, str2, count);
}

inline
wchar_t* defaultStrFind(const wchar_t* str1, const wchar_t* str2)
{
    return wcsstr(str1, str2);
}

inline
wchar_t* defaultStrFind(const wchar_t* str1, int ch)
{
    return wcschr(str1, ch);
}

inline
bool defaultIsWhiteSpace(const wchar_t ch)
{
    // some compilers (e.g. VC++ 6.0) return true for a call to isspace('\xEA') => exclude char(128) to char(255)
    return (ch < 128 || ch > 255) && iswspace(ch) != 0;
}

inline
wchar_t defaultToLower(const wchar_t ch)
{
    return towlower(ch);
}
#endif


#ifdef __WXDEBUG__
extern int allocCount; //test Zstring for memory leaks
void testZstringForMemoryLeak();
#endif


inline
size_t getCapacityToAllocate(const size_t length)
{
    return (length + (19 - length % 16)); //allocate some additional length to speed up concatenation
}


inline
void Zstring::allocate(const size_t       newLength,
                       StringDescriptor*& newDescr,
                       DefaultChar*&      newData)
{   //allocate and set data for new string
    const size_t newCapacity = getCapacityToAllocate(newLength);
    assert(newCapacity);

    newDescr = (StringDescriptor*) malloc( sizeof(StringDescriptor) + (newCapacity + 1) * sizeof(DefaultChar));
    if (newDescr == NULL)
        throw std::bad_alloc();
    newData = (DefaultChar*)(newDescr + 1);

    newDescr->refCount = 1;
    newDescr->length   = newLength;
    newDescr->capacity = newCapacity;

#ifdef __WXDEBUG__
    ++allocCount; //test Zstring for memory leaks

    static bool isRegistered = false;
    if (!isRegistered)
    {
        isRegistered = true;
        atexit(testZstringForMemoryLeak);
    }
#endif
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

    emptyString.incRef(); //implicitly handle case "this == &source" and avoid this check
    descr = emptyString.descr;
    data  = emptyString.data;
}


inline
Zstring::Zstring(const DefaultChar* source)
{
    initAndCopy(source, defaultLength(source));
}


inline
Zstring::Zstring(const DefaultChar* source, size_t length)
{
    initAndCopy(source, length);
}


inline
Zstring::Zstring(const Zstring& source)
{
    descr = source.descr;
    data  = source.data;
    incRef(); //reference counting!
}


inline
Zstring::~Zstring()
{
    decRef();
}


inline
void Zstring::initAndCopy(const DefaultChar* source, size_t length)
{
    allocate(length, descr, data);
    memcpy(data, source, length * sizeof(DefaultChar));
    data[length] = 0;
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
        free(descr);   //this must NEVER be changed!! E.g. Trim() relies on descr being start of allocated memory block
        descr = 0;
#ifdef __WXDEBUG__
        --allocCount; //test Zstring for memory leaks
#endif
    }
}


#ifdef FFS_WIN
inline
int Zstring::CmpNoCase(const DefaultChar* other) const
{
    return FreeFileSync::compareStringsWin32(c_str(), other); //way faster than wxString::CmpNoCase()!!
}


inline
int Zstring::CmpNoCase(const Zstring& other) const
{
    return FreeFileSync::compareStringsWin32(c_str(), other.c_str(), length(), other.length()); //way faster than wxString::CmpNoCase()!!
}
#endif


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
    data  = source.data;

    return *this;
}


inline
size_t Zstring::Find(DefaultChar ch, bool fromEnd) const
{
    if (fromEnd)
        return rfind(ch, npos);
    else
        return find(ch, 0);
}


// get all characters after the last occurence of ch
// (returns the whole string if ch not found)
inline
Zstring Zstring::AfterLast(DefaultChar ch) const
{
    size_t pos = rfind(ch, npos);
    if (pos == npos )
        return *this;
    else
        return c_str() + pos + 1;
}


// get all characters before the last occurence of ch
// (returns empty string if ch not found)
inline
Zstring Zstring::BeforeLast(DefaultChar ch) const
{
    size_t pos = rfind(ch, npos);

    if (pos != npos && pos != 0 )
        return Zstring(data, pos); //data is non-empty string in this context: else ch would not have been found!
    else
        return Zstring();
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
bool Zstring::StartsWith(const Zstring& begin) const
{
    const size_t beginLength = begin.length();
    if (length() < beginLength)
        return false;
    return compare(0, beginLength, begin) == 0;
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
bool Zstring::EndsWith(const Zstring& end) const
{
    const size_t thisLength = length();
    const size_t endLength  = end.length();
    if (thisLength < endLength)
        return false;
    return compare(thisLength - endLength, endLength, end) == 0;
}


inline
size_t Zstring::find(const DefaultChar* str, const size_t pos) const
{
    assert(pos <= length());
    const DefaultChar* thisStr = c_str();
    const DefaultChar* found = defaultStrFind(thisStr + pos, str);
    return found == NULL ? npos : found - thisStr;
}


inline
size_t Zstring::find(const DefaultChar ch, const size_t pos) const
{
    assert(pos <= length());
    const DefaultChar* thisStr = c_str();
    const DefaultChar* found = defaultStrFind(thisStr + pos, ch);
    return found == NULL ? npos : found - thisStr;
}


inline
int Zstring::Cmp(const DefaultChar* other) const
{
    return compare(other);
}


inline
int Zstring::Cmp(const Zstring& other) const
{
    return defaultCompare(c_str(), other.c_str()); //overload using strcmp(char*, char*) should be fastest!
}


inline
bool Zstring::operator==(const Zstring& other) const
{
    return length() != other.length() ? false : defaultCompare(c_str(), other.c_str()) == 0;
}


inline
bool Zstring::operator==(const DefaultChar* other) const
{
    return defaultCompare(c_str(), other) == 0; //overload using strcmp(char*, char*) should be fastest!
}


inline
bool Zstring::operator!=(const Zstring& other) const
{
    return length() != other.length() ? true: defaultCompare(c_str(), other.c_str()) != 0;
}


inline
bool Zstring::operator!=(const DefaultChar* other) const
{
    return defaultCompare(c_str(), other) != 0; //overload using strcmp(char*, char*) should be fastest!
}


inline
int Zstring::compare(const Zstring& other) const
{
    return defaultCompare(c_str(), other.c_str()); //overload using strcmp(char*, char*) should be fastest!
}


inline
int Zstring::compare(const DefaultChar* other) const
{
    return defaultCompare(c_str(), other); //overload using strcmp(char*, char*) should be fastest!
}


inline
int Zstring::compare(const size_t pos1, const size_t n1, const DefaultChar* other) const
{
    assert(length() - pos1 >= n1);
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
    return data;
}


inline
bool Zstring::empty() const
{
    return descr->length == 0;
}


inline
DefaultChar Zstring::operator[](const size_t pos) const
{
    assert(pos < length());
    return data[pos];
}


inline
Zstring Zstring::operator+(const Zstring& string2) const
{
    return Zstring(*this)+=string2;
}


inline
Zstring Zstring::operator+(const DefaultChar* string2) const
{
    return Zstring(*this)+=string2;
}


inline
Zstring Zstring::operator+(const DefaultChar ch) const
{
    return Zstring(*this)+=ch;
}

//##################### Zsubstr #############################
inline
Zsubstr Zstring::zsubstr(size_t pos) const
{
    assert(pos <= length());
    return Zsubstr(*this, pos); //return reference counted string
}


inline
Zsubstr::Zsubstr()
{
    m_data   = m_ref.c_str();
    m_length = 0;
}


inline
Zsubstr::Zsubstr(const Zstring& ref, const size_t pos) :
        m_ref(ref),
        m_data(ref.c_str() + pos),
        m_length(ref.length() - pos) {}


inline
const DefaultChar* Zsubstr::c_str() const
{
    return m_data;
}


inline
size_t Zsubstr::length() const
{
    return m_length;
}


inline
bool Zsubstr::StartsWith(const Zstring& begin) const
{
    const size_t beginLength = begin.length();
    if (length() < beginLength)
        return false;

    return defaultCompare(m_data, begin.c_str(), beginLength) == 0;
}


inline
size_t Zsubstr::findFromEnd(const DefaultChar ch) const
{
    if (length() == 0)
        return Zstring::npos;

    size_t pos = length() - 1;
    do //pos points to last char of the string
    {
        if (m_data[pos] == ch)
            return pos;
    }
    while (--pos != static_cast<size_t>(-1));
    return Zstring::npos;
}



#endif // ZSTRING_H_INCLUDED
