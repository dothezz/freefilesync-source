// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "comparison.h"
#include <stdexcept>
#include "shared/global_func.h"
#include "shared/i18n.h"
#include <wx/msgdlg.h>
#include <wx/log.h>
#include "algorithm.h"
#include "shared/util.h"
#include <memory>
#include "shared/string_conv.h"
#include "shared/file_handling.h"
#include "shared/resolve_path.h"
#include "shared/system_func.h"
#include "shared/file_traverser.h"
#include "library/filter.h"
#include <map>
#include "file_hierarchy.h"
#include <boost/bind.hpp>
#include "library/binary.h"
#include "library/dir_lock.h"
#include "library/cmp_filetime.h"

#ifdef FFS_WIN
#include "shared/perf.h"
#endif

using namespace ffs3;

const Zstring LOCK_FILE_ENDING = Zstr("ffs_lock");


std::vector<ffs3::FolderPairCfg> ffs3::extractCompareCfg(const MainConfiguration& mainCfg)
{
    //merge first and additional pairs
    std::vector<FolderPairEnh> allPairs;
    allPairs.push_back(mainCfg.firstPair);
    allPairs.insert(allPairs.end(),
                    mainCfg.additionalPairs.begin(), //add additional pairs
                    mainCfg.additionalPairs.end());

    const BaseFilter::FilterRef globalFilter(new NameFilter(mainCfg.globalFilter.includeFilter, mainCfg.globalFilter.excludeFilter));

    std::vector<FolderPairCfg> output;
    for (std::vector<FolderPairEnh>::const_iterator i = allPairs.begin(); i != allPairs.end(); ++i)
        output.push_back(
            FolderPairCfg(i->leftDirectory,
                          i->rightDirectory,

                          combineFilters(globalFilter,
                                         BaseFilter::FilterRef(
                                             new NameFilter(
                                                 i->localFilter.includeFilter,
                                                 i->localFilter.excludeFilter))),

                          i->altSyncConfig.get() ? i->altSyncConfig->syncConfiguration : mainCfg.syncConfiguration));

    return output;
}



class BaseDirCallback;

class DirCallback : public ffs3::TraverseCallback
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

    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details);
    virtual void onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details);
    virtual ReturnValDir onDir(const Zchar* shortName, const Zstring& fullName);
    virtual void onError(const wxString& errorText);

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
                    SymLinkHandling handleSymlinks,
                    const BaseFilter::FilterRef& filter,
                    StatusHandler* handler) :
        DirCallback(this, Zstring(), output, handler),
        handleSymlinks_(handleSymlinks),
        textScanning(wxToZ(wxString(_("Scanning:")) + wxT(" \n"))),
        filterInstance(filter) {}

    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const TraverseCallback::FileInfo& details);

private:
    typedef boost::shared_ptr<const DirCallback> CallbackPointer;

    const SymLinkHandling handleSymlinks_;
    const Zstring textScanning;
    std::vector<CallbackPointer> callBackBox;  //collection of callback pointers to handle ownership

    const BaseFilter::FilterRef filterInstance; //always bound!
};


void DirCallback::onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details)
{
    //do not list the database file(s) sync.ffs_db, sync.x64.ffs_db, etc. or lock files
    const Zstring fileNameShort(shortName);
    const size_t pos = fileNameShort.rfind(Zchar('.'));
    if (pos != Zstring::npos)
    {
        const Zchar* ending = shortName + pos + 1; //(returns the whole string if ch not found)
        // if (EqualFilename()(ending, SYNC_DB_FILE_ENDING) ||
        //            EqualFilename()(ending, LOCK_FILE_ENDING))    //let's hope this premature performance optimization doesn't bite back!
        if (ending == SYNC_DB_FILE_ENDING ||                    //
            ending == LOCK_FILE_ENDING)
            return;
    }


    //assemble status message (performance optimized)  = textScanning + wxT("\"") + fullName + wxT("\"")
    Zstring statusText = baseCallback_->textScanning;
    statusText.reserve(statusText.length() + fullName.length() + 2);
    statusText += Zchar('\"');
    statusText += fullName;
    statusText += Zchar('\"');

    //update UI/commandline status information
    statusHandler->reportInfo(statusText);

    //------------------------------------------------------------------------------------
    //apply filter before processing (use relative name!)
    if (!baseCallback_->filterInstance->passFileFilter(relNameParentPf_ + fileNameShort))
    {
        statusHandler->requestUiRefresh();
        return;
    }

    //warning: for windows retrieveFileID is slow as hell! approximately 3 * 10^-4 s per file!
    //therefore only large files (that take advantage of detection of renaming when synchronizing) should be evaluated!
    //testcase: scanning only files larger than 1 MB results in performance loss of 6%

    //#warning this call is NOT acceptable for Linux!
    //    //Linux: retrieveFileID takes about 50% longer in VM! (avoidable because of redundant stat() call!)
    //    const util::FileID fileIdentifier = details.fileSize >= baseCallback_->detectRenameThreshold_ ?
    //                                           util::retrieveFileID(fullName) :
    //                                           util::FileID();

    output_.addSubFile(fileNameShort, FileDescriptor(details.lastWriteTimeRaw, details.fileSize));

    //add 1 element to the progress indicator
    statusHandler->updateProcessedData(1, 0); //NO performance issue at all
    //trigger display refresh
    statusHandler->requestUiRefresh();
}


void DirCallback::onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details)
{
    if (baseCallback_->handleSymlinks_ == SYMLINK_IGNORE)
        return;

    //assemble status message (performance optimized)  = textScanning + wxT("\"") + fullName + wxT("\"")
    Zstring statusText = baseCallback_->textScanning;
    statusText.reserve(statusText.length() + fullName.length() + 2);
    statusText += Zchar('\"');
    statusText += fullName;
    statusText += Zchar('\"');

    //update UI/commandline status information
    statusHandler->reportInfo(statusText);

    //------------------------------------------------------------------------------------
    const Zstring& relName = relNameParentPf_ + shortName;

    //apply filter before processing (use relative name!)
    if (!baseCallback_->filterInstance->passFileFilter(relName)) //always use file filter: Link type may not be "stable" on Linux!
    {
        statusHandler->requestUiRefresh();
        return;
    }

    output_.addSubLink(shortName, LinkDescriptor(details.lastWriteTimeRaw, details.targetPath, details.dirLink ? LinkDescriptor::TYPE_DIR : LinkDescriptor::TYPE_FILE));

    //add 1 element to the progress indicator
    statusHandler->updateProcessedData(1, 0); //NO performance issue at all
    //trigger display refresh
    statusHandler->requestUiRefresh();
}


TraverseCallback::ReturnValDir DirCallback::onDir(const Zchar* shortName, const Zstring& fullName)
{
    using common::FILE_NAME_SEPARATOR;

    //assemble status message (performance optimized)  = textScanning + wxT("\"") + fullName + wxT("\"")
    Zstring statusText = baseCallback_->textScanning;
    statusText.reserve(statusText.length() + fullName.length() + 2);
    statusText += Zchar('\"');
    statusText += fullName;
    statusText += Zchar('\"');

    //update UI/commandline status information
    statusHandler->reportInfo(statusText);

    //------------------------------------------------------------------------------------
    const Zstring& relName = relNameParentPf_ + shortName;

    //apply filter before processing (use relative name!)
    bool subObjMightMatch = true;
    if (!baseCallback_->filterInstance->passDirFilter(relName, &subObjMightMatch))
    {
        statusHandler->requestUiRefresh();

        if (!subObjMightMatch)
            return Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_IGNORE>(); //do NOT traverse subdirs
    }
    else
    {
        statusHandler->updateProcessedData(1, 0); //NO performance issue at all
        statusHandler->requestUiRefresh();        //trigger display refresh
    }

    DirContainer& subDir = output_.addSubDir(shortName);

    DirCallback* subDirCallback = new DirCallback(baseCallback_, relName + FILE_NAME_SEPARATOR, subDir, statusHandler);
    baseCallback_->callBackBox.push_back(BaseDirCallback::CallbackPointer(subDirCallback)); //handle ownership
    //attention: ensure directory filtering is applied later to exclude actually filtered directories
    return ReturnValDir(Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_CONTINUE>(), *subDirCallback);
}


void DirCallback::onError(const wxString& errorText)
{
    while (true)
    {
        switch (statusHandler->reportError(errorText))
        {
            case ErrorHandler::IGNORE_ERROR:
                return;
            case ErrorHandler::RETRY:
                break; //I have to admit "retry" is a bit of a fake here... at least the user has opportunity to abort!
        }
    }
}


void BaseDirCallback::onFile(const Zchar* shortName, const Zstring& fullName, const TraverseCallback::FileInfo& details)
{
    DirCallback::onFile(shortName, fullName, details);
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
        if (!EqualFilename()(directoryName, b.directoryName))
            return LessFilename()(directoryName, b.directoryName);

        return *filter < *b.filter;
    }
};


//------------------------------------------------------------------------------------------
class CompareProcess::DirectoryBuffer  //buffer multiple scans of the same directories
{
public:
    DirectoryBuffer(SymLinkHandling handleSymlinks,
                    StatusHandler* statusUpdater) :
        handleSymlinks_(handleSymlinks),
        statusUpdater_(statusUpdater) {}

    const DirContainer& getDirectoryDescription(const Zstring& directoryPostfixed, const BaseFilter::FilterRef& filter);

private:
    typedef boost::shared_ptr<DirContainer> DirBufferValue; //exception safety: avoid memory leak
    typedef std::map<DirBufferKey, DirBufferValue> BufferType;

    DirContainer& insertIntoBuffer(const DirBufferKey& newKey);

    BufferType buffer;

    const SymLinkHandling handleSymlinks_;
    StatusHandler* statusUpdater_;
};
//------------------------------------------------------------------------------------------


#ifdef FFS_WIN
class DstHackCallbackImpl : public DstHackCallback
{
public:
    DstHackCallbackImpl(StatusHandler& statusUpdater) :
        textApplyingDstHack(wxToZ(_("Encoding extended time information: %x")).Replace(Zstr("%x"), Zstr("\n\"%x\""))),
        statusUpdater_(statusUpdater) {}

private:
    virtual void requestUiRefresh(const Zstring& filename) //applying DST hack imposes significant one-time performance drawback => callback to inform user
    {
        Zstring statusText = textApplyingDstHack;
        statusText.Replace(Zstr("%x"), filename);
        statusUpdater_.reportInfo(statusText);
        statusUpdater_.requestUiRefresh();
    }

    const Zstring textApplyingDstHack;
    StatusHandler& statusUpdater_;
};
#endif


DirContainer& CompareProcess::DirectoryBuffer::insertIntoBuffer(const DirBufferKey& newKey)
{
    DirBufferValue baseContainer(new DirContainer);
    buffer.insert(std::make_pair(newKey, baseContainer));

    if (ffs3::dirExists(newKey.directoryName)) //folder existence already checked in startCompareProcess(): do not treat as error when arriving here!
    {
        BaseDirCallback traverser(*baseContainer,
                                  handleSymlinks_,
                                  newKey.filter,
                                  statusUpdater_);

        bool followSymlinks = false;
        switch (handleSymlinks_)
        {
            case SYMLINK_IGNORE:
                followSymlinks = false; //=> symlinks will be reported via onSymlink() where they are excluded
                break;
            case SYMLINK_USE_DIRECTLY:
                followSymlinks = false;
                break;
            case SYMLINK_FOLLOW_LINK:
                followSymlinks = true;
                break;
        }

        std::auto_ptr<ffs3::DstHackCallback> dstCallback;
#ifdef FFS_WIN
        dstCallback.reset(new DstHackCallbackImpl(*statusUpdater_));
#endif

        //get all files and folders from directoryPostfixed (and subdirectories)
        traverseFolder(newKey.directoryName, followSymlinks, traverser, dstCallback.get()); //exceptions may be thrown!
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
namespace
{
void foldersAreValidForComparison(const std::vector<FolderPairCfg>& folderPairsForm, StatusHandler* statusUpdater)
{
    bool nonEmptyPairFound = false; //check if user entered at least one folder pair
    bool partiallyFilledPairFound = false;

    const wxString additionalInfo = _("You can ignore this error to consider the directory as empty.");

    for (std::vector<FolderPairCfg>::const_iterator i = folderPairsForm.begin(); i != folderPairsForm.end(); ++i)
    {
        if (!i->leftDirectory.empty() || !i->rightDirectory.empty()) //may be partially filled though
            nonEmptyPairFound = true;

        if ((i->leftDirectory.empty() && !i->rightDirectory.empty()) ||
            (!i->leftDirectory.empty() && i->rightDirectory.empty()))
            partiallyFilledPairFound = true;

        //check if folders exist
        if (!i->leftDirectory.empty())
            while (!ffs3::dirExists(i->leftDirectory))
            {
                ErrorHandler::Response rv = statusUpdater->reportError(wxString(_("Directory does not exist:")) + wxT(" \n") +
                                                                       wxT("\"") + zToWx(i->leftDirectory) + wxT("\"") + wxT("\n\n") +
                                                                       additionalInfo + wxT(" ") + ffs3::getLastErrorFormatted());
                if (rv == ErrorHandler::IGNORE_ERROR)
                    break;
                else if (rv == ErrorHandler::RETRY)
                    ;  //continue with loop
                else
                    throw std::logic_error("Programming Error: Unknown return value! (2)");
            }

        if (!i->rightDirectory.empty())
            while (!ffs3::dirExists(i->rightDirectory))
            {
                ErrorHandler::Response rv = statusUpdater->reportError(wxString(_("Directory does not exist:")) + wxT("\n") +
                                                                       wxT("\"") + zToWx(i->rightDirectory) + wxT("\"") + wxT("\n\n") +
                                                                       additionalInfo + wxT(" ") + ffs3::getLastErrorFormatted());
                if (rv == ErrorHandler::IGNORE_ERROR)
                    break;
                else if (rv == ErrorHandler::RETRY)
                    ;  //continue with loop
                else
                    throw std::logic_error("Programming Error: Unknown return value! (3)");
            }
    }

    //check for empty entries
    if (!nonEmptyPairFound || partiallyFilledPairFound)
    {
        while (true)
        {
            const ErrorHandler::Response rv = statusUpdater->reportError(wxString(_("A directory input field is empty.")) + wxT(" \n\n") +
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


namespace
{
struct EqualDependentDirectory : public std::binary_function<Zstring, Zstring, bool>
{
    bool operator()(const Zstring& lhs, const Zstring& rhs) const
    {
        return EqualFilename()(Zstring(lhs.c_str(), std::min(lhs.length(), rhs.length())),
                               Zstring(rhs.c_str(), std::min(lhs.length(), rhs.length())));
    }
};
}

//check whether one side is subdirectory of other side (folder pair wise!)
//similar check if one directory is read/written by multiple pairs not before beginning of synchronization
wxString checkFolderDependency(const std::vector<FolderPairCfg>& folderPairsForm) //returns warning message, empty if all ok
{
    typedef std::vector<std::pair<wxString, wxString> > DirDirList;
    DirDirList dependentDirs;

    for (std::vector<FolderPairCfg>::const_iterator i = folderPairsForm.begin(); i != folderPairsForm.end(); ++i)
        if (!i->leftDirectory.empty() && !i->rightDirectory.empty()) //empty folders names may be accepted by user
        {
            if (EqualDependentDirectory()(i->leftDirectory, i->rightDirectory)) //test wheter leftDirectory begins with rightDirectory or the other way round
                dependentDirs.push_back(std::make_pair(zToWx(i->leftDirectory), zToWx(i->rightDirectory)));
        }

    wxString warnignMsg;

    if (!dependentDirs.empty())
    {
        warnignMsg = _("Directories are dependent! Be careful when setting up synchronization rules:");
        for (DirDirList::const_iterator i = dependentDirs.begin(); i != dependentDirs.end(); ++i)
            warnignMsg += wxString(wxT("\n\n")) +
                          wxT("\"") + i->first  + wxT("\"\n") +
                          wxT("\"") + i->second + wxT("\"");
    }
    return warnignMsg;
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
        sameContent = filesHaveSameContent(filename1, filename2, callback); //throw FileError
    }
    catch (...)
    {
        //error situation: undo communication of processed amount of data
        handler->updateProcessedData(0, bytesComparedLast * -1);
        throw;
    }

    //inform about the (remaining) processed amount of data
    handler->updateProcessedData(0, common::convertToSigned(totalBytesToCmp) - bytesComparedLast);

    return sameContent;
}


struct ToBeRemoved
{
    bool operator()(const DirMapping& dirObj) const
    {
        return !dirObj.isActive() &&
               dirObj.useSubDirs(). size() == 0 &&
               dirObj.useSubLinks().size() == 0 &&
               dirObj.useSubFiles().size() == 0;
    }
};


std::set<Zstring> getFolders(const std::vector<FolderPairCfg>& directoryPairsFormatted)
{
    std::set<Zstring> output;
    for (std::vector<FolderPairCfg>::const_iterator i = directoryPairsFormatted.begin(); i != directoryPairsFormatted.end(); ++i)
    {
        output.insert(i->leftDirectory);
        output.insert(i->rightDirectory);
    }
    output.erase(Zstring()); //remove empty directory strings
    return output;
}


class RemoveFilteredDirs
{
public:
    RemoveFilteredDirs(const BaseFilter& filterProc) :
        filterProc_(filterProc) {}

    void execute(HierarchyObject& hierObj)
    {
        HierarchyObject::SubDirMapping& subDirs = hierObj.useSubDirs();

        //process subdirs recursively
        util::ProxyForEach<RemoveFilteredDirs> prx(*this); //grant std::for_each access to private parts of this class
        std::for_each(subDirs.begin(), subDirs.end(), prx);

        //remove superfluous directories
        subDirs.erase(
            std::remove_if(subDirs.begin(), subDirs.end(), ::ToBeRemoved()),
            subDirs.end());
    }

private:
    friend class util::ProxyForEach<RemoveFilteredDirs>; //friend declaration of std::for_each is NOT sufficient as implementation is compiler dependent!

    void operator()(DirMapping& dirObj)
    {
        dirObj.setActive(filterProc_.passDirFilter(dirObj.getObjRelativeName().c_str(), NULL)); //subObjMightMatch is always true in this context!
        execute(dirObj);
    }

    const BaseFilter& filterProc_;
};


void formatPair(FolderPairCfg& input)
{
    //ensure they end with common::FILE_NAME_SEPARATOR and replace macros
    input.leftDirectory  = ffs3::getFormattedDirectoryName(input.leftDirectory);
    input.rightDirectory = ffs3::getFormattedDirectoryName(input.rightDirectory);
}
}

//#############################################################################################################################

CompareProcess::CompareProcess(SymLinkHandling handleSymlinks,
                               size_t fileTimeTol,
                               xmlAccess::OptionalDialogs& warnings,
                               StatusHandler* handler) :
    fileTimeTolerance(fileTimeTol),
    m_warnings(warnings),
    statusUpdater(handler),
    txtComparingContentOfFiles(wxToZ(_("Comparing content of files %x")).Replace(Zstr("%x"), Zstr("\n\"%x\""), false))
{
    directoryBuffer.reset(new DirectoryBuffer(handleSymlinks, handler));
}


void CompareProcess::startCompareProcess(const std::vector<FolderPairCfg>& directoryPairs,
                                         const CompareVariant cmpVar,
                                         FolderComparison& output)
{
#ifdef NDEBUG
    wxLogNull noWxLogs; //hide wxWidgets log messages in release build
#endif

    //PERF_START;


    //init process: keep at beginning so that all gui elements are initialized properly
    statusUpdater->initNewProcess(-1, 0, StatusHandler::PROCESS_SCANNING); //it's not known how many files will be scanned => -1 objects

    //format directory pairs: ensure they end with common::FILE_NAME_SEPARATOR and replace macros!
    std::vector<FolderPairCfg> directoryPairsFormatted = directoryPairs;
    std::for_each(directoryPairsFormatted.begin(), directoryPairsFormatted.end(), formatPair);

    //-------------------some basic checks:------------------------------------------

    //ensure that folders are valid
    foldersAreValidForComparison(directoryPairsFormatted, statusUpdater);

    {
        //check if folders have dependencies
        wxString warningMessage = checkFolderDependency(directoryPairsFormatted);
        if (!warningMessage.empty())
            statusUpdater->reportWarning(warningMessage.c_str(), m_warnings.warningDependentFolders);
    }

    //-------------------end of basic checks------------------------------------------


    try
    {
        //prevent shutdown while (binary) comparison is in progress
        util::DisableStandby dummy2;

        //place a lock on all directories before traversing (sync.ffs_lock)
        std::map<Zstring, DirLock> lockHolder;
        {
            const std::set<Zstring> folderList = getFolders(directoryPairsFormatted);
            for (std::set<Zstring>::const_iterator i = folderList.begin(); i != folderList.end(); ++i)
            {
                class WaitOnLockHandler : public DirLockCallback
                {
                public:
                    WaitOnLockHandler(StatusHandler& statusUpdater) : waitHandler(statusUpdater) {}

                    virtual void requestUiRefresh()  //allowed to throw exceptions
                    {
                        waitHandler.requestUiRefresh();
                    }
                    virtual void reportInfo(const Zstring& text)
                    {
                        waitHandler.reportInfo(text);
                    }
                private:
                    StatusHandler& waitHandler;
                } callback(*statusUpdater);

                try
                {
                    lockHolder.insert(std::make_pair(*i, DirLock(*i + Zstr("sync.") + LOCK_FILE_ENDING, &callback)));
                }
                catch (const FileError& e)
                {
                    bool dummy = false; //this warning shall not be shown but logged only
                    statusUpdater->reportWarning(e.msg(), dummy);
                }
            }
        }

        //traverse/process folders
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
            if (!fpCfg.filter->isNull())
                RemoveFilteredDirs(*fpCfg.filter).execute(*j); //remove all excluded directories (but keeps those serving as parent folders for not excl. elements)

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

            ffs3::redetermineSyncDirection(fpCfg.syncConfiguration, *j, &redetCallback);


            //pass locks to directory structure
            std::map<Zstring, DirLock>::const_iterator iter = lockHolder.find(j->getBaseDir<LEFT_SIDE>());
            if (iter != lockHolder.end()) j->holdLock<LEFT_SIDE>(iter->second);
            iter = lockHolder.find(j->getBaseDir<RIGHT_SIDE>());
            if (iter != lockHolder.end()) j->holdLock<RIGHT_SIDE>(iter->second);
        }

        //only if everything was processed correctly output is written to!
        //note: output mustn't change during this process to be in sync with GUI grid view!!!
        output_tmp.swap(output);
    }
    catch (const std::bad_alloc& e)
    {
        statusUpdater->reportFatalError(wxString(_("Memory allocation failed!")) + wxT(" ") + wxString::FromAscii(e.what()));
    }
    catch (const std::exception& e)
    {
        statusUpdater->reportFatalError(wxString::FromAscii(e.what()));
    }
}

//--------------------assemble conflict descriptions---------------------------

//check for very old dates or dates in the future
wxString getConflictInvalidDate(const Zstring& fileNameFull, const wxLongLong& utcTime)
{
    wxString msg = _("File %x has an invalid date!");
    msg.Replace(wxT("%x"), wxString(wxT("\"")) + zToWx(fileNameFull) + wxT("\""));
    msg += wxString(wxT("\n\n")) + _("Date") + wxT(": ") + utcTimeToLocalString(utcTime);
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
    //    wxString left = wxString(_("Left")) + wxT(": ");
    //    wxString right = wxString(_("Right")) + wxT(": ");
    //    makeSameLength(left, right);

    const wxString left  = wxT("<-- ");
    const wxString right = wxT("--> ");

    wxString msg = _("Files %x have the same date but a different size!");
    msg.Replace(wxT("%x"), wxString(wxT("\"")) + zToWx(fileObj.getRelativeName<LEFT_SIDE>()) + wxT("\""));
    msg += wxT("\n\n");
    msg += left + wxT("\t") + _("Date") + wxT(": ") + utcTimeToLocalString(fileObj.getLastWriteTime<LEFT_SIDE>()) +
           wxT(" \t") + _("Size") + wxT(": ") + fileObj.getFileSize<LEFT_SIDE>().ToString() + wxT("\n");
    msg += right + wxT("\t") + _("Date") + wxT(": ") + utcTimeToLocalString(fileObj.getLastWriteTime<RIGHT_SIDE>()) +
           wxT(" \t") + _("Size") + wxT(": ") + fileObj.getFileSize<RIGHT_SIDE>().ToString();
    return wxString(_("Conflict detected:")) + wxT("\n") + msg;
}


//-----------------------------------------------------------------------------
void CompareProcess::categorizeSymlinkByTime(SymLinkMapping* linkObj) const
{
    const CmpFileTime timeCmp(fileTimeTolerance);

    //categorize symlinks that exist on both sides
    if ( //special handling: if symlinks have the same "content" they are seen as equal while other metadata is ignored
#ifdef FFS_WIN //type of symbolic link is relevant for Windows only
        linkObj->getLinkType<LEFT_SIDE>() == linkObj->getLinkType<RIGHT_SIDE>() &&
#endif
        !linkObj->getTargetPath<LEFT_SIDE>().empty() &&
        linkObj->getTargetPath<LEFT_SIDE>() == linkObj->getTargetPath<RIGHT_SIDE>())
    {
        //symlinks have same "content"
        if (linkObj->getShortName<LEFT_SIDE>() == linkObj->getShortName<RIGHT_SIDE>() &&
            timeCmp.getResult(linkObj->getLastWriteTime<LEFT_SIDE>(),
                              linkObj->getLastWriteTime<RIGHT_SIDE>()) == CmpFileTime::TIME_EQUAL)
            linkObj->setCategory<SYMLINK_EQUAL>();
        else
            linkObj->setCategory<SYMLINK_DIFFERENT_METADATA>();
        return;
    }

    switch (timeCmp.getResult(linkObj->getLastWriteTime<LEFT_SIDE>(),
                              linkObj->getLastWriteTime<RIGHT_SIDE>()))
    {
        case CmpFileTime::TIME_EQUAL:
            if (
#ifdef FFS_WIN //type of symbolic link is relevant for Windows only
                linkObj->getLinkType<LEFT_SIDE>() == linkObj->getLinkType<RIGHT_SIDE>() &&
#endif
                linkObj->getTargetPath<LEFT_SIDE>() == linkObj->getTargetPath<RIGHT_SIDE>()) //may both be empty if following link failed
            {
                if (linkObj->getShortName<LEFT_SIDE>() == linkObj->getShortName<RIGHT_SIDE>())
                    linkObj->setCategory<SYMLINK_EQUAL>();
                else
                    linkObj->setCategory<SYMLINK_DIFFERENT_METADATA>();
            }
            else
            {
                wxString conflictMsg = wxString(_("Conflict detected:")) + wxT("\n") + _("Symlinks %x have the same date but a different target!");
                conflictMsg.Replace(wxT("%x"), wxString(wxT("\"")) + zToWx(linkObj->getRelativeName<LEFT_SIDE>()) + wxT("\""));
                linkObj->setCategoryConflict(conflictMsg);
            }
            break;

        case CmpFileTime::TIME_LEFT_NEWER:
            linkObj->setCategory<SYMLINK_LEFT_NEWER>();
            break;

        case CmpFileTime::TIME_RIGHT_NEWER:
            linkObj->setCategory<SYMLINK_RIGHT_NEWER>();
            break;

        case CmpFileTime::TIME_LEFT_INVALID:
            linkObj->setCategoryConflict(getConflictInvalidDate(linkObj->getFullName<LEFT_SIDE>(), linkObj->getLastWriteTime<LEFT_SIDE>()));
            break;

        case CmpFileTime::TIME_RIGHT_INVALID:
            linkObj->setCategoryConflict(getConflictInvalidDate(linkObj->getFullName<RIGHT_SIDE>(), linkObj->getLastWriteTime<RIGHT_SIDE>()));
            break;
    }
}


void CompareProcess::compareByTimeSize(const std::vector<FolderPairCfg>& directoryPairsFormatted, FolderComparison& output) const
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
        std::vector<FileMapping*> uncategorizedFiles;
        std::vector<SymLinkMapping*> uncategorizedLinks;
        performBaseComparison(output.back(), uncategorizedFiles, uncategorizedLinks);

        //finish symlink categorization
        std::for_each(uncategorizedLinks.begin(), uncategorizedLinks.end(), boost::bind(&CompareProcess::categorizeSymlinkByTime, this, _1));

        //categorize files that exist on both sides
        const CmpFileTime timeCmp(fileTimeTolerance);

        for (std::vector<FileMapping*>::iterator i = uncategorizedFiles.begin(); i != uncategorizedFiles.end(); ++i)
        {
            FileMapping* const line = *i;

            switch (timeCmp.getResult(line->getLastWriteTime<LEFT_SIDE>(),
                                      line->getLastWriteTime<RIGHT_SIDE>()))
            {
                case CmpFileTime::TIME_EQUAL:
                    if (line->getFileSize<LEFT_SIDE>() == line->getFileSize<RIGHT_SIDE>())
                    {
                        if (line->getShortName<LEFT_SIDE>() == line->getShortName<RIGHT_SIDE>())
                            line->setCategory<FILE_EQUAL>();
                        else
                            line->setCategory<FILE_DIFFERENT_METADATA>();
                    }
                    else
                        line->setCategoryConflict(getConflictSameDateDiffSize(*line)); //same date, different filesize
                    break;

                case CmpFileTime::TIME_LEFT_NEWER:
                    line->setCategory<FILE_LEFT_NEWER>();
                    break;

                case CmpFileTime::TIME_RIGHT_NEWER:
                    line->setCategory<FILE_RIGHT_NEWER>();
                    break;

                case CmpFileTime::TIME_LEFT_INVALID:
                    line->setCategoryConflict(getConflictInvalidDate(line->getFullName<LEFT_SIDE>(), line->getLastWriteTime<LEFT_SIDE>()));
                    break;

                case CmpFileTime::TIME_RIGHT_INVALID:
                    line->setCategoryConflict(getConflictInvalidDate(line->getFullName<RIGHT_SIDE>(), line->getLastWriteTime<RIGHT_SIDE>()));
                    break;
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


void CompareProcess::categorizeSymlinkByContent(SymLinkMapping* linkObj) const
{
    //categorize symlinks that exist on both sides
    const CmpFileTime timeCmp(fileTimeTolerance);

    if (
#ifdef FFS_WIN //type of symbolic link is relevant for Windows only
        linkObj->getLinkType<LEFT_SIDE>() == linkObj->getLinkType<RIGHT_SIDE>() &&
#endif
        linkObj->getTargetPath<LEFT_SIDE>() == linkObj->getTargetPath<RIGHT_SIDE>())
    {
        //symlinks have same "content"
        if (linkObj->getShortName<LEFT_SIDE>() == linkObj->getShortName<RIGHT_SIDE>() &&
            timeCmp.getResult(linkObj->getLastWriteTime<LEFT_SIDE>(),
                              linkObj->getLastWriteTime<RIGHT_SIDE>()) == CmpFileTime::TIME_EQUAL)
            linkObj->setCategory<SYMLINK_EQUAL>();
        else
            linkObj->setCategory<SYMLINK_DIFFERENT_METADATA>();
    }
    else
        linkObj->setCategory<SYMLINK_DIFFERENT>();
}


void CompareProcess::compareByContent(const std::vector<FolderPairCfg>& directoryPairsFormatted, FolderComparison& output) const
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

        std::vector<SymLinkMapping*> uncategorizedLinks;
        //do basis scan and retrieve candidates for binary comparison (files existing on both sides)
        performBaseComparison(output.back(), compareCandidates, uncategorizedLinks);

        //finish symlink categorization
        std::for_each(uncategorizedLinks.begin(), uncategorizedLinks.end(), boost::bind(&CompareProcess::categorizeSymlinkByContent, this, _1));
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

    statusUpdater->initNewProcess(static_cast<int>(objectsTotal),
                                  common::convertToSigned(bytesTotal),
                                  StatusHandler::PROCESS_COMPARING_CONTENT);

    const CmpFileTime timeCmp(fileTimeTolerance);

    //compare files (that have same size) bytewise...
    for (std::vector<FileMapping*>::const_iterator j = filesToCompareBytewise.begin(); j != filesToCompareBytewise.end(); ++j)
    {
        FileMapping* const line = *j;

        Zstring statusText = txtComparingContentOfFiles;
        statusText.Replace(Zstr("%x"), line->getRelativeName<LEFT_SIDE>(), false);
        statusUpdater->reportInfo(statusText);

        //check files that exist in left and right model but have different content
        while (true)
        {
            //trigger display refresh
            statusUpdater->requestUiRefresh();

            try
            {
                if (filesHaveSameContentUpdating(line->getFullName<LEFT_SIDE>(),
                                                 line->getFullName<RIGHT_SIDE>(),
                                                 line->getFileSize<LEFT_SIDE>() * 2,
                                                 statusUpdater))
                {
                    if (line->getShortName<LEFT_SIDE>() == line->getShortName<RIGHT_SIDE>() &&
                        timeCmp.getResult(line->getLastWriteTime<LEFT_SIDE>(),
                                          line->getLastWriteTime<RIGHT_SIDE>()) == CmpFileTime::TIME_EQUAL)
                        line->setCategory<FILE_EQUAL>();
                    else
                        line->setCategory<FILE_DIFFERENT_METADATA>();
                }
                else
                    line->setCategory<FILE_DIFFERENT>();

                statusUpdater->updateProcessedData(2, 0); //processed data is communicated in subfunctions!
                break;
            }
            catch (FileError& error)
            {
                ErrorHandler::Response rv = statusUpdater->reportError(error.msg());
                if (rv == ErrorHandler::IGNORE_ERROR)
                {
                    line->setCategoryConflict(wxString(_("Conflict detected:")) + wxT("\n") + _("Comparing files by content failed."));
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
    MergeSides(std::vector<FileMapping*>& appendUndefinedFileOut,
               std::vector<SymLinkMapping*>& appendUndefinedLinkOut) :
        appendUndefinedFile(appendUndefinedFileOut),
        appendUndefinedLink(appendUndefinedLinkOut) {}

    void execute(const DirContainer& leftSide, const DirContainer& rightSide, HierarchyObject& output);

private:
    template <SelectedSide side>
    void fillOneSide(const DirContainer& dirCont, HierarchyObject& output);

    std::vector<FileMapping*>& appendUndefinedFile;
    std::vector<SymLinkMapping*>& appendUndefinedLink;
};


template <>
void MergeSides::fillOneSide<LEFT_SIDE>(const DirContainer& dirCont, HierarchyObject& output)
{
    //reserve() fulfills one task here: massive performance improvement!
    output.useSubFiles().reserve(dirCont.files.size());
    output.useSubDirs(). reserve(dirCont.dirs. size());
    output.useSubLinks().reserve(dirCont.links.size());

    for (DirContainer::FileList::const_iterator i = dirCont.files.begin(); i != dirCont.files.end(); ++i)
        output.addSubFile(i->second, i->first);

    for (DirContainer::LinkList::const_iterator i = dirCont.links.begin(); i != dirCont.links.end(); ++i)
        output.addSubLink(i->second, i->first);

    for (DirContainer::DirList::const_iterator i = dirCont.dirs.begin(); i != dirCont.dirs.end(); ++i)
    {
        DirMapping& newDirMap = output.addSubDir(i->first, Zstring());
        fillOneSide<LEFT_SIDE>(i->second, newDirMap); //recurse into subdirectories
    }
}


template <>
void MergeSides::fillOneSide<RIGHT_SIDE>(const DirContainer& dirCont, HierarchyObject& output)
{
    //reserve() fulfills one task here: massive performance improvement!
    output.useSubFiles().reserve(dirCont.files.size());
    output.useSubDirs ().reserve(dirCont.dirs. size());
    output.useSubLinks().reserve(dirCont.links.size());

    for (DirContainer::FileList::const_iterator i = dirCont.files.begin(); i != dirCont.files.end(); ++i)
        output.addSubFile(i->first, i->second);

    for (DirContainer::LinkList::const_iterator i = dirCont.links.begin(); i != dirCont.links.end(); ++i)
        output.addSubLink(i->first, i->second);

    for (DirContainer::DirList::const_iterator i = dirCont.dirs.begin(); i != dirCont.dirs.end(); ++i)
    {
        DirMapping& newDirMap = output.addSubDir(Zstring(), i->first);
        fillOneSide<RIGHT_SIDE>(i->second, newDirMap); //recurse into subdirectories
    }
}


void MergeSides::execute(const DirContainer& leftSide, const DirContainer& rightSide, HierarchyObject& output)
{
    //ATTENTION: HierarchyObject::retrieveById() can only work correctly if the following conditions are fulfilled:
    //1. on each level, files are added first, symlinks, then directories (=> file id < link id < dir id)
    //2. when a directory is added, all subdirectories must be added immediately (recursion) before the next dir on this level is added
    //3. entries may be deleted but NEVER new ones inserted!!!
    //=> this allows for a quasi-binary search by id!

    //HierarchyObject::addSubFile() must not invalidate references used in "appendUndefined"! Currently a std::list, so no problem.

    //reserve() fulfills two task here: 1. massive performance improvement! 2. ensure references in appendUndefined remain valid!
    output.useSubFiles().reserve(leftSide.files.size() + rightSide.files.size()); //assume worst case!
    output.useSubDirs(). reserve(leftSide.dirs. size() + rightSide.dirs. size()); //
    output.useSubLinks().reserve(leftSide.links.size() + rightSide.links.size()); //

    for (DirContainer::FileList::const_iterator i = leftSide.files.begin(); i != leftSide.files.end(); ++i)
    {
        DirContainer::FileList::const_iterator rightFile = rightSide.files.find(i->first);

        //find files that exist on left but not on right
        if (rightFile == rightSide.files.end())
            output.addSubFile(i->second, i->first);
        //find files that exist on left and right
        else
        {
            FileMapping& newEntry = output.addSubFile(
                                        i->first,
                                        i->second,
                                        FILE_EQUAL, //FILE_EQUAL is just a dummy-value here
                                        rightFile->first,
                                        rightFile->second);
            appendUndefinedFile.push_back(&newEntry);
        }
    }

    //find files that exist on right but not on left
    for (DirContainer::FileList::const_iterator j = rightSide.files.begin(); j != rightSide.files.end(); ++j)
    {
        if (leftSide.files.find(j->first) == leftSide.files.end())
            output.addSubFile(j->first, j->second);
    }


    //-----------------------------------------------------------------------------------------------
    for (DirContainer::LinkList::const_iterator i = leftSide.links.begin(); i != leftSide.links.end(); ++i)
    {
        DirContainer::LinkList::const_iterator rightLink = rightSide.links.find(i->first);

        //find links that exist on left but not on right
        if (rightLink == rightSide.links.end())
            output.addSubLink(i->second, i->first);
        //find links that exist on left and right
        else
        {
            SymLinkMapping& newEntry = output.addSubLink(
                                           i->first,
                                           i->second,
                                           SYMLINK_EQUAL, //SYMLINK_EQUAL is just a dummy-value here
                                           rightLink->first,
                                           rightLink->second);
            appendUndefinedLink.push_back(&newEntry);
        }
    }

    //find links that exist on right but not on left
    for (DirContainer::LinkList::const_iterator j = rightSide.links.begin(); j != rightSide.links.end(); ++j)
    {
        if (leftSide.links.find(j->first) == leftSide.links.end())
            output.addSubLink(j->first, j->second);
    }


    //-----------------------------------------------------------------------------------------------
    for (DirContainer::DirList::const_iterator i = leftSide.dirs.begin(); i != leftSide.dirs.end(); ++i)
    {
        DirContainer::DirList::const_iterator rightDir = rightSide.dirs.find(i->first);

        //find directories that exist on left but not on right
        if (rightDir == rightSide.dirs.end())
        {
            DirMapping& newDirMap = output.addSubDir(i->first, Zstring());
            fillOneSide<LEFT_SIDE>(i->second, newDirMap); //recurse into subdirectories
        }
        else //directories that exist on both sides
        {
            DirMapping& newDirMap = output.addSubDir(i->first, rightDir->first);
            execute(i->second, rightDir->second, newDirMap); //recurse into subdirectories
        }
    }

    //find directories that exist on right but not on left
    for (DirContainer::DirList::const_iterator j = rightSide.dirs.begin(); j != rightSide.dirs.end(); ++j)
    {
        if (leftSide.dirs.find(j->first) == leftSide.dirs.end())
        {
            DirMapping& newDirMap = output.addSubDir(Zstring(), j->first);
            fillOneSide<RIGHT_SIDE>(j->second, newDirMap); //recurse into subdirectories
        }
    }
}


//undefinedFiles and undefinedLinks are appended only!
void CompareProcess::performBaseComparison(BaseDirMapping& output, std::vector<FileMapping*>& undefinedFiles, std::vector<SymLinkMapping*>& undefinedLinks) const
{
    assert(output.useSubDirs(). empty());
    assert(output.useSubLinks().empty());
    assert(output.useSubFiles().empty());

    //PERF_START;

    //scan directories
    const DirContainer& directoryLeft = directoryBuffer->getDirectoryDescription(
                                            output.getBaseDir<LEFT_SIDE>(),
                                            output.getFilter());

    const DirContainer& directoryRight = directoryBuffer->getDirectoryDescription(
                                             output.getBaseDir<RIGHT_SIDE>(),
                                             output.getFilter());


    statusUpdater->reportInfo(wxToZ(_("Generating file list...")));
    statusUpdater->forceUiRefresh(); //keep total number of scanned files up to date

    //PERF_STOP;

    MergeSides(undefinedFiles, undefinedLinks).execute(directoryLeft, directoryRight, output);
}
