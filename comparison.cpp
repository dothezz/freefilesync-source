#include "comparison.h"
#include <stdexcept>
#include "shared/globalFunctions.h"
#include <wx/intl.h>
#include <wx/timer.h>
#include <wx/ffile.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include "algorithm.h"
#include <wx/thread.h>
#include <memory>
#include "library/statusHandler.h"
#include "shared/fileHandling.h"
#include "shared/systemFunctions.h"
#include "shared/fileTraverser.h"
#include "library/filter.h"
#include <map>
#include "fileHierarchy.h"
#include <boost/bind.hpp>

using namespace FreeFileSync;


std::vector<FreeFileSync::FolderPairCfg> FreeFileSync::extractCompareCfg(const MainConfiguration& mainCfg)
{
    std::vector<FolderPairCfg> output;

    //add main pair
    output.push_back(
        FolderPairCfg(mainCfg.mainFolderPair.leftDirectory,
                      mainCfg.mainFolderPair.rightDirectory,
                      mainCfg.filterIsActive,
                      mainCfg.includeFilter,
                      mainCfg.excludeFilter,
                      mainCfg.syncConfiguration));

    //add additional pairs
    for (std::vector<FolderPairEnh>::const_iterator i = mainCfg.additionalPairs.begin(); i != mainCfg.additionalPairs.end(); ++i)
        output.push_back(
            FolderPairCfg(i->leftDirectory,
                          i->rightDirectory,
                          mainCfg.filterIsActive,
                          i->altFilter.get() ? i->altFilter->includeFilter : mainCfg.includeFilter,
                          i->altFilter.get() ? i->altFilter->excludeFilter : mainCfg.excludeFilter,
                          i->altSyncConfig.get() ? i->altSyncConfig->syncConfiguration : mainCfg.syncConfiguration));

    return output;
}







template <bool filterActive>
class BaseDirCallback;


template <bool filterActive>
class DirCallback : public FreeFileSync::TraverseCallback
{
public:
    DirCallback(BaseDirCallback<filterActive>* baseCallback,
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
    BaseDirCallback<filterActive>* const baseCallback_;
    const Zstring relNameParentPf_;
    DirContainer& output_;
    StatusHandler* const statusHandler;
};



template <bool filterActive>
class BaseDirCallback : public DirCallback<filterActive>
{
    friend class DirCallback<filterActive>;
public:
    BaseDirCallback(DirContainer& output, const FilterProcess* filter, StatusHandler* handler) :
            DirCallback<filterActive>(this, Zstring(), output, handler),
            textScanning(Zstring(_("Scanning:")) + wxT(" \n")),
            filterInstance(filter) {}

    virtual TraverseCallback::ReturnValDir onDir(const DefaultChar* shortName, const Zstring& fullName);

private:
    typedef boost::shared_ptr<const DirCallback<filterActive> > CallbackPointer;

    const Zstring textScanning;
    std::vector<CallbackPointer> callBackBox;  //collection of callback pointers to handle ownership
    const FilterProcess* const filterInstance; //must NOT be NULL if (filterActive)
};


template <bool filterActive>
TraverseCallback::ReturnValue DirCallback<filterActive>::onFile(const DefaultChar* shortName, const Zstring& fullName, const FileInfo& details)
{
    //apply filter before processing (use relative name!)
    if (filterActive)
    {
        if (!baseCallback_->filterInstance->passFileFilter(relNameParentPf_ + shortName))
        {
            statusHandler->requestUiRefresh();
            return TRAVERSING_CONTINUE;
        }
    }

    output_.addSubFile(FileDescriptor(shortName, details.lastWriteTimeRaw, details.fileSize), relNameParentPf_);

    //assemble status message (performance optimized)  = textScanning + wxT("\"") + fullName + wxT("\"")
    Zstring statusText = baseCallback_->textScanning;
    statusText.reserve(statusText.length() + fullName.length() + 2);
    statusText += DefaultChar('\"');
    statusText += fullName;
    statusText += DefaultChar('\"');

    //update UI/commandline status information
    statusHandler->updateStatusText(statusText);
    //add 1 element to the progress indicator
    statusHandler->updateProcessedData(1, 0); //NO performance issue at all
    //trigger display refresh
    statusHandler->requestUiRefresh();

    return TRAVERSING_CONTINUE;
}


template <bool filterActive>
TraverseCallback::ReturnValDir DirCallback<filterActive>::onDir(const DefaultChar* shortName, const Zstring& fullName)
{
    using globalFunctions::FILE_NAME_SEPARATOR;

    Zstring relName = relNameParentPf_;
    relName += shortName;

    //apply filter before processing (use relative name!)
    if (filterActive)
    {
        bool subObjMightMatch = true;
        if (!baseCallback_->filterInstance->passDirFilter(relName, &subObjMightMatch))
        {
            statusHandler->requestUiRefresh();

            if (subObjMightMatch)
            {
                DirContainer& subDir = output_.addSubDir(DirDescriptor(shortName), relNameParentPf_);

                DirCallback* subDirCallback = new DirCallback(baseCallback_, relName += FILE_NAME_SEPARATOR, subDir, statusHandler);
                baseCallback_->callBackBox.push_back(typename BaseDirCallback<filterActive>::CallbackPointer(subDirCallback)); //handle ownership

                //attention: ensure directory filtering is applied to exclude actually filtered directories
                return ReturnValDir(ReturnValDir::Continue(), subDirCallback);
            }
            else
                return ReturnValDir::Ignore(); //do NOT traverse subdirs
        }
    }

    DirContainer& subDir = output_.addSubDir(DirDescriptor(shortName), relNameParentPf_);


    //assemble status message (performance optimized)  = textScanning + wxT("\"") + fullName + wxT("\"")
    Zstring statusText = baseCallback_->textScanning;
    statusText.reserve(statusText.length() + fullName.length() + 2);
    statusText += DefaultChar('\"');
    statusText += fullName;
    statusText += DefaultChar('\"');

    //update UI/commandline status information
    statusHandler->updateStatusText(statusText);
    //add 1 element to the progress indicator
    statusHandler->updateProcessedData(1, 0);     //NO performance issue at all
    //trigger display refresh
    statusHandler->requestUiRefresh();

    DirCallback* subDirCallback = new DirCallback(baseCallback_, relName+=FILE_NAME_SEPARATOR, subDir, statusHandler);
    baseCallback_->callBackBox.push_back(typename BaseDirCallback<filterActive>::CallbackPointer(subDirCallback)); //handle ownership

    return ReturnValDir(ReturnValDir::Continue(), subDirCallback);
}


template <bool filterActive>
TraverseCallback::ReturnValue DirCallback<filterActive>::onError(const wxString& errorText)
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


template <bool filterActive>
TraverseCallback::ReturnValDir BaseDirCallback<filterActive>::onDir(const DefaultChar* shortName, const Zstring& fullName)
{
//#ifdef FFS_WIN => transparency is more important: just scan every file
//    if (    fullName.EndsWith(wxT("\\RECYCLER")) ||
//            fullName.EndsWith(wxT("\\System Volume Information")))
//    {
//        DirCallback<filterActive>::statusHandler->requestUiRefresh();
//        return TraverseCallback::ReturnValDir::Ignore();
//    }
//#endif  // FFS_WIN
    return DirCallback<filterActive>::onDir(shortName, fullName);
}


//------------------------------------------------------------------------------------------
struct DirBufferKey
{
    DirBufferKey(const Zstring& dirname,
                 boost::shared_ptr<const FilterProcess>& filterInst) :
            directoryName(dirname),
            filterInstance(filterInst) {}

    Zstring directoryName;
    boost::shared_ptr<const FilterProcess> filterInstance; //buffering has to consider filtering!

    bool operator < (const DirBufferKey& b) const
    {
#ifdef FFS_WIN //Windows does NOT distinguish between upper/lower-case
        const int rv = directoryName.CmpNoCase(b.directoryName);
#elif defined FFS_LINUX //Linux DOES distinguish between upper/lower-case
        const int rv = directoryName.Cmp(b.directoryName);
#endif
        if (rv != 0)
            return rv < 0;

        return filterInstance.get() && b.filterInstance.get() ?
               *filterInstance < *b.filterInstance :
               filterInstance.get() < b.filterInstance.get(); //at least one of these is NULL
    }
};


//------------------------------------------------------------------------------------------
class CompareProcess::DirectoryBuffer  //buffer multiple scans of the same directories
{
public:
    DirectoryBuffer(const bool traverseDirectorySymlinks, StatusHandler* statusUpdater) :
            m_traverseDirectorySymlinks(traverseDirectorySymlinks),
            m_statusUpdater(statusUpdater) {}

    const DirContainer& getDirectoryDescription(const Zstring& directoryPostfixed, bool filterActive, const wxString& includeFilter, const wxString& excludeFilter);

private:
    typedef boost::shared_ptr<DirContainer> DirBufferValue; //exception safety: avoid memory leak
    typedef std::map<DirBufferKey, DirBufferValue> BufferType;

    static DirBufferKey createKey(const Zstring& directoryPostfixed, bool filterActive, const wxString& includeFilter, const wxString& excludeFilter);
    DirContainer& insertIntoBuffer(const DirBufferKey& newKey);

    BufferType buffer;

    const bool m_traverseDirectorySymlinks;
    StatusHandler* m_statusUpdater;
};
//------------------------------------------------------------------------------------------


DirBufferKey CompareProcess::DirectoryBuffer::createKey(const Zstring& directoryPostfixed, bool filterActive, const wxString& includeFilter, const wxString& excludeFilter)
{
    boost::shared_ptr<const FilterProcess> filterInstance(
        filterActive && FilterProcess(includeFilter, excludeFilter) != FilterProcess::nullFilter() ? //nullfilter: in: '*', ex ''
        new FilterProcess(includeFilter, excludeFilter) : NULL);

    return DirBufferKey(directoryPostfixed, filterInstance);
}


DirContainer& CompareProcess::DirectoryBuffer::insertIntoBuffer(const DirBufferKey& newKey)
{
    DirBufferValue baseContainer(new DirContainer);
    buffer.insert(std::make_pair(newKey, baseContainer));

    if (FreeFileSync::dirExists(newKey.directoryName.c_str())) //folder existence already checked in startCompareProcess(): do not treat as error when arriving here!
    {
        std::auto_ptr<TraverseCallback> traverser(newKey.filterInstance.get() ?
                static_cast<TraverseCallback*>(new BaseDirCallback<true>(*baseContainer.get(), newKey.filterInstance.get(), m_statusUpdater)) :
                static_cast<TraverseCallback*>(new BaseDirCallback<false>(*baseContainer.get(), NULL, m_statusUpdater)));

        //get all files and folders from directoryPostfixed (and subdirectories)
        traverseFolder(newKey.directoryName, m_traverseDirectorySymlinks, traverser.get()); //exceptions may be thrown!
    }
    return *baseContainer.get();
}


const DirContainer& CompareProcess::DirectoryBuffer::getDirectoryDescription(
    const Zstring& directoryPostfixed,
    bool filterActive,
    const wxString& includeFilter,
    const wxString& excludeFilter)
{
    const DirBufferKey searchKey = createKey(directoryPostfixed, filterActive, includeFilter, excludeFilter);

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
                    const ErrorHandler::Response rv = statusUpdater->reportError(wxString(_("Please fill all empty directory fields.")) + wxT("\n\n") +
                                                      + wxT("(") + additionalInfo + wxT(")"));
                    if (rv == ErrorHandler::IGNORE_ERROR)
                        break;
                    else if (rv == ErrorHandler::RETRY)
                        ;  //continue with loop
                    else
                        assert (false);
                }
            }
        }

        //check if folders exist
        if (!i->leftDirectory.empty())
            while (!FreeFileSync::dirExists(i->leftDirectory.c_str()))
            {
                ErrorHandler::Response rv = statusUpdater->reportError(wxString(_("Directory does not exist:")) + wxT("\n") +
                                            wxT("\"") + i->leftDirectory + wxT("\"") + wxT("\n\n") +
                                            FreeFileSync::getLastErrorFormatted() + wxT(" ") + additionalInfo);
                if (rv == ErrorHandler::IGNORE_ERROR)
                    break;
                else if (rv == ErrorHandler::RETRY)
                    ;  //continue with loop
                else
                    assert (false);
            }

        if (!i->rightDirectory.empty())
            while (!FreeFileSync::dirExists(i->rightDirectory.c_str()))
            {
                ErrorHandler::Response rv = statusUpdater->reportError(wxString(_("Directory does not exist:")) + wxT("\n") +
                                            wxT("\"") + i->rightDirectory + wxT("\"") + wxT("\n\n") +
                                            FreeFileSync::getLastErrorFormatted() + wxT(" ") + additionalInfo);
                if (rv == ErrorHandler::IGNORE_ERROR)
                    break;
                else if (rv == ErrorHandler::RETRY)
                    ;  //continue with loop
                else
                    assert (false);
            }
    }
}


bool dependencyExists(const std::set<Zstring>& folders, const Zstring& newFolder, wxString& warningMessage)
{
    for (std::set<Zstring>::const_iterator i = folders.begin(); i != folders.end(); ++i)
        if (newFolder.StartsWith(*i) || i->StartsWith(newFolder))
        {
            warningMessage = wxString(_("Directories are dependent! Be careful when setting up synchronization rules:")) + wxT("\n") +
                             wxT("\"") + i->c_str() + wxT("\",\n") +
                             wxT("\"") + newFolder.c_str() + wxT("\"");
            return true;
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
        txtComparingContentOfFiles(Zstring(_("Comparing content of files %x")).Replace(wxT("%x"), wxT("\n\"%x\""), false))
{
    directoryBuffer.reset(new DirectoryBuffer(traverseSymLinks, handler));
}


struct MemoryAllocator
{
    MemoryAllocator()
    {
        buffer = new unsigned char[bufferSize];
    }

    ~MemoryAllocator()
    {
        delete [] buffer;
    }

    static const unsigned int bufferSize = 512 * 1024; //512 kb seems to be the perfect buffer size
    unsigned char* buffer;
};


//callback functionality for status updates while comparing
class CompareCallback
{
public:
    virtual ~CompareCallback() {}
    virtual void updateCompareStatus(const wxLongLong& totalBytesTransferred) = 0;
};


bool filesHaveSameContent(const Zstring& filename1, const Zstring& filename2, CompareCallback* callback)
{
    static MemoryAllocator memory1;
    static MemoryAllocator memory2;

    wxFFile file1(filename1.c_str(), wxT("rb"));
    if (!file1.IsOpened())
        throw FileError(wxString(_("Error opening file:")) + wxT(" \"") + filename1.c_str() + wxT("\""));

    wxFFile file2(filename2.c_str(), wxT("rb"));
    if (!file2.IsOpened()) //NO cleanup necessary for (wxFFile) file1
        throw FileError(wxString(_("Error opening file:")) + wxT(" \"") + filename2.c_str() + wxT("\""));

    wxLongLong bytesCompared;
    do
    {
        const size_t length1 = file1.Read(memory1.buffer, MemoryAllocator::bufferSize);
        if (file1.Error()) throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + filename1.c_str() + wxT("\""));

        const size_t length2 = file2.Read(memory2.buffer, MemoryAllocator::bufferSize);
        if (file2.Error()) throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + filename2.c_str() + wxT("\""));

        if (length1 != length2 || ::memcmp(memory1.buffer, memory2.buffer, length1) != 0)
            return false;

        bytesCompared += length1 * 2;

        //send progress updates
        callback->updateCompareStatus(bytesCompared);
    }
    while (!file1.Eof());

    if (!file2.Eof())
        return false;

    return true;
}


//callback implementation
class CmpCallbackImpl : public CompareCallback
{
public:
    CmpCallbackImpl(StatusHandler* handler, wxLongLong& bytesComparedLast) :
            m_handler(handler),
            m_bytesComparedLast(bytesComparedLast) {}

    virtual void updateCompareStatus(const wxLongLong& totalBytesTransferred)
    {   //called every 512 kB

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


/* OLD IMPLEMENTATION USING A WORKER THREAD

//handle execution of a method while updating the UI
class UpdateWhileComparing : public UpdateWhileExecuting
{
public:
    UpdateWhileComparing() {}
    ~UpdateWhileComparing() {}

    Zstring file1;
    Zstring file2;
    bool success;
    Zstring errorMessage;
    bool sameContent;

private:
    void longRunner() //virtual method implementation
    {
        try
        {
            sameContent = filesHaveSameContent(file1, file2);
            success = true;
        }
        catch (FileError& error)
        {
            success = false;
            errorMessage = error.show();
        }
    }
};


bool filesHaveSameContentUpdating(const Zstring& filename1, const Zstring& filename2, const wxULongLong& totalBytesToCmp, StatusHandler* updateClass)
{
    static UpdateWhileComparing cmpAndUpdate; //single instantiation: thread enters wait phase after each execution

    cmpAndUpdate.waitUntilReady();

    //longRunner is called from thread, but no mutex needed here, since thread is in waiting state!
    cmpAndUpdate.file1 = filename1;
    cmpAndUpdate.file2 = filename2;

    cmpAndUpdate.execute(updateClass);

    //no mutex needed here since longRunner is finished
    if (!cmpAndUpdate.success)
        throw FileError(cmpAndUpdate.errorMessage);

    //inform about the processed amount of data
    updateClass->updateProcessedData(0, totalBytesToCmp.ToDouble());

    return cmpAndUpdate.sameContent;
}*/

void formatPair(FolderPairCfg& input)
{
    input.leftDirectory  = FreeFileSync::getFormattedDirectoryName(input.leftDirectory);
    input.rightDirectory = FreeFileSync::getFormattedDirectoryName(input.rightDirectory);
}


struct ToBeRemoved
{
    bool operator()(const DirMapping& dirObj) const
    {
        return !dirObj.selectedForSynchronization && dirObj.subDirs.size() == 0 && dirObj.subFiles.size() == 0;
    }
};


class RemoveFilteredDirs
{
public:
    RemoveFilteredDirs(const wxString& include, const wxString& exclude) :
            filterProc(include, exclude) {}

    void operator()(HierarchyObject& hierObj)
    {
        //process subdirs recursively
        std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), *this);

        //filter subdirectories
        std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), boost::bind(&RemoveFilteredDirs::filterDir, this, _1));

        //remove superfluous directories
        hierObj.subDirs.erase(std::remove_if(hierObj.subDirs.begin(), hierObj.subDirs.end(), ::ToBeRemoved()), hierObj.subDirs.end());
    }

    void filterDir(FreeFileSync::DirMapping& dirObj)
    {
        const Zstring relName = dirObj.isEmpty<FreeFileSync::LEFT_SIDE>() ?
                                dirObj.getRelativeName<FreeFileSync::RIGHT_SIDE>() :
                                dirObj.getRelativeName<FreeFileSync::LEFT_SIDE>();

        dirObj.selectedForSynchronization = filterProc.passDirFilter(relName, NULL); //subObjMightMatch is always true in this context!
    }

private:
    const FilterProcess filterProc;
};


//filters and removes all excluded directories (but keeps those serving as parent folders)
void filterAndRemoveDirs(BaseDirMapping& baseDir, const wxString& include, const wxString& exclude)
{
    RemoveFilteredDirs(include, exclude)(baseDir);
}


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

    //format directory pairs: ensure they end with globalFunctions::FILE_NAME_SEPARATOR!
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
            if (fpCfg.filterIsActive) //let's filter them now... (and remove those that contain excluded elements only)
                filterAndRemoveDirs(*j, fpCfg.includeFilter, fpCfg.excludeFilter);

            //set sync-direction initially
            FreeFileSync::redetermineSyncDirection(fpCfg.syncConfiguration, *j);
        }

        //only if everything was processed correctly output is written to!
        //note: output mustn't change during this process to be in sync with GUI grid view!!!
        output_tmp.swap(output);
    }
    catch (const std::exception& e)
    {
        if (dynamic_cast<const std::bad_alloc*>(&e) != NULL)
            statusUpdater->reportFatalError(wxString(_("System out of memory!")) + wxT(" ") + wxString::From8BitData(e.what()));
        else
            statusUpdater->reportFatalError(wxString::From8BitData(e.what()));
        return; //should be obsolete!
    }
}

//--------------------assemble conflict descriptions---------------------------

//check for very old dates or dates in the future
wxString getConflictInvalidDate(const Zstring& fileNameFull, const wxLongLong& utcTime)
{
    wxString msg = _("File %x has an invalid date!");
    msg.Replace(wxT("%x"), wxString(wxT("\"")) + fileNameFull.c_str() + wxT("\""));
    msg += wxString(wxT("\n\n")) + _("Date") + wxT(": ") + utcTimeToLocalString(utcTime, fileNameFull);
    return wxString(_("Conflict detected:")) + wxT("\n") + msg;
}


//check for changed files with same modification date
wxString getConflictSameDateDiffSize(const FileMapping& fileObj)
{
    //some beautification...
    wxString left = wxString(_("Left")) + wxT(": ");
    wxString right = wxString(_("Right")) + wxT(": ");
    const int maxPref = std::max(left.length(), right.length());
    left.Pad(maxPref - left.length(), wxT(' '), true);
    right.Pad(maxPref - right.length(), wxT(' '), true);

    wxString msg = _("Files %x have the same date but a different size!");
    msg.Replace(wxT("%x"), wxString(wxT("\"")) + fileObj.getRelativeName<LEFT_SIDE>() + wxT("\""));
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
    const int maxPref = std::max(left.length(), right.length());
    left.Pad(maxPref - left.length(), wxT(' '), true);
    right.Pad(maxPref - right.length(), wxT(' '), true);

    wxString msg = _("Files %x have a file time difference of less than 1 hour!\n\nIt's not safe to decide which one is newer due to Daylight Saving Time issues.");
    msg += wxString(wxT("\n")) + _("(Note that only FAT/FAT32 drives are affected by this problem!\nIn all other cases you can disable the setting \"ignore 1-hour difference\".)");
    msg.Replace(wxT("%x"), wxString(wxT("\"")) + fileObj.getRelativeName<LEFT_SIDE>() + wxT("\""));
    msg += wxT("\n\n");
    msg += left + wxT("\t") + _("Date") + wxT(": ") + utcTimeToLocalString(fileObj.getLastWriteTime<LEFT_SIDE>(), fileObj.getFullName<LEFT_SIDE>()) + wxT("\n");
    msg += right + wxT("\t") + _("Date") + wxT(": ") + utcTimeToLocalString(fileObj.getLastWriteTime<RIGHT_SIDE>(), fileObj.getFullName<RIGHT_SIDE>());
    return wxString(_("Conflict detected:")) + wxT("\n") + msg;
}

//-----------------------------------------------------------------------------
inline
bool sameFileTime(const wxLongLong& a, const wxLongLong& b, const unsigned tolerance)
{
    if (a < b)
        return b - a <= tolerance;
    else
        return a - b <= tolerance;
}


void CompareProcess::compareByTimeSize(const std::vector<FolderPairCfg>& directoryPairsFormatted, FolderComparison& output)
{
    //process one folder pair after each other
    for (std::vector<FolderPairCfg>::const_iterator pair = directoryPairsFormatted.begin(); pair != directoryPairsFormatted.end(); ++pair)
    {
        BaseDirMapping newEntry(pair->leftDirectory, pair->rightDirectory);
        output.push_back(newEntry); //attention: push_back() copies by value!!! performance: append BEFORE writing values into fileCmp!

        //do basis scan and retrieve files existing on both sides
        std::vector<FileMapping*> compareCandidates;
        performBaseComparison(*pair, output.back(), compareCandidates);

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
                    line->cmpResult = FILE_CONFLICT;
                    if (line->getLastWriteTime<LEFT_SIDE>() < 0 || line->getLastWriteTime<LEFT_SIDE>() > oneYearFromNow)
                        line->conflictDescription = getConflictInvalidDate(line->getFullName<LEFT_SIDE>(), line->getLastWriteTime<LEFT_SIDE>());
                    else
                        line->conflictDescription = getConflictInvalidDate(line->getFullName<RIGHT_SIDE>(), line->getLastWriteTime<RIGHT_SIDE>());
                }
                else //from this block on all dates are at least "valid"
                {
                    //last write time may differ by up to 2 seconds (NTFS vs FAT32)
                    if (sameFileTime(line->getLastWriteTime<LEFT_SIDE>(), line->getLastWriteTime<RIGHT_SIDE>(), fileTimeTolerance))
                    {
                        if (line->getFileSize<LEFT_SIDE>() == line->getFileSize<RIGHT_SIDE>())
                            line->cmpResult = FILE_EQUAL;
                        else
                        {
                            line->cmpResult = FILE_CONFLICT; //same date, different filesize
                            line->conflictDescription = getConflictSameDateDiffSize(*line);
                        }
                    }
                    else
                    {
                        //finally: DST +/- 1-hour check: test if time diff is exactly +/- 1-hour (respecting 2 second FAT precision)
                        if (ignoreOneHourDifference && sameFileTime(line->getLastWriteTime<LEFT_SIDE>(), line->getLastWriteTime<RIGHT_SIDE>(), 3600 + 2))
                        {
                            //date diff < 1 hour is a conflict: it's not safe to determine which file is newer
                            if (sameFileTime(line->getLastWriteTime<LEFT_SIDE>(), line->getLastWriteTime<RIGHT_SIDE>(), 3600 - 2 - 1))
                            {
                                line->cmpResult = FILE_CONFLICT;
                                line->conflictDescription = getConflictChangeWithinHour(*line);
                            }
                            else //exact +/- 1-hour detected: treat as equal
                            {
                                if (line->getFileSize<LEFT_SIDE>() == line->getFileSize<RIGHT_SIDE>())
                                    line->cmpResult = FILE_EQUAL;
                                else
                                {
                                    line->cmpResult = FILE_CONFLICT; //same date, different filesize
                                    line->conflictDescription = getConflictSameDateDiffSize(*line);
                                }
                            }
                        }
                        else
                        {
                            if (line->getLastWriteTime<LEFT_SIDE>() < line->getLastWriteTime<RIGHT_SIDE>())
                                line->cmpResult = FILE_RIGHT_NEWER;
                            else
                                line->cmpResult = FILE_LEFT_NEWER;
                        }
                    }
                }
            }
            else //same write time
            {
                if (line->getFileSize<LEFT_SIDE>() == line->getFileSize<RIGHT_SIDE>())
                    line->cmpResult = FILE_EQUAL;
                else
                {
                    line->cmpResult = FILE_CONFLICT; //same date, different filesize
                    line->conflictDescription = getConflictSameDateDiffSize(*line);
                }
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

    //process one folder pair after each other
    for (std::vector<FolderPairCfg>::const_iterator pair = directoryPairsFormatted.begin(); pair != directoryPairsFormatted.end(); ++pair)
    {
        BaseDirMapping newEntry(pair->leftDirectory, pair->rightDirectory);
        output.push_back(newEntry); //attention: push_back() copies by value!!! performance: append BEFORE writing values into fileCmp!

        //do basis scan and retrieve candidates for binary comparison (files existing on both sides)
        performBaseComparison(*pair, output.back(), compareCandidates);
    }

    //finish categorization...
    std::vector<FileMapping*> filesToCompareBytewise;

    //content comparison of file content happens AFTER finding corresponding files
    //in order to separate into two processes (scanning and comparing)

    for (std::vector<FileMapping*>::iterator i = compareCandidates.begin(); i != compareCandidates.end(); ++i)
    {
        //pre-check: files have different content if they have a different filesize
        if ((*i)->getFileSize<LEFT_SIDE>() != (*i)->getFileSize<RIGHT_SIDE>())
            (*i)->cmpResult = FILE_DIFFERENT;
        else
            filesToCompareBytewise.push_back(*i);
    }



    const int objectsTotal       = filesToCompareBytewise.size() * 2;
    const wxULongLong bytesTotal = getBytesToCompare(filesToCompareBytewise);

    statusUpdater->initNewProcess(objectsTotal,
                                  globalFunctions::convertToSigned(bytesTotal),
                                  StatusHandler::PROCESS_COMPARING_CONTENT);

    //compare files (that have same size) bytewise...
    for (std::vector<FileMapping*>::const_iterator j = filesToCompareBytewise.begin(); j != filesToCompareBytewise.end(); ++j)
    {
        FileMapping* const gridline = *j;

        Zstring statusText = txtComparingContentOfFiles;
        statusText.Replace(wxT("%x"), gridline->getRelativeName<LEFT_SIDE>(), false);
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
                    gridline->cmpResult = FILE_EQUAL;
                else
                    gridline->cmpResult = FILE_DIFFERENT;

                statusUpdater->updateProcessedData(2, 0); //processed data is communicated in subfunctions!
                break;
            }
            catch (FileError& error)
            {
                ErrorHandler::Response rv = statusUpdater->reportError(error.show());
                if (rv == ErrorHandler::IGNORE_ERROR)
                {
                    gridline->cmpResult = FILE_CONFLICT; //same date, different filesize
                    gridline->conflictDescription = wxString(_("Conflict detected:")) + wxT("\n") + _("Comparing files by content failed.");
                    break;
                }

                else if (rv == ErrorHandler::RETRY)
                    ;   //continue with loop
                else
                    assert (false);
            }
        }
    }
}


class MergeSides
{
public:
    MergeSides(const Zstring& baseDirLeftPf,
               const Zstring& baseDirRightPf,
               std::vector<FileMapping*>& appendUndefinedOut) :
            baseDirLeft(baseDirLeftPf),
            baseDirRight(baseDirRightPf),
            appendUndefined(appendUndefinedOut) {}

    void execute(const DirContainer& leftSide, const DirContainer& rightSide, HierarchyObject& output)
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
            DirContainer::SubFileList::const_iterator j = rightSide.getSubFiles().find(*i);

            //find files that exist on left but not on right
            if (j == rightSide.getSubFiles().end())
                output.addSubFile(i->getData(), FILE_LEFT_SIDE_ONLY, FileMapping::nullData(),
                                  RelNamesBuffered(baseDirLeft, //base sync dir postfixed
                                                   baseDirRight,
                                                   i->getParentRelNamePf())); //relative parent name postfixed
            //find files that exist on left and right
            else
            {
                appendUndefined.push_back(
                    &output.addSubFile(i->getData(), FILE_EQUAL, j->getData(), //FILE_EQUAL is just a dummy-value here
                                       RelNamesBuffered(baseDirLeft, baseDirRight, i->getParentRelNamePf())));
            }
        }

        //find files that exist on right but not on left
        for (DirContainer::SubFileList::const_iterator j = rightSide.getSubFiles().begin(); j != rightSide.getSubFiles().end(); ++j)
        {
            if (leftSide.getSubFiles().find(*j) == leftSide.getSubFiles().end())
                output.addSubFile(FileMapping::nullData(), FILE_RIGHT_SIDE_ONLY, j->getData(),
                                  RelNamesBuffered(baseDirLeft, baseDirRight, j->getParentRelNamePf()));
        }


//-----------------------------------------------------------------------------------------------
        for (DirContainer::SubDirList::const_iterator i = leftSide.getSubDirs().begin(); i != leftSide.getSubDirs().end(); ++i)
        {
            DirContainer::SubDirList::const_iterator j = rightSide.getSubDirs().find(*i);

            //find directories that exist on left but not on right
            if (j == rightSide.getSubDirs().end())
            {
                DirMapping& newDirMap = output.addSubDir(i->getData(), DIR_LEFT_SIDE_ONLY, DirMapping::nullData(),
                                        RelNamesBuffered(baseDirLeft, baseDirRight, i->getParentRelNamePf()));
                fillOneSide<true>(*i, newDirMap); //recurse into subdirectories
            }
            else //directories that exist on both sides
            {
                DirMapping& newDirMap = output.addSubDir(i->getData(), DIR_EQUAL, j->getData(),
                                        RelNamesBuffered(baseDirLeft, baseDirRight, i->getParentRelNamePf()));
                execute(*i, *j, newDirMap); //recurse into subdirectories
            }
        }

        //find directories that exist on right but not on left
        for (DirContainer::SubDirList::const_iterator j = rightSide.getSubDirs().begin(); j != rightSide.getSubDirs().end(); ++j)
        {
            if (leftSide.getSubDirs().find(*j) == leftSide.getSubDirs().end())
            {
                DirMapping& newDirMap = output.addSubDir(DirMapping::nullData(), DIR_RIGHT_SIDE_ONLY, j->getData(),
                                        RelNamesBuffered(baseDirLeft, baseDirRight, j->getParentRelNamePf()));
                fillOneSide<false>(*j, newDirMap); //recurse into subdirectories
            }
        }
    }

private:
    template <bool leftSide>
    void fillOneSide(const DirContainer& dirCont, HierarchyObject& output)
    {
        //reserve() fulfills two task here: 1. massive performance improvement! 2. ensure references in appendUndefined remain valid!
        output.subFiles.reserve(dirCont.getSubFiles().size());
        output.subDirs.reserve( dirCont.getSubDirs(). size());

        for (DirContainer::SubFileList::const_iterator i = dirCont.getSubFiles().begin(); i != dirCont.getSubFiles().end(); ++i)
        {
            if (leftSide)
                output.addSubFile(i->getData(), FILE_LEFT_SIDE_ONLY, FileMapping::nullData(),
                                  RelNamesBuffered(baseDirLeft, baseDirRight, i->getParentRelNamePf()));
            else
                output.addSubFile(FileMapping::nullData(), FILE_RIGHT_SIDE_ONLY, i->getData(),
                                  RelNamesBuffered(baseDirLeft, baseDirRight, i->getParentRelNamePf()));
        }

        for (DirContainer::SubDirList::const_iterator i = dirCont.getSubDirs().begin(); i != dirCont.getSubDirs().end(); ++i)
        {
            DirMapping& newDirMap = leftSide ?
                                    output.addSubDir(i->getData(), DIR_LEFT_SIDE_ONLY, DirMapping::nullData(),
                                                     RelNamesBuffered(baseDirLeft, baseDirRight, i->getParentRelNamePf())) :
                                    output.addSubDir(DirMapping::nullData(), DIR_RIGHT_SIDE_ONLY, i->getData(),
                                                     RelNamesBuffered(baseDirLeft, baseDirRight, i->getParentRelNamePf()));

            fillOneSide<leftSide>(*i, newDirMap); //recurse into subdirectories
        }
    }


    const Zstring& baseDirLeft;
    const Zstring& baseDirRight;
    std::vector<FileMapping*>& appendUndefined;
};


void CompareProcess::performBaseComparison(const FolderPairCfg& pair, BaseDirMapping& output, std::vector<FileMapping*>& appendUndefined)
{
    assert(output.subDirs.empty());
    assert(output.subFiles.empty());

    //PERF_START;

    //scan directories
    const DirContainer& directoryLeft = directoryBuffer->getDirectoryDescription(pair.leftDirectory,
                                        pair.filterIsActive,
                                        pair.includeFilter,
                                        pair.excludeFilter);

    const DirContainer& directoryRight = directoryBuffer->getDirectoryDescription(pair.rightDirectory,
                                         pair.filterIsActive,
                                         pair.includeFilter,
                                         pair.excludeFilter);


    statusUpdater->updateStatusText(_("Generating file list..."));
    statusUpdater->forceUiRefresh(); //keep total number of scanned files up to date

    //PERF_STOP;

    MergeSides(pair.leftDirectory,
               pair.rightDirectory,
               appendUndefined).execute(directoryLeft, directoryRight, output);
    //PERF_STOP;
}
