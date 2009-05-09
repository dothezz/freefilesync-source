#include "filter.h"
#include "zstring.h"
#include <wx/string.h>
#include <set>
#include <vector>
#include "resources.h"


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


inline
void mergeVectors(std::vector<Zstring>& changing, const std::vector<Zstring>& input)
{
    for (std::vector<Zstring>::const_iterator i = input.begin(); i != input.end(); ++i)
        changing.push_back(*i);
}


inline
void formatFilterString(Zstring& filter)
{
#ifdef FFS_WIN
    //Windows does NOT distinguish between upper/lower-case
    filter.MakeLower();
#elif defined FFS_LINUX
    //Linux DOES distinguish between upper/lower-case
//nothing to do here
#else
    adapt;
#endif
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
        mergeVectors(filterList, newEntries);
    }

    return filterList;
}


inline
void addFilterEntry(const Zstring& filtername, std::set<Zstring>& fileFilter, std::set<Zstring>& directoryFilter)
{
    //Test if filtername ends with GlobalResources::FILE_NAME_SEPARATOR, ignoring '*' and '?'.
    //If so, treat as filter for directory and add to directoryFilter.
    if (!filtername.empty())
    {
        const DefaultChar* filter = filtername.c_str();
        int i = filtername.length() - 1;
        while (filter[i] == DefaultChar('*') || filter[i] == DefaultChar('?'))
        {
            --i;

            if (i == -1)
                break;
        }

        if (i >= 0 && filter[i] == GlobalResources::FILE_NAME_SEPARATOR) //last FILE_NAME_SEPARATOR found
        {
            if (i != int(filtername.length()) - 1)  // "name\*"
            {
                fileFilter.insert(filtername);
                directoryFilter.insert(filtername);
            }
            //else: "name\" -> not inserted directly

            if (i > 0) // "name\*" or "name\": add "name" to directory filter
                directoryFilter.insert(Zstring(filtername.c_str(), i));
        }
        else
        {
            fileFilter.insert(filtername);
            directoryFilter.insert(filtername);
        }
    }
}


inline
bool matchesFilter(const Zstring& name, const std::set<Zstring>& filter)
{
    for (std::set<Zstring>::iterator j = filter.begin(); j != filter.end(); ++j)
        if (name.Matches(*j))
            return true;

    return false;
}


void FreeFileSync::filterGridData(FolderComparison& folderCmp, const wxString& includeFilter, const wxString& excludeFilter)
{
    //no need for regular expressions! In tests wxRegex was by factor of 10 slower than wxString::Matches()!!

    //load filter into vectors of strings
    //delimiters may be ';' or '\n'
    std::vector<Zstring> includeList = compoundStringToFilter(includeFilter.c_str());
    std::vector<Zstring> excludeList = compoundStringToFilter(excludeFilter.c_str());

//##############################################################
    //setup include/exclude filters for files and directories
    std::set<Zstring> filterFileIn;
    std::set<Zstring> filterFolderIn;
    for (std::vector<Zstring>::iterator i = includeList.begin(); i != includeList.end(); ++i)
    {
        formatFilterString(*i); //format entry
        addFilterEntry(*i, filterFileIn, filterFolderIn);
    }

    std::set<Zstring> filterFileEx;
    std::set<Zstring> filterFolderEx;
    for (std::vector<Zstring>::iterator i = excludeList.begin(); i != excludeList.end(); ++i)
    {
        formatFilterString(*i); //format entry
        addFilterEntry(*i, filterFileEx, filterFolderEx);
    }

//##############################################################

    //execute filtering...
    for (FolderComparison::iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        FileComparison& fileCmp = j->fileCmp;

        for (FileComparison::iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
        {
            Zstring filenameLeft  = i->fileDescrLeft.fullName;
            Zstring filenameRight = i->fileDescrRight.fullName;

            formatFilterString(filenameLeft);
            formatFilterString(filenameRight);


            //left hand side
            if (i->fileDescrLeft.objType == FileDescrLine::TYPE_FILE)
            {
                if (    !matchesFilter(filenameLeft, filterFileIn) || //process include filters
                        matchesFilter(filenameLeft, filterFileEx))   //process exclude filters
                {
                    i->selectedForSynchronization = false;
                    continue;
                }
            }
            else if (i->fileDescrLeft.objType == FileDescrLine::TYPE_DIRECTORY)
            {
                if (    !matchesFilter(filenameLeft, filterFolderIn) || //process include filters
                        matchesFilter(filenameLeft, filterFolderEx))   //process exclude filters
                {
                    i->selectedForSynchronization = false;
                    continue;
                }
            }

            //right hand side
            if (i->fileDescrRight.objType == FileDescrLine::TYPE_FILE)
            {
                if (    !matchesFilter(filenameRight, filterFileIn) || //process include filters
                        matchesFilter(filenameRight, filterFileEx))   //process exclude filters
                {
                    i->selectedForSynchronization = false;
                    continue;
                }
            }
            else if (i->fileDescrRight.objType == FileDescrLine::TYPE_DIRECTORY)
            {
                if (    !matchesFilter(filenameRight, filterFolderIn) || //process include filters
                        matchesFilter(filenameRight, filterFolderEx))   //process exclude filters
                {
                    i->selectedForSynchronization = false;
                    continue;
                }
            }

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


void FreeFileSync::includeAllRowsOnGrid(FolderComparison& folderCmp)
{
    //remove all filters on currentGridData
    inOrExcludeAllRows<true>(folderCmp);
}


void FreeFileSync::excludeAllRowsOnGrid(FolderComparison& folderCmp)
{
    //exclude all rows on currentGridData
    inOrExcludeAllRows<false>(folderCmp);
}
