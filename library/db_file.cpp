// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "db_file.h"
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include <wx/mstream.h>
#include "../shared/global_func.h"
#include "../shared/file_error.h"
#include "../shared/string_conv.h"
#include "../shared/file_handling.h"
#include "../shared/serialize.h"
#include "../shared/file_io.h"
#include "../shared/loki/ScopeGuard.h"
#include "../shared/i18n.h"
#include <boost/bind.hpp>

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "../shared/long_path_prefix.h"
#endif

using namespace zen;


namespace
{
//-------------------------------------------------------------------------------------------------------------------------------
const char FILE_FORMAT_DESCR[] = "FreeFileSync";
const int FILE_FORMAT_VER = 6;
//-------------------------------------------------------------------------------------------------------------------------------


class FileInputStreamDB : public FileInputStream
{
public:
    FileInputStreamDB(const Zstring& filename) : //throw (FileError)
        FileInputStream(filename)
    {
        //read FreeFileSync file identifier
        char formatDescr[sizeof(FILE_FORMAT_DESCR)] = {};
        Read(formatDescr, sizeof(formatDescr)); //throw (FileError)

        if (!std::equal(FILE_FORMAT_DESCR, FILE_FORMAT_DESCR + sizeof(FILE_FORMAT_DESCR), formatDescr))
            throw FileError(wxString(_("Incompatible synchronization database format:")) + wxT(" \n") + wxT("\"") + zToWx(filename) + wxT("\""));
    }

private:
};


class FileOutputStreamDB : public FileOutputStream
{
public:
    FileOutputStreamDB(const Zstring& filename) : //throw (FileError)
        FileOutputStream(filename)
    {
        //write FreeFileSync file identifier
        Write(FILE_FORMAT_DESCR, sizeof(FILE_FORMAT_DESCR)); //throw (FileError)
    }

private:
};
}
//#######################################################################################################################################


class ReadDirInfo : public zen::ReadInputStream
{
public:
    ReadDirInfo(wxInputStream& stream, const wxString& errorObjName, DirInformation& dirInfo) : ReadInputStream(stream, errorObjName)
    {
        //|-------------------------------------------------------------------------------------
        //| ensure 32/64 bit portability: use fixed size data types only e.g. boost::uint32_t |
        //|-------------------------------------------------------------------------------------

        //read filter settings -> currently not required, but persisting it doesn't hurt
        dirInfo.filter = HardFilter::loadFilter(getStream());
        check();

        //start recursion
        execute(dirInfo.baseDirContainer);
    }

private:
    void execute(DirContainer& dirCont) const
    {
        while (readNumberC<bool>())
            readSubFile(dirCont);

        while (readNumberC<bool>())
            readSubLink(dirCont);

        while (readNumberC<bool>())
            readSubDirectory(dirCont);
    }

    void readSubFile(DirContainer& dirCont) const
    {
        //attention: order of function argument evaluation is undefined! So do it one after the other...
        const Zstring shortName = readStringC<Zstring>(); //file name

        const boost::int64_t  modTime  = readNumberC<boost::int64_t>();
        const boost::uint64_t fileSize = readNumberC<boost::uint64_t>();

        //const util::FileID fileIdentifier(stream_);
        //check();

        dirCont.addSubFile(shortName,
                           FileDescriptor(modTime, fileSize));
    }


    void readSubLink(DirContainer& dirCont) const
    {
        //attention: order of function argument evaluation is undefined! So do it one after the other...
        const Zstring         shortName  = readStringC<Zstring>(); //file name
        const boost::int64_t  modTime    = readNumberC<boost::int64_t>();
        const Zstring         targetPath = readStringC<Zstring>(); //file name
        const LinkDescriptor::LinkType linkType  = static_cast<LinkDescriptor::LinkType>(readNumberC<boost::int32_t>());

        dirCont.addSubLink(shortName,
                           LinkDescriptor(modTime, targetPath, linkType));
    }


    void readSubDirectory(DirContainer& dirCont) const
    {
        const Zstring shortName = readStringC<Zstring>(); //directory name
        DirContainer& subDir = dirCont.addSubDir(shortName);
        execute(subDir); //recurse
    }
};

namespace
{
typedef std::string UniqueId;
typedef std::shared_ptr<std::vector<char> > MemoryStreamPtr; //byte stream representing DirInformation
typedef std::map<UniqueId, MemoryStreamPtr>   DirectoryTOC;    //list of streams ordered by a UUID pointing to their partner database
typedef std::pair<UniqueId, DirectoryTOC>     DbStreamData;    //header data: UUID representing this database, item data: list of dir-streams
}
/* Example
left side              right side
---------              ----------
DB-ID 123     <-\  /-> DB-ID 567
                 \/
Partner-ID 111   /\    Partner-ID 222
Partner-ID 567 _/  \_  Partner-ID 123
    ...                     ...
*/

class ReadFileStream : public zen::ReadInputStream
{
public:
    ReadFileStream(wxInputStream& stream, const wxString& filename, DbStreamData& output, int& versionId) : ReadInputStream(stream, filename)
    {
        //|-------------------------------------------------------------------------------------
        //| ensure 32/64 bit portability: used fixed size data types only e.g. boost::uint32_t |
        //|-------------------------------------------------------------------------------------

        boost::int32_t version = readNumberC<boost::int32_t>();
        if (version != FILE_FORMAT_VER) //read file format version
            throw FileError(wxString(_("Incompatible synchronization database format:")) + wxT(" \n") + wxT("\"") + filename + wxT("\""));
        versionId = version;

        //read DB id
        const CharArray tmp = readArrayC();
        output.first.assign(tmp->begin(), tmp->end());

        DirectoryTOC& dbList = output.second;
        dbList.clear();

        boost::uint32_t dbCount = readNumberC<boost::uint32_t>(); //number of databases: one for each sync-pair
        while (dbCount-- != 0)
        {
            //DB id of partner databases
            const CharArray tmp2 = readArrayC();
            const std::string partnerID(tmp2->begin(), tmp2->end());

            CharArray buffer = readArrayC(); //read db-entry stream (containing DirInformation)

            dbList.insert(std::make_pair(partnerID, buffer));
        }
    }
};


DbStreamData loadFile(const Zstring& filename) //throw (FileError)
{
    if (!zen::fileExists(filename))
        throw FileErrorDatabaseNotExisting(wxString(_("Initial synchronization:")) + wxT(" \n\n") +
                                           _("One of the FreeFileSync database files is not yet existing:") + wxT(" \n") +
                                           wxT("\"") + zToWx(filename) + wxT("\""));

    //read format description (uncompressed)
    FileInputStreamDB uncompressed(filename); //throw (FileError)

    wxZlibInputStream input(uncompressed, wxZLIB_ZLIB);

    DbStreamData output;
    int versionId = 0;
    ReadFileStream (input, zToWx(filename), output, versionId);
    return output;
}


std::pair<DirInfoPtr, DirInfoPtr> zen::loadFromDisk(const BaseDirMapping& baseMapping) //throw (FileError)
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
        DirectoryTOC::const_iterator dbRight = dbEntriesRight.second.find(dbEntriesLeft.first); //find left db-entry that corresponds to right database

        if (dbLeft == dbEntriesLeft.second.end())
            throw FileErrorDatabaseNotExisting(wxString(_("Initial synchronization:")) + wxT(" \n\n") +
                                               _("One of the FreeFileSync database entries within the following file is not yet existing:") + wxT(" \n") +
                                               wxT("\"") + zToWx(fileNameLeft) + wxT("\""));

        if (dbRight == dbEntriesRight.second.end())
            throw FileErrorDatabaseNotExisting(wxString(_("Initial synchronization:")) + wxT(" \n\n") +
                                               _("One of the FreeFileSync database entries within the following file is not yet existing:") + wxT(" \n") +
                                               wxT("\"") + zToWx(fileNameRight) + wxT("\""));

        //read streams into DirInfo
        std::shared_ptr<DirInformation> dirInfoLeft(new DirInformation);
        wxMemoryInputStream buffer(&(*dbLeft->second)[0], dbLeft->second->size()); //convert char-array to inputstream: no copying, ownership not transferred
        ReadDirInfo(buffer, zToWx(fileNameLeft), *dirInfoLeft);  //read file/dir information

        std::shared_ptr<DirInformation> dirInfoRight(new DirInformation);
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
class SaveDirInfo : public WriteOutputStream
{
public:
    SaveDirInfo(const BaseDirMapping& baseMapping, const DirContainer* oldDirInfo, const wxString& errorObjName, wxOutputStream& stream) : WriteOutputStream(errorObjName, stream)
    {
        //save filter settings
        baseMapping.getFilter()->saveFilter(getStream());
        check();

        //start recursion
        execute(baseMapping, oldDirInfo);
    }

private:
    void execute(const HierarchyObject& hierObj, const DirContainer* oldDirInfo)
    {
        std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), boost::bind(&SaveDirInfo::processFile, this, _1, oldDirInfo));
        writeNumberC<bool>(false); //mark last entry
        std::for_each(hierObj.useSubLinks().begin(), hierObj.useSubLinks().end(), boost::bind(&SaveDirInfo::processLink, this, _1, oldDirInfo));
        writeNumberC<bool>(false); //mark last entry
        std::for_each(hierObj.useSubDirs ().begin(), hierObj.useSubDirs ().end(), boost::bind(&SaveDirInfo::processDir,  this, _1, oldDirInfo));
        writeNumberC<bool>(false); //mark last entry
    }

    void processFile(const FileMapping& fileMap, const DirContainer* oldDirInfo)
    {
        const Zstring shortName = fileMap.getObjShortName();

        if (fileMap.getCategory() == FILE_EQUAL) //data in sync: write current state
        {
            if (!fileMap.isEmpty<side>())
            {
                writeNumberC<bool>(true); //mark beginning of entry
                writeStringC(shortName);
                writeNumberC<boost::int64_t >(to<boost::int64_t>(fileMap.getLastWriteTime<side>())); //last modification time
                writeNumberC<boost::uint64_t>(to<boost::uint64_t>(fileMap.getFileSize<side>())); //filesize
            }
        }
        else //not in sync: reuse last synchronous state
        {
            if (oldDirInfo) //no data is also a "synchronous state"!
            {
                DirContainer::FileList::const_iterator iter = oldDirInfo->files.find(shortName);
                if (iter != oldDirInfo->files.end())
                {
                    writeNumberC<bool>(true); //mark beginning of entry
                    writeStringC(shortName);
                    writeNumberC<boost::int64_t >(to<boost::int64_t>(iter->second.lastWriteTimeRaw)); //last modification time
                    writeNumberC<boost::uint64_t>(to<boost::uint64_t>(iter->second.fileSize)); //filesize
                }
            }
        }
    }

    void processLink(const SymLinkMapping& linkObj, const DirContainer* oldDirInfo)
    {
        const Zstring shortName = linkObj.getObjShortName();

        if (linkObj.getCategory() == FILE_EQUAL) //data in sync: write current state
        {
            if (!linkObj.isEmpty<side>())
            {
                writeNumberC<bool>(true); //mark beginning of entry
                writeStringC(shortName);
                writeNumberC<boost::int64_t>(to<boost::int64_t>(linkObj.getLastWriteTime<side>())); //last modification time
                writeStringC(linkObj.getTargetPath<side>());
                writeNumberC<boost::int32_t>(linkObj.getLinkType<side>());
            }
        }
        else //not in sync: reuse last synchronous state
        {
            if (oldDirInfo) //no data is also a "synchronous state"!
            {
                DirContainer::LinkList::const_iterator iter = oldDirInfo->links.find(shortName);
                if (iter != oldDirInfo->links.end())
                {
                    writeNumberC<bool>(true); //mark beginning of entry
                    writeStringC(shortName);
                    writeNumberC<boost::int64_t>(to<boost::int64_t>(iter->second.lastWriteTimeRaw)); //last modification time
                    writeStringC(iter->second.targetPath);
                    writeNumberC<boost::int32_t>(iter->second.type);
                }
            }
        }
    }

    void processDir(const DirMapping& dirMap, const DirContainer* oldDirInfo)
    {
        const Zstring shortName = dirMap.getObjShortName();

        const DirContainer* subDirInfo = NULL;
        if (oldDirInfo) //no data is also a "synchronous state"!
        {
            DirContainer::DirList::const_iterator iter = oldDirInfo->dirs.find(shortName);
            if (iter != oldDirInfo->dirs.end())
                subDirInfo = &iter->second;
        }

        if (dirMap.getCategory() == FILE_EQUAL) //data in sync: write current state
        {
            if (!dirMap.isEmpty<side>())
            {
                writeNumberC<bool>(true); //mark beginning of entry
                writeStringC(shortName);
                execute(dirMap, subDirInfo); //recurse
            }
        }
        else //not in sync: reuse last synchronous state
        {
            if (subDirInfo) //no data is also a "synchronous state"!
            {
                writeNumberC<bool>(true); //mark beginning of entry
                writeStringC(shortName);
            }
            execute(dirMap, subDirInfo); //recurse
        }
    }
};


class WriteFileStream : public WriteOutputStream
{
public:
    WriteFileStream(const DbStreamData& input, const wxString& filename, wxOutputStream& stream) : WriteOutputStream(filename, stream)
    {
        //save file format version
        writeNumberC<boost::int32_t>(FILE_FORMAT_VER);

        //write DB id
        writeArrayC(std::vector<char>(input.first.begin(), input.first.end()));

        const DirectoryTOC& dbList = input.second;

        writeNumberC<boost::uint32_t>(static_cast<boost::uint32_t>(dbList.size())); //number of database records: one for each sync-pair

        for (DirectoryTOC::const_iterator i = dbList.begin(); i != dbList.end(); ++i)
        {
            //DB id of partner database
            writeArrayC(std::vector<char>(i->first.begin(), i->first.end()));

            //write DirInformation stream
            writeArrayC(*(i->second));
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
    ::SetFileAttributes(zen::applyLongPathPrefix(filename).c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
}


bool entryExisting(const DirectoryTOC& table, const UniqueId& newKey, const MemoryStreamPtr& newValue)
{
    DirectoryTOC::const_iterator iter = table.find(newKey);
    if (iter == table.end())
        return false;

    if (!newValue.get() || !iter->second.get())
        return newValue.get() == iter->second.get();

    return *newValue == *iter->second;
}


void zen::saveToDisk(const BaseDirMapping& baseMapping) //throw (FileError)
{
    //transactional behaviour! write to tmp files first
    const Zstring fileNameLeftTmp  = baseMapping.getDBFilename<LEFT_SIDE>()  + Zstr(".tmp");
    const Zstring fileNameRightTmp = baseMapping.getDBFilename<RIGHT_SIDE>() + Zstr(".tmp");;

    //delete old tmp file, if necessary -> throws if deletion fails!
    removeFile(fileNameLeftTmp);  //
    removeFile(fileNameRightTmp); //throw (FileError)

    //load old database files...

    //read file data: db ID + mapping of partner-ID/DirInfo-stream: may throw!
    DbStreamData dbEntriesLeft;
    try
    {
        dbEntriesLeft = ::loadFile(baseMapping.getDBFilename<LEFT_SIDE>());
    }
    catch(FileError&)
    {
        //if error occurs: just overwrite old file! User is already informed about issues right after comparing!
        //dbEntriesLeft has empty mapping, but already a DB-ID!
        dbEntriesLeft.first = util::generateGUID();
    }

    //read file data: db ID + mapping of partner-ID/DirInfo-stream: may throw!
    DbStreamData dbEntriesRight;
    try
    {
        dbEntriesRight = ::loadFile(baseMapping.getDBFilename<RIGHT_SIDE>());
    }
    catch(FileError&)
    {
        dbEntriesRight.first = util::generateGUID();
    }


    //(try to) read old DirInfo
    std::shared_ptr<DirInformation> oldDirInfoLeft;
    try
    {
        DirectoryTOC::const_iterator iter = dbEntriesLeft.second.find(dbEntriesRight.first);
        if (iter != dbEntriesLeft.second.end())
            if (iter->second.get())
            {
                const std::vector<char>& memStream = *iter->second;
                wxMemoryInputStream buffer(&memStream[0], memStream.size()); //convert char-array to inputstream: no copying, ownership not transferred
                std::shared_ptr<DirInformation> dirInfoTmp = std::make_shared<DirInformation>();
                ReadDirInfo(buffer, zToWx(baseMapping.getDBFilename<LEFT_SIDE>()), *dirInfoTmp);  //read file/dir information
                oldDirInfoLeft = dirInfoTmp;
            }
    }
    catch(FileError&) {} //if error occurs: just overwrite old file! User is already informed about issues right after comparing!

    std::shared_ptr<DirInformation> oldDirInfoRight;
    try
    {
        DirectoryTOC::const_iterator iter = dbEntriesRight.second.find(dbEntriesLeft.first);
        if (iter != dbEntriesRight.second.end())
            if (iter->second.get())
            {
                const std::vector<char>& memStream = *iter->second;
                wxMemoryInputStream buffer(&memStream[0], memStream.size()); //convert char-array to inputstream: no copying, ownership not transferred
                std::shared_ptr<DirInformation> dirInfoTmp = std::make_shared<DirInformation>();
                ReadDirInfo(buffer, zToWx(baseMapping.getDBFilename<RIGHT_SIDE>()), *dirInfoTmp);  //read file/dir information
                oldDirInfoRight = dirInfoTmp;
            }
    }
    catch(FileError&) {}


    //create new database entries
    MemoryStreamPtr dbEntryLeft(new std::vector<char>);
    {
        wxMemoryOutputStream buffer;
        DirContainer* oldDir = oldDirInfoLeft.get() ? &oldDirInfoLeft->baseDirContainer : NULL;
        SaveDirInfo<LEFT_SIDE>(baseMapping, oldDir, zToWx(baseMapping.getDBFilename<LEFT_SIDE>()), buffer);
        dbEntryLeft->resize(buffer.GetSize());               //convert output stream to char-array
        buffer.CopyTo(&(*dbEntryLeft)[0], buffer.GetSize()); //
    }

    MemoryStreamPtr dbEntryRight(new std::vector<char>);
    {
        wxMemoryOutputStream buffer;
        DirContainer* oldDir = oldDirInfoRight.get() ? &oldDirInfoRight->baseDirContainer : NULL;
        SaveDirInfo<RIGHT_SIDE>(baseMapping, oldDir, zToWx(baseMapping.getDBFilename<RIGHT_SIDE>()), buffer);
        dbEntryRight->resize(buffer.GetSize());               //convert output stream to char-array
        buffer.CopyTo(&(*dbEntryRight)[0], buffer.GetSize()); //
    }

    //create/update DirInfo-streams
    {
        const bool updateRequiredLeft  = !entryExisting(dbEntriesLeft. second, dbEntriesRight.first, dbEntryLeft);
        const bool updateRequiredRight = !entryExisting(dbEntriesRight.second, dbEntriesLeft. first, dbEntryRight);
        //some users monitor the *.ffs_db file with RTS => don't touch the file if it isnt't strictly needed
        if (!updateRequiredLeft && !updateRequiredRight)
            return;
    }
    dbEntriesLeft .second[dbEntriesRight.first] = dbEntryLeft;
    dbEntriesRight.second[dbEntriesLeft .first] = dbEntryRight;

    //write (temp-) files...
    Loki::ScopeGuard guardTempFileLeft = Loki::MakeGuard(&zen::removeFile, fileNameLeftTmp);
    saveFile(dbEntriesLeft, fileNameLeftTmp);  //throw (FileError)

    Loki::ScopeGuard guardTempFileRight = Loki::MakeGuard(&zen::removeFile, fileNameRightTmp);
    saveFile(dbEntriesRight, fileNameRightTmp); //throw (FileError)

    //operation finished: rename temp files -> this should work transactionally:
    //if there were no write access, creation of temp files would have failed
    removeFile(baseMapping.getDBFilename<LEFT_SIDE>());
    removeFile(baseMapping.getDBFilename<RIGHT_SIDE>());
    renameFile(fileNameLeftTmp,  baseMapping.getDBFilename<LEFT_SIDE>());  //throw (FileError);
    renameFile(fileNameRightTmp, baseMapping.getDBFilename<RIGHT_SIDE>()); //throw (FileError);

    guardTempFileLeft. Dismiss(); //no need to delete temp file anymore
    guardTempFileRight.Dismiss(); //
}
