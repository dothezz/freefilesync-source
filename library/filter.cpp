#include "filter.h"
#include "zstring.h"
#include <wx/string.h>
#include <set>
#include <vector>
#include "resources.h"
#include "globalFunctions.h"

using FreeFileSync::FilterProcess;


void compoundStringToTable(const Zstring& compoundInput, const DefaultChar* delimiter, std::vector<Zstring>& output)
{
    output.clear();
    Zstring input(compoundInput);

    //make sure input ends with delimiter - no problem with empty strings here
    if (!input.EndsWith(delimiter))
        input += delimiter;

    unsigned int indexStart = 0;
    unsigned int indexEnd   = 0;
    while ((indexEnd = input.find(delimiter, indexStart)) != Zstring::npos)
    {
        if (indexStart != indexEnd) //do not add empty strings
        {
            Zstring newEntry = input.substr(indexStart, indexEnd - indexStart);

            newEntry.Trim(true);  //remove whitespace characters from right
            newEntry.Trim(false); //remove whitespace characters from left

            if (!newEntry.empty())
                output.push_back(newEntry);
        }
        indexStart = indexEnd + 1;
    }
}


std::vector<Zstring> compoundStringToFilter(const Zstring& filterString)
{
    //delimiters may be ';' or '\n'
    std::vector<Zstring> filterList;
    std::vector<Zstring> filterPreProcessing;
    compoundStringToTable(filterString, wxT(";"), filterPreProcessing);

    for (std::vector<Zstring>::const_iterator i = filterPreProcessing.begin(); i != filterPreProcessing.end(); ++i)
    {
        std::vector<Zstring> newEntries;
        compoundStringToTable(*i, wxT("\n"), newEntries);

        globalFunctions::mergeVectors(newEntries, filterList);
    }

    return filterList;
}


inline
void addFilterEntry(const Zstring& filtername, std::set<Zstring>& fileFilter, std::set<Zstring>& directoryFilter)
{
    Zstring filterFormatted = filtername;

#ifdef FFS_WIN
    //Windows does NOT distinguish between upper/lower-case
    filterFormatted.MakeLower();
#elif defined FFS_LINUX
    //Linux DOES distinguish between upper/lower-case
//nothing to do here
#endif

    //remove leading separators (keep BEFORE test for Zstring::empty()!)
    if (filterFormatted.length() > 0 && *filterFormatted.c_str() == FreeFileSync::FILE_NAME_SEPARATOR)
        filterFormatted = Zstring(filterFormatted.c_str() + 1);

    if (!filterFormatted.empty())
    {
        //Test if filterFormatted ends with FreeFileSync::FILE_NAME_SEPARATOR, ignoring '*' and '?'.
        //If so, treat as filter for directory and add to directoryFilter.
        const DefaultChar* filter = filterFormatted.c_str();
        int i = filterFormatted.length() - 1;
        while (filter[i] == DefaultChar('*') || filter[i] == DefaultChar('?'))
        {
            --i;

            if (i == -1)
                break;
        }

        if (i >= 0 && filter[i] == FreeFileSync::FILE_NAME_SEPARATOR) //last FILE_NAME_SEPARATOR found
        {
            if (i != int(filterFormatted.length()) - 1)  // "name\*"
            {
                fileFilter.insert(filterFormatted);
                directoryFilter.insert(filterFormatted);
            }
            //else // "name\"

            if (i > 0) // "name\*" or "name\": add "name" to directory filter
                directoryFilter.insert(Zstring(filterFormatted.c_str(), i));
        }
        else
        {
            fileFilter.insert(filterFormatted);
            directoryFilter.insert(filterFormatted);
        }
    }
}


inline
bool matchesFilter(const DefaultChar* name, const std::set<Zstring>& filter)
{
#ifdef FFS_WIN //Windows does NOT distinguish between upper/lower-case
    Zstring nameFormatted = name;
    nameFormatted.MakeLower();
#elif defined FFS_LINUX //Linux DOES distinguish between upper/lower-case
    const DefaultChar* const nameFormatted = name; //nothing to do here
#endif

    for (std::set<Zstring>::const_iterator j = filter.begin(); j != filter.end(); ++j)
        if (Zstring::Matches(nameFormatted, *j))
            return true;

    return false;
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
    for (std::vector<Zstring>::const_iterator i = includeList.begin(); i != includeList.end(); ++i)
        addFilterEntry(*i, filterFileIn, filterFolderIn);

    for (std::vector<Zstring>::const_iterator i = excludeList.begin(); i != excludeList.end(); ++i)
        addFilterEntry(*i, filterFileEx, filterFolderEx);
}


bool FilterProcess::matchesFileFilterIncl(const DefaultChar* relFilename) const
{
    return matchesFilter(relFilename, filterFileIn); //process include filters
}


bool FilterProcess::matchesFileFilterExcl(const DefaultChar* relFilename) const
{
    return matchesFilter(relFilename, filterFileEx); //process exclude filters
}


bool FilterProcess::matchesDirFilterIncl(const DefaultChar* relDirname) const
{
    return matchesFilter(relDirname, filterFolderIn); //process include filters
}


bool FilterProcess::matchesDirFilterExcl(const DefaultChar* relDirname) const
{
    return matchesFilter(relDirname, filterFolderEx); //process exclude filters
}


void FilterProcess::filterGridData(FreeFileSync::FolderComparison& folderCmp) const
{
    //execute filtering...
    for (FolderComparison::iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        FileComparison& fileCmp = j->fileCmp;

        for (FileComparison::iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
        {

            const FileDescrLine& fileDescr = i->fileDescrLeft.objType != FileDescrLine::TYPE_NOTHING ?
                                             i->fileDescrLeft :
                                             i->fileDescrRight;

            if (fileDescr.objType == FileDescrLine::TYPE_FILE)
            {
                if (    !matchesFileFilterIncl(fileDescr.relativeName.c_str()) ||
                        matchesFileFilterExcl(fileDescr.relativeName.c_str()))
                {
                    i->selectedForSynchronization = false;
                    continue;
                }
            }
            else if (fileDescr.objType == FileDescrLine::TYPE_DIRECTORY)
            {
                if (    !matchesDirFilterIncl(fileDescr.relativeName.c_str()) ||
                        matchesDirFilterExcl(fileDescr.relativeName.c_str()))
                {
                    i->selectedForSynchronization = false;
                    continue;
                }
            }
            else
                assert(false);

            i->selectedForSynchronization = true;
        }
    }
}


template <bool includeRows>
inline
void inOrExcludeAllRows(FreeFileSync::FolderComparison& folderCmp)
{
    //remove all filters on folderCmp
    for (FreeFileSync::FolderComparison::iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        FreeFileSync::FileComparison& fileCmp = j->fileCmp;
        for (FreeFileSync::FileComparison::iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
            i->selectedForSynchronization = includeRows;
    }
}


void FilterProcess::includeAllRowsOnGrid(FreeFileSync::FolderComparison& folderCmp)
{
    //remove all filters on currentGridData
    inOrExcludeAllRows<true>(folderCmp);
}


void FilterProcess::excludeAllRowsOnGrid(FreeFileSync::FolderComparison& folderCmp)
{
    //exclude all rows on currentGridData
    inOrExcludeAllRows<false>(folderCmp);
}
