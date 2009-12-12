#include "zstring.h"
#include <stdexcept>

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "dllLoader.h"
#endif  //FFS_WIN

#ifdef __WXDEBUG__
#include <wx/string.h>
#endif


#ifdef __WXDEBUG__
AllocationCount::~AllocationCount()
{
    if (activeStrings.size() > 0)
#ifdef FFS_WIN
    {
        int rowCount = 0;
        wxString leakingStrings;
        for (std::set<const DefaultChar*>::const_iterator i = activeStrings.begin();
                i != activeStrings.end() && ++rowCount <= 10;
                ++i)
        {
            leakingStrings += wxT("\"");
            leakingStrings += *i;
            leakingStrings += wxT("\"\n");
        }

        MessageBox(NULL, wxString(wxT("Memory leak detected!")) + wxT("\n\n")
                   + wxT("Candidates:\n") + leakingStrings,
                   wxString::Format(wxT("%i"), activeStrings.size()), 0);
    }
#else
        throw std::logic_error("Memory leak!");
#endif
}


AllocationCount& AllocationCount::getInstance()
{
    static AllocationCount global;
    return global;
}
#endif


#ifdef FFS_WIN

#ifndef LOCALE_INVARIANT
#define LOCALE_INVARIANT                0x007f
#endif


inline
int compareStringsWin32(const wchar_t* a, const wchar_t* b, const int aCount = -1, const int bCount = -1)
{
    //try to call "CompareStringOrdinal" first for low-level string comparison: unfortunately available not before Windows Vista!
    typedef int (WINAPI *CompareStringOrdinalFunc)(
        LPCWSTR lpString1,
        int     cchCount1,
        LPCWSTR lpString2,
        int     cchCount2,
        BOOL    bIgnoreCase);
    static const CompareStringOrdinalFunc ordinalCompare = Utility::loadDllFunKernel<CompareStringOrdinalFunc>("CompareStringOrdinal");


    //we're lucky here! This additional test for "CompareStringOrdinal" has no noticeable performance impact!!
    if (ordinalCompare != NULL)
    {
        const int rv = (*ordinalCompare)(
                           a,  	      //pointer to first string
                           aCount,	  //size, in bytes or characters, of first string
                           b,	      //pointer to second string
                           bCount,    //size, in bytes or characters, of second string
                           true); 	  //ignore case

        if (rv == 0)
            throw std::runtime_error("Error comparing strings (ordinal)!");
        else
            return rv - 2; //convert to C-style string compare result
    }
    else  //fallback to "CompareString". Attention: this function is NOT accurate: for example "weiß" == "weiss"!!!
    {
        //DON'T use lstrcmpi() here! It uses word sort and is locale dependent!
        //Use CompareString() with "SORT_STRINGSORT" instead!!!

        const int rv = CompareString(
                           LOCALE_INVARIANT,    //locale independent
                           NORM_IGNORECASE | SORT_STRINGSORT, //comparison-style options
                           a,  	                //pointer to first string
                           aCount,	            //size, in bytes or characters, of first string
                           b,	                //pointer to second string
                           bCount); 	        //size, in bytes or characters, of second string

        if (rv == 0)
            throw std::runtime_error("Error comparing strings!");
        else
            return rv - 2; //convert to C-style string compare result
    }
}
#endif


#ifdef FFS_WIN
int Zstring::CmpNoCase(const DefaultChar* other) const
{
    return ::compareStringsWin32(c_str(), other); //way faster than wxString::CmpNoCase()!!
}


int Zstring::CmpNoCase(const Zstring& other) const
{
    return ::compareStringsWin32(c_str(), other.c_str(), length(), other.length()); //way faster than wxString::CmpNoCase()!!
}
#endif


Zstring& Zstring::Replace(const DefaultChar* old, const DefaultChar* replacement, bool replaceAll)
{
    const size_t oldLen         = defaultLength(old);
    const size_t replacementLen = defaultLength(replacement);

    size_t pos = 0;
    for (;;)
    {
        pos = find(old, pos);
        if (pos == npos)
            break;

        replace(pos, oldLen, replacement, replacementLen);
        pos += replacementLen; //move past the string that was replaced

        // stop now?
        if (!replaceAll)
            break;
    }
    return *this;
}


bool matchesHelper(const DefaultChar* string, const DefaultChar* mask)
{
    for (DefaultChar ch; (ch = *mask) != 0; ++mask, ++string)
    {
        switch (ch)
        {
        case DefaultChar('?'):
            if (*string == 0)
                return false;
            break;

        case DefaultChar('*'):
            //advance to next non-*/? char
            do
            {
                ++mask;
                ch = *mask;
            }
            while (ch == DefaultChar('*') || ch == DefaultChar('?'));
            //if match ends with '*':
            if (ch == DefaultChar(0))
                return true;

            ++mask;
            while ((string = defaultStrFind(string, ch)) != NULL)
            {
                if (matchesHelper(string + 1, mask))
                    return true;
                ++string;
            }
            return false;

        default:
            if (*string != ch)
                return false;
        }
    }
    return *string == 0;
}


bool Zstring::Matches(const DefaultChar* mask) const
{
    return matchesHelper(c_str(), mask);
}


bool Zstring::Matches(const DefaultChar* name, const DefaultChar* mask)
{
    return matchesHelper(name, mask);
}


Zstring& Zstring::Trim(bool fromRight)
{
    const size_t thisLen = length();
    if (thisLen == 0)
        return *this;

    DefaultChar* const strBegin = data();

    if (fromRight)
    {
        const DefaultChar* cursor = strBegin + thisLen - 1;
        while (cursor != strBegin - 1 && defaultIsWhiteSpace(*cursor)) //break when pointing one char further than last skipped element
            --cursor;
        ++cursor;

        const size_t newLength = cursor - strBegin;
        if (newLength != thisLen)
        {
            if (descr->refCount > 1) //allocate new string
                *this = Zstring(strBegin, newLength);
            else //overwrite this string
            {
                descr->length     = newLength;
                strBegin[newLength] = 0;
            }
        }
    }
    else
    {
        const DefaultChar* cursor = strBegin;
        const DefaultChar* const strEnd = strBegin + thisLen;
        while (cursor != strEnd && defaultIsWhiteSpace(*cursor))
            ++cursor;

        const size_t diff = cursor - strBegin;
        if (diff)
        {
            if (descr->refCount > 1) //allocate new string
                *this = Zstring(cursor, thisLen - diff);
            else
            {
                //overwrite this string
                ::memmove(strBegin, cursor, (thisLen - diff + 1) * sizeof(DefaultChar)); //note: do not simply let data point to different location: this corrupts reserve()!
                descr->length -= diff;
            }
        }
    }

    return *this;
}


std::vector<Zstring> Zstring::Tokenize(const DefaultChar delimiter) const
{
    std::vector<Zstring> output;

    const size_t thisLen = length();
    size_t indexStart = 0;
    for (;;)
    {
        size_t indexEnd = find(delimiter, indexStart);
        if (indexEnd == Zstring::npos)
            indexEnd = thisLen;

        if (indexStart != indexEnd) //do not add empty strings
        {
            Zstring newEntry = substr(indexStart, indexEnd - indexStart);
            newEntry.Trim(true);  //remove whitespace characters from right
            newEntry.Trim(false); //remove whitespace characters from left

            if (!newEntry.empty())
                output.push_back(newEntry);
        }
        if (indexEnd == thisLen)
            break;

        indexStart = indexEnd + 1; //delimiter is a single character
    }

    return output;
}


#ifdef FFS_WIN
Zstring& Zstring::MakeLower()
{
    const size_t thisLen = length();
    if (thisLen == 0)
        return *this;

    reserve(thisLen);    //make unshared
    ::CharLower(data()); //use Windows' lower case conversion

    return *this;
}
#endif


//###############################################################
//std::string functions
Zstring Zstring::substr(size_t pos, size_t len) const
{
    if (len == npos)
    {
        assert(pos <= length());
        return Zstring(c_str() + pos, length() - pos); //reference counting not used: different length
    }
    else
    {
        assert(length() - pos >= len);
        return Zstring(c_str() + pos, len);
    }
}


size_t Zstring::rfind(const DefaultChar ch, size_t pos) const
{
    const size_t thisLen = length();
    if (thisLen == 0)
        return npos;

    if (pos == npos)
        pos = thisLen - 1;
    else
        assert(pos <= length());

    do //pos points to last char of the string
    {
        if (c_str()[pos] == ch)
            return pos;
    }
    while (--pos != static_cast<size_t>(-1));
    return npos;
}


Zstring& Zstring::replace(size_t pos1, size_t n1, const DefaultChar* str, size_t n2)
{
    assert(str < c_str() || c_str() + length() < str); //str mustn't point to data in this string
    assert(n1 <= length() - pos1);

    const size_t oldLen = length();
    if (oldLen == 0)
    {
        assert(pos1 == 0 && n1 == 0);
        return *this = Zstring(str, n2);
    }

    const size_t newLen = oldLen - n1 + n2;
    if (newLen > oldLen || descr->refCount > 1)
    {
        //allocate a new string
        StringDescriptor* newDescr = allocate(newLen);

        //assemble new string with replacement
        DefaultChar* const newData = reinterpret_cast<DefaultChar*>(newDescr + 1);
        ::memcpy(newData, c_str(), pos1 * sizeof(DefaultChar));
        ::memcpy(newData + pos1, str, n2 * sizeof(DefaultChar));
        ::memcpy(newData + pos1 + n2, c_str() + pos1 + n1, (oldLen - pos1 - n1) * sizeof(DefaultChar));
        newData[newLen] = 0;

        decRef();
        descr = newDescr;
    }
    else  //overwrite current string: case "n2 == 0" is handled implicitly
    {
        ::memcpy(data() + pos1, str, n2 * sizeof(DefaultChar));
        if (newLen < oldLen)
        {
            ::memmove(data() + pos1 + n2, data() + pos1 + n1, (oldLen - pos1 - n1) * sizeof(DefaultChar));
            data()[newLen]  = 0;
            descr->length = newLen;
        }
    }

    return *this;
}


Zstring& Zstring::operator=(const DefaultChar* source)
{
    const size_t sourceLen = defaultLength(source);

    if (descr->refCount > 1 || descr->capacity < sourceLen) //allocate new string
        *this = Zstring(source, sourceLen);
    else
    {
        //overwrite this string
        ::memcpy(data(), source, (sourceLen + 1) * sizeof(DefaultChar)); //include null-termination
        descr->length = sourceLen;
    }
    return *this;
}


Zstring& Zstring::assign(const DefaultChar* source, size_t len)
{
    if (descr->refCount > 1 || descr->capacity < len) //allocate new string
        *this = Zstring(source, len);
    else
    {
        //overwrite this string
        ::memcpy(data(), source, len * sizeof(DefaultChar)); //don't know if source is null-terminated
        data()[len] = 0; //include null-termination
        descr->length = len;
    }
    return *this;
}


Zstring& Zstring::operator+=(const Zstring& other)
{
    const size_t otherLen = other.length();
    if (otherLen != 0)
    {
        const size_t thisLen = length();
        const size_t newLen = thisLen + otherLen;
        reserve(newLen); //make unshared and check capacity

        ::memcpy(data() + thisLen, other.c_str(), (otherLen + 1) * sizeof(DefaultChar)); //include null-termination
        descr->length = newLen;
    }
    return *this;
}


Zstring& Zstring::operator+=(const DefaultChar* other)
{
    const size_t otherLen = defaultLength(other);
    if (otherLen != 0)
    {
        const size_t thisLen = length();
        const size_t newLen = thisLen + otherLen;
        reserve(newLen); //make unshared and check capacity

        ::memcpy(data() + thisLen, other, (otherLen + 1) * sizeof(DefaultChar)); //include NULL-termination
        descr->length = newLen;
    }
    return *this;
}


Zstring& Zstring::operator+=(DefaultChar ch)
{
    const size_t oldLen = length();
    reserve(oldLen + 1); //make unshared and check capacity
    data()[oldLen] = ch;
    data()[oldLen + 1] = 0;
    ++descr->length;
    return *this;
}


void Zstring::reserve(size_t capacityNeeded) //make unshared and check capacity
{
    assert(capacityNeeded != 0);

    if (descr->refCount > 1)
    {
        //allocate a new string
        const size_t oldLength = length();
        assert(oldLength <= getCapacityToAllocate(capacityNeeded));

        StringDescriptor* newDescr = allocate(capacityNeeded);
        newDescr->length = oldLength;

        ::memcpy(reinterpret_cast<DefaultChar*>(newDescr + 1), c_str(), (oldLength + 1) * sizeof(DefaultChar)); //include NULL-termination

        decRef();
        descr = newDescr;
    }
    else if (descr->capacity < capacityNeeded)
    {
        //try to resize the current string (allocate anew if necessary)
        const size_t newCapacity = getCapacityToAllocate(capacityNeeded);

#ifdef __WXDEBUG__
        AllocationCount::getInstance().dec(c_str()); //test Zstring for memory leaks
#endif

        descr = static_cast<StringDescriptor*>(::realloc(descr, sizeof(StringDescriptor) + (newCapacity + 1) * sizeof(DefaultChar)));
        if (descr == NULL)
            throw std::bad_alloc();

#ifdef __WXDEBUG__
        AllocationCount::getInstance().inc(c_str()); //test Zstring for memory leaks
#endif

        descr->capacity = newCapacity;
    }
}
