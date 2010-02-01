#include "filter.h"
#include "../shared/zstring.h"
#include <wx/string.h>
#include <set>
#include <stdexcept>
#include <vector>
#include "../shared/systemConstants.h"
#include "../structures.h"
#include <boost/bind.hpp>
#include "../shared/loki/LokiTypeInfo.h"
#include "../shared/serialize.h"

using FreeFileSync::BaseFilter;
using FreeFileSync::NameFilter;


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
    Utility::writeString(stream, uniqueClassIdentifier());

    //save actual object
    save(stream);
}


BaseFilter::FilterRef BaseFilter::loadFilter(wxInputStream& stream)
{
    //read type information
    const Zstring uniqueClassId = Utility::readString(stream);

    //read actual object
    if (uniqueClassId == DefaultStr("NullFilter"))
        return NullFilter::load(stream);
    else if (uniqueClassId == DefaultStr("NameFilter"))
        return NameFilter::load(stream);
    else if (uniqueClassId == DefaultStr("CombinedFilter"))
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
    filterFormatted.MakeUpper();
#elif defined FFS_LINUX
    //Linux DOES distinguish between upper/lower-case: nothing to do here
#endif

    static const Zstring sepAsterisk     = Zstring() + globalFunctions::FILE_NAME_SEPARATOR + DefaultChar('*');
    static const Zstring sepQuestionMark = Zstring() + globalFunctions::FILE_NAME_SEPARATOR + DefaultChar('?');
    static const Zstring asteriskSep     = Zstring(DefaultStr("*")) + globalFunctions::FILE_NAME_SEPARATOR;
    static const Zstring questionMarkSep = Zstring(DefaultStr("?")) + globalFunctions::FILE_NAME_SEPARATOR;

//--------------------------------------------------------------------------------------------------
    //add some syntactic sugar: handle beginning of filtername
    if (filterFormatted.StartsWith(globalFunctions::FILE_NAME_SEPARATOR))
    {
        //remove leading separators (keep BEFORE test for Zstring::empty()!)
        filterFormatted = filterFormatted.AfterFirst(globalFunctions::FILE_NAME_SEPARATOR);
    }
    else if (filterFormatted.StartsWith(asteriskSep) ||   // *\abc
             filterFormatted.StartsWith(questionMarkSep)) // ?\abc
    {
        addFilterEntry(Zstring(filterFormatted.c_str() + 1), fileFilter, directoryFilter); //prevent further recursion by prefix
    }

//--------------------------------------------------------------------------------------------------
    //even more syntactic sugar: handle end of filtername
    if (filterFormatted.EndsWith(globalFunctions::FILE_NAME_SEPARATOR))
    {
        const Zstring candidate = filterFormatted.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR);
        if (!candidate.empty())
            directoryFilter.insert(candidate); //only relevant for directory filtering
    }
    else if (filterFormatted.EndsWith(sepAsterisk) ||   // abc\*
             filterFormatted.EndsWith(sepQuestionMark)) // abc\?
    {
        fileFilter.insert(     filterFormatted);
        directoryFilter.insert(filterFormatted);

        const Zstring candidate = filterFormatted.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR);
        if (!candidate.empty())
            directoryFilter.insert(candidate); //only relevant for directory filtering
    }
    else if (!filterFormatted.empty())
    {
        fileFilter.     insert(filterFormatted);
        directoryFilter.insert(filterFormatted);
    }
}


class MatchFound : public std::unary_function<const DefaultChar*, bool>
{
public:
    MatchFound(const DefaultChar* name) : name_(name) {}

    bool operator()(const DefaultChar* mask) const
    {
        return Zstring::Matches(name_, mask);
    }
private:
    const DefaultChar* name_;
};


inline
bool matchesFilter(const DefaultChar* name, const std::set<Zstring>& filter)
{
#ifdef FFS_WIN //Windows does NOT distinguish between upper/lower-case
    Zstring nameFormatted = name;
    nameFormatted.MakeUpper();
#elif defined FFS_LINUX //Linux DOES distinguish between upper/lower-case
    const DefaultChar* const nameFormatted = name; //nothing to do here
#endif

    return std::find_if(filter.begin(), filter.end(), MatchFound(nameFormatted)) != filter.end();
}


//returns true if string matches at least the beginning of mask
inline
bool matchesMaskBegin(const DefaultChar* string, const DefaultChar* mask)
{
    for (DefaultChar ch; (ch = *mask) != 0; ++mask, ++string)
    {
        if (*string == 0)
            return true;

        switch (ch)
        {
        case DefaultChar('?'):
            break;

        case DefaultChar('*'):
            return true;

        default:
            if (*string != ch)
                return false;
        }
    }
    return *string == 0;
}


inline
bool matchesFilterBegin(const DefaultChar* name, const std::set<Zstring>& filter)
{
#ifdef FFS_WIN //Windows does NOT distinguish between upper/lower-case
    Zstring nameFormatted = name;
    nameFormatted.MakeUpper();
#elif defined FFS_LINUX //Linux DOES distinguish between upper/lower-case
    const DefaultChar* const nameFormatted = name; //nothing to do here
#endif

    return std::find_if(filter.begin(), filter.end(),
                        boost::bind(matchesMaskBegin, nameFormatted, _1)) != filter.end();
}


std::vector<Zstring> compoundStringToFilter(const Zstring& filterString)
{
    //delimiters may be ';' or '\n'
    std::vector<Zstring> output;

    const std::vector<Zstring> filterPreProcessing = filterString.Tokenize(wxT(';'));
    for (std::vector<Zstring>::const_iterator i = filterPreProcessing.begin(); i != filterPreProcessing.end(); ++i)
    {
        const std::vector<Zstring> newEntries = i->Tokenize(wxT('\n'));
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


bool NameFilter::passFileFilter(const DefaultChar* relFilename) const
{
    return  matchesFilter(relFilename, filterFileIn) && //process include filters
            !matchesFilter(relFilename, filterFileEx);  //process exclude filters
}


bool NameFilter::passDirFilter(const DefaultChar* relDirname, bool* subObjMightMatch) const
{
    assert(subObjMightMatch == NULL || *subObjMightMatch == true); //check correct usage

    if (matchesFilter(relDirname, filterFolderEx)) //process exclude filters
    {
        if (subObjMightMatch)
            *subObjMightMatch = false; //exclude subfolders/subfiles as well
        return false;
    }

    if (!matchesFilter(relDirname, filterFolderIn)) //process include filters
    {
        if (subObjMightMatch)
        {
            Zstring subNameBegin(relDirname);
            subNameBegin += globalFunctions::FILE_NAME_SEPARATOR;

            *subObjMightMatch = matchesFilterBegin(subNameBegin, filterFileIn) || //might match a file in subdirectory
                                matchesFilterBegin(subNameBegin, filterFolderIn); //or another subdirectory
        }
        return false;
    }

    return true;
}


bool NameFilter::isNull() const
{
    static NameFilter output(DefaultStr("*"), Zstring());
    return *this == output;
}


bool NameFilter::cmpLessSameType(const BaseFilter& other) const
{
    //typeid(*this) == typeid(other) in this context!
    assert(typeid(*this) == typeid(other));
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
    return DefaultStr("NameFilter");
}


void NameFilter::save(wxOutputStream& stream) const
{
    Utility::writeString(stream, includeFilterTmp);
    Utility::writeString(stream, excludeFilterTmp);
}


BaseFilter::FilterRef NameFilter::load(wxInputStream& stream) //"constructor"
{
    const Zstring include = Utility::readString(stream);
    const Zstring exclude = Utility::readString(stream);

    return FilterRef(new NameFilter(include, exclude));
}
