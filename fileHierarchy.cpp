#include "fileHierarchy.h"
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include "shared/globalFunctions.h"
#include "shared/fileError.h"
#include <boost/scoped_array.hpp>
#include <wx/intl.h>
#include "shared/stringConv.h"
#include "shared/fileHandling.h"
#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#endif

using namespace FreeFileSync;
using namespace globalFunctions;


struct LowerID
{
    bool operator()(const FileSystemObject& a, HierarchyObject::ObjectID b) const
    {
        return a.getId() < b;
    }

    bool operator()(const FileSystemObject& a, const FileSystemObject& b) const //used by VC++
    {
        return a.getId() < b.getId();
    }

    bool operator()(HierarchyObject::ObjectID a, const FileSystemObject& b) const
    {
        return a < b.getId();
    }
};


const FileSystemObject* HierarchyObject::retrieveById(ObjectID id) const //returns NULL if object is not found
{
    //ATTENTION: HierarchyObject::retrieveById() can only work correctly if the following conditions are fulfilled:
    //1. on each level, files are added first, then directories (=> file id < dir id)
    //2. when a directory is added, all subdirectories must be added immediately (recursion) before the next dir on this level is added
    //3. entries may be deleted but NEVER new ones inserted!!!
    //=> this allows for a quasi-binary search by id!

    //See MergeSides::execute()!


    //search within sub-files
    SubFileMapping::const_iterator i = std::lower_bound(subFiles.begin(), subFiles.end(), id, LowerID()); //binary search!
    if (i != subFiles.end())
    {
        //id <= i
        if (LowerID()(id, *i))
            return NULL; // --i < id < i
        else //id found
            return &(*i);
    }
    else //search within sub-directories
    {
        SubDirMapping::const_iterator j = std::lower_bound(subDirs.begin(), subDirs.end(), id, LowerID()); //binary search!
        if (j != subDirs.end() && !LowerID()(id, *j)) //id == j
            return &(*j);
        else if (j == subDirs.begin()) //either begin() == end() or id < begin()
            return NULL;
        else
            return (--j)->retrieveById(id); //j != begin() and id < j
    }
}


struct IsInvalid
{
    bool operator()(const FileSystemObject& fsObj) const
    {
        return fsObj.isEmpty();
    }
};


void FileSystemObject::removeEmptyNonRec(HierarchyObject& hierObj)
{
    //remove invalid files
    hierObj.subFiles.erase(std::remove_if(hierObj.subFiles.begin(), hierObj.subFiles.end(), IsInvalid()),
                           hierObj.subFiles.end());

    //remove invalid directories
    hierObj.subDirs.erase(std::remove_if(hierObj.subDirs.begin(), hierObj.subDirs.end(), IsInvalid()),
                          hierObj.subDirs.end());
}


void removeEmptyRec(HierarchyObject& hierObj)
{
    FileSystemObject::removeEmptyNonRec(hierObj);

    //recurse into remaining directories
    std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), removeEmptyRec);
}


void FileSystemObject::removeEmpty(BaseDirMapping& baseDir)
{
    removeEmptyRec(baseDir);
}


SyncOperation FileSystemObject::getSyncOperation(const CompareFilesResult cmpResult,
        const bool selectedForSynchronization,
        const SyncDirectionIntern syncDir)
{
    if (!selectedForSynchronization) return SO_DO_NOTHING;

    switch (cmpResult)
    {
    case FILE_LEFT_SIDE_ONLY:
        switch (syncDir)
        {
        case SYNC_DIR_INT_LEFT:
            return SO_DELETE_LEFT; //delete files on left
        case SYNC_DIR_INT_RIGHT:
            return SO_CREATE_NEW_RIGHT; //copy files to right
        case SYNC_DIR_INT_NONE:
            return SO_DO_NOTHING;
        case SYNC_DIR_INT_CONFLICT:
            return SO_UNRESOLVED_CONFLICT;
        }
        break;

    case FILE_RIGHT_SIDE_ONLY:
        switch (syncDir)
        {
        case SYNC_DIR_INT_LEFT:
            return SO_CREATE_NEW_LEFT; //copy files to left
        case SYNC_DIR_INT_RIGHT:
            return SO_DELETE_RIGHT; //delete files on right
        case SYNC_DIR_INT_NONE:
            return SO_DO_NOTHING;
        case SYNC_DIR_INT_CONFLICT:
            return SO_UNRESOLVED_CONFLICT;
        }
        break;

    case FILE_LEFT_NEWER:
    case FILE_RIGHT_NEWER:
    case FILE_DIFFERENT:
        switch (syncDir)
        {
        case SYNC_DIR_INT_LEFT:
            return SO_OVERWRITE_LEFT; //copy from right to left
        case SYNC_DIR_INT_RIGHT:
            return SO_OVERWRITE_RIGHT; //copy from left to right
        case SYNC_DIR_INT_NONE:
            return SO_DO_NOTHING;
        case SYNC_DIR_INT_CONFLICT:
            return SO_UNRESOLVED_CONFLICT;
        }
        break;

    case FILE_CONFLICT:
        switch (syncDir)
        {
        case SYNC_DIR_INT_LEFT:
            return SO_OVERWRITE_LEFT; //copy from right to left
        case SYNC_DIR_INT_RIGHT:
            return SO_OVERWRITE_RIGHT; //copy from left to right
        case SYNC_DIR_INT_NONE:
        case SYNC_DIR_INT_CONFLICT:
            return SO_UNRESOLVED_CONFLICT;
        }
        break;

    case FILE_EQUAL:
        assert(syncDir == SYNC_DIR_INT_NONE);
        return SO_DO_NOTHING;
    }

    return SO_DO_NOTHING; //dummy
}


//-------------------------------------------------------------------------------------------------
const Zstring& FreeFileSync::getSyncDBFilename()
{
#ifdef FFS_WIN
    static Zstring output(DefaultStr("sync.ffs_db"));
#elif defined FFS_LINUX
    static Zstring output(DefaultStr(".sync.ffs_db")); //files beginning with dots are hidden e.g. in Nautilus
#endif
    return output;
}


inline
Zstring readString(wxInputStream& stream)  //read string from file stream
{
    const unsigned int strLength = readNumber<unsigned int>(stream);
    if (strLength <= 1000)
    {
        DefaultChar buffer[1000];
        stream.Read(buffer, sizeof(DefaultChar) * strLength);
        return Zstring(buffer, strLength);
    }
    else
    {
        boost::scoped_array<DefaultChar> buffer(new DefaultChar[strLength]);
        stream.Read(buffer.get(), sizeof(DefaultChar) * strLength);
        return Zstring(buffer.get(), strLength);
    }
}


inline
void writeString(wxOutputStream& stream, const Zstring& str)  //write string to filestream
{
    globalFunctions::writeNumber<unsigned int>(stream, str.length());
    stream.Write(str.c_str(), sizeof(DefaultChar) * str.length());
}


//-------------------------------------------------------------------------------------------------------------------------------
const char FILE_FORMAT_DESCR[] = "FreeFileSync";
const int FILE_FORMAT_VER = 1;
//-------------------------------------------------------------------------------------------------------------------------------


template <SelectedSide side>
struct IsNonEmpty
{
    bool operator()(const FileSystemObject& fsObj) const
    {
        return !fsObj.isEmpty<side>();
    }
};


template <SelectedSide side>
class SaveRecursively
{
public:
    SaveRecursively(const BaseDirMapping& baseMapping, const Zstring& filename, wxOutputStream& stream) : filename_(filename), stream_(stream)
    {
        //save file format version
        writeNumberC<int>(FILE_FORMAT_VER);

        //save filter settings
        writeNumberC<bool>(baseMapping.getFilter().filterActive);
        writeStringC(baseMapping.getFilter().includeFilter.c_str());
        writeStringC(baseMapping.getFilter().excludeFilter.c_str());

        //start recursion
        execute(baseMapping);
    }

private:
    template<typename Iterator, typename Function>
    friend Function std::for_each(Iterator, Iterator, Function);

    void execute(const HierarchyObject& hierObj)
    {
        writeNumberC<unsigned int>(std::count_if(hierObj.subFiles.begin(), hierObj.subFiles.end(), IsNonEmpty<side>())); //number of (existing) files
        std::for_each(hierObj.subFiles.begin(), hierObj.subFiles.end(), *this);

        writeNumberC<unsigned int>(std::count_if(hierObj.subDirs.begin(), hierObj.subDirs.end(), IsNonEmpty<side>())); //number of (existing) directories
        std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), *this);
    }

    void operator()(const FileMapping& fileMap)
    {
        if (!fileMap.isEmpty<side>())
        {
            writeStringC(fileMap.getObjShortName()); //file name
            writeNumberC<long>(         fileMap.getLastWriteTime<side>().GetHi()); //last modification time
            writeNumberC<unsigned long>(fileMap.getLastWriteTime<side>().GetLo()); //
            writeNumberC<unsigned long>(fileMap.getFileSize<side>().GetHi()); //filesize
            writeNumberC<unsigned long>(fileMap.getFileSize<side>().GetLo()); //
        }
    }

    void operator()(const DirMapping& dirMap)
    {
        if (!dirMap.isEmpty<side>())
        {
            writeStringC(dirMap.getObjShortName()); //directory name
            execute(dirMap); //recurse
        }
    }

    template <class T>
    void writeNumberC(T number) //checked write operation
    {
        writeNumber<T>(stream_, number);
        check();
    }

    void writeStringC(const Zstring& str) //checked write operation
    {
        writeString(stream_, str);
        check();
    }

    void check()
    {
        if (stream_.GetLastError() != wxSTREAM_NO_ERROR)
            throw FileError(wxString(_("Error writing to synchronization database:")) + wxT(" \n") +
                            wxT("\"") + zToWx(filename_) + wxT("\""));
    }

    const Zstring& filename_; //used for error text only
    wxOutputStream& stream_;
};


//save/load DirContainer
void FreeFileSync::saveToDisk(const BaseDirMapping& baseMapping, SelectedSide side, const Zstring& filename) //throw (FileError)
{
    try //(try to) delete old file: overwriting directly doesn't always work
    {
        removeFile(filename, false);
    }
    catch (...) {}

    try
    {
        //write format description (uncompressed)
        wxFFileOutputStream uncompressed(zToWx(filename), wxT("wb"));
        uncompressed.Write(FILE_FORMAT_DESCR, sizeof(FILE_FORMAT_DESCR));
        if (uncompressed.GetLastError() != wxSTREAM_NO_ERROR)
            throw FileError(wxString(_("Error writing to synchronization database:")) + wxT(" \n") +
                            wxT("\"") + zToWx(filename) + wxT("\""));

        wxZlibOutputStream output(uncompressed, 4, wxZLIB_ZLIB);
        /* 4 - best compromise between speed and compression: (scanning 200.000 objects)
        0 (uncompressed)        8,95 MB -  422 ms
        2                       2,07 MB -  470 ms
        4                       1,87 MB -  500 ms
        6                       1,77 MB -  613 ms
        9 (maximal compression) 1,74 MB - 3330 ms */

        if (side == LEFT_SIDE)
            SaveRecursively<LEFT_SIDE>(baseMapping, filename, output);
        else
            SaveRecursively<RIGHT_SIDE>(baseMapping, filename, output);
    }
    catch (FileError&)
    {
        try //(try to) delete erroneous file
        {
            removeFile(filename, false);
        }
        catch (...) {}
        throw;
    }

    //(try to) hide database file
#ifdef FFS_WIN
    ::SetFileAttributes(filename.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
}


//-------------------------------------------------------------------------------------------------------------------------
class ReadRecursively
{
public:
    ReadRecursively(wxInputStream& stream, const Zstring& filename, DirInformation& dirInfo) : filename_(filename), stream_(stream)
    {
        if (readNumberC<int>() != FILE_FORMAT_VER) //read file format version
            throw FileError(wxString(_("Incompatible synchronization database format:")) + wxT(" \n") + wxT("\"") + zToWx(filename_) + wxT("\""));

        //save filter settings
        dirInfo.filterActive  = readNumberC<bool>();
        dirInfo.includeFilter = readStringC();
        dirInfo.excludeFilter = readStringC();

        //start recursion
        execute(dirInfo.baseDirContainer);
    }

private:
    void execute(DirContainer& dirCont)
    {
        unsigned int fileCount = readNumberC<unsigned int>();
        while (fileCount-- != 0)
            readSubFile(dirCont);

        unsigned int dirCount = readNumberC<unsigned int>();
        while (dirCount-- != 0)
            readSubDirectory(dirCont);
    }

    void readSubFile(DirContainer& dirCont)
    {
        //attention: order of function argument evaluation is undefined! So do it one after the other...
        const Zstring shortName = readStringC(); //file name

        const long          modHigh = readNumberC<long>();
        const unsigned long modLow  = readNumberC<unsigned long>();

        const unsigned long sizeHigh = readNumberC<unsigned long>();
        const unsigned long sizeLow  = readNumberC<unsigned long>();

        dirCont.addSubFile(shortName,
                           FileDescriptor(wxLongLong(modHigh, modLow), wxULongLong(sizeHigh, sizeLow)));
    }

    void readSubDirectory(DirContainer& dirCont)
    {
        const Zstring shortName = readStringC(); //directory name
        DirContainer& subDir = dirCont.addSubDir(shortName);
        execute(subDir); //recurse
    }

    void check()
    {
        if (stream_.GetLastError() != wxSTREAM_NO_ERROR)
            throw FileError(wxString(_("Error reading from synchronization database:")) + wxT(" \n") +
                            wxT("\"") +  zToWx(filename_) + wxT("\""));
    }

    template <class T>
    T readNumberC() //checked read operation
    {
        T output = readNumber<T>(stream_);
        check();
        return output;
    }

    Zstring readStringC() //checked read operation
    {
        Zstring output = readString(stream_);
        check();
        return output;
    }

    const Zstring& filename_; //used for error text only
    wxInputStream& stream_;
};


boost::shared_ptr<const DirInformation> FreeFileSync::loadFromDisk(const Zstring& filename) //throw (FileError)
{
    //read format description (uncompressed)
    wxFFileInputStream uncompressed(zToWx(filename), wxT("rb"));

    char formatDescr[sizeof(FILE_FORMAT_DESCR)];
    uncompressed.Read(formatDescr, sizeof(formatDescr));
    formatDescr[sizeof(FILE_FORMAT_DESCR) - 1] = 0;
    if (uncompressed.GetLastError() != wxSTREAM_NO_ERROR)
        throw FileError(wxString(_("Error reading from synchronization database:")) + wxT(" \n") + wxT("\"") +  zToWx(filename) + wxT("\""));
    if (std::string(formatDescr) != FILE_FORMAT_DESCR)
        throw FileError(wxString(_("Incompatible synchronization database format:")) + wxT(" \n") + wxT("\"") + zToWx(filename) + wxT("\""));


    wxZlibInputStream input(uncompressed, wxZLIB_ZLIB);

    boost::shared_ptr<DirInformation> dirInfo(new DirInformation);
    ReadRecursively(input, filename, *dirInfo);  //read file/dir information

    return dirInfo;
}
