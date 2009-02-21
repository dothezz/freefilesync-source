#ifndef SORTING_H_INCLUDED
#define SORTING_H_INCLUDED

#include "../FreeFileSync.h"
#include "resources.h"
#include "globalFunctions.h"

using namespace FreeFileSync;

enum SideToSort
{
    SORT_ON_LEFT,
    SORT_ON_RIGHT,
};


template <SideToSort side>
inline
void getDescrLine(const FileCompareLine& a, const FileCompareLine& b, const FileDescrLine*& descrLineA, const FileDescrLine*& descrLineB)
{
    if (side == SORT_ON_LEFT)
    {
        descrLineA = &a.fileDescrLeft;
        descrLineB = &b.fileDescrLeft;
    }
    else if (side == SORT_ON_RIGHT)
    {
        descrLineA = &a.fileDescrRight;
        descrLineB = &b.fileDescrRight;
    }
    else
        assert(false);
}


template <bool sortAscending>
inline
bool stringSmallerThan(const wxChar* stringA, const wxChar* stringB)
{
#ifdef FFS_WIN //case-insensitive comparison!
    return sortAscending ?
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
    return sortAscending ? *stringA < *stringB : *stringA > *stringB; //wxChar(0) is handled correctly
#endif
}


inline
int compareString(const wxChar* stringA, const wxChar* stringB, const int lengthA, const int lengthB)
{
#ifdef FFS_WIN //case-insensitive comparison!
    return FreeFileSync::compareStringsWin32(stringA, stringB, lengthA, lengthB); //way faster than wxString::CmpNoCase() in the windows build!!!
#else
    int i = 0;
    if (lengthA == lengthB)
    {
        for (i = 0; i < lengthA; ++i)
        {
            if (stringA[i] != stringB[i])
                break;
        }
        return i == lengthA ? 0 : stringA[i] < stringB[i] ? -1 : 1;
    }
    else if (lengthA < lengthB)
    {
        for (i = 0; i < lengthA; ++i)
        {
            if (stringA[i] != stringB[i])
                break;
        }
        return i == lengthA ? -1 : stringA[i] < stringB[i] ? -1 : 1;
    }
    else
    {
        for (i = 0; i < lengthB; ++i)
        {
            if (stringA[i] != stringB[i])
                break;
        }
        return i == lengthB ? 1 : stringA[i] < stringB[i] ? -1 : 1;
    }
#endif
}


template <bool sortAscending, SideToSort side>
inline
bool sortByFileName(const FileCompareLine& a, const FileCompareLine& b)
{
    const FileDescrLine* descrLineA = NULL;
    const FileDescrLine* descrLineB = NULL;
    getDescrLine<side>(a, b, descrLineA, descrLineB);

    //presort types: first files, then directories then empty rows
    if (descrLineA->objType == FileDescrLine::TYPE_NOTHING)
        return false;  //empty rows always last
    else if (descrLineB->objType == FileDescrLine::TYPE_NOTHING)
        return true;  //empty rows always last
    else if (descrLineA->objType == FileDescrLine::TYPE_DIRECTORY)
        return false;
    else if (descrLineB->objType == FileDescrLine::TYPE_DIRECTORY)
        return true;
    else
    {
        const wxChar* stringA = descrLineA->relativeName.c_str();
        const wxChar* stringB = descrLineB->relativeName.c_str();

        size_t pos = descrLineA->relativeName.Find(GlobalResources::FILE_NAME_SEPARATOR, true); //start search beginning from end
        if (pos != std::string::npos)
            stringA += pos + 1;

        pos = descrLineB->relativeName.Find(GlobalResources::FILE_NAME_SEPARATOR, true); //start search beginning from end
        if (pos != std::string::npos)
            stringB += pos + 1;

        return stringSmallerThan<sortAscending>(stringA, stringB);
    }
}


template <bool sortAscending, SideToSort side>
bool sortByRelativeName(const FileCompareLine& a, const FileCompareLine& b)
{
    const FileDescrLine* descrLineA = NULL;
    const FileDescrLine* descrLineB = NULL;
    getDescrLine<side>(a, b, descrLineA, descrLineB);

    //extract relative name and filename
    const wxChar* relStringA  = descrLineA->relativeName.c_str(); //mustn't be NULL for CompareString() API to work correctly
    const wxChar* fileStringA = relStringA;
    int relLengthA  = 0;
    int fileLengthA = 0;

    if (descrLineA->objType == FileDescrLine::TYPE_DIRECTORY)
        relLengthA  = descrLineA->relativeName.length();
    else if (descrLineA->objType == FileDescrLine::TYPE_FILE)
    {
        relLengthA = descrLineA->relativeName.Find(GlobalResources::FILE_NAME_SEPARATOR, true); //start search beginning from end
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


    const wxChar* relStringB  = descrLineB->relativeName.c_str(); //mustn't be NULL for CompareString() API to work correctly
    const wxChar* fileStringB = relStringB;
    int relLengthB  = 0;
    int fileLengthB = 0;

    if (descrLineB->objType == FileDescrLine::TYPE_DIRECTORY)
        relLengthB  = descrLineB->relativeName.length();
    else if (descrLineB->objType == FileDescrLine::TYPE_FILE)
    {
        relLengthB = descrLineB->relativeName.Find(GlobalResources::FILE_NAME_SEPARATOR, true); //start search beginning from end
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
    int rv = compareString(relStringA, relStringB, relLengthA, relLengthB);
    if (rv != 0)
        return sortAscending ? (rv < 0) : (rv > 0);
    else //compare the filenames
    {
        if (descrLineB->objType == FileDescrLine::TYPE_DIRECTORY)  //directories shall appear before files
            return false;
        else if (descrLineA->objType == FileDescrLine::TYPE_DIRECTORY)
            return true;

        return sortAscending ?
               compareString(fileStringA, fileStringB, fileLengthA, fileLengthB) < 0 :
               compareString(fileStringA, fileStringB, fileLengthA, fileLengthB) > 0;
    }
}


template <bool sortAscending, SideToSort side>
inline
bool sortByFileSize(const FileCompareLine& a, const FileCompareLine& b)
{
    const FileDescrLine* descrLineA = NULL;
    const FileDescrLine* descrLineB = NULL;
    getDescrLine<side>(a, b, descrLineA, descrLineB);

    //presort types: first files, then directories then empty rows
    if (descrLineA->objType == FileDescrLine::TYPE_NOTHING)
        return false;  //empty rows always last
    else if (descrLineB->objType == FileDescrLine::TYPE_NOTHING)
        return true;  //empty rows always last
    else if (descrLineA->objType == FileDescrLine::TYPE_DIRECTORY)
        return false;
    else if (descrLineB->objType == FileDescrLine::TYPE_DIRECTORY)
        return true;
    else
        return sortAscending ?
               descrLineA->fileSize < descrLineB->fileSize :
               descrLineA->fileSize > descrLineB->fileSize;
}


template <bool sortAscending, SideToSort side>
inline
bool sortByDate(const FileCompareLine& a, const FileCompareLine& b)
{
    const FileDescrLine* descrLineA = NULL;
    const FileDescrLine* descrLineB = NULL;
    getDescrLine<side>(a, b, descrLineA, descrLineB);

    //presort types: first files, then directories then empty rows
    if (descrLineA->objType == FileDescrLine::TYPE_NOTHING)
        return false;  //empty rows always last
    else if (descrLineB->objType == FileDescrLine::TYPE_NOTHING)
        return true;  //empty rows always last
    else if (descrLineA->objType == FileDescrLine::TYPE_DIRECTORY)
        return false;
    else if (descrLineB->objType == FileDescrLine::TYPE_DIRECTORY)
        return true;
    else
        return sortAscending ?
               descrLineA->lastWriteTimeRaw < descrLineB->lastWriteTimeRaw :
               descrLineA->lastWriteTimeRaw > descrLineB->lastWriteTimeRaw;
}


template <bool sortAscending>
inline
bool sortByCmpResult(const FileCompareLine& a, const FileCompareLine& b)
{
    //presort result: equal shall appear at end of list
    if (a.cmpResult == FILE_EQUAL)
        return false;
    if (b.cmpResult == FILE_EQUAL)
        return true;

    return sortAscending ?
           a.cmpResult < b.cmpResult :
           a.cmpResult > b.cmpResult;
}


#endif // SORTING_H_INCLUDED
