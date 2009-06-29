#include "zstring.h"
#include "globalFunctions.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#endif  //FFS_WIN



#ifdef __WXDEBUG__
AllocationCount::~AllocationCount()
{
    if (count != 0)
#ifdef FFS_WIN
        MessageBox(NULL, wxT("Fatal Error! Allocation problem with Zstring! (No problem if it occurs while Unit testing only!)"), wxString::Format(wxT("%i"), count), 0);
#else
        std::abort();
#endif
}


AllocationCount& AllocationCount::getInstance()
{
    static AllocationCount global;
    return global;
}
#endif


#ifdef FFS_WIN
int FreeFileSync::compareStringsWin32(const wchar_t* a, const wchar_t* b, const int aCount, const int bCount)
{
    //DON'T use lstrcmpi() here! It uses word sort, which unfortunately is NOT always a strict weak sorting function for some locales (e.g. swedish)
    //Use CompareString() with "SORT_STRINGSORT" instead!!!

    const int rv = CompareString(
                       LOCALE_USER_DEFAULT, //locale identifier
                       NORM_IGNORECASE | SORT_STRINGSORT,	  //comparison-style options
                       a,  	                //pointer to first string
                       aCount,	            //size, in bytes or characters, of first string
                       b,	                //pointer to second string
                       bCount); 	        //size, in bytes or characters, of second string

    if (rv == 0)
        throw RuntimeException(wxString(wxT("Error comparing strings!")));
    else
        return rv - 2; //convert to C-style string compare result
}
#endif


Zstring& Zstring::Replace(const DefaultChar* old, const DefaultChar* replacement, bool replaceAll)
{
    const size_t oldLen         = defaultLength(old);
    const size_t replacementLen = defaultLength(replacement);

    size_t pos = 0;
    while (true)
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
    for (DefaultChar ch; (ch = *mask) != 0; ++mask)
    {
        switch (ch)
        {
        case DefaultChar('?'):
            if (*string == 0)
                return false;
            else
                ++string;
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
            else
                ++string;
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

    if (fromRight)
    {
        const DefaultChar* cursor = data + thisLen - 1;
        while (cursor != data - 1 && defaultIsWhiteSpace(*cursor)) //break when pointing one char further than last skipped element
            --cursor;
        ++cursor;

        const size_t newLength = cursor - data;
        if (newLength != thisLen)
        {
            if (descr->refCount > 1) //allocate new string
                *this = Zstring(data, newLength);
            else //overwrite this strin
            {
                descr->length   = newLength;
                data[newLength] = DefaultChar(0);
            }
        }
    }
    else
    {
        DefaultChar* cursor = data;
        DefaultChar ch;
        while ((ch = *cursor) != 0 && defaultIsWhiteSpace(ch))
            ++cursor;

        const size_t diff = cursor - data;
        if (diff)
        {
            if (descr->refCount > 1) //allocate new string
                *this = Zstring(cursor, thisLen - diff);
            else
            {   //overwrite this string
                data = cursor; //no problem when deallocating data, since descr points to begin of allocated area
                descr->capacity -= diff;
                descr->length   -= diff;
            }
        }
    }

    return *this;
}


Zstring& Zstring::MakeLower()
{
    const size_t thisLen = length();
    if (thisLen == 0)
        return *this;

    if (descr->refCount > 1) //allocate new string
    {
        StringDescriptor* newDescr;
        DefaultChar*      newData;
        allocate(thisLen, newDescr, newData);

        for (unsigned int i = 0; i < thisLen; ++i)
            newData[i] = defaultToLower(data[i]);
        newData[thisLen] = 0;

        decRef();
        descr = newDescr;
        data  = newData;
    }
    else
    {   //overwrite this string
        for (unsigned int i = 0; i < thisLen; ++i)
            data[i] = defaultToLower(data[i]);
    }

    return *this;
}


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
        if (data[pos] == ch)
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
    {   //allocate a new string
        StringDescriptor* newDescr;
        DefaultChar* newData;
        allocate(newLen, newDescr, newData);

        //assemble new string with replacement
        memcpy(newData, data, pos1 * sizeof(DefaultChar));
        memcpy(newData + pos1, str, n2 * sizeof(DefaultChar));
        memcpy(newData + pos1 + n2, data + pos1 + n1, (oldLen - pos1 - n1) * sizeof(DefaultChar));
        newData[newLen] = 0;

        decRef();
        data  = newData;
        descr = newDescr;
    }
    else  //overwrite current string: case "n2 == 0" is handled implicitly
    {
        memcpy(data + pos1, str, n2 * sizeof(DefaultChar));
        if (newLen < oldLen)
        {
            memmove(data + pos1 + n2, data + pos1 + n1, (oldLen - pos1 - n1) * sizeof(DefaultChar));
            data[newLen]  = 0;
            descr->length = newLen;
        }
    }

    return *this;
}


Zstring& Zstring::operator=(const DefaultChar* source)
{
    const size_t sourceLen = defaultLength(source);
    if (sourceLen == 0)
        return *this = Zstring();

    if (descr->refCount > 1 || descr->capacity < sourceLen) //allocate new string
        *this = Zstring(source, sourceLen);
    else
    {   //overwrite this string
        memcpy(data, source, sourceLen * sizeof(DefaultChar));
        data[sourceLen] = 0;
        descr->length = sourceLen;
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
        copyBeforeWrite(newLen);

        memcpy(data + thisLen, other.c_str(), otherLen * sizeof(DefaultChar));
        data[newLen] = 0;
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
        copyBeforeWrite(newLen);

        memcpy(data + thisLen, other, otherLen * sizeof(DefaultChar));
        data[newLen] = 0;
        descr->length = newLen;
    }
    return *this;
}


Zstring& Zstring::operator+=(DefaultChar ch)
{
    const size_t oldLen = length();
    copyBeforeWrite(oldLen + 1);
    data[oldLen] = ch;
    data[oldLen + 1] = 0;
    ++descr->length;
    return *this;
}


void Zstring::copyBeforeWrite(const size_t capacityNeeded)
{
    assert(capacityNeeded != 0);

    if (descr->refCount > 1)
    {   //allocate a new string
        const size_t oldLength = length();
        assert(oldLength <= getCapacityToAllocate(capacityNeeded));

        StringDescriptor* newDescr;
        DefaultChar*      newData;
        allocate(capacityNeeded, newDescr, newData);
        newDescr->length = oldLength;

        if (oldLength)
        {
            memcpy(newData, data, oldLength * sizeof(DefaultChar));
            newData[oldLength] = 0;
        }
        decRef();
        descr = newDescr;
        data  = newData;
    }
    else if (descr->capacity < capacityNeeded)
    {   //try to resize the current string (allocate anew if necessary)
        const size_t newCapacity = getCapacityToAllocate(capacityNeeded);

        descr = (StringDescriptor*) realloc(descr, sizeof(StringDescriptor) + (newCapacity + 1) * sizeof(DefaultChar));
        if (descr == NULL)
            throw std::bad_alloc();
        data = (DefaultChar*)(descr + 1);
        descr->capacity = newCapacity;
    }
}
