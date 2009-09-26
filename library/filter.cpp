#include "filter.h"
#include "../shared/zstring.h"
#include <wx/string.h>
#include <set>
#include <vector>
#include "../shared/systemConstants.h"
#include "../structures.h"
#include <boost/bind.hpp>

using FreeFileSync::FilterProcess;


inline
void addFilterEntry(const Zstring& filtername, std::set<Zstring>& fileFilter, std::set<Zstring>& directoryFilter)
{
    Zstring filterFormatted = filtername;

#ifdef FFS_WIN
    //Windows does NOT distinguish between upper/lower-case
    filterFormatted.MakeLower();
#elif defined FFS_LINUX
    //Linux DOES distinguish between upper/lower-case: nothing to do here
#endif

    static const Zstring sepAsterisk     = Zstring() + globalFunctions::FILE_NAME_SEPARATOR + wxT('*');
    static const Zstring sepQuestionMark = Zstring() + globalFunctions::FILE_NAME_SEPARATOR + wxT('?');
    static const Zstring asteriskSep     = Zstring(wxT("*")) + globalFunctions::FILE_NAME_SEPARATOR;
    static const Zstring questionMarkSep = Zstring(wxT("?")) + globalFunctions::FILE_NAME_SEPARATOR;

    //add some syntactic sugar: handle beginning of filtername
    if (filterFormatted.StartsWith(globalFunctions::FILE_NAME_SEPARATOR))
    {  //remove leading separators (keep BEFORE test for Zstring::empty()!)
        filterFormatted = filterFormatted.AfterFirst(globalFunctions::FILE_NAME_SEPARATOR);
    }
    else if (filterFormatted.StartsWith(asteriskSep) ||   // *\abc
             filterFormatted.StartsWith(questionMarkSep)) // ?\abc
    {
        addFilterEntry(Zstring(filterFormatted.c_str() + 1), fileFilter, directoryFilter); //prevent further recursion by prefix
    }


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
    nameFormatted.MakeLower();
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
    nameFormatted.MakeLower();
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
//##############################################################


FilterProcess::FilterProcess(const wxString& includeFilter, const wxString& excludeFilter)
{
    //no need for regular expressions! In tests wxRegex was by factor of 10 slower than wxString::Matches()!!

    //load filter into vectors of strings
    //delimiters may be ';' or '\n'
    const std::vector<Zstring> includeList = compoundStringToFilter(includeFilter.c_str());
    const std::vector<Zstring> excludeList = compoundStringToFilter(excludeFilter.c_str());

    //setup include/exclude filters for files and directories
    std::for_each(includeList.begin(), includeList.end(), boost::bind(addFilterEntry, _1, boost::ref(filterFileIn), boost::ref(filterFolderIn)));
    std::for_each(excludeList.begin(), excludeList.end(), boost::bind(addFilterEntry, _1, boost::ref(filterFileEx), boost::ref(filterFolderEx)));
}


bool FilterProcess::passFileFilter(const DefaultChar* relFilename) const
{
    return  matchesFilter(relFilename, filterFileIn) && //process include filters
            !matchesFilter(relFilename, filterFileEx);  //process exclude filters
}


bool FilterProcess::passDirFilter(const DefaultChar* relDirname, bool* subObjMightMatch) const
{
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

    assert(subObjMightMatch == NULL || *subObjMightMatch == true);
    return true;
}


class FilterData
{
public:
    FilterData(const FilterProcess& filterProcIn) : filterProc(filterProcIn) {}

    void operator()(FreeFileSync::FileMapping& fileObj)
    {
        const Zstring relName = fileObj.isEmpty<FreeFileSync::LEFT_SIDE>() ?
                                fileObj.getRelativeName<FreeFileSync::RIGHT_SIDE>() :
                                fileObj.getRelativeName<FreeFileSync::LEFT_SIDE>();

        fileObj.selectedForSynchronization = filterProc.passFileFilter(relName);
    }

    void operator()(FreeFileSync::DirMapping& dirObj)
    {
        const Zstring relName = dirObj.isEmpty<FreeFileSync::LEFT_SIDE>() ?
                                dirObj.getRelativeName<FreeFileSync::RIGHT_SIDE>() :
                                dirObj.getRelativeName<FreeFileSync::LEFT_SIDE>();

        bool subObjMightMatch = true;
        dirObj.selectedForSynchronization = filterProc.passDirFilter(relName, &subObjMightMatch);

        if (subObjMightMatch) //use same logic like directory traversing here: evaluate filter in subdirs only if objects could match
            filterProc.filterAll(dirObj); //process sub-dirs/files
        else
            FilterProcess::setActiveStatus(false, dirObj); //exclude all files dirs in subfolders
    }

private:
    const FilterProcess& filterProc;
};


void FilterProcess::filterAll(FreeFileSync::HierarchyObject& baseDirectory) const
{
    //files
    std::for_each(baseDirectory.subFiles.begin(), baseDirectory.subFiles.end(), FilterData(*this));

    //directories
    std::for_each(baseDirectory.subDirs.begin(), baseDirectory.subDirs.end(), FilterData(*this));

    //recursion happens in FilterData
}


template <bool include>
struct SetSelected
{
    void operator()(FreeFileSync::FileSystemObject& fsObj) const
    {
        fsObj.selectedForSynchronization = include;
    }
};


template <bool include>
void inOrExcludeAllRows(FreeFileSync::HierarchyObject& hierObj)
{
    //directories
    std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), SetSelected<include>());
    //files
    std::for_each(hierObj.subFiles.begin(), hierObj.subFiles.end(), SetSelected<include>());
    //recurse into sub-dirs
    std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), inOrExcludeAllRows<include>);
}


void FilterProcess::setActiveStatus(bool newStatus, FreeFileSync::FolderComparison& folderCmp)
{
    if (newStatus)
        std::for_each(folderCmp.begin(), folderCmp.end(), inOrExcludeAllRows<true>); //include all rows
    else
        std::for_each(folderCmp.begin(), folderCmp.end(), inOrExcludeAllRows<false>); //exclude all rows
}


void FilterProcess::setActiveStatus(bool newStatus, FreeFileSync::FileSystemObject& fsObj)
{
    fsObj.selectedForSynchronization = newStatus;

    DirMapping* dirObj = dynamic_cast<DirMapping*>(&fsObj);
    if (dirObj) //process subdirectories also!
    {
        if (newStatus)
            inOrExcludeAllRows<true>(*dirObj);
        else
            inOrExcludeAllRows<false>(*dirObj);
    }
}


const FilterProcess& FilterProcess::nullFilter() //filter equivalent to include '*', exclude ''
{
    static FilterProcess output(wxT("*"), wxEmptyString);
    return output;
}


bool FilterProcess::operator==(const FilterProcess& other) const
{
    return filterFileIn   == other.filterFileIn   &&
           filterFolderIn == other.filterFolderIn &&
           filterFileEx   == other.filterFileEx   &&
           filterFolderEx == other.filterFolderEx;
}


bool FilterProcess::operator!=(const FilterProcess& other) const
{
    return !(*this == other);
}


bool FilterProcess::operator<(const FilterProcess& other) const
{
    if (filterFileIn != other.filterFileIn)
        return filterFileIn < other.filterFileIn;

    if (filterFolderIn != other.filterFolderIn)
        return filterFolderIn < other.filterFolderIn;

    if (filterFileEx != other.filterFileEx)
        return filterFileEx < other.filterFileEx;

    if (filterFolderEx != other.filterFolderEx)
        return filterFolderEx < other.filterFolderEx;

    return false; //vectors equal
}

