// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "hard_filter.h"
#include "../shared/zstring.h"
#include <wx/string.h>
#include <set>
#include <stdexcept>
#include <vector>
#include "../structures.h"
#include <boost/bind.hpp>
#include "../shared/loki/LokiTypeInfo.h"
#include "../shared/serialize.h"

using namespace zen;


//--------------------------------------------------------------------------------------------------
bool zen::operator<(const HardFilter& lhs, const HardFilter& rhs)
{
    if (Loki::TypeInfo(typeid(lhs)) != typeid(rhs))
        return Loki::TypeInfo(typeid(lhs)) < typeid(rhs);

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


//--------------------------------------------------------------------------------------------------
void addFilterEntry(const Zstring& filtername, std::set<Zstring>& fileFilter, std::set<Zstring>& directoryFilter)
{
    Zstring filterFormatted = filtername;

#ifdef FFS_WIN
    //Windows does NOT distinguish between upper/lower-case
    MakeUpper(filterFormatted);
#elif defined FFS_LINUX
    //Linux DOES distinguish between upper/lower-case: nothing to do here
#endif

    const Zstring sepAsterisk     = Zstring(FILE_NAME_SEPARATOR) + Zchar('*');
    const Zstring sepQuestionMark = Zstring(FILE_NAME_SEPARATOR) + Zchar('?');
    const Zstring asteriskSep     = Zstring(Zstr('*')) + FILE_NAME_SEPARATOR;
    const Zstring questionMarkSep = Zstring(Zstr('?')) + FILE_NAME_SEPARATOR;

    //--------------------------------------------------------------------------------------------------
    //add some syntactic sugar: handle beginning of filtername
    if (filterFormatted.StartsWith(FILE_NAME_SEPARATOR))
    {
        //remove leading separators (keep BEFORE test for Zstring::empty()!)
        filterFormatted = filterFormatted.AfterFirst(FILE_NAME_SEPARATOR);
    }
    else if (filterFormatted.StartsWith(asteriskSep) ||   // *\abc
             filterFormatted.StartsWith(questionMarkSep)) // ?\abc
    {
        addFilterEntry(Zstring(filterFormatted.c_str() + 1), fileFilter, directoryFilter); //prevent further recursion by prefix
    }

    //--------------------------------------------------------------------------------------------------
    //even more syntactic sugar: handle end of filtername
    if (filterFormatted.EndsWith(FILE_NAME_SEPARATOR))
    {
        const Zstring candidate = filterFormatted.BeforeLast(FILE_NAME_SEPARATOR);
        if (!candidate.empty())
            directoryFilter.insert(candidate); //only relevant for directory filtering
    }
    else if (filterFormatted.EndsWith(sepAsterisk) ||   // abc\*
             filterFormatted.EndsWith(sepQuestionMark)) // abc\?
    {
        fileFilter.insert(     filterFormatted);
        directoryFilter.insert(filterFormatted);

        const Zstring candidate = filterFormatted.BeforeLast(FILE_NAME_SEPARATOR);
        if (!candidate.empty())
            directoryFilter.insert(candidate); //only relevant for directory filtering
    }
    else if (!filterFormatted.empty())
    {
        fileFilter.     insert(filterFormatted);
        directoryFilter.insert(filterFormatted);
    }
}


namespace
{
template <class T>
inline
const T* cStringFind(const T* str1, T ch) //strchr()
{
    while (*str1 != ch) //ch is allowed to be 0 by contract! must return end of string in this case
    {
        if (*str1 == 0)
            return NULL;
        ++str1;
    }
    return str1;
}

bool matchesMask(const Zchar* str, const Zchar* mask)
{
    for (Zchar ch; (ch = *mask) != 0; ++mask, ++str)
    {
        switch (ch)
        {
            case Zchar('?'):
                if (*str == 0)
                    return false;
                break;

            case Zchar('*'):
                //advance to next non-*/? char
                do
                {
                    ++mask;
                    ch = *mask;
                }
                while (ch == Zchar('*') || ch == Zchar('?'));
                //if mask ends with '*':
                if (ch == 0)
                    return true;

                ++mask;
                while ((str = cStringFind(str, ch)) != NULL)
                {
                    ++str;
                    if (matchesMask(str, mask))
                        return true;
                }
                return false;

            default:
                if (*str != ch)
                    return false;
        }
    }
    return *str == 0;
}

//returns true if string matches at least the beginning of mask
inline
bool matchesMaskBegin(const Zchar* str, const Zchar* mask)
{
    for (Zchar ch; (ch = *mask) != 0; ++mask, ++str)
    {
        if (*str == 0)
            return true;

        switch (ch)
        {
            case Zchar('?'):
                break;

            case Zchar('*'):
                return true;

            default:
                if (*str != ch)
                    return false;
        }
    }
    return *str == 0;
}
}


class MatchFound : public std::unary_function<Zstring, bool>
{
public:
    MatchFound(const Zstring& name) : name_(name) {}

    bool operator()(const Zstring& mask) const
    {
        return matchesMask(name_.c_str(), mask.c_str());
    }
private:
    const Zstring& name_;
};


inline
bool matchesFilter(const Zstring& nameFormatted, const std::set<Zstring>& filter)
{
    return std::find_if(filter.begin(), filter.end(), MatchFound(nameFormatted)) != filter.end();
}


inline
bool matchesFilterBegin(const Zstring& nameFormatted, const std::set<Zstring>& filter)
{
    for (std::set<Zstring>::const_iterator i = filter.begin(); i != filter.end(); ++i)
        if (matchesMaskBegin(nameFormatted.c_str(), i->c_str()))
            return true;
    return false;

    //    return std::find_if(filter.begin(), filter.end(),
    //                      boost::bind(matchesMaskBegin, nameFormatted.c_str(), _1)) != filter.end();
}


std::vector<Zstring> compoundStringToFilter(const Zstring& filterString)
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
    //no need for regular expressions! In tests wxRegex was by factor of 10 slower than wxString::Matches()!!

    //load filter into vectors of strings
    //delimiters may be ';' or '\n'
    const std::vector<Zstring>& includeList = compoundStringToFilter(includeFilter);
    const std::vector<Zstring>& excludeList = compoundStringToFilter(excludeFilter);

    //setup include/exclude filters for files and directories
    std::for_each(includeList.begin(), includeList.end(), [&](const Zstring& entry) { addFilterEntry(entry, filterFileIn, filterFolderIn); });
    std::for_each(excludeList.begin(), excludeList.end(), [&](const Zstring& entry) { addFilterEntry(entry, filterFileEx, filterFolderEx); });
}


bool NameFilter::passFileFilter(const Zstring& relFilename) const
{
#ifdef FFS_WIN //Windows does NOT distinguish between upper/lower-case
    Zstring nameFormatted = relFilename;
    MakeUpper(nameFormatted);
#elif defined FFS_LINUX //Linux DOES distinguish between upper/lower-case
    const Zstring& nameFormatted = relFilename; //nothing to do here
#endif

    return  matchesFilter(nameFormatted, filterFileIn) && //process include filters
            !matchesFilter(nameFormatted, filterFileEx);  //process exclude filters
}


bool NameFilter::passDirFilter(const Zstring& relDirname, bool* subObjMightMatch) const
{
    assert(subObjMightMatch == NULL || *subObjMightMatch == true); //check correct usage

#ifdef FFS_WIN //Windows does NOT distinguish between upper/lower-case
    Zstring nameFormatted = relDirname;
    MakeUpper(nameFormatted);
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
