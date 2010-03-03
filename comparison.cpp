// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "comparison.h"
#include <stdexcept>
#include "shared/globalFunctions.h"
#include <wx/intl.h>
#include <wx/timer.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include "algorithm.h"
#include "ui/util.h"
#include <memory>
#include "shared/stringConv.h"
#include "library/statusHandler.h"
#include "shared/fileHandling.h"
#include "shared/systemFunctions.h"
#include "shared/fileTraverser.h"
#include "library/filter.h"
#include <map>
#include "fileHierarchy.h"
#include <boost/bind.hpp>
#include "library/binary.h"

using namespace FreeFileSync;


std::vector<FreeFileSync::FolderPairCfg> FreeFileSync::extractCompareCfg(const MainConfiguration& mainCfg)
{
    //merge first and additional pairs
    std::vector<FolderPairEnh> allPairs;
    allPairs.push_back(mainCfg.firstPair);
    allPairs.insert(allPairs.end(),
                    mainCfg.additionalPairs.begin(), //add additional pairs
                    mainCfg.additionalPairs.end());

    const BaseFilter::FilterRef globalFilter(new NameFilter(mainCfg.includeFilter, mainCfg.excludeFilter));

    std::vector<FolderPairCfg> output;
    for (std::vector<FolderPairEnh>::const_iterator i = allPairs.begin(); i != allPairs.end(); ++i)
        output.push_back(
            FolderPairCfg(i->leftDirectory,
                          i->rightDirectory,

                          mainCfg.filterIsActive ?
                          combineFilters(globalFilter,
                                         BaseFilter::FilterRef(
                                             new NameFilter(
                                                     i->localFilter.includeFilter,
                                                     i->localFilter.excludeFilter))) :
                          BaseFilter::FilterRef(new NullFilter),

                          i->altSyncConfig.get() ? i->altSyncConfig->syncConfiguration : mainCfg.syncConfiguration));

    return output;
}



class BaseDirCallback;

class DirCallback : public FreeFileSync::TraverseCallback
{
public:
    DirCallback(BaseDirCallback* baseCallback,
                const Zstring& relNameParentPf, //postfixed with FILE_NAME_SEPARATOR!
                DirContainer& output,
                StatusHandler* handler) :
        baseCallback_(baseCallback),
        relNameParentPf_(relNameParentPf),
        output_(output),
        statusHandler(handler) {}

    virtual ~DirCallback() {}

    virtual ReturnValue onFile(const DefaultChar* shortName, const Zstring& fullName, const FileInfo& details);
    virtual ReturnValDir onDir(const DefaultChar* shortName, const Zstring& fullName);
    virtual ReturnValue onError(const wxString& errorText);

private:
    BaseDirCallback* const baseCallback_;
    const Zstring relNameParentPf_;
    DirContainer& output_;
    StatusHandler* const statusHandler;
};



class BaseDirCallback : public DirCallback
{
    friend class DirCallback;
public:
    BaseDirCallback(DirContainer& output,
                    const BaseFilter::FilterRef& filter,
                    StatusHandler* handler) :
        DirCallback(this, Zstring(), output, handler),
        textScanning(wxToZ(wxString(_("Scanning:")) + wxT(" \n"))),
        filterInstance(filter) {}

    virtual TraverseCallback::ReturnValue onFile(const DefaultChar* shortName, const Zstring& fullName, const TraverseCallback::FileInfo& details);

private:
    typedef boost::shared_ptr<const DirCallback> CallbackPointer;

    const Zstring textScanning;
    std::vector<CallbackPointer> callBackBox;  //collection of callback pointers to handle ownership

    const BaseFilter::FilterRef filterInstance; //always bound!
};


TraverseCallback::ReturnValue DirCallback::onFile(const DefaultChar* shortName, const Zstring& fullName, const FileInfo& details)
{
    //assemble status message (performance optimized)  = textScanning + wxT("\"") + fullName + wxT("\"")
    Zstring statusText = baseCallback_->textScanning;
    statusText.reserve(statusText.length() + fullName.length() + 2);
    statusText += DefaultChar('\"');
    statusText += fullName;
    statusText += DefaultChar('\"');

    //update UI/commandline status information
    statusHandler->updateStatusText(statusText);

//------------------------------------------------------------------------------------
    //apply filter before processing (use relative name!)
    if (!baseCallback_->filterInstance->passFileFilter(relNameParentPf_ + shortName))
    {
        statusHandler->requestUiRefresh();
        return TRAVERSING_CONTINUE;
    }

    //warning: for windows retrieveFileID is slow as hell! approximately 3 * 10^-4 s per file!
    //therefore only large files (that take advantage of detection of renaming when synchronizing) should be evaluated!
    //testcase: scanning only files larger than 1 MB results in performance loss of 6%

//#warning this call is NOT acceptable for Linux!
//    //Linux: retrieveFileID takes about 50% longer in VM! (avoidable because of redundant stat() call!)
//    const Utility::FileID fileIdentifier = details.fileSize >= baseCallback_->detectRenameThreshold_ ?
//                                           Utility::retrieveFileID(fullName) :
//                                           Utility::FileID();

    output_.addSubFile(shortName, FileDescriptor(details.lastWriteTimeRaw, details.fileSize));

    //add 1 element to the progress indicator
    statusHandler->updateProcessedData(1, 0); //NO performance issue at all
    //trigger display refresh
    statusHandler->requestUiRefresh();

    return TRAVERSING_CONTINUE;
}


TraverseCallback::ReturnValDir DirCallback::onDir(const DefaultChar* shortName, const Zstring& fullName)
{
    using globalFunctions::FILE_NAME_SEPARATOR;

    //assemble status message (performance optimized)  = textScanning + wxT("\"") + fullName + wxT("\"")
    Zstring statusText = baseCallback_->textScanning;
    statusText.reserve(statusText.length() + fullName.length() + 2);
    statusText += DefaultChar('\"');
    statusText += fullName;
    statusText += DefaultChar('\"');

    //update UI/commandline status information
    statusHandler->updateStatusText(statusText);

//------------------------------------------------------------------------------------
    Zstring relName = relNameParentPf_;
    relName += shortName;

    //apply filter before processing (use relative name!)
    bool subObjMightMatch = true;
    if (!baseCallback_->filterInstance->passDirFilter(relName, &subObjMightMatch))
    {
        statusHandler->requestUiRefresh();

        if (subObjMightMatch)
        {
            DirContainer& subDir = output_.addSubDir(shortName);

            DirCallback* subDirCallback = new DirCallback(baseCallback_, relName += FILE_NAME_SEPARATOR, subDir, statusHandler);
            baseCallback_->callBackBox.push_back(BaseDirCallback::CallbackPointer(subDirCallback)); //handle ownership

            //attention: ensure directory filtering is applied later to exclude actually filtered directories
            return ReturnValDir(Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_CONTINUE>(), subDirCallback);
        }
        else
            return Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_IGNORE>(); //do NOT traverse subdirs
    }


    DirContainer& subDir = output_.addSubDir(shortName);

    //add 1 element to the progress indicator
    statusHandler->updateProcessedData(1, 0);     //NO performance issue at all
    //trigger display refresh
    statusHandler->requestUiRefresh();

    DirCallback* subDirCallback = new DirCallback(baseCallback_, relName += FILE_NAME_SEPARATOR, subDir, statusHandler);
    baseCallback_->callBackBox.push_back(BaseDirCallback::CallbackPointer(subDirCallback)); //handle ownership

    return ReturnValDir(Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_CONTINUE>(), subDirCallback);
}


TraverseCallback::ReturnValue DirCallback::onError(const wxString& errorText)
{
    while (true)
    {
        switch (statusHandler->reportError(errorText))
        {
        case ErrorHandler::IGNORE_ERROR:
            return TRAVERSING_CONTINUE;
        case ErrorHandler::RETRY:
            break; //I have to admit "retry" is a bit of a fake here... at least the user has opportunity to abort!
        }
    }

    return TRAVERSING_CONTINUE; //dummy value
}


TraverseCallback::ReturnValue BaseDirCallback::onFile(
    const DefaultChar* shortName,
    const Zstring& fullName,
    const TraverseCallback::FileInfo& details)
{
    //do not list the database file sync.ffs_db
    if (getSyncDBFilename().cmpFileName(shortName) == 0)
        return TraverseCallback::TRAVERSING_CONTINUE;

    return DirCallback::onFile(shortName, fullName, details);
}


//------------------------------------------------------------------------------------------
struct DirBufferKey
{
    DirBufferKey(const Zstring& dirname,
                 const BaseFilter::FilterRef& filterIn) : //filter interface: always bound by design!
        directoryName(dirname),
        filter(filterIn->isNull() ? //some optimization for "Null" filter
               BaseFilter::FilterRef(new NullFilter) :
               filterIn) {}

    const Zstring directoryName;
    const BaseFilter::FilterRef filter;  //buffering has to consider filtering!

    bool operator<(const DirBufferKey& b) const
    {
        const int rv = directoryName.cmpFileName(b.directoryName);
        if (rv != 0)
            return rv < 0;

        return *filter < *b.filter;
    }
};


//------------------------------------------------------------------------------------------
class CompareProcess::DirectoryBuffer  //buffer multiple scans of the same directories
{
public:
    DirectoryBuffer(const bool traverseDirectorySymlinks,
                    StatusHandler* statusUpdater) :
        m_traverseDirectorySymlinks(traverseDirectorySymlinks),
        m_statusUpdater(statusUpdater) {}

    const DirContainer& getDirectoryDescription(const Zstring& directoryPostfixed, const BaseFilter::FilterRef& filter);

private:
    typedef boost::shared_ptr<DirContainer> DirBufferValue; //exception safety: avoid memory leak
    typedef std::map<DirBufferKey, DirBufferValue> BufferType;

    DirContainer& insertIntoBuffer(const DirBufferKey& newKey);

    BufferType buffer;

    const bool m_traverseDirectorySymlinks;
    StatusHandler* m_statusUpdater;
};
//------------------------------------------------------------------------------------------

DirContainer& CompareProcess::DirectoryBuffer::insertIntoBuffer(const DirBufferKey& newKey)
{
    DirBufferValue baseContainer(new DirContainer);
    buffer.insert(std::make_pair(newKey, baseContainer));

    if (FreeFileSync::dirExists(newKey.directoryName.c_str())) //folder existence already checked in startCompareProcess(): do not treat as error when arriving here!
    {
        std::auto_ptr<TraverseCallback> traverser(new BaseDirCallback(*baseContainer,
                newKey.filter,
                m_statusUpdater));

        //get all files and folders from directoryPostfixed (and subdirectories)
        traverseFolder(newKey.directoryName,
                       m_traverseDirectorySymlinks,
                       traverser.get()); //exceptions may be thrown!
    }
    return *baseContainer.get();
}


const DirContainer& CompareProcess::DirectoryBuffer::getDirectoryDescription(
    const Zstring& directoryPostfixed,
    const BaseFilter::FilterRef& filter)
{
    const DirBufferKey searchKey(directoryPostfixed, filter);

    BufferType::const_iterator entryFound = buffer.find(searchKey);
    if (entryFound != buffer.end())
        return *entryFound->second.get(); //entry found in buffer; return
    else
        return insertIntoBuffer(searchKey); //entry not found; create new one
}


//------------------------------------------------------------------------------------------
void foldersAreValidForComparison(const std::vector<FolderPairCfg>& folderPairsForm, StatusHandler* statusUpdater)
{
    bool checkEmptyDirnameActive = true; //check for empty dirs just once
    const wxString additionalInfo = _("You can ignore the error to consider not existing directories as empty.");

    for (std::vector<FolderPairCfg>::const_iterator i = folderPairsForm.begin(); i != folderPairsForm.end(); ++i)
    {
        //check if folder name is empty
        if (i->leftDirectory.empty() || i->rightDirectory.empty())
        {
            if (checkEmptyDirnameActive)
            {
                checkEmptyDirnameActive = false;
                while (true)
                {
                    const ErrorHandler::Response rv = statusUpdater->reportError(wxString(_("At least one directory input field is empty.")) + wxT(" \n\n") +
                                                      + wxT("(") + additionalInfo + wxT(")"));
                    if (rv == ErrorHandler::IGNORE_ERROR)
                        break;
                    else if (rv == ErrorHandler::RETRY)
                        ;  //continue with loop
                    else
                        throw std::logic_error("Programming Error: Unknown return value! (1)");
                }
            }
        }

        //check if folders exist
        if (!i->leftDirectory.empty())
            while (!FreeFileSync::dirExists(i->leftDirectory.c_str()))
            {
                ErrorHandler::Response rv = statusUpdater->reportError(wxString(_("Directory does not exist:")) + wxT(" \n") +
                                            wxT("\"") + zToWx(i->leftDirectory) + wxT("\"") + wxT("\n\n") +
                                            additionalInfo + wxT(" ") + FreeFileSync::getLastErrorFormatted());
                if (rv == ErrorHandler::IGNORE_ERROR)
                    break;
                else if (rv == ErrorHandler::RETRY)
                    ;  //continue with loop
                else
                    throw std::logic_error("Programming Error: Unknown return value! (2)");
            }

        if (!i->rightDirectory.empty())
            while (!FreeFileSync::dirExists(i->rightDirectory.c_str()))
            {
                ErrorHandler::Response rv = statusUpdater->reportError(wxString(_("Directory does not exist:")) + wxT("\n") +
                                            wxT("\"") + zToWx(i->rightDirectory) + wxT("\"") + wxT("\n\n") +
                                            additionalInfo + wxT(" ") + FreeFileSync::getLastErrorFormatted());
                if (rv == ErrorHandler::IGNORE_ERROR)
                    break;
                else if (rv == ErrorHandler::RETRY)
                    ;  //continue with loop
                else
                    throw std::logic_error("Programming Error: Unknown return value! (3)");
            }
    }
}


bool dependencyExists(const std::set<Zstring>& folders, const Zstring& newFolder, wxString& warningMessage)
{
    for (std::set<Zstring>::const_iterator i = folders.begin(); i != folders.end(); ++i)
    {
        const size_t commonLen = std::min(newFolder.length(), i->length());
        if (Zstring(newFolder.c_str(), commonLen).cmpFileName(Zstring(i->c_str(), commonLen)) == 0) //test wheter i begins with newFolder or the other way round
        {
            warningMessage = wxString(_("Directories are dependent! Be careful when setting up synchronization rules:")) + wxT("\n") +
                             wxT("\"") + zToWx(*i) + wxT("\"\n") +
                             wxT("\"") + zToWx(newFolder) + wxT("\"");
            return true;
        }
    }
    return false;
}


bool foldersHaveDependencies(const std::vector<FolderPairCfg>& folderPairsFrom, wxString& warningMessage)
{
    warningMessage.Clear();

    std::set<Zstring> folders;
    for (std::vector<FolderPairCfg>::const_iterator i = folderPairsFrom.begin(); i != folderPairsFrom.end(); ++i)
    {
        if (!i->leftDirectory.empty()) //empty folders names might be accepted by user
        {
            if (dependencyExists(folders, i->leftDirectory, warningMessage))
                return true;
            folders.insert(i->leftDirectory);
        }

        if (!i->rightDirectory.empty()) //empty folders names might be accepted by user
        {
            if (dependencyExists(folders, i->rightDirectory, warningMessage))
                return true;
            folders.insert(i->rightDirectory);
        }
    }

    return false;
}


CompareProcess::CompareProcess(const bool traverseSymLinks,
                               const unsigned int fileTimeTol,
                               const bool ignoreOneHourDiff,
                               xmlAccess::OptionalDialogs& warnings,
                               StatusHandler* handler) :
    fileTimeTolerance(fileTimeTol),
    ignoreOneHourDifference(ignoreOneHourDiff),
    m_warnings(warnings),
    statusUpdater(handler),
    txtComparingContentOfFiles(wxToZ(_("Comparing content of files %x")).Replace(DefaultStr("%x"), DefaultStr("\n\"%x\""), false))
{
    directoryBuffer.reset(new DirectoryBuffer(traverseSymLinks, handler));
}


//callback implementation
class CmpCallbackImpl : public CompareCallback
{
public:
    CmpCallbackImpl(StatusHandler* handler, wxLongLong& bytesComparedLast) :
        m_handler(handler),
        m_bytesComparedLast(bytesComparedLast) {}

    virtual void updateCompareStatus(const wxLongLong& totalBytesTransferred)
    {
        //called every 512 kB

        //inform about the (differential) processed amount of data
        m_handler->updateProcessedData(0, totalBytesTransferred - m_bytesComparedLast);
        m_bytesComparedLast = totalBytesTransferred;

        m_handler->requestUiRefresh(); //exceptions may be thrown here!
    }

private:
    StatusHandler* m_handler;
    wxLongLong& m_bytesComparedLast;
};


bool filesHaveSameContentUpdating(const Zstring& filename1, const Zstring& filename2, const wxULongLong& totalBytesToCmp, StatusHandler* handler)
{
    wxLongLong bytesComparedLast; //amount of bytes that have been compared and communicated to status handler

    CmpCallbackImpl callback(handler, bytesComparedLast);

    bool sameContent = true;
    try
    {
        sameContent = filesHaveSameContent(filename1, filename2, &callback);
    }
    catch (...)
    {
        //error situation: undo communication of processed amount of data
        handler->updateProcessedData(0, bytesComparedLast * -1);

        throw;
    }

    //inform about the (remaining) processed amount of data
    handler->updateProcessedData(0, globalFunctions::convertToSigned(totalBytesToCmp) - bytesComparedLast);

    return sameContent;
}


struct ToBeRemoved
{
    bool operator()(const DirMapping& dirObj) const
    {
        return !dirObj.isActive() && dirObj.subDirs.size() == 0 && dirObj.subFiles.size() == 0;
    }
};


class RemoveFilteredDirs
{
public:
    RemoveFilteredDirs(const BaseFilter& filterProc) :
        filterProc_(filterProc) {}

    void execute(HierarchyObject& hierObj)
    {
        //process subdirs recursively
        std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), *this);

        //remove superfluous directories
        hierObj.subDirs.erase(std::remove_if(hierObj.subDirs.begin(), hierObj.subDirs.end(), ::ToBeRemoved()), hierObj.subDirs.end());
    }

private:
    template<typename Iterator, typename Function>
    friend Function std::for_each(Iterator, Iterator, Function);

    void operator()(DirMapping& dirObj)
    {
        dirObj.setActive(filterProc_.passDirFilter(dirObj.getObjRelativeName().c_str(), NULL)); //subObjMightMatch is always true in this context!
        execute(dirObj);
    }

    const BaseFilter& filterProc_;
};


inline
void formatPair(FolderPairCfg& input)
{
    //ensure they end with globalFunctions::FILE_NAME_SEPARATOR and replace macros
    input.leftDirectory  = FreeFileSync::getFormattedDirectoryName(input.leftDirectory);
    input.rightDirectory = FreeFileSync::getFormattedDirectoryName(input.rightDirectory);
}




//#############################################################################################################################

void CompareProcess::startCompareProcess(const std::vector<FolderPairCfg>& directoryPairs,
        const CompareVariant cmpVar,
        FolderComparison& output)
{
#ifndef __WXDEBUG__
    wxLogNull noWxLogs; //hide wxWidgets log messages in release build
#endif

    //PERF_START;

    //init process: keep at beginning so that all gui elements are initialized properly
    statusUpdater->initNewProcess(-1, 0, StatusHandler::PROCESS_SCANNING); //it's not known how many files will be scanned => -1 objects

    //format directory pairs: ensure they end with globalFunctions::FILE_NAME_SEPARATOR and replace macros!
    std::vector<FolderPairCfg> directoryPairsFormatted = directoryPairs;
    std::for_each(directoryPairsFormatted.begin(), directoryPairsFormatted.end(), formatPair);

    //-------------------some basic checks:------------------------------------------

    //check if folders are valid
    foldersAreValidForComparison(directoryPairsFormatted, statusUpdater);

    //check if folders have dependencies
    {
        wxString warningMessage;
        if (foldersHaveDependencies(directoryPairsFormatted, warningMessage))
            statusUpdater->reportWarning(warningMessage.c_str(), m_warnings.warningDependentFolders);
    }

    //-------------------end of basic checks------------------------------------------


    try
    {
        FolderComparison output_tmp; //write to output not before END of process!
        switch (cmpVar)
        {
        case CMP_BY_TIME_SIZE:
            compareByTimeSize(directoryPairsFormatted, output_tmp);
            break;
        case CMP_BY_CONTENT:
            compareByContent(directoryPairsFormatted, output_tmp);
            break;
        }


        assert (output_tmp.size() == directoryPairsFormatted.size());

        for (FolderComparison::iterator j = output_tmp.begin(); j != output_tmp.end(); ++j)
        {
            const FolderPairCfg& fpCfg = directoryPairsFormatted[j - output_tmp.begin()];

            //attention: some filtered directories are still in the comparison result! (see include filter handling!)
            if (!fpCfg.filter->isNull()) //let's filter them now... (and remove those that contain excluded elements only)
            {
                //filters and removes all excluded directories (but keeps those serving as parent folders)
                RemoveFilteredDirs(*fpCfg.filter).execute(*j);
            }

            //set initial sync-direction
            class RedetermineCallback : public DeterminationProblem
            {
            public:
                RedetermineCallback(bool& warningSyncDatabase, StatusHandler& statusUpdater) :
                    warningSyncDatabase_(warningSyncDatabase),
                    statusUpdater_(statusUpdater) {}

                virtual void reportWarning(const wxString& text)
                {
                    statusUpdater_.reportWarning(text, warningSyncDatabase_);
                }
            private:
                bool& warningSyncDatabase_;
                StatusHandler& statusUpdater_;
            } redetCallback(m_warnings.warningSyncDatabase, *statusUpdater);

            FreeFileSync::redetermineSyncDirection(fpCfg.syncConfiguration, *j, &redetCallback);
        }

        //only if everything was processed correctly output is written to!
        //note: output mustn't change during this process to be in sync with GUI grid view!!!
        output_tmp.swap(output);
    }
    catch (const std::exception& e)
    {
        if (dynamic_cast<const std::bad_alloc*>(&e) != NULL)
            statusUpdater->reportFatalError(wxString(_("System out of memory!")) + wxT(" ") + wxString::FromAscii(e.what()));
        else
            statusUpdater->reportFatalError(wxString::FromAscii(e.what()));
        return; //should be obsolete!
    }
}

//--------------------assemble conflict descriptions---------------------------

//check for very old dates or dates in the future
wxString getConflictInvalidDate(const Zstring& fileNameFull, const wxLongLong& utcTime)
{
    wxString msg = _("File %x has an invalid date!");
    msg.Replace(wxT("%x"), wxString(wxT("\"")) + zToWx(fileNameFull) + wxT("\""));
    msg += wxString(wxT("\n\n")) + _("Date") + wxT(": ") + utcTimeToLocalString(utcTime, fileNameFull);
    return wxString(_("Conflict detected:")) + wxT("\n") + msg;
}


namespace
{
inline
void makeSameLength(wxString& first, wxString& second)
{
    const size_t maxPref = std::max(first.length(), second.length());
    first.Pad(maxPref - first.length(), wxT(' '), true);
    second.Pad(maxPref - second.length(), wxT(' '), true);
}
}


//check for changed files with same modification date
wxString getConflictSameDateDiffSize(const FileMapping& fileObj)
{
    //some beautification...
    wxString left = wxString(_("Left")) + wxT(": ");
    wxString right = wxString(_("Right")) + wxT(": ");
    makeSameLength(left, right);

    wxString msg = _("Files %x have the same date but a different size!");
    msg.Replace(wxT("%x"), wxString(wxT("\"")) + zToWx(fileObj.getRelativeName<LEFT_SIDE>()) + wxT("\""));
    msg += wxT("\n\n");
    msg += left + wxT("\t") + _("Date") + wxT(": ") + utcTimeToLocalString(fileObj.getLastWriteTime<LEFT_SIDE>(),
            fileObj.getFullName<LEFT_SIDE>()) + wxT(" \t") + _("Size") + wxT(": ") + fileObj.getFileSize<LEFT_SIDE>().ToString() + wxT("\n");
    msg += right + wxT("\t") + _("Date") + wxT(": ") + utcTimeToLocalString(fileObj.getLastWriteTime<RIGHT_SIDE>(),
            fileObj.getFullName<RIGHT_SIDE>()) + wxT(" \t") + _("Size") + wxT(": ") + fileObj.getFileSize<RIGHT_SIDE>().ToString();
    return wxString(_("Conflict detected:")) + wxT("\n") + msg;
}


//check for files that have a difference in file modification date below 1 hour when DST check is active
wxString getConflictChangeWithinHour(const FileMapping& fileObj)
{
    //some beautification...
    wxString left = wxString(_("Left")) + wxT(": ");
    wxString right = wxString(_("Right")) + wxT(": ");
    makeSameLength(left, right);

    wxString msg = _("Files %x have a file time difference of less than 1 hour!\n\nIt's not safe to decide which one is newer due to Daylight Saving Time issues.");
    msg += wxString(wxT("\n")) + _("(Note that only FAT/FAT32 drives are affected by this problem!\nIn all other cases you can disable the setting \"ignore 1-hour difference\".)");
    msg.Replace(wxT("%x"), wxString(wxT("\"")) + zToWx(fileObj.getRelativeName<LEFT_SIDE>()) + wxT("\""));
    msg += wxT("\n\n");
    msg += left + wxT("\t") + _("Date") + wxT(": ") + utcTimeToLocalString(fileObj.getLastWriteTime<LEFT_SIDE>(), fileObj.getFullName<LEFT_SIDE>()) + wxT("\n");
    msg += right + wxT("\t") + _("Date") + wxT(": ") + utcTimeToLocalString(fileObj.getLastWriteTime<RIGHT_SIDE>(), fileObj.getFullName<RIGHT_SIDE>());
    return wxString(_("Conflict detected:")) + wxT("\n") + msg;
}

//-----------------------------------------------------------------------------
inline
bool sameFileTime(const wxLongLong& a, const wxLongLong& b, const unsigned int tolerance)
{
    if (a < b)
        return b - a <= tolerance;
    else
        return a - b <= tolerance;
}


void CompareProcess::compareByTimeSize(const std::vector<FolderPairCfg>& directoryPairsFormatted, FolderComparison& output)
{
    output.reserve(output.size() + directoryPairsFormatted.size());

    //process one folder pair after each other
    for (std::vector<FolderPairCfg>::const_iterator pair = directoryPairsFormatted.begin(); pair != directoryPairsFormatted.end(); ++pair)
    {
        BaseDirMapping newEntry(pair->leftDirectory,
                                pair->rightDirectory,
                                pair->filter);
        output.push_back(newEntry); //attention: push_back() copies by value!!! performance: append BEFORE writing values into fileCmp!

        //do basis scan and retrieve files existing on both sides as "compareCandidates"
        std::vector<FileMapping*> compareCandidates;
        performBaseComparison(output.back(), compareCandidates);

        //PERF_START;

        //categorize files that exist on both sides
        for (std::vector<FileMapping*>::iterator i = compareCandidates.begin(); i != compareCandidates.end(); ++i)
        {
            FileMapping* const line = *i;
            if (line->getLastWriteTime<LEFT_SIDE>() != line->getLastWriteTime<RIGHT_SIDE>())
            {
                //number of seconds since Jan 1st 1970 + 1 year (needn't be too precise)
                static const long oneYearFromNow = wxGetUTCTime() + 365 * 24 * 3600;

                //check for erroneous dates (but only if dates are not (EXACTLY) the same)
                if (    line->getLastWriteTime<LEFT_SIDE>()  < 0 || //earlier than Jan 1st 1970
                        line->getLastWriteTime<RIGHT_SIDE>() < 0 || //earlier than Jan 1st 1970
                        line->getLastWriteTime<LEFT_SIDE>()  > oneYearFromNow || //dated more than one year in future
                        line->getLastWriteTime<RIGHT_SIDE>() > oneYearFromNow)   //dated more than one year in future
                {
                    if (line->getLastWriteTime<LEFT_SIDE>() < 0 || line->getLastWriteTime<LEFT_SIDE>() > oneYearFromNow)
                        line->setCategoryConflict(getConflictInvalidDate(line->getFullName<LEFT_SIDE>(), line->getLastWriteTime<LEFT_SIDE>()));
                    else
                        line->setCategoryConflict(getConflictInvalidDate(line->getFullName<RIGHT_SIDE>(), line->getLastWriteTime<RIGHT_SIDE>()));
                }
                else //from this block on all dates are at least "valid"
                {
                    //last write time may differ by up to 2 seconds (NTFS vs FAT32)
                    if (sameFileTime(line->getLastWriteTime<LEFT_SIDE>(), line->getLastWriteTime<RIGHT_SIDE>(), fileTimeTolerance))
                    {
                        if (line->getFileSize<LEFT_SIDE>() == line->getFileSize<RIGHT_SIDE>())
                            line->setCategory<FILE_EQUAL>();
                        else
                            line->setCategoryConflict(getConflictSameDateDiffSize(*line)); //same date, different filesize
                    }
                    else
                    {
                        //finally: DST +/- 1-hour check: test if time diff is exactly +/- 1-hour (respecting 2 second FAT precision)
                        if (ignoreOneHourDifference && sameFileTime(line->getLastWriteTime<LEFT_SIDE>(), line->getLastWriteTime<RIGHT_SIDE>(), 3600 + 2))
                        {
                            //date diff < 1 hour is a conflict: it's not safe to determine which file is newer
                            if (sameFileTime(line->getLastWriteTime<LEFT_SIDE>(), line->getLastWriteTime<RIGHT_SIDE>(), 3600 - 2 - 1))
                                line->setCategoryConflict(getConflictChangeWithinHour(*line));
                            else //exact +/- 1-hour detected: treat as equal
                            {
                                if (line->getFileSize<LEFT_SIDE>() == line->getFileSize<RIGHT_SIDE>())
                                    line->setCategory<FILE_EQUAL>();
                                else
                                    line->setCategoryConflict(getConflictSameDateDiffSize(*line)); //same date, different filesize
                            }
                        }
                        else
                        {
                            if (line->getLastWriteTime<LEFT_SIDE>() < line->getLastWriteTime<RIGHT_SIDE>())
                                line->setCategory<FILE_RIGHT_NEWER>();
                            else
                                line->setCategory<FILE_LEFT_NEWER>();
                        }
                    }
                }
            }
            else //same write time
            {
                if (line->getFileSize<LEFT_SIDE>() == line->getFileSize<RIGHT_SIDE>())
                    line->setCategory<FILE_EQUAL>();
                else
                    line->setCategoryConflict(getConflictSameDateDiffSize(*line)); //same date, different filesize
            }
        }
    }
}


wxULongLong getBytesToCompare(const std::vector<FileMapping*>& rowsToCompare)
{
    wxULongLong dataTotal;

    for (std::vector<FileMapping*>::const_iterator j = rowsToCompare.begin(); j != rowsToCompare.end(); ++j)
        dataTotal += (*j)->getFileSize<LEFT_SIDE>();  //left and right filesizes should be the same

    return dataTotal * 2;
}


void CompareProcess::compareByContent(const std::vector<FolderPairCfg>& directoryPairsFormatted, FolderComparison& output)
{
    //PERF_START;
    std::vector<FileMapping*> compareCandidates;

    //attention: make sure pointers in "compareCandidates" remain valid!!!
    output.reserve(output.size() + directoryPairsFormatted.size());

    //process one folder pair after each other
    for (std::vector<FolderPairCfg>::const_iterator pair = directoryPairsFormatted.begin(); pair != directoryPairsFormatted.end(); ++pair)
    {
        BaseDirMapping newEntry(pair->leftDirectory,
                                pair->rightDirectory,
                                pair->filter);
        output.push_back(newEntry); //attention: push_back() copies by value!!! performance: append BEFORE writing values into fileCmp!

        //do basis scan and retrieve candidates for binary comparison (files existing on both sides)
        performBaseComparison(output.back(), compareCandidates);
    }

    //finish categorization...
    std::vector<FileMapping*> filesToCompareBytewise;

    //content comparison of file content happens AFTER finding corresponding files
    //in order to separate into two processes (scanning and comparing)

    for (std::vector<FileMapping*>::iterator i = compareCandidates.begin(); i != compareCandidates.end(); ++i)
    {
        //pre-check: files have different content if they have a different filesize
        if ((*i)->getFileSize<LEFT_SIDE>() != (*i)->getFileSize<RIGHT_SIDE>())
            (*i)->setCategory<FILE_DIFFERENT>();
        else
            filesToCompareBytewise.push_back(*i);
    }



    const size_t objectsTotal    = filesToCompareBytewise.size() * 2;
    const wxULongLong bytesTotal = getBytesToCompare(filesToCompareBytewise);

    statusUpdater->initNewProcess(objectsTotal,
                                  globalFunctions::convertToSigned(bytesTotal),
                                  StatusHandler::PROCESS_COMPARING_CONTENT);

    //compare files (that have same size) bytewise...
    for (std::vector<FileMapping*>::const_iterator j = filesToCompareBytewise.begin(); j != filesToCompareBytewise.end(); ++j)
    {
        FileMapping* const gridline = *j;

        Zstring statusText = txtComparingContentOfFiles;
        statusText.Replace(DefaultStr("%x"), gridline->getRelativeName<LEFT_SIDE>(), false);
        statusUpdater->updateStatusText(statusText);

        //check files that exist in left and right model but have different content
        while (true)
        {
            //trigger display refresh
            statusUpdater->requestUiRefresh();

            try
            {
                if (filesHaveSameContentUpdating(gridline->getFullName<LEFT_SIDE>(),
                                                 gridline->getFullName<RIGHT_SIDE>(),
                                                 gridline->getFileSize<LEFT_SIDE>() * 2,
                                                 statusUpdater))
                    gridline->setCategory<FILE_EQUAL>();
                else
                    gridline->setCategory<FILE_DIFFERENT>();

                statusUpdater->updateProcessedData(2, 0); //processed data is communicated in subfunctions!
                break;
            }
            catch (FileError& error)
            {
                ErrorHandler::Response rv = statusUpdater->reportError(error.show());
                if (rv == ErrorHandler::IGNORE_ERROR)
                {
                    gridline->setCategoryConflict(wxString(_("Conflict detected:")) + wxT("\n") + _("Comparing files by content failed."));
                    break;
                }

                else if (rv == ErrorHandler::RETRY)
                    ;   //continue with loop
                else
                    throw std::logic_error("Programming Error: Unknown return value!");
            }
        }
    }
}


class MergeSides
{
public:
    MergeSides(std::vector<FileMapping*>& appendUndefinedOut) :
        appendUndefined(appendUndefinedOut) {}

    void execute(const DirContainer& leftSide, const DirContainer& rightSide, HierarchyObject& output);

private:
    template <SelectedSide side>
    void fillOneSide(const DirContainer& dirCont, HierarchyObject& output);

    std::vector<FileMapping*>& appendUndefined;
};


template <>
void MergeSides::fillOneSide<LEFT_SIDE>(const DirContainer& dirCont, HierarchyObject& output)
{
    //reserve() fulfills two task here: 1. massive performance improvement! 2. ensure references in appendUndefined remain valid!
    output.subFiles.reserve(dirCont.getSubFiles().size());
    output.subDirs.reserve( dirCont.getSubDirs(). size());

    for (DirContainer::SubFileList::const_iterator i = dirCont.getSubFiles().begin(); i != dirCont.getSubFiles().end(); ++i)
        output.addSubFile(i->second.getData(), i->first);

    for (DirContainer::SubDirList::const_iterator i = dirCont.getSubDirs().begin(); i != dirCont.getSubDirs().end(); ++i)
    {
        DirMapping& newDirMap = output.addSubDir(true, i->first, false);
        fillOneSide<LEFT_SIDE>(i->second, newDirMap); //recurse into subdirectories
    }
}


template <>
void MergeSides::fillOneSide<RIGHT_SIDE>(const DirContainer& dirCont, HierarchyObject& output)
{
    //reserve() fulfills two task here: 1. massive performance improvement! 2. ensure references in appendUndefined remain valid!
    output.subFiles.reserve(dirCont.getSubFiles().size());
    output.subDirs.reserve( dirCont.getSubDirs(). size());

    for (DirContainer::SubFileList::const_iterator i = dirCont.getSubFiles().begin(); i != dirCont.getSubFiles().end(); ++i)
        output.addSubFile(i->first, i->second.getData());

    for (DirContainer::SubDirList::const_iterator i = dirCont.getSubDirs().begin(); i != dirCont.getSubDirs().end(); ++i)
    {
        DirMapping& newDirMap = output.addSubDir(false, i->first, true);
        fillOneSide<RIGHT_SIDE>(i->second, newDirMap); //recurse into subdirectories
    }
}


void MergeSides::execute(const DirContainer& leftSide, const DirContainer& rightSide, HierarchyObject& output)
{
    //ATTENTION: HierarchyObject::retrieveById() can only work correctly if the following conditions are fulfilled:
    //1. on each level, files are added first, then directories (=> file id < dir id)
    //2. when a directory is added, all subdirectories must be added immediately (recursion) before the next dir on this level is added
    //3. entries may be deleted but NEVER new ones inserted!!!
    //=> this allows for a quasi-binary search by id!

    //reserve() fulfills two task here: 1. massive performance improvement! 2. ensure references in appendUndefined remain valid!
    output.subFiles.reserve(leftSide.getSubFiles().size() + rightSide.getSubFiles().size()); //assume worst case!
    output.subDirs.reserve( leftSide.getSubDirs().size()  + rightSide.getSubDirs().size());  //

    for (DirContainer::SubFileList::const_iterator i = leftSide.getSubFiles().begin(); i != leftSide.getSubFiles().end(); ++i)
    {
        DirContainer::SubFileList::const_iterator j = rightSide.getSubFiles().find(i->first);

        //find files that exist on left but not on right
        if (j == rightSide.getSubFiles().end())
            output.addSubFile(i->second.getData(), i->first);
        //find files that exist on left and right
        else
        {
            appendUndefined.push_back(
                &output.addSubFile(i->second.getData(), i->first, FILE_EQUAL, j->second.getData())); //FILE_EQUAL is just a dummy-value here
        }
    }

    //find files that exist on right but not on left
    for (DirContainer::SubFileList::const_iterator j = rightSide.getSubFiles().begin(); j != rightSide.getSubFiles().end(); ++j)
    {
        if (leftSide.getSubFiles().find(j->first) == leftSide.getSubFiles().end())
            output.addSubFile(j->first, j->second.getData());
    }


//-----------------------------------------------------------------------------------------------
    for (DirContainer::SubDirList::const_iterator i = leftSide.getSubDirs().begin(); i != leftSide.getSubDirs().end(); ++i)
    {
        DirContainer::SubDirList::const_iterator j = rightSide.getSubDirs().find(i->first);

        //find directories that exist on left but not on right
        if (j == rightSide.getSubDirs().end())
        {
            DirMapping& newDirMap = output.addSubDir(true, i->first, false);
            fillOneSide<LEFT_SIDE>(i->second, newDirMap); //recurse into subdirectories
        }
        else //directories that exist on both sides
        {
            DirMapping& newDirMap = output.addSubDir(true, i->first, true);
            execute(i->second, j->second, newDirMap); //recurse into subdirectories
        }
    }

    //find directories that exist on right but not on left
    for (DirContainer::SubDirList::const_iterator j = rightSide.getSubDirs().begin(); j != rightSide.getSubDirs().end(); ++j)
    {
        if (leftSide.getSubDirs().find(j->first) == leftSide.getSubDirs().end())
        {
            DirMapping& newDirMap = output.addSubDir(false, j->first, true);
            fillOneSide<RIGHT_SIDE>(j->second, newDirMap); //recurse into subdirectories
        }
    }
}


void CompareProcess::performBaseComparison(BaseDirMapping& output, std::vector<FileMapping*>& appendUndefined)
{
    assert(output.subDirs.empty());
    assert(output.subFiles.empty());

    //PERF_START;

    //scan directories
    const DirContainer& directoryLeft = directoryBuffer->getDirectoryDescription(
                                            output.getBaseDir<LEFT_SIDE>(),
                                            output.getFilter());

    const DirContainer& directoryRight = directoryBuffer->getDirectoryDescription(
            output.getBaseDir<RIGHT_SIDE>(),
            output.getFilter());


    statusUpdater->updateStatusText(wxToZ(_("Generating file list...")));
    statusUpdater->forceUiRefresh(); //keep total number of scanned files up to date

    //PERF_STOP;

    MergeSides(appendUndefined).execute(directoryLeft, directoryRight, output);
}

