#ifndef SORTING_H_INCLUDED
#define SORTING_H_INCLUDED

#include "../structures.h"
#include "../library/resources.h"
#include "../library/globalFunctions.h"


namespace FreeFileSync
{
    enum SideToSort
    {
        SORT_ON_LEFT,
        SORT_ON_RIGHT,
    };

    enum SortDirection
    {
        ASCENDING,
        DESCENDING,
    };


    template <SortDirection sortAscending>
    inline
    bool stringSmallerThan(const wxChar* stringA, const wxChar* stringB)
    {
#ifdef FFS_WIN //case-insensitive comparison!
        return sortAscending == ASCENDING ?
               FreeFileSync::compareStringsWin32(stringA, stringB) < 0 : //way faster than wxString::CmpNoCase() in windows build!!!
               FreeFileSync::compareStringsWin32(stringA, stringB) > 0;
#else
        while (*stringA == *stringB)
        {
            if (*stringA == wxChar(0)) //strings are equal
                return false;

            ++stringA;
            ++stringB;
        }
        return sortAscending == ASCENDING ? *stringA < *stringB : *stringA > *stringB; //wxChar(0) is handled correctly
#endif
    }


    inline
    int compareString(const wxChar* stringA, const wxChar* stringB, const int lengthA, const int lengthB)
    {
#ifdef FFS_WIN //case-insensitive comparison!
        return FreeFileSync::compareStringsWin32(stringA, stringB, lengthA, lengthB); //way faster than wxString::CmpNoCase() in the windows build!!!
#else
        for (int i = 0; i < std::min(lengthA, lengthB); ++i)
        {
            if (stringA[i] != stringB[i])
                return stringA[i] - stringB[i];
        }
        return lengthA - lengthB;
#endif
    }


    template <SortDirection sortAscending, SideToSort side>
    inline
    bool sortByFileName(const FileCompareLine& a, const FileCompareLine& b)
    {
        const FileDescrLine* const descrLineA = side == SORT_ON_LEFT ? &a.fileDescrLeft : &a.fileDescrRight;
        const FileDescrLine* const descrLineB = side == SORT_ON_LEFT ? &b.fileDescrLeft : &b.fileDescrRight;

        //presort types: first files, then directories then empty rows
        if (descrLineA->objType == FileDescrLine::TYPE_NOTHING)
            return false;  //empty rows always last
        else if (descrLineB->objType == FileDescrLine::TYPE_NOTHING)
            return true;  //empty rows always last


        if (descrLineA->objType == FileDescrLine::TYPE_DIRECTORY) //sort directories by relative name
        {
            if (descrLineB->objType == FileDescrLine::TYPE_DIRECTORY)
                return stringSmallerThan<sortAscending>(descrLineA->relativeName.c_str(), descrLineB->relativeName.c_str());
            else
                return false;
        }
        else
        {
            if (descrLineB->objType == FileDescrLine::TYPE_DIRECTORY)
                return true;
            else
            {
                const wxChar* stringA = descrLineA->relativeName.c_str();
                const wxChar* stringB = descrLineB->relativeName.c_str();

                size_t pos = descrLineA->relativeName.findFromEnd(FreeFileSync::FILE_NAME_SEPARATOR); //start search beginning from end
                if (pos != std::string::npos)
                    stringA += pos + 1;

                pos = descrLineB->relativeName.findFromEnd(FreeFileSync::FILE_NAME_SEPARATOR); //start search beginning from end
                if (pos != std::string::npos)
                    stringB += pos + 1;

                return stringSmallerThan<sortAscending>(stringA, stringB);
            }
        }
    }


    template <SortDirection sortAscending, SideToSort side>
    bool sortByRelativeName(const FileCompareLine& a, const FileCompareLine& b)
    {
        const FileDescrLine* const descrLineA = side == SORT_ON_LEFT ? &a.fileDescrLeft : &a.fileDescrRight;
        const FileDescrLine* const descrLineB = side == SORT_ON_LEFT ? &b.fileDescrLeft : &b.fileDescrRight;

        //extract relative name and filename
        const wxChar* const relStringA  = descrLineA->relativeName.c_str(); //mustn't be NULL for CompareString() API to work correctly
        const wxChar* fileStringA = relStringA;
        int relLengthA  = 0;
        int fileLengthA = 0;

        if (descrLineA->objType == FileDescrLine::TYPE_DIRECTORY)
            relLengthA = descrLineA->relativeName.length();
        else if (descrLineA->objType == FileDescrLine::TYPE_FILE)
        {
            relLengthA = descrLineA->relativeName.findFromEnd(FreeFileSync::FILE_NAME_SEPARATOR); //start search beginning from end
            if (relLengthA == wxNOT_FOUND)
            {
                relLengthA  = 0;
                fileLengthA = descrLineA->relativeName.length();
            }
            else
            {
                fileStringA += relLengthA + 1;
                fileLengthA = descrLineA->relativeName.length() - (relLengthA + 1);
            }
        }
        else
            return false; //empty rows should be on end of list


        const wxChar* const relStringB = descrLineB->relativeName.c_str(); //mustn't be NULL for CompareString() API to work correctly
        const wxChar* fileStringB = relStringB;
        int relLengthB  = 0;
        int fileLengthB = 0;

        if (descrLineB->objType == FileDescrLine::TYPE_DIRECTORY)
            relLengthB = descrLineB->relativeName.length();
        else if (descrLineB->objType == FileDescrLine::TYPE_FILE)
        {
            relLengthB = descrLineB->relativeName.findFromEnd(FreeFileSync::FILE_NAME_SEPARATOR); //start search beginning from end
            if (relLengthB == wxNOT_FOUND)
            {
                relLengthB  = 0;
                fileLengthB = descrLineB->relativeName.length();
            }
            else
            {
                fileStringB += relLengthB + 1;
                fileLengthB = descrLineB->relativeName.length() - (relLengthB + 1);
            }
        }
        else
            return true; //empty rows should be on end of list

        //compare relative names without filenames first
        const int rv = compareString(relStringA, relStringB, relLengthA, relLengthB);
        if (rv != 0)
            return sortAscending == ASCENDING ? rv < 0 : rv > 0;
        else //compare the filenames
        {
            if (descrLineB->objType == FileDescrLine::TYPE_DIRECTORY)  //directories shall appear before files
                return false;
            else if (descrLineA->objType == FileDescrLine::TYPE_DIRECTORY)
                return true;

            return sortAscending == ASCENDING ?
                   compareString(fileStringA, fileStringB, fileLengthA, fileLengthB) < 0 :
                   compareString(fileStringA, fileStringB, fileLengthA, fileLengthB) > 0;
        }
    }


    template <SortDirection sortAscending, SideToSort side>
    inline
    bool sortByFullName(const FileCompareLine& a, const FileCompareLine& b)
    {
        const FileDescrLine* const descrLineA = side == SORT_ON_LEFT ? &a.fileDescrLeft : &a.fileDescrRight;
        const FileDescrLine* const descrLineB = side == SORT_ON_LEFT ? &b.fileDescrLeft : &b.fileDescrRight;

        //presort types: first files, then directories then empty rows
        if (descrLineA->objType == FileDescrLine::TYPE_NOTHING)
            return false;  //empty rows always last
        else if (descrLineB->objType == FileDescrLine::TYPE_NOTHING)
            return true;  //empty rows always last
        else
#ifdef FFS_WIN //case-insensitive comparison!
            return sortAscending == ASCENDING ?
                   FreeFileSync::compareStringsWin32(descrLineA->fullName.c_str(), descrLineB->fullName.c_str()) < 0 : //way faster than wxString::CmpNoCase() in windows build!!!
                   FreeFileSync::compareStringsWin32(descrLineA->fullName.c_str(), descrLineB->fullName.c_str()) > 0;
#else
            return sortAscending == ASCENDING ?
                   descrLineA->fullName.Cmp(descrLineB->fullName) < 0 :
                   descrLineA->fullName.Cmp(descrLineB->fullName) > 0;
#endif
    }


    template <SortDirection sortAscending, SideToSort side>
    inline
    bool sortByFileSize(const FileCompareLine& a, const FileCompareLine& b)
    {
        const FileDescrLine* const descrLineA = side == SORT_ON_LEFT ? &a.fileDescrLeft : &a.fileDescrRight;
        const FileDescrLine* const descrLineB = side == SORT_ON_LEFT ? &b.fileDescrLeft : &b.fileDescrRight;

        //presort types: first files, then directories then empty rows
        if (descrLineA->objType == FileDescrLine::TYPE_NOTHING)
            return false;  //empty rows always last
        else if (descrLineB->objType == FileDescrLine::TYPE_NOTHING)
            return true;  //empty rows always last


        if (descrLineA->objType == FileDescrLine::TYPE_DIRECTORY) //sort directories by relative name
        {
            if (descrLineB->objType == FileDescrLine::TYPE_DIRECTORY)
                return stringSmallerThan<sortAscending>(descrLineA->relativeName.c_str(), descrLineB->relativeName.c_str());
            else
                return false;
        }
        else
        {
            if (descrLineB->objType == FileDescrLine::TYPE_DIRECTORY)
                return true;
            else
                return sortAscending == ASCENDING ?
                       descrLineA->fileSize > descrLineB->fileSize : //sortAscending shall result in list beginning with largest files first
                       descrLineA->fileSize < descrLineB->fileSize;
        }
    }


    template <SortDirection sortAscending, SideToSort side>
    inline
    bool sortByDate(const FileCompareLine& a, const FileCompareLine& b)
    {
        const FileDescrLine* const descrLineA = side == SORT_ON_LEFT ? &a.fileDescrLeft : &a.fileDescrRight;
        const FileDescrLine* const descrLineB = side == SORT_ON_LEFT ? &b.fileDescrLeft : &b.fileDescrRight;

        //presort types: first files, then directories then empty rows
        if (descrLineA->objType == FileDescrLine::TYPE_NOTHING)
            return false;  //empty rows always last
        else if (descrLineB->objType == FileDescrLine::TYPE_NOTHING)
            return true;  //empty rows always last

        if (descrLineA->objType == FileDescrLine::TYPE_DIRECTORY) //sort directories by relative name
        {
            if (descrLineB->objType == FileDescrLine::TYPE_DIRECTORY)
                return stringSmallerThan<sortAscending>(descrLineA->relativeName.c_str(), descrLineB->relativeName.c_str());
            else
                return false;
        }
        else
        {
            if (descrLineB->objType == FileDescrLine::TYPE_DIRECTORY)
                return true;
            else
                return sortAscending == ASCENDING ?
                       descrLineA->lastWriteTimeRaw > descrLineB->lastWriteTimeRaw :
                       descrLineA->lastWriteTimeRaw < descrLineB->lastWriteTimeRaw;
        }
    }


    template <SortDirection sortAscending>
    inline
    bool sortByCmpResult(const FileCompareLine& a, const FileCompareLine& b)
    {
        //presort result: equal shall appear at end of list
        if (a.cmpResult == FILE_EQUAL)
            return false;
        if (b.cmpResult == FILE_EQUAL)
            return true;

        return sortAscending == ASCENDING ?
               a.cmpResult < b.cmpResult :
               a.cmpResult > b.cmpResult;
    }


    template <SortDirection sortAscending>
    inline
    bool sortBySyncDirection(const FileCompareLine& a, const FileCompareLine& b)
    {
        return sortAscending == ASCENDING ?
               a.direction < b.direction :
               a.direction > b.direction;
    }


    template <SortDirection sortAscending, SideToSort side>
    inline
    bool sortByDirectory(const FolderCompareLine& a, const FolderCompareLine& b)
    {
        const Zstring* const dirNameA = side == SORT_ON_LEFT ? &a.syncPair.leftDirectory : &a.syncPair.rightDirectory;
        const Zstring* const dirNameB = side == SORT_ON_LEFT ? &b.syncPair.leftDirectory : &b.syncPair.rightDirectory;

#ifdef FFS_WIN //case-insensitive comparison!
        return sortAscending == ASCENDING ?
               FreeFileSync::compareStringsWin32(dirNameA->c_str(), dirNameB->c_str()) < 0 : //way faster than wxString::CmpNoCase() in windows build!!!
               FreeFileSync::compareStringsWin32(dirNameA->c_str(), dirNameB->c_str()) > 0;
#elif defined FFS_LINUX
        return sortAscending == ASCENDING ?
               dirNameA->Cmp(*dirNameB) < 0 :
               dirNameA->Cmp(*dirNameB) > 0;
#endif
    }
}

#endif // SORTING_H_INCLUDED
