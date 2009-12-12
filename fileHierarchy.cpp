#include "fileHierarchy.h"
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include "shared/globalFunctions.h"
#include "shared/fileError.h"
#include <wx/intl.h>
#include "shared/stringConv.h"
#include "shared/fileHandling.h"
#include <wx/mstream.h>
#include "shared/serialize.h"

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


//-------------------------------------------------------------------------------------------------------------------------------
const char FILE_FORMAT_DESCR[] = "FreeFileSync";
const int FILE_FORMAT_VER = 3;
//-------------------------------------------------------------------------------------------------------------------------------


class ReadDirInfo : public Utility::ReadInputStream
{
public:
    ReadDirInfo(wxInputStream& stream, const wxString& errorObjName, DirInformation& dirInfo) : ReadInputStream(stream, errorObjName)
    {
        //read filter settings
        dirInfo.filter = FilterProcess::loadFilter(stream_);
        check();

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
};


typedef boost::shared_ptr<std::vector<char> > MemoryStreamPtr;     //byte stream representing DirInformation
typedef std::map<Utility::UniqueId, MemoryStreamPtr> DirectoryTOC; //list of streams ordered by a UUID pointing to their partner database
typedef std::pair<Utility::UniqueId, DirectoryTOC> DbStreamData;   //header data: UUID representing this database, item data: list of dir-streams
/* Example
left side              right side
---------              ----------
DB-ID 123     <-\  /-> DB-ID 567
                 \/
Partner-ID 111   /\    Partner-ID 222
Partner-ID 567 _/  \_  Partner-ID 123
    ...                     ...
*/

class ReadFileStream : public Utility::ReadInputStream
{
public:
    ReadFileStream(wxInputStream& stream, const wxString& filename, DbStreamData& output) : ReadInputStream(stream, filename)
    {
        if (readNumberC<int>() != FILE_FORMAT_VER) //read file format version
            throw FileError(wxString(_("Incompatible synchronization database format:")) + wxT(" \n") + wxT("\"") + filename + wxT("\""));

        //read DB id
        output.first = Utility::UniqueId(stream_);
        check();

        DirectoryTOC& dbList = output.second;
        dbList.clear();

        size_t dbCount = readNumberC<size_t>(); //number of databases: one for each sync-pair
        while (dbCount-- != 0)
        {
            const Utility::UniqueId partnerID(stream_); //DB id of partner databases
            check();

            CharArray buffer = readArrayC(); //read db-entry stream (containing DirInformation)

            dbList.insert(std::make_pair(partnerID, buffer));
        }
    }
};


DbStreamData loadFile(const Zstring& filename) //throw (FileError)
{
    if (!FreeFileSync::fileExists(filename))
        throw FileError(wxString(_("Initial synchronization:")) + wxT(" ") +
                        _("The database file is not yet existing, but will be created during synchronization:") + wxT("\n") +
                        wxT(" \"") + zToWx(filename) + wxT("\""));


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

    DbStreamData output;
    ReadFileStream(input, zToWx(filename), output);
    return output;
}


std::pair<DirInfoPtr, DirInfoPtr> FreeFileSync::loadFromDisk(const BaseDirMapping& baseMapping) //throw (FileError)
{
    const Zstring fileNameLeft  = baseMapping.getDBFilename<LEFT_SIDE>();
    const Zstring fileNameRight = baseMapping.getDBFilename<RIGHT_SIDE>();

    //read file data: db ID + mapping of partner-ID/DirInfo-stream
    const DbStreamData dbEntriesLeft  = ::loadFile(fileNameLeft);
    const DbStreamData dbEntriesRight = ::loadFile(fileNameRight);

    //find associated DirInfo-streams
    DirectoryTOC::const_iterator dbLeft = dbEntriesLeft.second.find(dbEntriesRight.first); //find left db-entry that corresponds to right database
    if (dbLeft == dbEntriesLeft.second.end())
        throw FileError(wxString(_("Initial synchronization:")) + wxT(" ") +
                        _("The required database entry is not yet existing, but will be created during synchronization:") + wxT("\n") +
                        wxT(" \"") + zToWx(fileNameLeft) + wxT("\""));

    DirectoryTOC::const_iterator dbRight = dbEntriesRight.second.find(dbEntriesLeft.first); //find left db-entry that corresponds to right database
    if (dbRight == dbEntriesRight.second.end())
        throw FileError(wxString(_("Initial synchronization:")) + wxT(" ") +
                        _("The required database entry is not yet existing, but will be created during synchronization:") + wxT("\n") +
                        wxT(" \"") + zToWx(fileNameRight) + wxT("\""));

    //read streams into DirInfo
    boost::shared_ptr<DirInformation> dirInfoLeft(new DirInformation);
    wxMemoryInputStream buffer(&(*dbLeft->second)[0], dbLeft->second->size()); //convert char-array to inputstream: no copying, ownership not transferred
    ReadDirInfo(buffer, zToWx(fileNameLeft), *dirInfoLeft);  //read file/dir information

    boost::shared_ptr<DirInformation> dirInfoRight(new DirInformation);
    wxMemoryInputStream buffer2(&(*dbRight->second)[0], dbRight->second->size()); //convert char-array to inputstream: no copying, ownership not transferred
    ReadDirInfo(buffer2, zToWx(fileNameRight), *dirInfoRight);  //read file/dir information

    return std::make_pair(dirInfoLeft, dirInfoRight);
}


//-------------------------------------------------------------------------------------------------------------------------

template <SelectedSide side>
struct IsNonEmpty
{
    bool operator()(const FileSystemObject& fsObj) const
    {
        return !fsObj.isEmpty<side>();
    }
};


template <SelectedSide side>
class SaveDirInfo : public Utility::WriteOutputStream
{
public:
    SaveDirInfo(const BaseDirMapping& baseMapping, const wxString& errorObjName, wxOutputStream& stream) : WriteOutputStream(errorObjName, stream)
    {
        //save filter settings
        baseMapping.getFilter()->saveFilter(stream_);
        check();

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
};


class WriteFileStream : public Utility::WriteOutputStream
{
public:
    WriteFileStream(const DbStreamData& input, const wxString& filename, wxOutputStream& stream) : WriteOutputStream(filename, stream)
    {
        //save file format version
        writeNumberC<int>(FILE_FORMAT_VER);

        //write DB id
        input.first.toStream(stream_);
        check();

        const DirectoryTOC& dbList = input.second;

        writeNumberC<size_t>(dbList.size()); //number of database records: one for each sync-pair

        for (DirectoryTOC::const_iterator i = dbList.begin(); i != dbList.end(); ++i)
        {
            i->first.toStream(stream_); //DB id of partner database
            check();

            writeArrayC(*(i->second)); //write DirInformation stream
        }
    }
};


//save/load DirContainer
void saveFile(const DbStreamData& dbStream, const Zstring& filename) //throw (FileError)
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

    WriteFileStream(dbStream, zToWx(filename), output);

    //(try to) hide database file
#ifdef FFS_WIN
    output.Close();
    ::SetFileAttributes(filename.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
}


void FreeFileSync::saveToDisk(const BaseDirMapping& baseMapping) //throw (FileError)
{
    //transactional behaviour! write to tmp files first
    const Zstring fileNameLeftTmp  = baseMapping.getDBFilename<LEFT_SIDE>()  + DefaultStr(".tmp");
    const Zstring fileNameRightTmp = baseMapping.getDBFilename<RIGHT_SIDE>() + DefaultStr(".tmp");;

    //delete old tmp file, if necessary -> throws if deletion fails!
    removeFile(fileNameLeftTmp,  false);
    removeFile(fileNameRightTmp, false);

    try
    {
        //load old database files...

        //read file data: db ID + mapping of partner-ID/DirInfo-stream: may throw!
        DbStreamData dbEntriesLeft;
        if (FreeFileSync::fileExists(baseMapping.getDBFilename<LEFT_SIDE>()))
            try
            {
                dbEntriesLeft = ::loadFile(baseMapping.getDBFilename<LEFT_SIDE>());
            }
            catch(FileError&) {} //if error occurs: just overwrite old file! User is informed about issues right after comparing!
        //else -> dbEntriesLeft has empty mapping, but already a DB-ID!

        //read file data: db ID + mapping of partner-ID/DirInfo-stream: may throw!
        DbStreamData dbEntriesRight;
        if (FreeFileSync::fileExists(baseMapping.getDBFilename<RIGHT_SIDE>()))
            try
            {
                dbEntriesRight = ::loadFile(baseMapping.getDBFilename<RIGHT_SIDE>());
            }
            catch(FileError&) {} //if error occurs: just overwrite old file! User is informed about issues right after comparing!

        //create new database entries
        MemoryStreamPtr dbEntryLeft(new std::vector<char>);
        {
            wxMemoryOutputStream buffer;
            SaveDirInfo<LEFT_SIDE>(baseMapping, zToWx(baseMapping.getDBFilename<LEFT_SIDE>()), buffer);
            dbEntryLeft->resize(buffer.GetSize());               //convert output stream to char-array
            buffer.CopyTo(&(*dbEntryLeft)[0], buffer.GetSize()); //
        }

        MemoryStreamPtr dbEntryRight(new std::vector<char>);
        {
            wxMemoryOutputStream buffer;
            SaveDirInfo<RIGHT_SIDE>(baseMapping, zToWx(baseMapping.getDBFilename<RIGHT_SIDE>()), buffer);
            dbEntryRight->resize(buffer.GetSize());               //convert output stream to char-array
            buffer.CopyTo(&(*dbEntryRight)[0], buffer.GetSize()); //
        }

        //create/update DirInfo-streams
        dbEntriesLeft.second[dbEntriesRight.first] = dbEntryLeft;
        dbEntriesRight.second[dbEntriesLeft.first] = dbEntryRight;

        //write (temp-) files...
        saveFile(dbEntriesLeft,  fileNameLeftTmp); //throw (FileError)
        saveFile(dbEntriesRight, fileNameRightTmp); //throw (FileError)

        //operation finished: rename temp files -> this should work transactionally:
        //if there were no write access, creation of temp files would have failed
        removeFile(baseMapping.getDBFilename<LEFT_SIDE>(),  false);
        removeFile(baseMapping.getDBFilename<RIGHT_SIDE>(), false);
        renameFile(fileNameLeftTmp, baseMapping.getDBFilename<LEFT_SIDE>()); //throw (FileError);
        renameFile(fileNameRightTmp, baseMapping.getDBFilename<RIGHT_SIDE>()); //throw (FileError);

    }
    catch (...)
    {
        try //clean up: (try to) delete old tmp file
        {
            removeFile(fileNameLeftTmp,  false);
            removeFile(fileNameRightTmp, false);
        }
        catch (...) {}

        throw;
    }
}

