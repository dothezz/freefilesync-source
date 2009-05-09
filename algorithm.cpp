#include "algorithm.h"
#include <wx/intl.h>
#include <cmath>
#include <wx/log.h>
#include "library/resources.h"
#include "library/globalFunctions.h"
#include "library/statusHandler.h"
#include "library/fileHandling.h"
#include <wx/msgdlg.h>

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"

#elif defined FFS_LINUX
#include <string.h>
#include <errno.h>
#endif

using namespace FreeFileSync;


wxString FreeFileSync::formatFilesizeToShortString(const wxLongLong& filesize)
{
    return FreeFileSync::formatFilesizeToShortString(filesize.ToDouble());
}


wxString FreeFileSync::formatFilesizeToShortString(const wxULongLong& filesize)
{
    return FreeFileSync::formatFilesizeToShortString(filesize.ToDouble());
};


wxString FreeFileSync::formatFilesizeToShortString(const double filesize)
{
    if (filesize < 0)
        return _("Error");

    double nrOfBytes = filesize;

    bool unitByte = true;
    wxString unit = _(" Byte");
    if (nrOfBytes > 999)
    {
        unitByte = false;
        nrOfBytes /= 1024;
        unit = _(" kB");
        if (nrOfBytes > 999)
        {
            nrOfBytes /= 1024;
            unit = _(" MB");
            if (nrOfBytes > 999)
            {
                nrOfBytes /= 1024;
                unit = _(" GB");
                if (nrOfBytes > 999)
                {
                    nrOfBytes /= 1024;
                    unit = _(" TB");
                    if (nrOfBytes > 999)
                    {
                        nrOfBytes /= 1024;
                        unit = _(" PB");
                    }
                }
            }
        }
    }

    wxString temp;
    if (unitByte) //no decimal places in case of bytes
    {
        double integer = 0;
        modf(nrOfBytes, &integer); //get integer part of nrOfBytes
        temp = globalFunctions::numberToWxString(int(integer));
    }
    else
    {
        nrOfBytes*= 100;    //we want up to two decimal places
        double integer = 0;
        modf(nrOfBytes, &integer); //get integer part of nrOfBytes

        temp = globalFunctions::numberToWxString(int(integer));

        switch (temp.length())
        {
        case 0:
            return _("Error");
        case 1:
            temp = wxString(wxT("0")) + GlobalResources::DECIMAL_POINT + wxT("0") + temp;
            break; //0,01
        case 2:
            temp = wxString(wxT("0")) + GlobalResources::DECIMAL_POINT + temp;
            break; //0,11
        case 3:
            temp.insert(1, GlobalResources::DECIMAL_POINT);
            break; //1,11
        case 4:
            temp = temp.substr(0, 3);
            temp.insert(2, GlobalResources::DECIMAL_POINT);
            break; //11,1
        case 5:
            temp = temp.substr(0, 3);
            break; //111
        default:
            return _("Error");
        }
    }
    return (temp + unit);
}


Zstring FreeFileSync::getFormattedDirectoryName(const Zstring& dirname)
{   //Formatting is needed since functions expect the directory to end with '\' to be able to split the relative names.
    //Also improves usability.

    Zstring dirnameTmp = dirname;
    dirnameTmp.Trim(true);  //remove whitespace characters from right
    dirnameTmp.Trim(false); //remove whitespace characters from left

    if (dirnameTmp.empty()) //an empty string is interpreted as "\"; this is not desired
        return Zstring();

    if (!endsWithPathSeparator(dirnameTmp))
        dirnameTmp += GlobalResources::FILE_NAME_SEPARATOR;

    //don't do directory formatting with wxFileName, as it doesn't respect //?/ - prefix
    return dirnameTmp;
}


void FreeFileSync::swapGrids(FolderComparison& folderCmp)
{
    for (FolderComparison::iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        std::swap(j->syncPair.leftDirectory, j->syncPair.rightDirectory);

        FileComparison& fileCmp = j->fileCmp;
        for (FileComparison::iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
        {
            //swap compare result
            if (i->cmpResult == FILE_LEFT_SIDE_ONLY)
                i->cmpResult = FILE_RIGHT_SIDE_ONLY;
            else if (i->cmpResult == FILE_RIGHT_SIDE_ONLY)
                i->cmpResult = FILE_LEFT_SIDE_ONLY;
            else if (i->cmpResult == FILE_RIGHT_NEWER)
                i->cmpResult = FILE_LEFT_NEWER;
            else if (i->cmpResult == FILE_LEFT_NEWER)
                i->cmpResult = FILE_RIGHT_NEWER;

            //swap file descriptors
            std::swap(i->fileDescrLeft, i->fileDescrRight);
        }
    }
}

/*
void FreeFileSync::adjustModificationTimes(const Zstring& parentDirectory, const int timeInSeconds, ErrorHandler* errorHandler) throw(AbortThisProcess)
{
#ifndef __WXDEBUG__
    wxLogNull noWxLogs; //prevent wxWidgets logging
#endif

    if (timeInSeconds == 0)
        return;

    std::vector<Zstring> fileList;
    std::vector<Zstring> dirList;

    while (true)  //should be executed in own scope so that directory access does not disturb deletion
    {
        try
        {   //get all files and folders from directory (and subdirectories)
            FreeFileSync::getAllFilesAndDirs(parentDirectory, fileList, dirList);
            break;
        }
        catch (const FileError& e)
        {
            ErrorHandler::Response rv = errorHandler->reportError(e.show());
            if (rv == ErrorHandler::IGNORE_ERROR)
                break;
            else if (rv == ErrorHandler::RETRY)
                ;   //continue with loop
            else
                assert (false);
        }
    }

    //this part is only a bit slower than direct Windows API-access!
    wxDateTime modTime;
    for (unsigned int j = 0; j < fileList.size(); ++j)
    {
        while (true)  //own scope for directory access to not disturb file access!
        {
            try
            {
                wxFileName file(fileList[j].c_str());
                if (!file.GetTimes(NULL, &modTime, NULL)) //get modification time
                    throw FileError(Zstring(_("Error changing modification time:")) + wxT(" \"") + fileList[j] + wxT("\""));

                modTime.Add(wxTimeSpan(0, 0, timeInSeconds, 0));

                if (!file.SetTimes(NULL, &modTime, NULL)) //get modification time
                    throw FileError(Zstring(_("Error changing modification time:")) + wxT(" \"") + fileList[j] + wxT("\""));

                break;
            }
            catch (const FileError& error)
            {
                ErrorHandler::Response rv = errorHandler->reportError(error.show());
                if (rv == ErrorHandler::IGNORE_ERROR)
                    break;
                else if (rv == ErrorHandler::RETRY)
                    ;   //continue with loop
                else
                    assert (false);
            }
        }
    }
}
*/


//add(!) all files and subfolder gridlines that are dependent from the directory
void FreeFileSync::addSubElements(const FileComparison& fileCmp, const FileCompareLine& relevantRow, std::set<int>& subElements)
{
    const FileDescrLine& relFileDescrLeft  = relevantRow.fileDescrLeft;
    const FileDescrLine& relFileDescrRight = relevantRow.fileDescrRight;

    Zstring relevantDirectory;
    if (relFileDescrLeft.objType == FileDescrLine::TYPE_DIRECTORY)
        relevantDirectory = Zstring(relFileDescrLeft.relativeName.c_str(), relFileDescrLeft.relativeName.length());
    else if (relFileDescrRight.objType == FileDescrLine::TYPE_DIRECTORY)
        relevantDirectory = Zstring(relFileDescrRight.relativeName.c_str(), relFileDescrRight.relativeName.length());
    else
        return;

    relevantDirectory += GlobalResources::FILE_NAME_SEPARATOR; //FILE_NAME_SEPARATOR needed to exclude subfile/dirs only

    for (FileComparison::const_iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
    {
        if (i->fileDescrLeft.objType != FileDescrLine::TYPE_NOTHING)
        {
            if (i->fileDescrLeft.relativeName.StartsWith(relevantDirectory))
                subElements.insert(i - fileCmp.begin());
        }
        //"else if": no need to do a redundant check on both sides: relative names should be the same!
        else if (i->fileDescrRight.objType != FileDescrLine::TYPE_NOTHING)
        {
            if (i->fileDescrRight.relativeName.StartsWith(relevantDirectory))
                subElements.insert(i - fileCmp.begin());
        }
    }
}


//############################################################################################################
struct SortedFileName
{
    unsigned position;
    Zstring name;

    bool operator < (const SortedFileName& b) const
    {
        return position < b.position;
    }
};


//assemble message containing all files to be deleted
wxString FreeFileSync::deleteFromGridAndHDPreview(const FileComparison& fileCmp,
        const std::set<int>& rowsToDeleteOnLeft,
        const std::set<int>& rowsToDeleteOnRight,
        const bool deleteOnBothSides)
{
    if (deleteOnBothSides)
    {
        //mix selected rows from left and right
        std::set<int> rowsToDelete = rowsToDeleteOnLeft;
        for (std::set<int>::const_iterator i = rowsToDeleteOnRight.begin(); i != rowsToDeleteOnRight.end(); ++i)
            rowsToDelete.insert(*i);

        wxString filesToDelete;
        for (std::set<int>::const_iterator i = rowsToDelete.begin(); i != rowsToDelete.end(); ++i)
        {
            const FileCompareLine& currentCmpLine = fileCmp[*i];

            if (currentCmpLine.fileDescrLeft.objType != FileDescrLine::TYPE_NOTHING)
                filesToDelete += (currentCmpLine.fileDescrLeft.fullName + wxT("\n")).c_str();

            if (currentCmpLine.fileDescrRight.objType != FileDescrLine::TYPE_NOTHING)
                filesToDelete += (currentCmpLine.fileDescrRight.fullName + wxT("\n")).c_str();

            filesToDelete+= wxT("\n");
        }

        return filesToDelete;
    }
    else //delete selected files only
    {
        std::set<SortedFileName> outputTable; //sort selected files from left and right ascending by row number

        SortedFileName newEntry;
        for (std::set<int>::const_iterator i = rowsToDeleteOnLeft.begin(); i != rowsToDeleteOnLeft.end(); ++i)
        {
            const FileCompareLine& currentCmpLine = fileCmp[*i];

            if (currentCmpLine.fileDescrLeft.objType != FileDescrLine::TYPE_NOTHING)
            {
                newEntry.position = *i * 10;
                newEntry.name = currentCmpLine.fileDescrLeft.fullName;
                outputTable.insert(newEntry);
            }
        }

        for (std::set<int>::const_iterator i = rowsToDeleteOnRight.begin(); i != rowsToDeleteOnRight.end(); ++i)
        {
            const FileCompareLine& currentCmpLine = fileCmp[*i];

            if (currentCmpLine.fileDescrRight.objType != FileDescrLine::TYPE_NOTHING)
            {
                newEntry.position = *i * 10 + 1; //ensure files on left are before files on right for the same row number
                newEntry.name = currentCmpLine.fileDescrRight.fullName;
                outputTable.insert(newEntry);
            }
        }

        wxString filesToDelete;
        for (std::set<SortedFileName>::iterator i = outputTable.begin(); i != outputTable.end(); ++i)
            filesToDelete += (i->name + wxT("\n")).c_str();

        return filesToDelete;
    }
}


class RemoveAtExit //this class ensures, that the result of the method below is ALWAYS written on exit, even if exceptions are thrown!
{
public:
    RemoveAtExit(FileComparison& fileCmp) :
            gridToWrite(fileCmp) {}

    ~RemoveAtExit()
    {
        removeRowsFromVector(gridToWrite, rowsProcessed);
    }

    void removeRow(int nr)
    {
        rowsProcessed.insert(nr);
    }

private:
    FileComparison& gridToWrite;
    std::set<int> rowsProcessed;
};


template <bool leftSide> //update compareGrid row information after deletion from leftSide (or !leftSide)
inline
void updateCmpLineAfterDeletion(FileCompareLine& relevantRow, const int rowNr, RemoveAtExit& markForRemoval)
{
    FileDescrLine* fileDescr;        //get descriptor for file to be deleted; evaluated at compile time
    FileDescrLine* fileDescrPartner; //file descriptor for "other side"
    if (leftSide)
    {
        fileDescr        = &relevantRow.fileDescrLeft;
        fileDescrPartner = &relevantRow.fileDescrRight;
    }
    else
    {
        fileDescr        = &relevantRow.fileDescrRight;
        fileDescrPartner = &relevantRow.fileDescrLeft;
    }


    //remove deleted entries from grid
    if (fileDescrPartner->objType == FileDescrLine::TYPE_NOTHING)
        markForRemoval.removeRow(rowNr);
    else
    {
        //initialize fileDescr for deleted file/folder
        *fileDescr = FileDescrLine();

        //adapt the compare result
        if (leftSide) //again evaluated at compile time
            relevantRow.cmpResult = FILE_RIGHT_SIDE_ONLY;
        else
            relevantRow.cmpResult = FILE_LEFT_SIDE_ONLY;
    }
}


template <bool leftSide>
void deleteFromGridAndHDOneSide(FileComparison& fileCmp,
                                const std::set<int>& rowsToDeleteOneSide,
                                const bool useRecycleBin,
                                RemoveAtExit& markForRemoval,
                                ErrorHandler* errorHandler)
{
    for (std::set<int>::iterator i = rowsToDeleteOneSide.begin(); i != rowsToDeleteOneSide.end(); ++i)
    {
        FileCompareLine& currentCmpLine = fileCmp[*i];

        FileDescrLine* fileDescr = NULL; //get descriptor for file to be deleted; evaluated at compile time
        if (leftSide)
            fileDescr = &currentCmpLine.fileDescrLeft;
        else
            fileDescr = &currentCmpLine.fileDescrRight;

        while (true)
        {
            try
            {
                if (fileDescr->objType == FileDescrLine::TYPE_FILE)
                    FreeFileSync::removeFile(fileDescr->fullName, useRecycleBin);
                else if (fileDescr->objType == FileDescrLine::TYPE_DIRECTORY)
                    FreeFileSync::removeDirectory(fileDescr->fullName, useRecycleBin);
                else if (fileDescr->objType == FileDescrLine::TYPE_NOTHING)
                    break; //nothing to do
                else
                {
                    assert(false);
                    break;
                }

                //retrieve all files and subfolder gridlines that are dependent from this deleted entry
                std::set<int> rowsToDelete;
                rowsToDelete.insert(*i);
                FreeFileSync::addSubElements(fileCmp, currentCmpLine, rowsToDelete);

                //remove deleted entries from fileCmp (or adapt it if deleted from one side only)
                for (std::set<int>::iterator j = rowsToDelete.begin(); j != rowsToDelete.end(); ++j)
                    updateCmpLineAfterDeletion<leftSide>(fileCmp[*j], *j, markForRemoval);

                break;
            }
            catch (const FileError& error)
            {
                ErrorHandler::Response rv = errorHandler->reportError(error.show());

                if (rv == ErrorHandler::IGNORE_ERROR)
                    break;

                else if (rv == ErrorHandler::RETRY)
                    ;   //continue in loop
                else
                    assert (false);
            }
        }
    }
}


void FreeFileSync::deleteFromGridAndHD(FileComparison& fileCmp,
                                       const std::set<int>& rowsToDeleteOnLeft,
                                       const std::set<int>& rowsToDeleteOnRight,
                                       const bool deleteOnBothSides,
                                       const bool useRecycleBin,
                                       ErrorHandler* errorHandler)
{
    //remove deleted rows from fileCmp (AFTER all rows to be deleted are known: consider row references!
    RemoveAtExit markForRemoval(fileCmp); //ensure that fileCmp is always written to, even if method is exitted via exceptions

    if (deleteOnBothSides)
    {
        //mix selected rows from left and right
        std::set<int> rowsToDeleteBothSides = rowsToDeleteOnLeft;
        for (std::set<int>::const_iterator i = rowsToDeleteOnRight.begin(); i != rowsToDeleteOnRight.end(); ++i)
            rowsToDeleteBothSides.insert(*i);

        deleteFromGridAndHDOneSide<true>(fileCmp,
                                         rowsToDeleteBothSides,
                                         useRecycleBin,
                                         markForRemoval,
                                         errorHandler);

        deleteFromGridAndHDOneSide<false>(fileCmp,
                                          rowsToDeleteBothSides,
                                          useRecycleBin,
                                          markForRemoval,
                                          errorHandler);
    }
    else
    {
        deleteFromGridAndHDOneSide<true>(fileCmp,
                                         rowsToDeleteOnLeft,
                                         useRecycleBin,
                                         markForRemoval,
                                         errorHandler);

        deleteFromGridAndHDOneSide<false>(fileCmp,
                                          rowsToDeleteOnRight,
                                          useRecycleBin,
                                          markForRemoval,
                                          errorHandler);
    }
}
//############################################################################################################


inline
void writeTwoDigitNumber(unsigned int number, wxChar*& position)
{
    assert (number < 100);

    *position = '0' + number / 10;
    position[1] = '0' + number % 10;

    position += 2;
}


inline
void writeFourDigitNumber(unsigned int number, wxChar*& position)
{
    assert (number < 10000);

    *position = '0' + number / 1000;
    number %= 1000;
    position[1] = '0' + number / 100;
    number %= 100;
    position[2] = '0' + number / 10;
    number %= 10;
    position[3] = '0' + number;

    position += 4;
}


wxString FreeFileSync::utcTimeToLocalString(const wxLongLong& utcTime)
{
#ifdef FFS_WIN
    //convert ansi C time to FILETIME
    wxLongLong fileTimeLong(utcTime);
    fileTimeLong += wxLongLong(2, 3054539008UL); //timeshift between ansi C time and FILETIME in seconds == 11644473600s
    fileTimeLong *= 10000000;

    FILETIME lastWriteTimeUtc;
    lastWriteTimeUtc.dwLowDateTime  = fileTimeLong.GetLo();             //GetLo() returns unsigned
    lastWriteTimeUtc.dwHighDateTime = unsigned(fileTimeLong.GetHi());   //GetHi() returns signed

    FILETIME localFileTime;
    if (FileTimeToLocalFileTime(   //convert to local time
                &lastWriteTimeUtc, //pointer to UTC file time to convert
                &localFileTime 	   //pointer to converted file time
            ) == 0)
        throw RuntimeException(wxString(_("Conversion error:")) + wxT(" FILETIME -> local FILETIME"));

    SYSTEMTIME time;
    if (FileTimeToSystemTime(
                &localFileTime, //pointer to file time to convert
                &time 	        //pointer to structure to receive system time
            ) == 0)
        throw RuntimeException(wxString(_("Conversion error:")) + wxT(" FILETIME -> SYSTEMTIME"));

    //assemble time string (performance optimized)
    wxChar formattedTime[21];
    wxChar* p = formattedTime;

    writeFourDigitNumber(time.wYear, p);
    *(p++) = wxChar('-');
    writeTwoDigitNumber(time.wMonth, p);
    *(p++) = wxChar('-');
    writeTwoDigitNumber(time.wDay, p);
    *(p++) = wxChar(' ');
    *(p++) = wxChar(' ');
    writeTwoDigitNumber(time.wHour, p);
    *(p++) = wxChar(':');
    writeTwoDigitNumber(time.wMinute, p);
    *(p++) = wxChar(':');
    writeTwoDigitNumber(time.wSecond, p);
    *p = 0;

    return wxString(formattedTime);

#elif defined FFS_LINUX
    tm* timeinfo;
    const time_t fileTime = utcTime.ToLong();
    timeinfo = localtime(&fileTime); //convert to local time
    char buffer[50];
    strftime(buffer, 50, "%Y-%m-%d  %H:%M:%S", timeinfo);

    return wxString(buffer);
#endif
}


#ifdef FFS_WIN
Zstring FreeFileSync::getLastErrorFormatted(const unsigned long lastError) //try to get additional Windows error information
{
    unsigned long lastErrorCode = lastError;
    //determine error code if none was specified
    if (lastErrorCode == 0)
        lastErrorCode = GetLastError();

    Zstring output = Zstring(wxT("Windows Error Code ")) + wxString::Format(wxT("%u"), lastErrorCode).c_str();

    WCHAR buffer[1001];
    if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, lastErrorCode, 0, buffer, 1001, NULL) != 0)
        output += Zstring(wxT(": ")) + buffer;
    return output;
}

#elif defined FFS_LINUX
Zstring FreeFileSync::getLastErrorFormatted(const int lastError) //try to get additional Linux error information
{
    int lastErrorCode = lastError;
    //determine error code if none was specified
    if (lastErrorCode == 0)
        lastErrorCode = errno;

    Zstring output = Zstring(wxT("Linux Error Code ")) + wxString::Format(wxT("%i"), lastErrorCode).c_str();
    output += Zstring(wxT(": ")) + strerror(lastErrorCode);
    return output;
}
#endif

/*Statistical theory: detect daylight saving time (DST) switch by comparing files that exist on both sides (and have same filesizes). If there are "enough"
that have a shift by +-1h then assert that DST switch occured.
What is "enough" =: N? N should be large enough AND small enough that the following two errors remain small:

Error 1: A DST switch is detected although there was none
Error 2: A DST switch is not detected although it took place

Error 1 results in lower bound, error 2 in upper bound for N.

Target: Choose N such that probability of error 1 and error 2 is lower than 0.001 (0.1%)

Definitions:
p1: probability that a file with same filesize on both sides was changed nevertheless
p2: probability that a changed file has +1h shift in filetime due to a change

M: number of files with same filesize on both sides in total
N: number of files with same filesize and time-diff +1h when DST check shall detect "true"

X: number of files with same filesize that have a +1h difference after change

Error 1 ("many files have +1h shift by chance") imposes:
Probability of error 1: (binomial distribution)

P(X >= N) = 1 - P(X <= N - 1) =
1 - sum_i=0^N-1 p3^i * (1 - p3)^(M - i) (M above i)   shall be   <= 0.0005

with p3 := p1 * p2

Probability of error 2 also will be <= 0.0005 if we choose N as lowest number that satisfies the preceding formula. Proof is left to the reader.

The function M |-> N behaves almost linearly and can be empirically approximated by:

N(M) =

2                   for      0 <= M <= 500
125/1000000 * M + 5 for    500 <  M <= 50000
77/1000000 * M + 10 for  50000 <  M <= 400000
60/1000000 * M + 35 for 400000 <  M


inline
bool sameFileTime(const time_t a, const time_t b)
{
    if (a < b)
        return b - a <= FILE_TIME_PRECISION;
    else
        return a - b <= FILE_TIME_PRECISION;
}


#ifdef FFS_WIN
unsigned int getThreshold(const unsigned filesWithSameSizeTotal)
{
    if (filesWithSameSizeTotal <= 500)
        return 2;
    else if (filesWithSameSizeTotal <= 50000)
        return unsigned(125.0/1000000 * filesWithSameSizeTotal + 5.0);
    else if (filesWithSameSizeTotal <= 400000)
        return unsigned(77.0/1000000 * filesWithSameSizeTotal + 10.0);
    else
        return unsigned(60.0/1000000 * filesWithSameSizeTotal + 35.0);
}


void FreeFileSync::checkForDSTChange(const FileCompareResult& gridData,
                                     const std::vector<FolderPair>& directoryPairsFormatted,
                                     int& timeShift, wxString& driveName)
{
    driveName.Clear();
    timeShift = 0;

    TIME_ZONE_INFORMATION dummy;
    DWORD rv = GetTimeZoneInformation(&dummy);
    if (rv == TIME_ZONE_ID_UNKNOWN) return;
    bool dstActive = rv == TIME_ZONE_ID_DAYLIGHT;

    for (std::vector<FolderPair>::const_iterator i = directoryPairsFormatted.begin(); i != directoryPairsFormatted.end(); ++i)
    {
        bool leftDirIsFat = isFatDrive(i->leftDirectory);
        bool rightDirIsFat = isFatDrive(i->rightDirectory);

        if (leftDirIsFat || rightDirIsFat)
        {
            unsigned int filesTotal        = 0; //total number of files (with same size on both sides)
            unsigned int plusOneHourCount  = 0; //number of files with +1h time shift
            unsigned int minusOneHourCount = 0; // "

            for (FileCompareResult::const_iterator j = gridData.begin(); j != gridData.end(); ++j)
            {
                const FileDescrLine& leftFile  = j->fileDescrLeft;
                const FileDescrLine& rightFile = j->fileDescrRight;

                if (    leftFile.objType == FileDescrLine::TYPE_FILE && rightFile.objType == FileDescrLine::TYPE_FILE &&
                        leftFile.fileSize == rightFile.fileSize &&
                        leftFile.directory.CmpNoCase(i->leftDirectory.c_str()) == 0 && //Windows does NOT distinguish between upper/lower-case
                        rightFile.directory.CmpNoCase(i->rightDirectory.c_str()) == 0) //
                {
                    ++filesTotal;

                    if (sameFileTime(leftFile.lastWriteTimeRaw - 3600, rightFile.lastWriteTimeRaw))
                        ++plusOneHourCount;
                    else if (sameFileTime(leftFile.lastWriteTimeRaw + 3600, rightFile.lastWriteTimeRaw))
                        ++minusOneHourCount;
                }
            }

            unsigned int threshold = getThreshold(filesTotal);
            if (plusOneHourCount >= threshold)
            {
                if (dstActive)
                {
                    if (rightDirIsFat) //it should be FAT; else this were some kind of error
                    {
                        timeShift = 3600;
                        driveName = getDriveName(i->rightDirectory);
                    }
                }
                else
                {
                    if (leftDirIsFat) //it should be FAT; else this were some kind of error
                    {
                        timeShift = -3600;
                        driveName = getDriveName(i->leftDirectory);
                    }
                }
                return;
            }
            else if (minusOneHourCount >= threshold)
            {
                if (dstActive)
                {
                    if (leftDirIsFat) //it should be FAT; else this were some kind of error
                    {
                        timeShift = 3600;
                        driveName = getDriveName(i->leftDirectory);
                    }
                }
                else
                {
                    if (rightDirIsFat) //it should be FAT; else this were some kind of error
                    {
                        timeShift = -3600;
                        driveName = getDriveName(i->rightDirectory);
                    }
                }
                return;
            }
        }
    }
}
#endif  //FFS_WIN
*/
