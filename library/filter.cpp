// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "filter.h"
#include "../shared/zstring.h"
#include <wx/string.h>
#include <set>
#include <stdexcept>
#include <vector>
#include "../shared/system_constants.h"
#include "../structures.h"
#include <boost/bind.hpp>
#include "../shared/loki/LokiTypeInfo.h"
#include "../shared/serialize.h"

using ffs3::BaseFilter;
using ffs3::NameFilter;


//--------------------------------------------------------------------------------------------------
bool BaseFilter::operator==(const BaseFilter& other) const
{
    return !(*this < other) && !(other < *this);
}


bool BaseFilter::operator!=(const BaseFilter& other) const
{
    return !(*this == other);
}


bool BaseFilter::operator<(const BaseFilter& other) const
{
    if (Loki::TypeInfo(typeid(*this)) != typeid(other))
        return Loki::TypeInfo(typeid(*this)) < typeid(other);

    //this and other are same type:
    return cmpLessSameType(other);
}


void BaseFilter::saveFilter(wxOutputStream& stream) const //serialize derived object
{
    //save type information
    util::writeString(stream, uniqueClassIdentifier());

    //save actual object
    save(stream);
}


BaseFilter::FilterRef BaseFilter::loadFilter(wxInputStream& stream)
{
    //read type information
    const Zstring uniqueClassId = util::readString(stream);

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
inline
void addFilterEntry(const Zstring& filtername, std::set<Zstring>& fileFilter, std::set<Zstring>& directoryFilter)
{
    Zstring filterFormatted = filtername;

#ifdef FFS_WIN
    //Windows does NOT distinguish between upper/lower-case
    MakeUpper(filterFormatted);
#elif defined FFS_LINUX
    //Linux DOES distinguish between upper/lower-case: nothing to do here
#endif

    static const Zstring sepAsterisk     = Zstring(common::FILE_NAME_SEPARATOR) + Zchar('*');
    static const Zstring sepQuestionMark = Zstring(common::FILE_NAME_SEPARATOR) + Zchar('?');
    static const Zstring asteriskSep     = Zstring(Zchar('*')) + common::FILE_NAME_SEPARATOR;
    static const Zstring questionMarkSep = Zstring(Zchar('?')) + common::FILE_NAME_SEPARATOR;

    //--------------------------------------------------------------------------------------------------
    //add some syntactic sugar: handle beginning of filtername
    if (filterFormatted.StartsWith(common::FILE_NAME_SEPARATOR))
    {
        //remove leading separators (keep BEFORE test for Zstring::empty()!)
        filterFormatted = filterFormatted.AfterFirst(common::FILE_NAME_SEPARATOR);
    }
    else if (filterFormatted.StartsWith(asteriskSep) ||   // *\abc
             filterFormatted.StartsWith(questionMarkSep)) // ?\abc
    {
        addFilterEntry(Zstring(filterFormatted.c_str() + 1), fileFilter, directoryFilter); //prevent further recursion by prefix
    }

    //--------------------------------------------------------------------------------------------------
    //even more syntactic sugar: handle end of filtername
    if (filterFormatted.EndsWith(common::FILE_NAME_SEPARATOR))
    {
        const Zstring candidate = filterFormatted.BeforeLast(common::FILE_NAME_SEPARATOR);
        if (!candidate.empty())
            directoryFilter.insert(candidate); //only relevant for directory filtering
    }
    else if (filterFormatted.EndsWith(sepAsterisk) ||   // abc\*
             filterFormatted.EndsWith(sepQuestionMark)) // abc\?
    {
        fileFilter.insert(     filterFormatted);
        directoryFilter.insert(filterFormatted);

        const Zstring candidate = filterFormatted.BeforeLast(common::FILE_NAME_SEPARATOR);
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


bool matchesMask(const Zchar* string, const Zchar* mask)
{
    for (Zchar ch; (ch = *mask) != 0; ++mask, ++string)
    {
        switch (ch)
        {
            case Zchar('?'):
                if (*string == 0)
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
                //if match ends with '*':
                if (ch == 0)
                    return true;

                ++mask;
                while ((string = cStringFind(string, ch)) != NULL)
                {
                    ++string;
                    if (matchesMask(string, mask))
                        return true;
                }
                return false;

            default:
                if (*string != ch)
                    return false;
        }
    }
    return *string == 0;
}

//returns true if string matches at least the beginning of mask
inline
bool matchesMaskBegin(const Zchar* string, const Zchar* mask)
{
    for (Zchar ch; (ch = *mask) != 0; ++mask, ++string)
    {
        if (*string == 0)
            return true;

        switch (ch)
        {
            case Zchar('?'):
                break;

            case Zchar('*'):
                return true;

            default:
                if (*string != ch)
                    return false;
        }
    }
    return *string == 0;
}
}


class MatchFound : public std::unary_function<Zstring, bool>
{
public:
    MatchFound(const Zstring& name) : name_(name) {}

    bool operator()(const Zstring& mask) const
    {
        return matchesMask(name_, mask);
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
    return std::find_if(filter.begin(), filter.end(),
                        boost::bind(matchesMaskBegin, nameFormatted.c_str(), _1)) != filter.end();
}


std::vector<Zstring> compoundStringToFilter(const Zstring& filterString)
{
    //delimiters may be ';' or '\n'
    std::vector<Zstring> output;

    const std::vector<Zstring> filterPreProcessing = filterString.Split(Zchar(';'));
    for (std::vector<Zstring>::const_iterator i = filterPreProcessing.begin(); i != filterPreProcessing.end(); ++i)
    {
        const std::vector<Zstring> newEntries = i->Split(Zchar('\n'));
        output.insert(output.end(), newEntries.begin(), newEntries.end());
    }

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
    const std::vector<Zstring> includeList = compoundStringToFilter(includeFilter);
    const std::vector<Zstring> excludeList = compoundStringToFilter(excludeFilter);

    //setup include/exclude filters for files and directories
    std::for_each(includeList.begin(), includeList.end(), boost::bind(addFilterEntry, _1, boost::ref(filterFileIn), boost::ref(filterFolderIn)));
    std::for_each(excludeList.begin(), excludeList.end(), boost::bind(addFilterEntry, _1, boost::ref(filterFileEx), boost::ref(filterFolderEx)));
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
            const Zstring& subNameBegin = nameFormatted + common::FILE_NAME_SEPARATOR; //const-ref optimization

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


bool NameFilter::cmpLessSameType(const BaseFilter& other) const
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
    util::writeString(stream, includeFilterTmp);
    util::writeString(stream, excludeFilterTmp);
}


BaseFilter::FilterRef NameFilter::load(wxInputStream& stream) //"constructor"
{
    const Zstring include = util::readString(stream);
    const Zstring exclude = util::readString(stream);

    return FilterRef(new NameFilter(include, exclude));
}
