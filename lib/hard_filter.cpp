// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "hard_filter.h"
#include <zen/zstring.h>
#include <wx/string.h>
#include <set>
#include <stdexcept>
#include <vector>
#include "../structures.h"
#include <wx+/serialize.h>
#include <typeinfo>

using namespace zen;

//inline bool operator<(const std::type_info& lhs, const std::type_info& rhs) { return lhs.before(rhs) != 0; } -> not working on GCC :(


//--------------------------------------------------------------------------------------------------
bool zen::operator<(const HardFilter& lhs, const HardFilter& rhs)
{
    if (typeid(lhs) != typeid(rhs))
        return typeid(lhs).before(typeid(rhs)) != 0; //note: in worst case, order is guaranteed to be stable only during each program run

    //this and other are same type:
    return lhs.cmpLessSameType(rhs);
}


void HardFilter::saveFilter(wxOutputStream& stream) const //serialize derived object
{
    //save type information
    writeString(stream, uniqueClassIdentifier());

    //save actual object
    save(stream);
}


HardFilter::FilterRef HardFilter::loadFilter(wxInputStream& stream)
{
    //read type information
    const Zstring uniqueClassId = readString<Zstring>(stream);

    //read actual object
    if (uniqueClassId == Zstr("NullFilter"))
        return NullFilter::load(stream);
    else if (uniqueClassId == Zstr("NameFilter"))
        return NameFilter::load(stream);
    else if (uniqueClassId == Zstr("CombinedFilter"))
        return CombinedFilter::load(stream);
    else
        throw std::logic_error("Programming Error: Unknown filter!");
}


namespace
{
//constructing them in addFilterEntry becomes perf issue for large filter lists
const Zstring asterisk(Zstr('*'));
const Zstring sepAsterisk         = FILE_NAME_SEPARATOR + asterisk;
const Zstring asteriskSep         = asterisk + FILE_NAME_SEPARATOR;
const Zstring asteriskSepAsterisk = asteriskSep + asterisk;
}


void addFilterEntry(const Zstring& filtername, std::vector<Zstring>& fileFilter, std::vector<Zstring>& directoryFilter)
{
    Zstring filterFormatted = filtername;

#ifdef FFS_WIN
    //Windows does NOT distinguish between upper/lower-case
    makeUpper(filterFormatted);
#elif defined FFS_LINUX
    //Linux DOES distinguish between upper/lower-case: nothing to do here
#endif
    if (startsWith(filterFormatted, FILE_NAME_SEPARATOR)) // \abc
        filterFormatted = afterFirst(filterFormatted, FILE_NAME_SEPARATOR); //leading separator is optional!

    //some syntactic sugar:
    if (filterFormatted == asteriskSepAsterisk) // *\*   := match everything except files directly in base directory
    {
        fileFilter.     push_back(filterFormatted);
        directoryFilter.push_back(asterisk);
        return;
    }
    //more syntactic sugar: handle beginning of filtername
    else if (startsWith(filterFormatted, asteriskSep)) // *\abc
    {
        addFilterEntry(filterFormatted.c_str() + 2, fileFilter, directoryFilter); //recursion is finite
    }
    //--------------------------------------------------------------------------------------------------
    //even more syntactic sugar: handle end of filtername
    if (endsWith(filterFormatted, FILE_NAME_SEPARATOR))
    {
        const Zstring candidate = beforeLast(filterFormatted, FILE_NAME_SEPARATOR);
        if (!candidate.empty())
            directoryFilter.push_back(candidate); //only relevant for directory filtering
    }
    else if (endsWith(filterFormatted, sepAsterisk)) // abc\*
    {
        fileFilter     .push_back(filterFormatted);
        directoryFilter.push_back(filterFormatted);

        const Zstring candidate = beforeLast(filterFormatted, FILE_NAME_SEPARATOR);
        if (!candidate.empty())
            directoryFilter.push_back(candidate); //only relevant for directory filtering
    }
    else if (!filterFormatted.empty())
    {
        fileFilter.     push_back(filterFormatted);
        directoryFilter.push_back(filterFormatted);
    }
}


namespace
{
template <class Char> inline
const Char* cStringFind(const Char* str, Char ch) //strchr()
{
    for (;;)
    {
        const Char s = *str;
        if (s == ch) //ch is allowed to be 0 by contract! must return end of string in this case
            return str;

        if (s == 0)
            return nullptr;
        ++str;
    }
}


bool matchesMask(const Zchar* str, const Zchar* mask)
{
    for (;; ++mask, ++str)
    {
        Zchar m = *mask;
        if (m == 0)
            return *str == 0;

        switch (m)
        {
            case Zstr('?'):
                if (*str == 0)
                    return false;
                break;

            case Zstr('*'):
                //advance mask to next non-* char
                do
                {
                    m = *++mask;
                }
                while (m == Zstr('*'));

                if (m == 0) //mask ends with '*':
                    return true;

                //*? - pattern
                if (m == Zstr('?'))
                {
                    ++mask;
                    while (*str++ != 0)
                        if (matchesMask(str, mask))
                            return true;
                    return false;
                }

                //*[letter] - pattern
                ++mask;
                for (;;)
                {
                    str = cStringFind(str, m);
                    if (!str)
                        return false;

                    ++str;
                    if (matchesMask(str, mask))
                        return true;
                }

            default:
                if (*str != m)
                    return false;
        }
    }
}


//returns true if string matches at least the beginning of mask
inline
bool matchesMaskBegin(const Zchar* str, const Zchar* mask)
{
    for (;; ++mask, ++str)
    {
        const Zchar m = *mask;
        if (m == 0)
            return *str == 0;

        switch (m)
        {
            case Zstr('?'):
                if (*str == 0)
                    return true;
                break;

            case Zstr('*'):
                return true;

            default:
                if (*str != m)
                    return *str == 0;
        }
    }
}
}


inline
bool matchesFilter(const Zstring& name, const std::vector<Zstring>& filter)
{
    return std::any_of(filter.begin(), filter.end(), [&](const Zstring& mask) { return matchesMask(name.c_str(), mask.c_str()); });
}


inline
bool matchesFilterBegin(const Zstring& name, const std::vector<Zstring>& filter)
{
    return std::any_of(filter.begin(), filter.end(), [&](const Zstring& mask) { return matchesMaskBegin(name.c_str(), mask.c_str()); });
}


std::vector<Zstring> splitByDelimiter(const Zstring& filterString)
{
    //delimiters may be ';' or '\n'
    std::vector<Zstring> output;

    const std::vector<Zstring> blocks = split(filterString, Zchar(';'));
    std::for_each(blocks.begin(), blocks.end(),
                  [&](const Zstring& item)
    {
        const std::vector<Zstring> blocks2 = split(item, Zchar('\n'));

        std::for_each(blocks2.begin(), blocks2.end(),
                      [&](Zstring entry)
        {
            trim(entry);
            if (!entry.empty())
                output.push_back(entry);
        });
    });

    return output;
}

//#################################################################################################
NameFilter::NameFilter(const Zstring& includeFilter, const Zstring& excludeFilter) :
    includeFilterTmp(includeFilter), //save constructor arguments for serialization
    excludeFilterTmp(excludeFilter)
{
    //no need for regular expressions: In tests wxRegex was by factor of 10 slower than wxString::Matches()

    //load filter into vectors of strings
    //delimiters may be ';' or '\n'
    const std::vector<Zstring>& includeList = splitByDelimiter(includeFilter);
    const std::vector<Zstring>& excludeList = splitByDelimiter(excludeFilter);

    //setup include/exclude filters for files and directories
    std::for_each(includeList.begin(), includeList.end(), [&](const Zstring& entry) { addFilterEntry(entry, filterFileIn, filterFolderIn); });
    std::for_each(excludeList.begin(), excludeList.end(), [&](const Zstring& entry) { addFilterEntry(entry, filterFileEx, filterFolderEx); });

    auto removeDuplicates = [](std::vector<Zstring>& cont)
    {
        std::vector<Zstring> output;
        std::set<Zstring> used;
        std::copy_if(cont.begin(), cont.end(), std::back_inserter(output), [&](const Zstring& item) { return used.insert(item).second; });
        output.swap(cont);
    };

    removeDuplicates(filterFileIn);
    removeDuplicates(filterFolderIn);
    removeDuplicates(filterFileEx);
    removeDuplicates(filterFolderEx);
}


bool NameFilter::passFileFilter(const Zstring& relFilename) const
{
#ifdef FFS_WIN //Windows does NOT distinguish between upper/lower-case
    Zstring nameFormatted = relFilename;
    makeUpper(nameFormatted);
#elif defined FFS_LINUX //Linux DOES distinguish between upper/lower-case
    const Zstring& nameFormatted = relFilename; //nothing to do here
#endif

    return matchesFilter(nameFormatted, filterFileIn) && //process include filters
           !matchesFilter(nameFormatted, filterFileEx);  //process exclude filters
}


bool NameFilter::passDirFilter(const Zstring& relDirname, bool* subObjMightMatch) const
{
    assert(!subObjMightMatch || *subObjMightMatch == true); //check correct usage

#ifdef FFS_WIN //Windows does NOT distinguish between upper/lower-case
    Zstring nameFormatted = relDirname;
    makeUpper(nameFormatted);
#elif defined FFS_LINUX //Linux DOES distinguish between upper/lower-case
    const Zstring& nameFormatted = relDirname; //nothing to do here
#endif

    if (matchesFilter(nameFormatted, filterFolderEx)) //process exclude filters
    {
        if (subObjMightMatch)
            *subObjMightMatch = false; //exclude subfolders/subfiles as well
        return false;
    }

    if (!matchesFilter(nameFormatted, filterFolderIn)) //process include filters
    {
        if (subObjMightMatch)
        {
            const Zstring& subNameBegin = nameFormatted + FILE_NAME_SEPARATOR; //const-ref optimization

            *subObjMightMatch = matchesFilterBegin(subNameBegin, filterFileIn) || //might match a file in subdirectory
                                matchesFilterBegin(subNameBegin, filterFolderIn); //or another subdirectory
        }
        return false;
    }

    return true;
}


bool NameFilter::isNull(const Zstring& includeFilter, const Zstring& excludeFilter)
{
    Zstring include = includeFilter;
    Zstring exclude = excludeFilter;
    trim(include);
    trim(exclude);

    return include == Zstr("*") && exclude.empty();
    //return NameFilter(includeFilter, excludeFilter).isNull(); -> very expensive for huge lists
}

bool NameFilter::isNull() const
{
    static NameFilter output(Zstr("*"), Zstring());
    return *this == output;
}


bool NameFilter::cmpLessSameType(const HardFilter& other) const
{
    assert(typeid(*this) == typeid(other)); //always given in this context!

    const NameFilter& otherNameFilt = static_cast<const NameFilter&>(other);

    if (filterFileIn != otherNameFilt.filterFileIn)
        return filterFileIn < otherNameFilt.filterFileIn;

    if (filterFolderIn != otherNameFilt.filterFolderIn)
        return filterFolderIn < otherNameFilt.filterFolderIn;

    if (filterFileEx != otherNameFilt.filterFileEx)
        return filterFileEx < otherNameFilt.filterFileEx;

    if (filterFolderEx != otherNameFilt.filterFolderEx)
        return filterFolderEx < otherNameFilt.filterFolderEx;

    return false; //vectors equal
}


Zstring NameFilter::uniqueClassIdentifier() const
{
    return Zstr("NameFilter");
}


void NameFilter::save(wxOutputStream& stream) const
{
    writeString(stream, includeFilterTmp);
    writeString(stream, excludeFilterTmp);
}


HardFilter::FilterRef NameFilter::load(wxInputStream& stream) //"constructor"
{
    const Zstring include = readString<Zstring>(stream);
    const Zstring exclude = readString<Zstring>(stream);

    return FilterRef(new NameFilter(include, exclude));
}