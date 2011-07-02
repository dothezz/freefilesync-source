// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "comparison.h"
#include <map>
#include <stdexcept>
#include <memory>
#include "shared/global_func.h"
#include "shared/i18n.h"
#include <wx/msgdlg.h>
#include <wx/log.h>
#include <boost/bind.hpp>
#include "algorithm.h"
#include "shared/util.h"
#include "shared/string_conv.h"
#include "shared/file_handling.h"
#include "shared/resolve_path.h"
#include "shared/last_error.h"
#include "shared/file_traverser.h"
#include "file_hierarchy.h"
#include "library/binary.h"
#include "library/cmp_filetime.h"
#include "library/lock_holder.h"

#ifdef FFS_WIN
#include "shared/perf.h"
#endif

using namespace zen;


std::vector<zen::FolderPairCfg> zen::extractCompareCfg(const MainConfiguration& mainCfg)
{
    //merge first and additional pairs
    std::vector<FolderPairEnh> allPairs;
    allPairs.push_back(mainCfg.firstPair);
    allPairs.insert(allPairs.end(),
                    mainCfg.additionalPairs.begin(), //add additional pairs
                    mainCfg.additionalPairs.end());

    std::vector<FolderPairCfg> output;
    for (std::vector<FolderPairEnh>::const_iterator i = allPairs.begin(); i != allPairs.end(); ++i)
        output.push_back(
            FolderPairCfg(getFormattedDirectoryName(i->leftDirectory),  //ensure they end with common::FILE_NAME_SEPARATOR and replace macros
                          getFormattedDirectoryName(i->rightDirectory),
                          normalizeFilters(mainCfg.globalFilter, i->localFilter),
                          i->altSyncConfig.get() ? i->altSyncConfig->syncConfiguration : mainCfg.syncConfiguration));
    return output;
}



class BaseDirCallback;

class DirCallback : public zen::TraverseCallback
{
public:
    DirCallback(BaseDirCallback* baseCallback,
                const Zstring& relNameParentPf, //postfixed with FILE_NAME_SEPARATOR!
                DirContainer& output,
                ProcessCallback* handler) :
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
    ProcessCallback* const statusHandler;
};



class BaseDirCallback : public DirCallback
{
    friend class DirCallback;
public:
    BaseDirCallback(DirContainer& output,
                    SymLinkHandling handleSymlinks,
                    const HardFilter::FilterRef& filter,
                    ProcessCallback* handler) :
        DirCallback(this, Zstring(), output, handler),
        handleSymlinks_(handleSymlinks),
        textScanning(wxToZ(wxString(_("Scanning:")) + wxT(" \n"))),
        filterInstance(filter) {}

    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const TraverseCallback::FileInfo& details);

private:
    typedef std::shared_ptr<const DirCallback> CallbackPointer;

    const SymLinkHandling handleSymlinks_;
    const Zstring textScanning;
    std::vector<CallbackPointer> callBackBox;  //collection of callback pointers to handle ownership

    const HardFilter::FilterRef filterInstance; //always bound!
};


void DirCallback::onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details)
{
    //do not list the database file(s) sync.ffs_db, sync.x64.ffs_db, etc. or lock files
    const Zstring fileNameShort(shortName);
    const size_t pos = fileNameShort.rfind(Zchar('.'));
    if (pos != Zstring::npos)
    {
        const Zchar* ending = shortName + pos + 1;
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
                 const HardFilter::FilterRef& filterIn) : //filter interface: always bound by design!
        directoryName(dirname),
        filter(filterIn->isNull() ? //some optimization for "Null" filter
               HardFilter::FilterRef(new NullFilter) :
               filterIn) {}

    const Zstring directoryName;
    const HardFilter::FilterRef filter;  //buffering has to consider filtering!

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
                    ProcessCallback& procCallback) :
        handleSymlinks_(handleSymlinks),
        procCallback_(procCallback) {}

    const DirContainer& getDirectoryDescription(const Zstring& directoryPostfixed, const HardFilter::FilterRef& filter);

private:
    typedef std::shared_ptr<DirContainer> DirBufferValue; //exception safety: avoid memory leak
    typedef std::map<DirBufferKey, DirBufferValue> BufferType;

    DirContainer& insertIntoBuffer(const DirBufferKey& newKey);

    BufferType buffer;

    const SymLinkHandling handleSymlinks_;
    ProcessCallback& procCallback_;
};
//------------------------------------------------------------------------------------------


#ifdef FFS_WIN
class DstHackCallbackImpl : public DstHackCallback
{
public:
    DstHackCallbackImpl(ProcessCallback& procCallback) :
        textApplyingDstHack(wxToZ(_("Encoding extended time information: %x")).Replace(Zstr("%x"), Zstr("\n\"%x\""))),
        procCallback_(procCallback) {}

private:
    virtual void requestUiRefresh(const Zstring& filename) //applying DST hack imposes significant one-time performance drawback => callback to inform user
    {
        Zstring statusText = textApplyingDstHack;
        statusText.Replace(Zstr("%x"), filename);
        procCallback_.reportInfo(statusText);
        procCallback_.requestUiRefresh();
    }

    const Zstring textApplyingDstHack;
    ProcessCallback& procCallback_;
};
#endif


DirContainer& CompareProcess::DirectoryBuffer::insertIntoBuffer(const DirBufferKey& newKey)
{
    DirBufferValue baseContainer(new DirContainer);
    buffer.insert(std::make_pair(newKey, baseContainer));

    if (!newKey.directoryName.empty() &&
        zen::dirExists(newKey.directoryName)) //folder existence already checked in startCompareProcess(): do not treat as error when arriving here!
    {
        BaseDirCallback traverser(*baseContainer,
                                  handleSymlinks_,
                                  newKey.filter,
                                  &procCallback_);

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

        std::auto_ptr<zen::DstHackCallback> dstCallback;
#ifdef FFS_WIN
        dstCallback.reset(new DstHackCallbackImpl(procCallback_));
#endif

        //get all files and folders from directoryPostfixed (and subdirectories)
        traverseFolder(newKey.directoryName, followSymlinks, traverser, dstCallback.get()); //exceptions may be thrown!
    }
    return *baseContainer.get();
}


const DirContainer& CompareProcess::DirectoryBuffer::getDirectoryDescription(
    const Zstring& directoryPostfixed,
    const HardFilter::FilterRef& filter)
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
void foldersAreValidForComparison(const std::vector<FolderPairCfg>& folderPairsForm, ProcessCallback& procCallback)
{
    bool nonEmptyPairFound = false; //check if user entered at least one folder pair
    bool partiallyFilledPairFound = false;

    const wxString additionalInfo = _("You can ignore this error to consider the directory as empty.");

    for (std::vector<FolderPairCfg>::const_iterator i = folderPairsForm.begin(); i != folderPairsForm.end(); ++i)
    {
        if (!i->leftDirectoryFmt.empty() || !i->rightDirectoryFmt.empty()) //may be partially filled though
            nonEmptyPairFound = true;

        if ((i->leftDirectoryFmt.empty() && !i->rightDirectoryFmt.empty()) ||
            (!i->leftDirectoryFmt.empty() && i->rightDirectoryFmt.empty()))
            partiallyFilledPairFound = true;

        //check if folders exist
        if (!i->leftDirectoryFmt.empty())
            while (!zen::dirExists(i->leftDirectoryFmt))
            {
                wxString errorMessage = wxString(_("Directory does not exist:")) + wxT("\n") + wxT("\"") + zToWx(i->leftDirectoryFmt) + wxT("\"");
                ErrorHandler::Response rv = procCallback.reportError(errorMessage + wxT("\n\n") + additionalInfo + wxT(" ") + zen::getLastErrorFormatted());

                if (rv == ErrorHandler::IGNORE_ERROR)
                    break;
                else if (rv == ErrorHandler::RETRY)
                    ;  //continue with loop
                else
                    throw std::logic_error("Programming Error: Unknown return value! (2)");
            }

        if (!i->rightDirectoryFmt.empty())
            while (!zen::dirExists(i->rightDirectoryFmt))
            {
                wxString errorMessage = wxString(_("Directory does not exist:")) + wxT("\n") + wxT("\"") + zToWx(i->rightDirectoryFmt) + wxT("\"");
                ErrorHandler::Response rv = procCallback.reportError(errorMessage + wxT("\n\n") + additionalInfo + wxT(" ") + zen::getLastErrorFormatted());
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
            const ErrorHandler::Response rv = procCallback.reportError(wxString(_("A directory input field is empty.")) + wxT(" \n\n") +
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
        if (!i->leftDirectoryFmt.empty() && !i->rightDirectoryFmt.empty()) //empty folders names may be accepted by user
        {
            if (EqualDependentDirectory()(i->leftDirectoryFmt, i->rightDirectoryFmt)) //test wheter leftDirectory begins with rightDirectory or the other way round
                dependentDirs.push_back(std::make_pair(zToWx(i->leftDirectoryFmt), zToWx(i->rightDirectoryFmt)));
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
    CmpCallbackImpl(ProcessCallback& pc, zen::UInt64& bytesComparedLast) :
        pc_(pc),
        m_bytesComparedLast(bytesComparedLast) {}

    virtual void updateCompareStatus(zen::UInt64 totalBytesTransferred)
    {
        //called every 512 kB

        //inform about the (differential) processed amount of data
        pc_.updateProcessedData(0, to<zen::Int64>(totalBytesTransferred) - to<zen::Int64>(m_bytesComparedLast));
        m_bytesComparedLast = totalBytesTransferred;

        pc_.requestUiRefresh(); //exceptions may be thrown here!
    }

private:
    ProcessCallback& pc_;
    zen::UInt64& m_bytesComparedLast;
};


bool filesHaveSameContentUpdating(const Zstring& filename1, const Zstring& filename2, zen::UInt64 totalBytesToCmp, ProcessCallback& pc)
{
    zen::UInt64 bytesComparedLast; //amount of bytes that have been compared and communicated to status handler

    CmpCallbackImpl callback(pc, bytesComparedLast);

    bool sameContent = true;
    try
    {
        sameContent = filesHaveSameContent(filename1, filename2, callback); //throw FileError
    }
    catch (...)
    {
        //error situation: undo communication of processed amount of data
        pc.updateProcessedData(0, -1 * to<zen::Int64>(bytesComparedLast));
        throw;
    }

    //inform about the (remaining) processed amount of data
    pc.updateProcessedData(0, to<zen::Int64>(totalBytesToCmp) - to<zen::Int64>(bytesComparedLast));

    return sameContent;
}


struct ToBeRemoved
{
    bool operator()(const DirMapping& dirObj) const
    {
        return !dirObj.isActive() &&
               dirObj.useSubDirs ().empty() &&
               dirObj.useSubLinks().empty() &&
               dirObj.useSubFiles().empty();
    }
};


class RemoveFilteredDirs
{
public:
    RemoveFilteredDirs(const HardFilter& filterProc) :
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

    const HardFilter& filterProc_;
};
}

//#############################################################################################################################

CompareProcess::CompareProcess(SymLinkHandling handleSymlinks,
                               size_t fileTimeTol,
                               xmlAccess::OptionalDialogs& warnings,
                               ProcessCallback& handler) :
    fileTimeTolerance(fileTimeTol),
    m_warnings(warnings),
    procCallback(handler),
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
    procCallback.initNewProcess(-1, 0, ProcessCallback::PROCESS_SCANNING); //it's not known how many files will be scanned => -1 objects

    //-------------------some basic checks:------------------------------------------

    //ensure that folders are valid
    foldersAreValidForComparison(directoryPairs, procCallback);

    {
        //check if folders have dependencies
        wxString warningMessage = checkFolderDependency(directoryPairs);
        if (!warningMessage.empty())
            procCallback.reportWarning(warningMessage.c_str(), m_warnings.warningDependentFolders);
    }

    //-------------------end of basic checks------------------------------------------


    try
    {
        //prevent shutdown while (binary) comparison is in progress
        util::DisableStandby dummy2;
        (void)dummy2;

        //traverse/process folders
        FolderComparison output_tmp; //write to output not before END of process!
        switch (cmpVar)
        {
            case CMP_BY_TIME_SIZE:
                compareByTimeSize(directoryPairs, output_tmp);
                break;
            case CMP_BY_CONTENT:
                compareByContent(directoryPairs, output_tmp);
                break;
        }


        assert (output_tmp.size() == directoryPairs.size());

        for (FolderComparison::iterator j = output_tmp.begin(); j != output_tmp.end(); ++j)
        {
            const FolderPairCfg& fpCfg = directoryPairs[j - output_tmp.begin()];

            //attention: some filtered directories are still in the comparison result! (see include filter handling!)
            if (!fpCfg.filter.nameFilter->isNull())
                RemoveFilteredDirs(*fpCfg.filter.nameFilter).execute(*j); //remove all excluded directories (but keeps those serving as parent folders for not excl. elements)

            //apply soft filtering / hard filter already applied
            addSoftFiltering(*j, fpCfg.filter.timeSizeFilter);

            //set initial sync-direction
            class RedetermineCallback : public DeterminationProblem
            {
            public:
                RedetermineCallback(bool& warningSyncDatabase, ProcessCallback& procCallback) :
                    warningSyncDatabase_(warningSyncDatabase),
                    procCallback_(procCallback) {}

                virtual void reportWarning(const wxString& text)
                {
                    procCallback_.reportWarning(text, warningSyncDatabase_);
                }
            private:
                bool& warningSyncDatabase_;
                ProcessCallback& procCallback_;
            } redetCallback(m_warnings.warningSyncDatabase, procCallback);

            zen::redetermineSyncDirection(fpCfg.syncConfiguration, *j, &redetCallback);
        }

        //only if everything was processed correctly output is written to!
        //note: output mustn't change during this process to be in sync with GUI grid view!!!
        output_tmp.swap(output);
    }
    catch (const std::bad_alloc& e)
    {
        procCallback.reportFatalError(wxString(_("Memory allocation failed!")) + wxT(" ") + wxString::FromAscii(e.what()));
    }
    catch (const std::exception& e)
    {
        procCallback.reportFatalError(wxString::FromAscii(e.what()));
    }
}

//--------------------assemble conflict descriptions---------------------------

//check for very old dates or dates in the future
wxString getConflictInvalidDate(const Zstring& fileNameFull, zen::Int64 utcTime)
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
           wxT(" \t") + _("Size") + wxT(": ") + toStringSep(fileObj.getFileSize<LEFT_SIDE>()) + wxT("\n");
    msg += right + wxT("\t") + _("Date") + wxT(": ") + utcTimeToLocalString(fileObj.getLastWriteTime<RIGHT_SIDE>()) +
           wxT(" \t") + _("Size") + wxT(": ") + toStringSep(fileObj.getFileSize<RIGHT_SIDE>());
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


void CompareProcess::compareByTimeSize(const std::vector<FolderPairCfg>& directoryPairs, FolderComparison& output) const
{
    output.reserve(output.size() + directoryPairs.size());

    //process one folder pair after each other
    for (std::vector<FolderPairCfg>::const_iterator pair = directoryPairs.begin(); pair != directoryPairs.end(); ++pair)
    {
        BaseDirMapping newEntry(pair->leftDirectoryFmt,
                                pair->rightDirectoryFmt,
                                pair->filter.nameFilter);
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


zen::UInt64 getBytesToCompare(const std::vector<FileMapping*>& rowsToCompare)
{
    zen::UInt64 dataTotal;

    for (std::vector<FileMapping*>::const_iterator j = rowsToCompare.begin(); j != rowsToCompare.end(); ++j)
        dataTotal += (*j)->getFileSize<LEFT_SIDE>();  //left and right filesizes should be the same

    return dataTotal * 2U;
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


void CompareProcess::compareByContent(const std::vector<FolderPairCfg>& directoryPairs, FolderComparison& output) const
{
    //PERF_START;
    std::vector<FileMapping*> compareCandidates;

    //attention: make sure pointers in "compareCandidates" remain valid!!!
    output.reserve(output.size() + directoryPairs.size());

    //process one folder pair after each other
    for (std::vector<FolderPairCfg>::const_iterator pair = directoryPairs.begin(); pair != directoryPairs.end(); ++pair)
    {
        BaseDirMapping newEntry(pair->leftDirectoryFmt,
                                pair->rightDirectoryFmt,
                                pair->filter.nameFilter);
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
    const zen::UInt64 bytesTotal = getBytesToCompare(filesToCompareBytewise);

    procCallback.initNewProcess(static_cast<int>(objectsTotal),
                                to<zen::Int64>(bytesTotal),
                                ProcessCallback::PROCESS_COMPARING_CONTENT);

    const CmpFileTime timeCmp(fileTimeTolerance);

    //compare files (that have same size) bytewise...
    for (std::vector<FileMapping*>::const_iterator j = filesToCompareBytewise.begin(); j != filesToCompareBytewise.end(); ++j)
    {
        FileMapping* const line = *j;

        Zstring statusText = txtComparingContentOfFiles;
        statusText.Replace(Zstr("%x"), line->getRelativeName<LEFT_SIDE>(), false);
        procCallback.reportInfo(statusText);

        //check files that exist in left and right model but have different content
        while (true)
        {
            //trigger display refresh
            procCallback.requestUiRefresh();

            try
            {
                if (filesHaveSameContentUpdating(line->getFullName<LEFT_SIDE>(),
                                                 line->getFullName<RIGHT_SIDE>(),
                                                 line->getFileSize<LEFT_SIDE>() * 2U,
                                                 procCallback))
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

                procCallback.updateProcessedData(2, 0); //processed data is communicated in subfunctions!
                break;
            }
            catch (FileError& error)
            {
                ErrorHandler::Response rv = procCallback.reportError(error.msg());
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


    procCallback.reportInfo(wxToZ(_("Generating file list...")));
    procCallback.forceUiRefresh(); //keep total number of scanned files up to date

    //PERF_STOP;

    MergeSides(undefinedFiles, undefinedLinks).execute(directoryLeft, directoryRight, output);
}
