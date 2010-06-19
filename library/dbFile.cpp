// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "dbFile.h"
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include "../shared/globalFunctions.h"
#include "../shared/fileError.h"
#include <wx/intl.h>
#include "../shared/stringConv.h"
#include "../shared/fileHandling.h"
#include <wx/mstream.h>
#include "../shared/serialize.h"
#include "../shared/fileIO.h"
#include "../shared/loki/ScopeGuard.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "../shared/longPathPrefix.h"
#endif

using namespace FreeFileSync;


namespace
{
//-------------------------------------------------------------------------------------------------------------------------------
const char FILE_FORMAT_DESCR[] = "FreeFileSync";
const int FILE_FORMAT_VER = 4;
//-------------------------------------------------------------------------------------------------------------------------------


//yet another layer of indirection: Since 32/64 bit builds are binary incompatible, we want them to write into "distinct parts" of the db-file
//just like we were actually accessing different files
class FileInputStreamDB : public FileInputStream
{
public:
    FileInputStreamDB(const Zstring& filename) : //throw FileError()
        FileInputStream(filename)
    {
        //read FreeFileSync file identifier
        char formatDescr[sizeof(FILE_FORMAT_DESCR)];
        Read(formatDescr, sizeof(formatDescr)); //throw (FileError)
        formatDescr[sizeof(formatDescr) - 1] = 0;

        if (std::string(formatDescr) != FILE_FORMAT_DESCR)
            throw FileError(wxString(_("Incompatible synchronization database format:")) + wxT(" \n") + wxT("\"") + zToWx(filename) + wxT("\""));
    }

private:
};


class FileOutputStreamDB : public FileOutputStream
{
public:
    FileOutputStreamDB(const Zstring& filename) : //throw FileError()
        FileOutputStream(filename)
    {
        //write FreeFileSync file identifier
        Write(FILE_FORMAT_DESCR, sizeof(FILE_FORMAT_DESCR)); //throw (FileError)
    }

private:
};
}
//#######################################################################################################################################




class ReadDirInfo : public Utility::ReadInputStream
{
public:
    ReadDirInfo(wxInputStream& stream, const wxString& errorObjName, DirInformation& dirInfo) : ReadInputStream(stream, errorObjName)
    {
        //read filter settings
        dirInfo.filter = BaseFilter::loadFilter(getStream());
        check();

        //start recursion
        execute(dirInfo.baseDirContainer);
    }

private:
    void execute(DirContainer& dirCont) const
    {
        size_t fileCount = readNumberC<size_t>();
        while (fileCount-- != 0)
            readSubFile(dirCont);

        size_t symlinkCount = readNumberC<size_t>();
        while (symlinkCount-- != 0)
            readSubLink(dirCont);

        size_t dirCount = readNumberC<size_t>();
        while (dirCount-- != 0)
            readSubDirectory(dirCont);
    }

    void readSubFile(DirContainer& dirCont) const
    {
        //attention: order of function argument evaluation is undefined! So do it one after the other...
        const Zstring shortName = readStringC(); //file name

        const long          modHigh = readNumberC<long>();
        const unsigned long modLow  = readNumberC<unsigned long>();

        const unsigned long sizeHigh = readNumberC<unsigned long>();
        const unsigned long sizeLow  = readNumberC<unsigned long>();

        //const Utility::FileID fileIdentifier(stream_);
        //check();

        dirCont.addSubFile(shortName,
                           FileDescriptor(wxLongLong(modHigh, modLow),
                                          wxULongLong(sizeHigh, sizeLow)));
    }


    void readSubLink(DirContainer& dirCont) const
    {
        //attention: order of function argument evaluation is undefined! So do it one after the other...
        const Zstring        shortName  = readStringC(); //file name
        const long           modHigh    = readNumberC<long>();
        const unsigned long  modLow     = readNumberC<unsigned long>();
        const Zstring        targetPath = readStringC(); //file name
        const LinkDescriptor::LinkType linkType  = static_cast<LinkDescriptor::LinkType>(readNumberC<int>());

        dirCont.addSubLink(shortName,
                           LinkDescriptor(wxLongLong(modHigh, modLow), targetPath, linkType));
    }


    void readSubDirectory(DirContainer& dirCont) const
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
        output.first = Utility::UniqueId(getStream());
        check();

        DirectoryTOC& dbList = output.second;
        dbList.clear();

        size_t dbCount = readNumberC<size_t>(); //number of databases: one for each sync-pair
        while (dbCount-- != 0)
        {
            const Utility::UniqueId partnerID(getStream()); //DB id of partner databases
            check();

            CharArray buffer = readArrayC(); //read db-entry stream (containing DirInformation)

            dbList.insert(std::make_pair(partnerID, buffer));
        }
    }
};


DbStreamData loadFile(const Zstring& filename) //throw (FileError)
{
    if (!FreeFileSync::fileExists(filename))
        throw FileError(wxString(_("Initial synchronization:")) + wxT(" \n\n") +
                        _("One of the FreeFileSync database files is not yet existing:") + wxT(" \n") +
                        wxT("\"") + zToWx(filename) + wxT("\""));

    //read format description (uncompressed)
    FileInputStreamDB uncompressed(filename); //throw (FileError)

    wxZlibInputStream input(uncompressed, wxZLIB_ZLIB);

    DbStreamData output;
    ReadFileStream(input, zToWx(filename), output);
    return output;
}


std::pair<DirInfoPtr, DirInfoPtr> FreeFileSync::loadFromDisk(const BaseDirMapping& baseMapping) //throw (FileError)
{
    const Zstring fileNameLeft  = baseMapping.getDBFilename<LEFT_SIDE>();
    const Zstring fileNameRight = baseMapping.getDBFilename<RIGHT_SIDE>();

    try
    {
        //read file data: db ID + mapping of partner-ID/DirInfo-stream
        const DbStreamData dbEntriesLeft  = ::loadFile(fileNameLeft);
        const DbStreamData dbEntriesRight = ::loadFile(fileNameRight);

        //find associated DirInfo-streams
        DirectoryTOC::const_iterator dbLeft = dbEntriesLeft.second.find(dbEntriesRight.first); //find left db-entry that corresponds to right database
        if (dbLeft == dbEntriesLeft.second.end())
            throw FileError(wxString(_("Initial synchronization:")) + wxT(" \n\n") +
                            _("One of the FreeFileSync database entries within the following file is not yet existing:") + wxT(" \n") +
                            wxT("\"") + zToWx(fileNameLeft) + wxT("\""));

        DirectoryTOC::const_iterator dbRight = dbEntriesRight.second.find(dbEntriesLeft.first); //find left db-entry that corresponds to right database
        if (dbRight == dbEntriesRight.second.end())
            throw FileError(wxString(_("Initial synchronization:")) + wxT(" \n\n") +
                            _("One of the FreeFileSync database entries within the following file is not yet existing:") + wxT(" \n") +
                            wxT("\"") + zToWx(fileNameRight) + wxT("\""));

        //read streams into DirInfo
        boost::shared_ptr<DirInformation> dirInfoLeft(new DirInformation);
        wxMemoryInputStream buffer(&(*dbLeft->second)[0], dbLeft->second->size()); //convert char-array to inputstream: no copying, ownership not transferred
        ReadDirInfo(buffer, zToWx(fileNameLeft), *dirInfoLeft);  //read file/dir information

        boost::shared_ptr<DirInformation> dirInfoRight(new DirInformation);
        wxMemoryInputStream buffer2(&(*dbRight->second)[0], dbRight->second->size()); //convert char-array to inputstream: no copying, ownership not transferred
        ReadDirInfo(buffer2, zToWx(fileNameRight), *dirInfoRight);  //read file/dir information

        return std::make_pair(dirInfoLeft, dirInfoRight);
    }
    catch (const std::bad_alloc&) //this is most likely caused by a corrupted database file
    {
        throw FileError(wxString(_("Error reading from synchronization database:")) + wxT(" (bad_alloc)"));
    }
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
        baseMapping.getFilter()->saveFilter(getStream());
        check();

        //start recursion
        execute(baseMapping);
    }

private:
    friend class Utility::Proxy<SaveDirInfo<side> >; //friend declaration of std::for_each is NOT sufficient as implementation is compiler dependent!

    void execute(const HierarchyObject& hierObj)
    {
        Utility::Proxy<SaveDirInfo<side> > prx(*this); //grant std::for_each access to private parts of this class

        writeNumberC<size_t>(std::count_if(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), IsNonEmpty<side>())); //number of (existing) files
        std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), prx);

        writeNumberC<size_t>(std::count_if(hierObj.useSubLinks().begin(), hierObj.useSubLinks().end(), IsNonEmpty<side>())); //number of (existing) files
        std::for_each(hierObj.useSubLinks().begin(), hierObj.useSubLinks().end(), prx);

        writeNumberC<size_t>(std::count_if(hierObj.useSubDirs().begin(), hierObj.useSubDirs().end(), IsNonEmpty<side>())); //number of (existing) directories
        std::for_each(hierObj.useSubDirs().begin(), hierObj.useSubDirs().end(), prx);
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

            //fileMap.getFileID<side>().toStream(stream_); //unique file identifier
            //check();
        }
    }

    void operator()(const SymLinkMapping& linkObj)
    {
        if (!linkObj.isEmpty<side>())
        {
            writeStringC(linkObj.getObjShortName());
            writeNumberC<long>(         linkObj.getLastWriteTime<side>().GetHi()); //last modification time
            writeNumberC<unsigned long>(linkObj.getLastWriteTime<side>().GetLo()); //
            writeStringC(linkObj.getTargetPath<side>());
            writeNumberC<int>(linkObj.getLinkType<side>());
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
        input.first.toStream(getStream());
        check();

        const DirectoryTOC& dbList = input.second;

        writeNumberC<size_t>(dbList.size()); //number of database records: one for each sync-pair

        for (DirectoryTOC::const_iterator i = dbList.begin(); i != dbList.end(); ++i)
        {
            i->first.toStream(getStream()); //DB id of partner database
            check();

            writeArrayC(*(i->second)); //write DirInformation stream
        }
    }
};


//save/load DirContainer
void saveFile(const DbStreamData& dbStream, const Zstring& filename) //throw (FileError)
{
    {
        //write format description (uncompressed)
        FileOutputStreamDB uncompressed(filename); //throw (FileError)

        wxZlibOutputStream output(uncompressed, 4, wxZLIB_ZLIB);
        /* 4 - best compromise between speed and compression: (scanning 200.000 objects)
        0 (uncompressed)        8,95 MB -  422 ms
        2                       2,07 MB -  470 ms
        4                       1,87 MB -  500 ms
        6                       1,77 MB -  613 ms
        9 (maximal compression) 1,74 MB - 3330 ms */

        WriteFileStream(dbStream, zToWx(filename), output);
    }
    //(try to) hide database file
#ifdef FFS_WIN
    ::SetFileAttributes(FreeFileSync::applyLongPathPrefix(filename).c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
}


void FreeFileSync::saveToDisk(const BaseDirMapping& baseMapping) //throw (FileError)
{
    //transactional behaviour! write to tmp files first
    const Zstring fileNameLeftTmp  = baseMapping.getDBFilename<LEFT_SIDE>()  + DefaultStr(".tmp");
    const Zstring fileNameRightTmp = baseMapping.getDBFilename<RIGHT_SIDE>() + DefaultStr(".tmp");;

    //delete old tmp file, if necessary -> throws if deletion fails!
    removeFile(fileNameLeftTmp);
    removeFile(fileNameRightTmp);

    //load old database files...

    //read file data: db ID + mapping of partner-ID/DirInfo-stream: may throw!
    DbStreamData dbEntriesLeft;
    if (FreeFileSync::fileExists(baseMapping.getDBFilename<LEFT_SIDE>()))
        try
        {
            dbEntriesLeft = ::loadFile(baseMapping.getDBFilename<LEFT_SIDE>());
        }
        catch(FileError&) {} //if error occurs: just overwrite old file! User is already informed about issues right after comparing!
    //else -> dbEntriesLeft has empty mapping, but already a DB-ID!

    //read file data: db ID + mapping of partner-ID/DirInfo-stream: may throw!
    DbStreamData dbEntriesRight;
    if (FreeFileSync::fileExists(baseMapping.getDBFilename<RIGHT_SIDE>()))
        try
        {
            dbEntriesRight = ::loadFile(baseMapping.getDBFilename<RIGHT_SIDE>());
        }
        catch(FileError&) {} //if error occurs: just overwrite old file! User is already informed about issues right after comparing!

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


    struct TryCleanUp //ensure cleanup if working with temporary failed!
    {
        static void tryDeleteFile(const Zstring& filename) //throw ()
        {
            try
            {
                removeFile(filename);
            }
            catch (...) {}
        }
    };

    //write (temp-) files...
    Loki::ScopeGuard guardTempFileLeft = Loki::MakeGuard(&TryCleanUp::tryDeleteFile, fileNameLeftTmp);
    saveFile(dbEntriesLeft, fileNameLeftTmp);  //throw (FileError)

    Loki::ScopeGuard guardTempFileRight = Loki::MakeGuard(&TryCleanUp::tryDeleteFile, fileNameRightTmp);
    saveFile(dbEntriesRight, fileNameRightTmp); //throw (FileError)

    //operation finished: rename temp files -> this should work transactionally:
    //if there were no write access, creation of temp files would have failed
    removeFile(baseMapping.getDBFilename<LEFT_SIDE>());
    removeFile(baseMapping.getDBFilename<RIGHT_SIDE>());
    renameFile(fileNameLeftTmp, baseMapping.getDBFilename<LEFT_SIDE>());   //throw (FileError);
    renameFile(fileNameRightTmp, baseMapping.getDBFilename<RIGHT_SIDE>()); //throw (FileError);

    guardTempFileLeft.Dismiss(); //no need to delete temp file anymore
    guardTempFileRight.Dismiss(); //
}
