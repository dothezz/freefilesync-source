/***************************************************************
 * Purpose:   High performance string class
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   Jan. 2009
 **************************************************************/

#ifndef ZSTRING_H_INCLUDED
#define ZSTRING_H_INCLUDED

#include <wx/string.h>

namespace FreeFileSync
{
#ifdef FFS_WIN
    //super fast case-insensitive string comparison: way faster than wxString::CmpNoCase()!!!
    int compareStringsWin32(const wchar_t* a, const wchar_t* b);
    int compareStringsWin32(const wchar_t* a, const wchar_t* b, const int aCount, const int bCount);
#endif  //FFS_WIN
}

#ifdef FFS_WIN
#define ZSTRING_WIDE_CHAR //use wide character strings

#elif defined FFS_LINUX
#define ZSTRING_CHAR //use char strings
#endif


#ifdef ZSTRING_CHAR
typedef char defaultChar;
#elif defined ZSTRING_WIDE_CHAR
typedef wchar_t defaultChar;
#endif

class Zstring
{
public:
    Zstring();
    Zstring(const defaultChar* source);                //string is copied: O(length)
    Zstring(const defaultChar* source, size_t length); //string is copied: O(length)
    Zstring(const Zstring& source);                    //reference-counting => O(1)
    Zstring(const wxString& source);                   //string is copied: O(length)
    ~Zstring();

    operator const defaultChar*() const;               //implicit conversion to C string
    operator const wxString() const;                   //implicit conversion to wxString

    //wxWidgets functions
    bool StartsWith(const defaultChar* begin) const;
    bool StartsWith(const Zstring& begin) const;
    bool EndsWith(const defaultChar* end) const;
    bool EndsWith(const Zstring& end) const;
#ifdef FFS_WIN
    int CmpNoCase(const defaultChar* other) const;
    int CmpNoCase(const Zstring& other) const;
#endif
    int Cmp(const defaultChar* other) const;
    int Cmp(const Zstring& other) const;
    size_t Replace(const defaultChar* old, const defaultChar* replacement, bool replaceAll = true);
    Zstring AfterLast(defaultChar ch) const;
    Zstring BeforeLast(defaultChar ch) const;
    size_t Find(defaultChar ch, bool fromEnd) const;
    bool Matches(const defaultChar* mask) const;
    Zstring& Trim(bool fromRight); //from right or left
    Zstring& MakeLower();

    //std::string functions
    size_t length() const;
    const defaultChar* c_str() const;
    Zstring substr(size_t pos = 0, size_t len = npos) const;
    bool empty() const;
    int compare(const defaultChar* other) const;
    int compare(const Zstring& other) const;
    int compare(const size_t pos1, const size_t n1, const defaultChar* other) const;
    size_t find(const defaultChar* str, const size_t pos = 0 ) const;
    size_t find(const defaultChar ch, const size_t pos = 0) const;
    size_t rfind(const defaultChar ch, size_t pos = npos) const;
    Zstring& replace(size_t pos1, size_t n1, const defaultChar* str, size_t n2);

    Zstring& operator=(const Zstring& source);
    Zstring& operator=(const defaultChar* source);

    bool operator==(const Zstring& other) const;
    bool operator==(const defaultChar* other) const;

    Zstring& operator+=(const Zstring& other);
    Zstring& operator+=(const defaultChar* other);
    Zstring& operator+=(defaultChar ch);

    Zstring operator+(const Zstring& string2) const;
    Zstring operator+(const defaultChar* string2) const;
    Zstring operator+(const defaultChar ch) const;

    static const size_t	npos = static_cast<size_t>(-1);

private:
    void initAndCopy(const defaultChar* source, size_t length);
    void incRef();                                     //support for reference-counting
    void decRef();                                     //
    void copyBeforeWrite(const size_t capacityNeeded); //and copy-on-write

    struct StringDescriptor
    {
        StringDescriptor(const unsigned int refC, const size_t len, const size_t cap) : refCount(refC), length(len), capacity(cap) {}
        unsigned int refCount;
        size_t       length;
        size_t       capacity; //allocated length without null-termination
    };
    static void allocate(const unsigned int newRefCount, const size_t newLength, const size_t newCapacity, Zstring::StringDescriptor*& newDescr, defaultChar*& newData);

    StringDescriptor* descr;
    defaultChar*      data;
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
    return (ch < 128) && isspace(ch) != 0;
}

inline
char defaultToLower(const char ch)
{
    return tolower(ch);
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
void Zstring::allocate(const unsigned int newRefCount,
                       const size_t       newLength,
                       const size_t       newCapacity,
                       StringDescriptor*& newDescr,
                       defaultChar*&      newData)
{   //allocate and set data for new string
    if (newCapacity)
    {
        newDescr = (StringDescriptor*) malloc( sizeof(StringDescriptor) + (newCapacity + 1) * sizeof(defaultChar));
        if (newDescr == NULL)
            throw; //std::bad_alloc& e
        newData = (defaultChar*)(newDescr + 1);
    }
    else
    {
        newDescr = (StringDescriptor*) malloc( sizeof(StringDescriptor));
        if (newDescr == NULL)
            throw; //std::bad_alloc& e
        newData = NULL;
    }

    newDescr->refCount = newRefCount;
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
    allocate(1, 0, 0, descr, data);
}


inline
Zstring::Zstring(const defaultChar* source)
{
    initAndCopy(source, defaultLength(source));
}


inline
Zstring::Zstring(const defaultChar* source, size_t length)
{
    initAndCopy(source, length);
}


inline
Zstring::Zstring(const Zstring& source)
{
    descr = source.descr;
    data = source.data;
    incRef(); //reference counting!
}


inline
Zstring::Zstring(const wxString& source)
{
    initAndCopy(source.c_str(), source.length());
}


inline
Zstring::~Zstring()
{
    decRef();
}


inline
size_t getCapacityToAllocate(const size_t length)
{
    return (length + (19 - length % 16)); //allocate some additional length to speed up concatenation
}


inline
void Zstring::initAndCopy(const defaultChar* source, size_t length)
{
    const size_t newCapacity = getCapacityToAllocate(length);
    allocate(1, length, newCapacity, descr, data);
    memcpy(data, source, length * sizeof(defaultChar));
    data[length] = 0;
}


inline
void Zstring::incRef()
{
    assert(descr);
    ++descr->refCount;
}


inline
void Zstring::decRef()
{
    assert(descr && descr->refCount >= 1);
    if (--descr->refCount == 0)
    {
        assert(descr); //descr points to the begin of the allocated memory block
        free(descr);   //this must NEVER be changed!! E.g. Trim() relies on descr being start of allocated memory block
#ifdef __WXDEBUG__
        --allocCount; //test Zstring for memory leaks
#endif
    }
}


#ifdef FFS_WIN
inline
int Zstring::CmpNoCase(const defaultChar* other) const
{
    return FreeFileSync::compareStringsWin32(c_str(), other); //way faster than wxString::CmpNoCase()!!
}


inline
int Zstring::CmpNoCase(const Zstring& other) const
{
    return FreeFileSync::compareStringsWin32(c_str(), other.c_str()); //way faster than wxString::CmpNoCase()!!
}
#endif


inline
Zstring::operator const defaultChar*() const
{
    return c_str();
}


inline
Zstring& Zstring::operator=(const Zstring& source)
{
    if (this != &source)
    {
        decRef();
        descr = source.descr;
        data  = source.data;
        incRef();
    }
    return *this;
}


inline
size_t Zstring::Find(defaultChar ch, bool fromEnd) const
{
    if (fromEnd)
        return rfind(ch, npos);
    else
        return find(ch, 0);
}


// get all characters after the last occurence of ch
// (returns the whole string if ch not found)
inline
Zstring Zstring::AfterLast(defaultChar ch) const
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
Zstring Zstring::BeforeLast(defaultChar ch) const
{
    size_t pos = rfind(ch, npos);

    if (pos != npos && pos != 0 )
        return Zstring(data, pos); //data is non-empty string in this context: else ch would not have been found!
    else
        return Zstring();
}


inline
bool Zstring::StartsWith(const defaultChar* begin) const
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
bool Zstring::EndsWith(const defaultChar* end) const
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
size_t Zstring::find(const defaultChar* str, const size_t pos) const
{
    assert(pos <= length());
    const defaultChar* thisStr = c_str();
    const defaultChar* found = defaultStrFind(thisStr + pos, str);
    return found == NULL ? npos : found - thisStr;
}


inline
size_t Zstring::find(const defaultChar ch, const size_t pos) const
{
    assert(pos <= length());
    const defaultChar* thisStr = c_str();
    const defaultChar* found = defaultStrFind(thisStr + pos, ch);
    return found == NULL ? npos : found - thisStr;
}


inline
Zstring::operator const wxString() const
{
    return wxString(c_str());
}


inline
int Zstring::Cmp(const defaultChar* other) const
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
    return defaultCompare(c_str(), other.c_str()) == 0; //overload using strcmp(char*, char*) should be fastest!
}


inline
bool Zstring::operator==(const defaultChar* other) const
{
    return compare(other) == 0;
}


inline
int Zstring::compare(const Zstring& other) const
{
    return defaultCompare(c_str(), other.c_str()); //overload using strcmp(char*, char*) should be fastest!
}


inline
int Zstring::compare(const defaultChar* other) const
{
    return defaultCompare(c_str(), other); //overload using strcmp(char*, char*) should be fastest!
}


inline
int Zstring::compare(const size_t pos1, const size_t n1, const defaultChar* other) const
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
const defaultChar* Zstring::c_str() const
{
    if (length())
        return data;
    else
#ifdef ZSTRING_CHAR
        return "";
#elif defined ZSTRING_WIDE_CHAR
        return L"";
#endif
}


inline
bool Zstring::empty() const
{
    return length() == 0;
}


inline
Zstring Zstring::operator+(const Zstring& string2) const
{
    return Zstring(*this)+=string2;
}


inline
Zstring Zstring::operator+(const defaultChar* string2) const
{
    return Zstring(*this)+=string2;
}


inline
Zstring Zstring::operator+(const defaultChar ch) const
{
    return Zstring(*this)+=ch;
}


#endif // ZSTRING_H_INCLUDED
