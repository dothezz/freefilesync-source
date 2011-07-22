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
#include "shared/loki/ScopeGuard.h"
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
#include "library/db_file.h"

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
    std::transform(allPairs.begin(), allPairs.end(), std::back_inserter(output),
                   [&](const FolderPairEnh& enhPair) -> FolderPairCfg
                   {
            return FolderPairCfg(getFormattedDirectoryName(enhPair.leftDirectory),  //ensure they end with FILE_NAME_SEPARATOR and replace macros
                          getFormattedDirectoryName(enhPair.rightDirectory),
                          normalizeFilters(mainCfg.globalFilter, enhPair.localFilter),
                          mainCfg.handleSymlinks,
                          enhPair.altSyncConfig.get() ? enhPair.altSyncConfig->syncConfiguration : mainCfg.syncConfiguration);
                   });
    return output;
}


class DirCallback;

struct TraverserConfig
{
public:
    TraverserConfig(SymLinkHandling handleSymlinks,
                    const HardFilter::FilterRef& filter,
                    std::set<Zstring>& failedReads,
                    ProcessCallback& handler) :
        handleSymlinks_(handleSymlinks),
        textScanning(toZ(_("Scanning:")) + Zstr(" \n")),
        filterInstance(filter),
        statusHandler(handler),
        failedReads_(failedReads) {}

    typedef std::shared_ptr<DirCallback> CallbackPointer;
    std::vector<CallbackPointer> callBackBox;  //collection of callback pointers to handle ownership

    const SymLinkHandling handleSymlinks_;
    const Zstring textScanning;
    const HardFilter::FilterRef filterInstance; //always bound!
    ProcessCallback& statusHandler;

    std::set<Zstring>& failedReads_; //relative postfixed names of directories that could not be read (empty for root)
};


class DirCallback : public zen::TraverseCallback
{
public:
    DirCallback(TraverserConfig& config,
                const Zstring& relNameParentPf, //postfixed with FILE_NAME_SEPARATOR!
                DirContainer& output) :
        cfg(config),
        relNameParentPf_(relNameParentPf),
        output_(output) {}

    virtual void         onFile   (const Zchar* shortName, const Zstring& fullName, const FileInfo& details);
    virtual void         onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details);
    virtual ReturnValDir onDir    (const Zchar* shortName, const Zstring& fullName);
    virtual HandleError  onError  (const std::wstring& errorText);

private:
    TraverserConfig& cfg;
    const Zstring relNameParentPf_;
    DirContainer& output_;
};


void DirCallback::onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details)
{
    const Zstring fileNameShort = shortName;

    //do not list the database file(s) sync.ffs_db, sync.x64.ffs_db, etc. or lock files
    if (endsWith(fileNameShort, SYNC_DB_FILE_ENDING) ||
        endsWith(fileNameShort, LOCK_FILE_ENDING))
        return;

    //assemble status message (performance optimized)  = textScanning + wxT("\"") + fullName + wxT("\"")
    Zstring statusText = cfg.textScanning;
    statusText.reserve(statusText.length() + fullName.length() + 2);
    statusText += Zchar('\"');
    statusText += fullName;
    statusText += Zchar('\"');

    //update UI/commandline status information
    cfg.statusHandler.reportInfo(utf8CvrtTo<wxString>(statusText));

    //------------------------------------------------------------------------------------
    //apply filter before processing (use relative name!)
    if (!cfg.filterInstance->passFileFilter(relNameParentPf_ + fileNameShort))
        return;

    //warning: for windows retrieveFileID is slow as hell! approximately 3 * 10^-4 s per file!
    //therefore only large files (that take advantage of detection of renaming when synchronizing) should be evaluated!
    //testcase: scanning only files larger than 1 MB results in performance loss of 6%

    //#warning this call is NOT acceptable for Linux!
    //    //Linux: retrieveFileID takes about 50% longer in VM! (avoidable because of redundant stat() call!)
    //    const util::FileID fileIdentifier = details.fileSize >= cfg.detectRenameThreshold_ ?
    //                                           util::retrieveFileID(fullName) :
    //                                           util::FileID();

    output_.addSubFile(fileNameShort, FileDescriptor(details.lastWriteTimeRaw, details.fileSize));

    //add 1 element to the progress indicator
    cfg.statusHandler.updateProcessedData(1, 0); //NO performance issue at all
    cfg.statusHandler.requestUiRefresh(); //may throw
}


void DirCallback::onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details)
{
    if (cfg.handleSymlinks_ == SYMLINK_IGNORE)
        return;

    //assemble status message (performance optimized)  = textScanning + wxT("\"") + fullName + wxT("\"")
    Zstring statusText = cfg.textScanning;
    statusText.reserve(statusText.length() + fullName.length() + 2);
    statusText += Zchar('\"');
    statusText += fullName;
    statusText += Zchar('\"');

    //update UI/commandline status information
    cfg.statusHandler.reportInfo(utf8CvrtTo<wxString>(statusText));

    //------------------------------------------------------------------------------------
    const Zstring& relName = relNameParentPf_ + shortName;

    //apply filter before processing (use relative name!)
    if (!cfg.filterInstance->passFileFilter(relName)) //always use file filter: Link type may not be "stable" on Linux!
        return;

    output_.addSubLink(shortName, LinkDescriptor(details.lastWriteTimeRaw, details.targetPath, details.dirLink ? LinkDescriptor::TYPE_DIR : LinkDescriptor::TYPE_FILE));

    //add 1 element to the progress indicator
    cfg.statusHandler.updateProcessedData(1, 0); //NO performance issue at all
    cfg.statusHandler.requestUiRefresh(); //may throw
}


TraverseCallback::ReturnValDir DirCallback::onDir(const Zchar* shortName, const Zstring& fullName)
{
    //assemble status message (performance optimized)  = textScanning + wxT("\"") + fullName + wxT("\"")
    Zstring statusText = cfg.textScanning;
    statusText.reserve(statusText.length() + fullName.length() + 2);
    statusText += Zchar('\"');
    statusText += fullName;
    statusText += Zchar('\"');

    //update UI/commandline status information
    cfg.statusHandler.reportInfo(utf8CvrtTo<wxString>(statusText));

    //------------------------------------------------------------------------------------
    const Zstring& relName = relNameParentPf_ + shortName;

    //apply filter before processing (use relative name!)
    bool subObjMightMatch = true;
    if (!cfg.filterInstance->passDirFilter(relName, &subObjMightMatch))
    {
        if (!subObjMightMatch)
            return Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_IGNORE>(); //do NOT traverse subdirs
    }
    else
    {
        cfg.statusHandler.updateProcessedData(1, 0); //NO performance issue at all
        cfg.statusHandler.requestUiRefresh(); //may throw
    }

    DirContainer& subDir = output_.addSubDir(shortName);

    TraverserConfig::CallbackPointer subDirCallback = std::make_shared<DirCallback>(cfg, relName + FILE_NAME_SEPARATOR, subDir);
    cfg.callBackBox.push_back(subDirCallback); //handle lifetime
    //attention: ensure directory filtering is applied later to exclude actually filtered directories
    return ReturnValDir(Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_CONTINUE>(), *subDirCallback.get());
}


DirCallback::HandleError DirCallback::onError(const std::wstring& errorText)
{
    switch (cfg.statusHandler.reportError(errorText))
    {
        case ProcessCallback::IGNORE_ERROR:
            cfg.failedReads_.insert(relNameParentPf_);
            return TRAV_ERROR_IGNORE;

        case ProcessCallback::RETRY:
            return TRAV_ERROR_RETRY;
    }

    assert(false);
    return TRAV_ERROR_IGNORE;
}


//------------------------------------------------------------------------------------------
struct DirBufferKey
{
    DirBufferKey(const Zstring& dirname,
                 const HardFilter::FilterRef& filterIn, //filter interface: always bound by design!
                 SymLinkHandling handleSymlinks) :
        directoryName(dirname),
        filter(filterIn),
        handleSymlinks_(handleSymlinks) {}

    const Zstring directoryName;
    const HardFilter::FilterRef filter;
    const SymLinkHandling handleSymlinks_;

    bool operator<(const DirBufferKey& other) const
    {
        if (handleSymlinks_ != other.handleSymlinks_)
            return handleSymlinks_ < other.handleSymlinks_;

        if (!EqualFilename()(directoryName, other.directoryName))
            return LessFilename()(directoryName, other.directoryName);

        return *filter < *other.filter;
    }
};


struct DirBufferValue
{
    DirContainer dirCont;
    std::set<Zstring> failedReads; //relative postfixed names of directories that could not be read (empty for root), e.g. access denied, or temporal network drop
};


//------------------------------------------------------------------------------------------
class CompareProcess::DirectoryBuffer  //buffer multiple scans of the same directories
{
public:
    DirectoryBuffer(ProcessCallback& procCallback) :
        procCallback_(procCallback) {}

    const DirBufferValue& getDirectoryDescription(const Zstring& directoryPostfixed, const HardFilter::FilterRef& filter, SymLinkHandling handleSymlinks)
    {
        const DirBufferKey searchKey(directoryPostfixed, filter, handleSymlinks);

        auto iter = buffer.find(searchKey);
        if (iter != buffer.end())
            return iter->second; //entry found in buffer; return
        else
            return insertIntoBuffer(searchKey); //entry not found; create new one
    }

private:
    typedef std::map<DirBufferKey, DirBufferValue> BufferType;

    DirBufferValue& insertIntoBuffer(const DirBufferKey& newKey);

    BufferType buffer;

    ProcessCallback& procCallback_;
};
//------------------------------------------------------------------------------------------


#ifdef FFS_WIN
class DstHackCallbackImpl : public DstHackCallback
{
public:
    DstHackCallbackImpl(ProcessCallback& procCallback) :
        textApplyingDstHack(toZ(_("Encoding extended time information: %x")).Replace(Zstr("%x"), Zstr("\n\"%x\""))),
        procCallback_(procCallback) {}

private:
    virtual void requestUiRefresh(const Zstring& filename) //applying DST hack imposes significant one-time performance drawback => callback to inform user
    {
        Zstring statusText = textApplyingDstHack;
        statusText.Replace(Zstr("%x"), filename);
        procCallback_.reportInfo(utf8CvrtTo<wxString>(statusText));
    }

    const Zstring textApplyingDstHack;
    ProcessCallback& procCallback_;
};
#endif


DirBufferValue& CompareProcess::DirectoryBuffer::insertIntoBuffer(const DirBufferKey& newKey)
{
    DirBufferValue& bufferVal = buffer[newKey]; //default construct value

    if (!newKey.directoryName.empty() &&
        zen::dirExists(newKey.directoryName)) //folder existence already checked in startCompareProcess(): do not treat as error when arriving here!
    {
        TraverserConfig travCfg(newKey.handleSymlinks_, //shared by all(!) instances of DirCallback while traversing a folder hierarchy
                                newKey.filter,
                                bufferVal.failedReads,
                                procCallback_);

        DirCallback traverser(travCfg,
                              Zstring(),
                              bufferVal.dirCont);

        bool followSymlinks = false;
        switch (newKey.handleSymlinks_)
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

        DstHackCallback* dstCallbackPtr = NULL;
#ifdef FFS_WIN
        DstHackCallbackImpl dstCallback(procCallback_);
        dstCallbackPtr = &dstCallback;
#endif

        //get all files and folders from directoryPostfixed (and subdirectories)
        traverseFolder(newKey.directoryName, followSymlinks, traverser, dstCallbackPtr); //exceptions may be thrown!
    }
    return bufferVal;
}


//------------------------------------------------------------------------------------------
namespace
{
void foldersAreValidForComparison(const std::vector<FolderPairCfg>& folderPairsForm, ProcessCallback& procCallback)
{
    bool nonEmptyPairFound = false; //check if user entered at least one folder pair
    bool partiallyFilledPairFound = false;

    const std::wstring additionalInfo = _("You can ignore this error to consider the directory as empty.");

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
                std::wstring errorMessage = _("Directory does not exist:") + "\n" + "\"" + i->leftDirectoryFmt + "\"";
                ProcessCallback::Response rv = procCallback.reportError(errorMessage + "\n\n" + additionalInfo + " " + zen::getLastErrorFormatted());

                if (rv == ProcessCallback::IGNORE_ERROR)
                    break;
                else if (rv == ProcessCallback::RETRY)
                    ;  //continue with loop
                else
                    throw std::logic_error("Programming Error: Unknown return value! (2)");
            }

        if (!i->rightDirectoryFmt.empty())
            while (!zen::dirExists(i->rightDirectoryFmt))
            {
                std::wstring errorMessage = _("Directory does not exist:") + "\n" + "\"" + i->rightDirectoryFmt + "\"";
                ProcessCallback::Response rv = procCallback.reportError(errorMessage + "\n\n" + additionalInfo + " " + zen::getLastErrorFormatted());
                if (rv == ProcessCallback::IGNORE_ERROR)
                    break;
                else if (rv == ProcessCallback::RETRY)
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
            const ProcessCallback::Response rv = procCallback.reportError(_("A directory input field is empty.") + " \n\n" +
                                                                          + "(" + additionalInfo + ")");
            if (rv == ProcessCallback::IGNORE_ERROR)
                break;
            else if (rv == ProcessCallback::RETRY)
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
                dependentDirs.push_back(std::make_pair(toWx(i->leftDirectoryFmt), toWx(i->rightDirectoryFmt)));
        }

    wxString warnignMsg;

    if (!dependentDirs.empty())
    {
        warnignMsg = _("Directories are dependent! Be careful when setting up synchronization rules:");
        for (auto i = dependentDirs.begin(); i != dependentDirs.end(); ++i)
            warnignMsg += wxString(L"\n\n") +
                          "\"" + i->first  + "\"\n" +
                          "\"" + i->second + "\"";
    }
    return warnignMsg;
}


//callback implementation
class CmpCallbackImpl : public CompareCallback
{
public:
    CmpCallbackImpl(ProcessCallback& pc, UInt64& bytesReported) :
        pc_(pc),
        bytesReported_(bytesReported) {}

    virtual void updateCompareStatus(UInt64 totalBytesTransferred)
    {
        //inform about the (differential) processed amount of data
        pc_.updateProcessedData(0, to<Int64>(totalBytesTransferred) - to<Int64>(bytesReported_)); //throw()! -> this ensures client and service provider are in sync!
        bytesReported_ = totalBytesTransferred;                                                   //

        pc_.requestUiRefresh(); //may throw
    }

private:
    ProcessCallback& pc_;
    UInt64& bytesReported_;
};


bool filesHaveSameContentUpdating(const Zstring& filename1, const Zstring& filename2, UInt64 totalBytesToCmp, ProcessCallback& pc)
{
    UInt64 bytesReported; //amount of bytes that have been compared and communicated to status handler

    //in error situation: undo communication of processed amount of data
    Loki::ScopeGuard guardStatistics = Loki::MakeGuard([&]() { pc.updateProcessedData(0, -1 * to<Int64>(bytesReported)); });

    CmpCallbackImpl callback(pc, bytesReported);
    bool sameContent = filesHaveSameContent(filename1, filename2, callback); //throw FileError

    //inform about the (remaining) processed amount of data
    pc.updateProcessedData(0, to<Int64>(totalBytesToCmp) - to<Int64>(bytesReported));
    bytesReported = totalBytesToCmp;

    guardStatistics.Dismiss();
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

CompareProcess::CompareProcess(size_t fileTimeTol,
                               xmlAccess::OptionalDialogs& warnings,
                               ProcessCallback& handler) :
    fileTimeTolerance(fileTimeTol),
    m_warnings(warnings),
    procCallback(handler),
    txtComparingContentOfFiles(toZ(_("Comparing content of files %x")).Replace(Zstr("%x"), Zstr("\n\"%x\""), false))
{
    directoryBuffer.reset(new DirectoryBuffer(handler));
}


CompareProcess::~CompareProcess() {} //std::auto_ptr does not work with forward declarations (Or we need a non-inline ~CompareProcess())!


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
        procCallback.reportFatalError(_("Memory allocation failed!") + " " + e.what());
    }
    catch (const std::exception& e)
    {
        procCallback.reportFatalError(wxString::FromAscii(e.what()));
    }
}

//--------------------assemble conflict descriptions---------------------------

//check for very old dates or date2s in the future
wxString getConflictInvalidDate(const Zstring& fileNameFull, zen::Int64 utcTime)
{
    wxString msg = _("File %x has an invalid date!");
    replace(msg, L"%x", wxString(L"\"") + fileNameFull + "\"");
    msg += wxString(L"\n\n") + _("Date") + ": " + utcTimeToLocalString(utcTime);
    return wxString(_("Conflict detected:")) + "\n" + msg;
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
    replace(msg, wxT("%x"), wxString(wxT("\"")) + fileObj.getRelativeName<LEFT_SIDE>() + "\"");
    msg += L"\n\n";
    msg += left + "\t" + _("Date") + ": " + utcTimeToLocalString(fileObj.getLastWriteTime<LEFT_SIDE>()) +
           " \t" + _("Size") + ": " + toStringSep(fileObj.getFileSize<LEFT_SIDE>()) + wxT("\n");
    msg += right + "\t" + _("Date") + ": " + utcTimeToLocalString(fileObj.getLastWriteTime<RIGHT_SIDE>()) +
           " \t" + _("Size") + ": " + toStringSep(fileObj.getFileSize<RIGHT_SIDE>());
    return _("Conflict detected:") + "\n" + msg;
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
                wxString conflictMsg = _("Conflict detected:") + "\n" + _("Symlinks %x have the same date but a different target!");
                replace(conflictMsg, L"%x", wxString(L"\"") + linkObj->getRelativeName<LEFT_SIDE>() + "\"");
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


void CompareProcess::compareByTimeSize(const std::vector<FolderPairCfg>& directoryPairs, FolderComparison& output)
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
        performComparison(*pair, output.back(), uncategorizedFiles, uncategorizedLinks);

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


void CompareProcess::compareByContent(const std::vector<FolderPairCfg>& directoryPairs, FolderComparison& output)
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
        performComparison(*pair, output.back(), compareCandidates, uncategorizedLinks);

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
        procCallback.reportInfo(utf8CvrtTo<wxString>(statusText));

        //check files that exist in left and right model but have different content
        while (true)
        {
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
                procCallback.requestUiRefresh(); //may throw
                break;
            }
            catch (FileError& error)
            {
                ProcessCallback::Response rv = procCallback.reportError(error.msg());
                if (rv == ProcessCallback::IGNORE_ERROR)
                {
                    line->setCategoryConflict(wxString(_("Conflict detected:")) + wxT("\n") + _("Comparing files by content failed."));
                    break;
                }

                else if (rv == ProcessCallback::RETRY)
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


//create comparison result table and fill category except for files existing on both sides: undefinedFiles and undefinedLinks are appended!
void CompareProcess::performComparison(const FolderPairCfg& fpCfg,
                                       BaseDirMapping& output,
                                       std::vector<FileMapping*>& undefinedFiles,
                                       std::vector<SymLinkMapping*>& undefinedLinks)
{
    assert(output.useSubDirs(). empty());
    assert(output.useSubLinks().empty());
    assert(output.useSubFiles().empty());

    //PERF_START;

    //scan directories
    const DirBufferValue& bufValueLeft = directoryBuffer->getDirectoryDescription(
                                             output.getBaseDir<LEFT_SIDE>(),
                                             fpCfg.filter.nameFilter,
                                             fpCfg.handleSymlinks);

    const DirBufferValue& bufValueRight = directoryBuffer->getDirectoryDescription(
                                              output.getBaseDir<RIGHT_SIDE>(),
                                              fpCfg.filter.nameFilter,
                                              fpCfg.handleSymlinks);

    procCallback.reportInfo(_("Generating file list..."));
    procCallback.forceUiRefresh(); //keep total number of scanned files up to date

    //PERF_STOP;
    MergeSides(undefinedFiles, undefinedLinks).execute(bufValueLeft.dirCont, bufValueRight.dirCont, output);

    //##################### in/exclude rows according to filtering #####################

    //attention: some filtered directories are still in the comparison result! (see include filter handling!)
    if (!fpCfg.filter.nameFilter->isNull())
        RemoveFilteredDirs(*fpCfg.filter.nameFilter).execute(output); //remove all excluded directories (but keeps those serving as parent folders for not excl. elements)

    //apply soft filtering / hard filter already applied
    addSoftFiltering(output, fpCfg.filter.timeSizeFilter);

    //properly handle (user-ignored) traversing errors: just uncheck them, no need to physically delete them (<automatic> mode will be grateful)
    std::set<Zstring> failedReads;
    failedReads.insert(bufValueLeft .failedReads.begin(), bufValueLeft .failedReads.end());
    failedReads.insert(bufValueRight.failedReads.begin(), bufValueRight.failedReads.end());

    if (!failedReads.empty())
    {
        Zstring filterFailedRead;
		 //exclude subfolders only
        std::for_each(failedReads.begin(), failedReads.end(),
                      [&](const Zstring& relDirPf) { filterFailedRead += relDirPf + Zstr("?*\n"); });
                      //note: relDirPf is empty for base dir, otherwise postfixed! e.g. "subdir\"

        addHardFiltering(output, filterFailedRead);
    }
    //##################################################################################
}
