#include "algorithm.h"
#include <wx/intl.h>
#include <cmath>
#include <wx/filename.h>
#include <wx/log.h>
#include "library/resources.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#endif  //FFS_WIN

using namespace FreeFileSync;


wxString FreeFileSync::formatFilesizeToShortString(const wxULongLong& filesize)
{
    return formatFilesizeToShortString(filesize.ToDouble());
}


wxString FreeFileSync::formatFilesizeToShortString(const double filesize)
{
    double nrOfBytes = filesize;

    wxString unit = _(" Byte");
    if (nrOfBytes > 999)
    {
        nrOfBytes/= 1024;
        unit = _(" kB");
        if (nrOfBytes > 999)
        {
            nrOfBytes/= 1024;
            unit = _(" MB");
            if (nrOfBytes > 999)
            {
                nrOfBytes/= 1024;
                unit = _(" GB");
                if (nrOfBytes > 999)
                {
                    nrOfBytes/= 1024;
                    unit = _(" TB");
                    if (nrOfBytes > 999)
                    {
                        nrOfBytes/= 1024;
                        unit = _(" PB");
                    }
                }
            }
        }
    }

    wxString temp;
    if (unit == _(" Byte")) //no decimal places in case of bytes
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

        int length = temp.Len();

        switch (length)
        {
        case 0:
            temp = _("Error");
            break;
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
{   //Formatting is needed since functions in FreeFileSync.cpp expect the directory to end with '\' to be able to split the relative names.
    //Also improves usability.

    Zstring dirnameTmp = dirname;
    dirnameTmp.Trim(true);  //remove whitespace characters from right
    dirnameTmp.Trim(false); //remove whitespace characters from left

    if (dirnameTmp.empty()) //an empty string is interpreted as "\"; this is not desired
        return Zstring();

    //let wxWidgets do the directory formatting, e.g. replace '/' with '\' for Windows
    wxFileName directory = wxFileName::DirName(dirnameTmp.c_str());

    return Zstring(directory.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));
}


void FreeFileSync::swapGrids(FileCompareResult& grid)
{
    FileDescrLine tmp;

    for (FileCompareResult::iterator i = grid.begin(); i != grid.end(); ++i)
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
        tmp = i->fileDescrLeft;
        i->fileDescrLeft = i->fileDescrRight;
        i->fileDescrRight = tmp;
    }
}


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


void FreeFileSync::filterCurrentGridData(FileCompareResult& currentGridData, const wxString& includeFilter, const wxString& excludeFilter)
{
    //no need for regular expressions! In tests wxRegex was by factor of 10 slower than wxString::Matches()!!

    //load filter into vectors of strings
    //delimiters may be ';' or '\n'
    std::vector<Zstring> includeList = compoundStringToFilter(includeFilter.c_str());
    std::vector<Zstring> excludeList = compoundStringToFilter(excludeFilter.c_str());

    //format entries
    for (std::vector<Zstring>::iterator i = includeList.begin(); i != includeList.end(); ++i)
        formatFilterString(*i);
    for (std::vector<Zstring>::iterator i = excludeList.begin(); i != excludeList.end(); ++i)
        formatFilterString(*i);

//##############################################################

    //filter currentGridData
    for (FileCompareResult::iterator i = currentGridData.begin(); i != currentGridData.end(); ++i)
    {
        Zstring filenameLeft  = i->fileDescrLeft.fullName;
        Zstring filenameRight = i->fileDescrRight.fullName;

        formatFilterString(filenameLeft);
        formatFilterString(filenameRight);

        //process include filters
        if (i->fileDescrLeft.objType != FileDescrLine::TYPE_NOTHING)
        {
            bool includedLeft = false;
            for (std::vector<Zstring>::const_iterator j = includeList.begin(); j != includeList.end(); ++j)
                if (filenameLeft.Matches(*j))
                {
                    includedLeft = true;
                    break;
                }

            if (!includedLeft)
            {
                i->selectedForSynchronization = false;
                continue;
            }
        }

        if (i->fileDescrRight.objType != FileDescrLine::TYPE_NOTHING)
        {
            bool includedRight = false;
            for (std::vector<Zstring>::const_iterator j = includeList.begin(); j != includeList.end(); ++j)
                if (filenameRight.Matches(*j))
                {
                    includedRight = true;
                    break;
                }

            if (!includedRight)
            {
                i->selectedForSynchronization = false;
                continue;
            }
        }

        //process exclude filters
        if (i->fileDescrLeft.objType != FileDescrLine::TYPE_NOTHING)
        {
            bool excluded = false;
            for (std::vector<Zstring>::const_iterator j = excludeList.begin(); j != excludeList.end(); ++j)
                if (filenameLeft.Matches(*j))
                {
                    excluded = true;
                    break;
                }

            if (excluded)
            {
                i->selectedForSynchronization = false;
                continue;
            }
        }

        if (i->fileDescrRight.objType != FileDescrLine::TYPE_NOTHING)
        {
            bool excluded = false;
            for (std::vector<Zstring>::const_iterator j = excludeList.begin(); j != excludeList.end(); ++j)
                if (filenameRight.Matches(*j))
                {
                    excluded = true;
                    break;
                }

            if (excluded)
            {
                i->selectedForSynchronization = false;
                continue;
            }
        }

        i->selectedForSynchronization = true;
    }
}


void FreeFileSync::removeFilterOnCurrentGridData(FileCompareResult& currentGridData)
{
    //remove all filters on currentGridData
    for (FileCompareResult::iterator i = currentGridData.begin(); i != currentGridData.end(); ++i)
        i->selectedForSynchronization = true;
}


//add(!) all files and subfolder gridlines that are dependent from the directory
void FreeFileSync::addSubElements(std::set<int>& subElements, const FileCompareResult& grid, const FileCompareLine& relevantRow)
{
    Zstring relevantDirectory;

    if (relevantRow.fileDescrLeft.objType == FileDescrLine::TYPE_DIRECTORY)
        relevantDirectory = relevantRow.fileDescrLeft.relativeName + GlobalResources::FILE_NAME_SEPARATOR; //FILE_NAME_SEPARATOR needed to exclude subfile/dirs only
    else if (relevantRow.fileDescrRight.objType == FileDescrLine::TYPE_DIRECTORY)
        relevantDirectory = relevantRow.fileDescrRight.relativeName + GlobalResources::FILE_NAME_SEPARATOR;
    else
        return;

    for (FileCompareResult::const_iterator i = grid.begin(); i != grid.end(); ++i)
        if (    i->fileDescrLeft.relativeName.StartsWith(relevantDirectory) ||
                i->fileDescrRight.relativeName.StartsWith(relevantDirectory))
            subElements.insert(i - grid.begin());
}


class AlwaysWriteToGrid //this class ensures, that the result of the method below is ALWAYS written on exit, even if exceptions are thrown!
{
public:
    AlwaysWriteToGrid(FileCompareResult& grid) :
            gridToWrite(grid)
    {}

    ~AlwaysWriteToGrid()
    {
        removeRowsFromVector(gridToWrite, rowsProcessed);
    }

    void rowProcessedSuccessfully(int nr)
    {
        rowsProcessed.insert(nr);
    }

private:
    FileCompareResult& gridToWrite;
    std::set<int> rowsProcessed;
};


void FreeFileSync::deleteOnGridAndHD(FileCompareResult& grid, const std::set<int>& rowsToDelete, ErrorHandler* errorHandler, const bool useRecycleBin) throw(AbortThisProcess)
{
    //remove deleted rows from grid
    AlwaysWriteToGrid writeOutput(grid);  //ensure that grid is always written to, even if method is exitted via exceptions

    //remove from hd
    for (std::set<int>::iterator i = rowsToDelete.begin(); i != rowsToDelete.end(); ++i)
    {
        const FileCompareLine& currentCmpLine = grid[*i];

        while (true)
        {
            try
            {
                if (currentCmpLine.fileDescrLeft.objType == FileDescrLine::TYPE_FILE)
                    FreeFileSync::removeFile(currentCmpLine.fileDescrLeft.fullName, useRecycleBin);
                else if (currentCmpLine.fileDescrLeft.objType == FileDescrLine::TYPE_DIRECTORY)
                    FreeFileSync::removeDirectory(currentCmpLine.fileDescrLeft.fullName, useRecycleBin);

                if (currentCmpLine.fileDescrRight.objType == FileDescrLine::TYPE_FILE)
                    FreeFileSync::removeFile(currentCmpLine.fileDescrRight.fullName, useRecycleBin);
                else if (currentCmpLine.fileDescrRight.objType == FileDescrLine::TYPE_DIRECTORY)
                    FreeFileSync::removeDirectory(currentCmpLine.fileDescrRight.fullName, useRecycleBin);

                //remove deleted row from grid
                writeOutput.rowProcessedSuccessfully(*i);

                //retrieve all files and subfolder gridlines that are dependent from this deleted entry
                std::set<int> additionalRowsToDelete;
                addSubElements(additionalRowsToDelete, grid, grid[*i]);

                //...and remove them also
                for (std::set<int>::iterator j = additionalRowsToDelete.begin(); j != additionalRowsToDelete.end(); ++j)
                    writeOutput.rowProcessedSuccessfully(*j);

                break;
            }
            catch (const FileError& error)
            {
                //if (updateClass) -> is mandatory
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


wxString FreeFileSync::utcTimeToLocalString(const time_t utcTime)
{
#ifdef FFS_WIN

    //convert ansi C time to FILETIME
    wxULongLong fileTimeLong(utcTime);
    fileTimeLong += wxULongLong(2, 3054539008UL); //timeshift between ansi C time and FILETIME in seconds == 11644473600s
    fileTimeLong *= 10000000;

    FILETIME lastWriteTimeUtc;
    lastWriteTimeUtc.dwLowDateTime  = fileTimeLong.GetLo();
    lastWriteTimeUtc.dwHighDateTime = fileTimeLong.GetHi();

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
    timeinfo = localtime(&utcTime); //convert to local time
    char buffer[50];
    strftime(buffer, 50, "%Y-%m-%d  %H:%M:%S", timeinfo);

    return wxString(buffer);
#endif
}


#ifdef FFS_WIN
inline
Zstring getDriveName(const Zstring& directoryName) //GetVolume() doesn't work under Linux!
{
    const Zstring volumeName = wxFileName(directoryName.c_str()).GetVolume().c_str();
    if (volumeName.empty())
        return Zstring();

    return volumeName + wxFileName::GetVolumeSeparator().c_str() + GlobalResources::FILE_NAME_SEPARATOR;
}
#endif  //FFS_WIN


#ifdef FFS_WIN
bool FreeFileSync::isFatDrive(const Zstring& directoryName)
{
    const Zstring driveName = getDriveName(directoryName);
    if (driveName.empty())
        return false;

    wxChar fileSystem[32];
    if (!GetVolumeInformation(driveName.c_str(), NULL, 0, NULL, NULL, NULL, fileSystem, 32))
        return false;

    return Zstring(fileSystem).StartsWith(wxT("FAT"));
}
#endif


inline
bool sameFileTime(const time_t a, const time_t b)
{
    if (a < b)
        return b - a <= FILE_TIME_PRECISION;
    else
        return a - b <= FILE_TIME_PRECISION;
}


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

*/

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


