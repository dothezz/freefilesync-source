#include "algorithm.h"
#include <wx/intl.h>
#include <stdexcept>
#include <wx/log.h>
#include "library/resources.h"
#include "shared/systemFunctions.h"
#include "shared/fileHandling.h"
#include <wx/msgdlg.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/filepicker.h>
#include "shared/localization.h"
#include "library/filter.h"
#include <boost/bind.hpp>
#include "shared/globalFunctions.h"
#include <wx/scrolwin.h>

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
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

    if (filesize <= 999)
        return wxString::Format(wxT("%i"), static_cast<int>(filesize)) + _(" Byte"); //no decimal places in case of bytes

    double nrOfBytes = filesize;

    nrOfBytes /= 1024;
    wxString unit = _(" kB");
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

    //print just three significant digits: 0,01 | 0,11 | 1,11 | 11,1 | 111

    const unsigned int leadDigitCount = globalFunctions::getDigitCount(static_cast<unsigned int>(nrOfBytes)); //number of digits before decimal point
    if (leadDigitCount == 0 || leadDigitCount > 3)
        return _("Error");

    if (leadDigitCount == 3)
        return wxString::Format(wxT("%i"), static_cast<int>(nrOfBytes)) + unit;
    else if (leadDigitCount == 2)
    {
        wxString output = wxString::Format(wxT("%i"), static_cast<int>(nrOfBytes * 10));
        output.insert(leadDigitCount, FreeFileSync::DECIMAL_POINT);
        return output + unit;
    }
    else //leadDigitCount == 1
    {
        wxString output = wxString::Format(wxT("%03i"), static_cast<int>(nrOfBytes * 100));
        output.insert(leadDigitCount, FreeFileSync::DECIMAL_POINT);
        return output + unit;
    }

    //return wxString::Format(wxT("%.*f"), 3 - leadDigitCount, nrOfBytes) + unit;
}


wxString FreeFileSync::includeNumberSeparator(const wxString& number)
{
    wxString output(number);
    for (int i = output.size() - 3; i > 0; i -= 3)
        output.insert(i,  FreeFileSync::THOUSANDS_SEPARATOR);

    return output;
}


template <class T>
void setDirectoryNameImpl(const wxString& dirname, T* txtCtrl, wxDirPickerCtrl* dirPicker)
{
    txtCtrl->SetValue(dirname);
    const Zstring leftDirFormatted = FreeFileSync::getFormattedDirectoryName(dirname.c_str());
    if (wxDirExists(leftDirFormatted.c_str()))
        dirPicker->SetPath(leftDirFormatted.c_str());
}


void FreeFileSync::setDirectoryName(const wxString& dirname, wxTextCtrl* txtCtrl, wxDirPickerCtrl* dirPicker)
{
    setDirectoryNameImpl(dirname, txtCtrl, dirPicker);
}


void FreeFileSync::setDirectoryName(const wxString& dirname, wxComboBox* txtCtrl, wxDirPickerCtrl* dirPicker)
{
    txtCtrl->SetSelection(wxNOT_FOUND);
    setDirectoryNameImpl(dirname, txtCtrl, dirPicker);
}


void FreeFileSync::scrollToBottom(wxScrolledWindow* scrWindow)
{
    int height = 0;
    scrWindow->GetClientSize(NULL, &height);

    int pixelPerLine = 0;
    scrWindow->GetScrollPixelsPerUnit(NULL, &pixelPerLine);

    if (height > 0 && pixelPerLine > 0)
    {
        const int scrollLinesTotal    = scrWindow->GetScrollLines(wxVERTICAL);
        const int scrollLinesOnScreen = height / pixelPerLine;
        const int scrollPosBottom     = scrollLinesTotal - scrollLinesOnScreen;

        if (0 <= scrollPosBottom)
            scrWindow->Scroll(0, scrollPosBottom);
    }
}


void swapGridsFP(HierarchyObject& hierObj)
{
    //swap directories
    std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), std::mem_fun_ref(&FileSystemObject::swap));
    //swap files
    std::for_each(hierObj.subFiles.begin(), hierObj.subFiles.end(), std::mem_fun_ref(&FileSystemObject::swap));
    //recurse into sub-dirs
    std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), swapGridsFP);
}


void FreeFileSync::swapGrids2(const MainConfiguration& config, FolderComparison& folderCmp)
{
    std::for_each(folderCmp.begin(), folderCmp.end(), swapGridsFP);
    redetermineSyncDirection(config, folderCmp);
}


class Redetermine
{
public:
    Redetermine(const SyncConfiguration& configIn) : config(configIn) {}

    void operator()(FileSystemObject& fsObj) const
    {
        switch (fsObj.getCategory())
        {
        case FILE_LEFT_SIDE_ONLY:
            fsObj.syncDir = convertToSyncDirection(config.exLeftSideOnly);
            break;

        case FILE_RIGHT_SIDE_ONLY:
            fsObj.syncDir = convertToSyncDirection(config.exRightSideOnly);
            break;

        case FILE_RIGHT_NEWER:
            fsObj.syncDir = convertToSyncDirection(config.rightNewer);
            break;

        case FILE_LEFT_NEWER:
            fsObj.syncDir = convertToSyncDirection(config.leftNewer);
            break;

        case FILE_DIFFERENT:
            fsObj.syncDir = convertToSyncDirection(config.different);
            break;

        case FILE_CONFLICT:
            fsObj.syncDir = convertToSyncDirection(config.conflict);
            break;

        case FILE_EQUAL:
            fsObj.syncDir = SYNC_DIR_NONE;
        }
    }
private:
    const SyncConfiguration& config;
};


void FreeFileSync::redetermineSyncDirection(const SyncConfiguration& config, HierarchyObject& baseDirectory)
{
    //do not handle i->selectedForSynchronization in this method! handled in synchronizeFile(), synchronizeFolder()!

    //swap directories
    std::for_each(baseDirectory.subDirs.begin(), baseDirectory.subDirs.end(), Redetermine(config));
    //swap files
    std::for_each(baseDirectory.subFiles.begin(), baseDirectory.subFiles.end(), Redetermine(config));
    //recurse into sub-dirs
    std::for_each(baseDirectory.subDirs.begin(), baseDirectory.subDirs.end(),
                  boost::bind(static_cast<void (*)(const SyncConfiguration&, HierarchyObject&)>(redetermineSyncDirection), boost::cref(config), _1));
}


void FreeFileSync::redetermineSyncDirection(const MainConfiguration& currentMainCfg, FolderComparison& folderCmp)
{
    if (folderCmp.size() == 0)
        return;
    else if (folderCmp.size() != currentMainCfg.additionalPairs.size() + 1)
        throw std::logic_error("Programming Error: Contract violation!");

    //main pair
    redetermineSyncDirection(currentMainCfg.syncConfiguration, folderCmp[0]);

    //add. folder pairs
    for (std::vector<FolderPairEnh>::const_iterator i = currentMainCfg.additionalPairs.begin(); i != currentMainCfg.additionalPairs.end(); ++i)
    {
        redetermineSyncDirection(i->altSyncConfig.get() ? i->altSyncConfig->syncConfiguration : currentMainCfg.syncConfiguration,
                                 folderCmp[i - currentMainCfg.additionalPairs.begin() + 1]);
    }
}


class SetNewDirection
{
public:
    SetNewDirection(SyncDirection newDirection) :
            newDirection_(newDirection) {}

    void operator()(FileSystemObject& fsObj) const
    {
        fsObj.syncDir = newDirection_;
    }

    void setSyncDirectionSub(FreeFileSync::HierarchyObject& hierObj)
    {
        //directories
        std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), *this);
        //files
        std::for_each(hierObj.subFiles.begin(), hierObj.subFiles.end(), *this);
        //recurse into sub-dirs
        std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), boost::bind(&SetNewDirection::setSyncDirectionSub, this, _1));
    }

private:
    SyncDirection newDirection_;
};


void FreeFileSync::setSyncDirection(SyncDirection newDirection, FileSystemObject& fsObj)
{
    fsObj.syncDir = newDirection;

    DirMapping* dirObj = dynamic_cast<DirMapping*>(&fsObj);
    if (dirObj) //process subdirectories also!
        SetNewDirection(newDirection).setSyncDirectionSub(*dirObj);
}


void FreeFileSync::applyFiltering(const MainConfiguration& currentMainCfg, FolderComparison& folderCmp)
{
    assert (folderCmp.size() == 0 || folderCmp.size() == currentMainCfg.additionalPairs.size() + 1);

    if (folderCmp.size() != currentMainCfg.additionalPairs.size() + 1)
        return;

    //main pair
    FreeFileSync::FilterProcess(currentMainCfg.includeFilter, currentMainCfg.excludeFilter).filterAll(folderCmp[0]);

    //add. folder pairs
    for (std::vector<FolderPairEnh>::const_iterator i = currentMainCfg.additionalPairs.begin(); i != currentMainCfg.additionalPairs.end(); ++i)
    {
        HierarchyObject& baseDirectory = folderCmp[i - currentMainCfg.additionalPairs.begin() + 1];

        if (i->altFilter.get())
            FreeFileSync::FilterProcess(i->altFilter->includeFilter, i->altFilter->excludeFilter).filterAll(baseDirectory);
        else
            FreeFileSync::FilterProcess(currentMainCfg.includeFilter, currentMainCfg.excludeFilter).filterAll(baseDirectory);
    }
}


//############################################################################################################
std::pair<wxString, int> FreeFileSync::deleteFromGridAndHDPreview( //assemble message containing all files to be deleted
    const std::vector<FileSystemObject*>& rowsToDeleteOnLeft,
    const std::vector<FileSystemObject*>& rowsToDeleteOnRight,
    const bool deleteOnBothSides)
{
    wxString filesToDelete;
    int totalDelCount = 0;

    if (deleteOnBothSides)
    {
        //mix selected rows from left and right
        std::set<FileSystemObject*> rowsToDelete(rowsToDeleteOnLeft.begin(), rowsToDeleteOnLeft.end());
        rowsToDelete.insert(rowsToDeleteOnRight.begin(), rowsToDeleteOnRight.end());

        for (std::set<FileSystemObject*>::const_iterator i = rowsToDelete.begin(); i != rowsToDelete.end(); ++i)
        {
            const FileSystemObject& currObj = *(*i);

            if (!currObj.isEmpty<LEFT_SIDE>())
            {
                filesToDelete += (currObj.getFullName<LEFT_SIDE>() + wxT("\n")).c_str();
                ++totalDelCount;
            }

            if (!currObj.isEmpty<RIGHT_SIDE>())
            {
                filesToDelete += (currObj.getFullName<RIGHT_SIDE>() + wxT("\n")).c_str();
                ++totalDelCount;
            }

            filesToDelete += wxT("\n");
        }
    }
    else //delete selected files only
    {
        for (std::vector<FileSystemObject*>::const_iterator i = rowsToDeleteOnLeft.begin(); i != rowsToDeleteOnLeft.end(); ++i)
        {
            const FileSystemObject& currObj = *(*i);

            if (!currObj.isEmpty<LEFT_SIDE>())
            {
                filesToDelete += (currObj.getFullName<LEFT_SIDE>() + wxT("\n")).c_str();
                ++totalDelCount;
            }
        }

        for (std::vector<FileSystemObject*>::const_iterator i = rowsToDeleteOnRight.begin(); i != rowsToDeleteOnRight.end(); ++i)
        {
            const FileSystemObject& currObj = *(*i);

            if (!currObj.isEmpty<RIGHT_SIDE>())
            {
                filesToDelete += (currObj.getFullName<RIGHT_SIDE>() + wxT("\n")).c_str();
                ++totalDelCount;
            }
        }
    }

    return std::make_pair(filesToDelete, totalDelCount);
}


template <SelectedSide side>
void deleteFromGridAndHDOneSide(std::vector<FileSystemObject*>& rowsToDeleteOneSide,
                                const bool useRecycleBin,
                                DeleteFilesHandler* statusHandler)
{
    for (std::vector<FileSystemObject*>::const_iterator i = rowsToDeleteOneSide.begin(); i != rowsToDeleteOneSide.end(); ++i)
    {
        if (!(*i)->isEmpty<side>())
        {
            while (true)
            {
                try
                {
                    FileMapping* fileObj = dynamic_cast<FileMapping*>(*i);
                    if (fileObj != NULL)
                    {
                        FreeFileSync::removeFile(fileObj->getFullName<side>(), useRecycleBin);
                        fileObj->removeObject<side>();
                        statusHandler->deletionSuccessful(); //notify successful file/folder deletion
                    }
                    else
                    {
                        DirMapping* dirObj = dynamic_cast<DirMapping*>(*i);
                        if (dirObj != NULL)
                        {
                            FreeFileSync::removeDirectory(dirObj->getFullName<side>(), useRecycleBin);
                            dirObj->removeObject<side>(); //directory: removes recursively!
                            statusHandler->deletionSuccessful(); //notify successful file/folder deletion
                        }
                        else
                            assert(!"It's no file, no dir, what is it then?");
                    }

                    break;
                }
                catch (const FileError& error)
                {
                    DeleteFilesHandler::Response rv = statusHandler->reportError(error.show());

                    if (rv == DeleteFilesHandler::IGNORE_ERROR)
                        break;

                    else if (rv == DeleteFilesHandler::RETRY)
                        ;   //continue in loop
                    else
                        assert (false);
                }
            }
        }
    }
}


class FinalizeDeletion
{
public:
    FinalizeDeletion(FolderComparison& folderCmp, const MainConfiguration& mainConfig) :
            folderCmp_(folderCmp),
            mainConfig_(mainConfig) {}

    ~FinalizeDeletion()
    {
        std::for_each(folderCmp_.begin(), folderCmp_.end(), FileSystemObject::removeEmpty);
        redetermineSyncDirection(mainConfig_, folderCmp_);
    }

private:
    FolderComparison& folderCmp_;
    const MainConfiguration& mainConfig_;
};


void FreeFileSync::deleteFromGridAndHD(FolderComparison& folderCmp,                        //attention: rows will be physically deleted!
                                       std::vector<FileSystemObject*>& rowsToDeleteOnLeft,  //refresh GUI grid after deletion to remove invalid rows
                                       std::vector<FileSystemObject*>& rowsToDeleteOnRight, //all pointers need to be bound!
                                       const bool deleteOnBothSides,
                                       const bool useRecycleBin,
                                       const MainConfiguration& mainConfig,
                                       DeleteFilesHandler* statusHandler)
{
    if (folderCmp.size() == 0)
        return;
    else if (folderCmp.size() != mainConfig.additionalPairs.size() + 1)
        throw std::logic_error("Programming Error: Contract violation!");

    FinalizeDeletion dummy(folderCmp, mainConfig); //ensure cleanup: redetermination of sync-directions and removal of invalid rows


    if (deleteOnBothSides)
    {
        //mix selected rows from left and right (and remove duplicates)
        std::set<FileSystemObject*> temp(rowsToDeleteOnLeft.begin(), rowsToDeleteOnLeft.end());
        temp.insert(rowsToDeleteOnRight.begin(), rowsToDeleteOnRight.end());

        std::vector<FileSystemObject*> rowsToDeleteBothSides(temp.begin(), temp.end());

        deleteFromGridAndHDOneSide<LEFT_SIDE>(rowsToDeleteBothSides,
                                              useRecycleBin,
                                              statusHandler);

        deleteFromGridAndHDOneSide<RIGHT_SIDE>(rowsToDeleteBothSides,
                                               useRecycleBin,
                                               statusHandler);
    }
    else
    {
        deleteFromGridAndHDOneSide<LEFT_SIDE>(rowsToDeleteOnLeft,
                                              useRecycleBin,
                                              statusHandler);

        deleteFromGridAndHDOneSide<RIGHT_SIDE>(rowsToDeleteOnRight,
                                               useRecycleBin,
                                               statusHandler);
    }
}
//############################################################################################################


inline
void writeTwoDigitNumber(unsigned int number, wxString& string)
{
    assert (number < 100);

    string += '0' + number / 10;
    string += '0' + number % 10;
}


inline
void writeFourDigitNumber(unsigned int number, wxString& string)
{
    assert (number < 10000);

    string += '0' + number / 1000;
    number %= 1000;
    string += '0' + number / 100;
    number %= 100;
    string += '0' + number / 10;
    number %= 10;
    string += '0' + number;
}


wxString FreeFileSync::utcTimeToLocalString(const wxLongLong& utcTime, const Zstring& filename)
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
    if (::FileTimeToLocalFileTime( //convert to local time
                &lastWriteTimeUtc, //pointer to UTC file time to convert
                &localFileTime 	   //pointer to converted file time
            ) == 0)
        throw std::runtime_error(std::string((wxString(_("Conversion error:")) + wxT(" FILETIME -> local FILETIME: ") +
                                              wxT("(") + wxULongLong(lastWriteTimeUtc.dwHighDateTime, lastWriteTimeUtc.dwLowDateTime).ToString() + wxT(") ") +
                                              filename.c_str() + wxT("\n\n") + getLastErrorFormatted()).To8BitData()));

    if (localFileTime.dwHighDateTime > 0x7fffffff)
        return _("Error");  //this actually CAN happen if UTC time is just below this border and ::FileTimeToLocalFileTime() adds 2 hours due to DST or whatever!
    //Testcase (UTC): dateHigh = 2147483647 (=0x7fffffff) -> year 30000
    //                dateLow  = 4294967295

    SYSTEMTIME time;
    if (::FileTimeToSystemTime(
                &localFileTime, //pointer to file time to convert
                &time 	        //pointer to structure to receive system time
            ) == 0)
        throw std::runtime_error(std::string((wxString(_("Conversion error:")) + wxT(" local FILETIME -> SYSTEMTIME: ") +
                                              wxT("(") + wxULongLong(localFileTime.dwHighDateTime, localFileTime.dwLowDateTime).ToString() + wxT(") ") +
                                              filename.c_str()  + wxT("\n\n") + getLastErrorFormatted()).To8BitData()));

    //assemble time string (performance optimized)
    wxString formattedTime;
    formattedTime.reserve(20);

    writeFourDigitNumber(time.wYear, formattedTime);
    formattedTime += wxChar('-');
    writeTwoDigitNumber(time.wMonth, formattedTime);
    formattedTime += wxChar('-');
    writeTwoDigitNumber(time.wDay, formattedTime);
    formattedTime += wxChar(' ');
    formattedTime += wxChar(' ');
    writeTwoDigitNumber(time.wHour, formattedTime);
    formattedTime += wxChar(':');
    writeTwoDigitNumber(time.wMinute, formattedTime);
    formattedTime += wxChar(':');
    writeTwoDigitNumber(time.wSecond, formattedTime);

    return formattedTime;

#elif defined FFS_LINUX
    tm* timeinfo;
    const time_t fileTime = utcTime.ToLong();
    timeinfo = localtime(&fileTime); //convert to local time
    char buffer[50];
    strftime(buffer, 50, "%Y-%m-%d  %H:%M:%S", timeinfo);

    return wxString(buffer);
#endif
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
