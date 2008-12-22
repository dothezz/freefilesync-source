#ifndef SORTING_H_INCLUDED
#define SORTING_H_INCLUDED

#include "../FreeFileSync.h"
#include "resources.h"


enum SideToSort
{
    SORT_ON_LEFT,
    SORT_ON_RIGHT,
};


template <bool sortAscending>
inline
bool cmpString(const wxString& a, const wxString& b)
{
    if (a.IsEmpty())
        return false;   //if a and b are empty: false, if a empty, b not empty: also false, since empty rows should appear at the end
    else if (b.IsEmpty())
        return true;    //empty rows after filled rows: return true

    //if a and b not empty:
    if (sortAscending)
        return (a < b);
    else
        return (a > b);
}


template <bool sortAscending>
inline
bool cmpLargeInt(const wxULongLong& a, const wxULongLong& b)
{
    if (sortAscending)
        return (a < b);
    else
        return (a > b);
}


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
    else assert(false);
}


inline
wxChar formatChar(const wxChar& c)
{
    return c;
    //return wxTolower(c); <- this is slow as hell! sorting slower by factor 10
}


inline
int compareString(const wxChar* stringA, const wxChar* stringB, const int lengthA, const int lengthB)
{
    int i = 0;
    if (lengthA == lengthB)
    {
        for (i = 0; i < lengthA; ++i)
        {
            if (formatChar(stringA[i]) != formatChar(stringB[i]))
                break;
        }
        return i == lengthA ? 0 : formatChar(stringA[i]) < formatChar(stringB[i]) ? -1 : 1;
    }
    else if (lengthA < lengthB)
    {
        for (i = 0; i < lengthA; ++i)
        {
            if (formatChar(stringA[i]) != formatChar(stringB[i]))
                break;
        }
        return i == lengthA ? -1 : formatChar(stringA[i]) < formatChar(stringB[i]) ? -1 : 1;
    }
    else
    {
        for (i = 0; i < lengthB; ++i)
        {
            if (formatChar(stringA[i]) != formatChar(stringB[i]))
                break;
        }
        return i == lengthB ? 1 : formatChar(stringA[i]) < formatChar(stringB[i]) ? -1 : 1;
    }
}


template <bool sortAscending, SideToSort side>
inline
bool sortByFileName(const FileCompareLine& a, const FileCompareLine& b)
{
    const FileDescrLine* descrLineA;
    const FileDescrLine* descrLineB;
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
        const wxChar* stringA  = NULL;
        const wxChar* stringB  = NULL;
        int lenghtA  = 0;
        int lenghtB  = 0;

        int pos = descrLineA->relativeName.Find(GlobalResources::fileNameSeparator, true); //start search beginning from end
        if (pos == wxNOT_FOUND)
        {
            stringA = descrLineA->relativeName.c_str();
            lenghtA = descrLineA->relativeName.Len();
        }
        else
        {
            stringA = descrLineA->relativeName.c_str() + pos + 1;
            lenghtA = descrLineA->relativeName.Len() - (pos + 1);
        }

        pos = descrLineB->relativeName.Find(GlobalResources::fileNameSeparator, true); //start search beginning from end
        if (pos == wxNOT_FOUND)
        {
            stringB = descrLineB->relativeName.c_str();
            lenghtB = descrLineB->relativeName.Len();
        }
        else
        {
            stringB = descrLineB->relativeName.c_str() + pos + 1;
            lenghtB = descrLineB->relativeName.Len() - (pos + 1);
        }

        int rv = compareString(stringA, stringB, lenghtA, lenghtB);
        return sortAscending ? (rv == -1) : (rv != -1);
    }
}


template <bool sortAscending, SideToSort side>
inline
bool sortByRelativeName(const FileCompareLine& a, const FileCompareLine& b)
{
    const FileDescrLine* descrLineA;
    const FileDescrLine* descrLineB;
    getDescrLine<side>(a, b, descrLineA, descrLineB);

    //extract relative name and filename
    const wxChar* relStringA  = NULL;
    const wxChar* fileStringA = NULL;
    int relLenghtA  = 0;
    int fileLengthA = 0;
    if (descrLineA->objType == FileDescrLine::TYPE_DIRECTORY)
    {
        relStringA  = descrLineA->relativeName.c_str();
        relLenghtA  = descrLineA->relativeName.Len();
    }
    else if (descrLineA->objType == FileDescrLine::TYPE_FILE)
    {
        relLenghtA = descrLineA->relativeName.Find(GlobalResources::fileNameSeparator, true); //start search beginning from end
        if (relLenghtA == wxNOT_FOUND)
        {
            relLenghtA  = 0;
            fileStringA = descrLineA->relativeName.c_str();
            fileLengthA = descrLineA->relativeName.Len();
        }
        else
        {
            relStringA  = descrLineA->relativeName.c_str();
            fileStringA = descrLineA->relativeName.c_str() + relLenghtA + 1;
            fileLengthA = descrLineA->relativeName.Len() - (relLenghtA + 1);
        }
    }
    else
        return false; //empty rows should be on end of list


    const wxChar* relStringB  = NULL;
    const wxChar* fileStringB = NULL;
    int relLenghtB  = 0;
    int fileLengthB = 0;
    if (descrLineB->objType == FileDescrLine::TYPE_DIRECTORY)
    {
        relStringB  = descrLineB->relativeName.c_str();
        relLenghtB  = descrLineB->relativeName.Len();
    }
    else if (descrLineB->objType == FileDescrLine::TYPE_FILE)
    {
        relLenghtB = descrLineB->relativeName.Find(GlobalResources::fileNameSeparator, true); //start search beginning from end
        if (relLenghtB == wxNOT_FOUND)
        {
            relLenghtB  = 0;
            fileStringB = descrLineB->relativeName.c_str();
            fileLengthB = descrLineB->relativeName.Len();
        }
        else
        {
            relStringB  = descrLineB->relativeName.c_str();
            fileStringB = descrLineB->relativeName.c_str() + relLenghtB + 1;
            fileLengthB = descrLineB->relativeName.Len() - (relLenghtB + 1);
        }
    }
    else
        return true; //empty rows should be on end of list

    //compare relative names without filenames first
    int rv = compareString(relStringA, relStringB, relLenghtA, relLenghtB);
    if (rv != 0)
        return sortAscending ? (rv == -1) : (rv != -1);
    else //compare the filenames
    {
        if (descrLineB->objType == FileDescrLine::TYPE_DIRECTORY)  //directories shall appear before files
            return false;
        else if (descrLineA->objType == FileDescrLine::TYPE_DIRECTORY)
            return true;

        rv = compareString(fileStringA, fileStringB, fileLengthA, fileLengthB);
        return sortAscending ? (rv == -1) : (rv != -1);
    }
}


template <bool sortAscending, SideToSort side>
inline
bool sortByFileSize(const FileCompareLine& a, const FileCompareLine& b)
{
    const FileDescrLine* descrLineA;
    const FileDescrLine* descrLineB;
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
    else       //use unformatted filesizes and sort by size
        return  cmpLargeInt<sortAscending>(descrLineA->fileSize, descrLineB->fileSize);
}


template <bool sortAscending, SideToSort side>
inline
bool sortByDate(const FileCompareLine& a, const FileCompareLine& b)
{
    const FileDescrLine* descrLineA;
    const FileDescrLine* descrLineB;
    getDescrLine<side>(a, b, descrLineA, descrLineB);

    return cmpString<sortAscending>(descrLineA->lastWriteTime, descrLineB->lastWriteTime);
}


#endif // SORTING_H_INCLUDED
