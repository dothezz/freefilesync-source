// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "db_file.h"
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include <wx/mstream.h>
#include <zen/file_error.h>
#include <wx+/string_conv.h>
#include <zen/file_handling.h>
#include <wx+/serialize.h>
#include <zen/file_io.h>
#include <zen/scope_guard.h>
#include <zen/guid.h>
#include <boost/bind.hpp>

#ifdef FFS_WIN
#include <zen/win.h> //includes "windows.h"
#include <zen/long_path_prefix.h>
#endif

using namespace zen;


namespace
{
//-------------------------------------------------------------------------------------------------------------------------------
const char FILE_FORMAT_DESCR[] = "FreeFileSync";
const int FILE_FORMAT_VER = 7;
//-------------------------------------------------------------------------------------------------------------------------------


template <SelectedSide side> inline
Zstring getDBFilename(const BaseDirMapping& baseMap, bool tempfile = false)
{
    //Linux and Windows builds are binary incompatible: char/wchar_t case, sensitive/insensitive
    //32 and 64 bit db files ARE designed to be binary compatible!
    //Give db files different names.
    //make sure they end with ".ffs_db". These files will not be included into comparison when located in base sync directories
#ifdef FFS_WIN
    Zstring dbname = Zstring(Zstr("sync")) + (tempfile ? Zstr(".tmp") : Zstr("")) + SYNC_DB_FILE_ENDING;
#elif defined FFS_LINUX
    //files beginning with dots are hidden e.g. in Nautilus
    Zstring dbname = Zstring(Zstr(".sync")) + (tempfile ? Zstr(".tmp") : Zstr("")) + SYNC_DB_FILE_ENDING;
#endif

    return baseMap.getBaseDirPf<side>() + dbname;
}



class FileInputStreamDB : public FileInputStream
{
public:
    FileInputStreamDB(const Zstring& filename) : //throw FileError
        FileInputStream(filename)
    {
        //read FreeFileSync file identifier
        char formatDescr[sizeof(FILE_FORMAT_DESCR)] = {};
        Read(formatDescr, sizeof(formatDescr)); //throw FileError

        if (!std::equal(FILE_FORMAT_DESCR, FILE_FORMAT_DESCR + sizeof(FILE_FORMAT_DESCR), formatDescr))
            throw FileError(_("Incompatible synchronization database format:") + " \n" + "\"" + filename + "\"");
    }

private:
};


class FileOutputStreamDB : public FileOutputStream
{
public:
    FileOutputStreamDB(const Zstring& filename) : //throw FileError
        FileOutputStream(filename)
    {
        //write FreeFileSync file identifier
        Write(FILE_FORMAT_DESCR, sizeof(FILE_FORMAT_DESCR)); //throw FileError
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

        const std::int64_t  modTime  = readNumberC<std::int64_t>();
        const std::uint64_t fileSize = readNumberC<std::uint64_t>();

        //const util::FileID fileIdentifier(stream_);
        //check();

        dirCont.addSubFile(shortName,
                           FileDescriptor(modTime, fileSize));
    }


    void readSubLink(DirContainer& dirCont) const
    {
        //attention: order of function argument evaluation is undefined! So do it one after the other...
        const Zstring      shortName  = readStringC<Zstring>(); //file name
        const std::int64_t modTime    = readNumberC<std::int64_t>();
        const Zstring      targetPath = readStringC<Zstring>(); //file name
        const LinkDescriptor::LinkType linkType = static_cast<LinkDescriptor::LinkType>(readNumberC<std::int32_t>());

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
typedef std::shared_ptr<std::vector<char> > MemoryStreamPtr;  //byte stream representing DirInformation
typedef std::map<UniqueId, MemoryStreamPtr> StreamMapping;    //list of streams ordered by session UUID
}

class ReadFileStream : public zen::ReadInputStream
{
public:
    ReadFileStream(wxInputStream& stream, const wxString& filename, StreamMapping& streamList) : ReadInputStream(stream, filename)
    {
        //|-------------------------------------------------------------------------------------
        //| ensure 32/64 bit portability: used fixed size data types only e.g. boost::uint32_t |
        //|-------------------------------------------------------------------------------------

        std::int32_t version = readNumberC<std::int32_t>();

        if (version != FILE_FORMAT_VER) //read file format version
            throw FileError(_("Incompatible synchronization database format:") + " \n" + "\"" + filename.c_str() + "\"");

        streamList.clear();

        boost::uint32_t dbCount = readNumberC<boost::uint32_t>(); //number of databases: one for each sync-pair
        while (dbCount-- != 0)
        {
            //DB id of partner databases
            const CharArray tmp2 = readArrayC();
            const std::string sessionID(tmp2->begin(), tmp2->end());

            CharArray buffer = readArrayC(); //read db-entry stream (containing DirInformation)

            streamList.insert(std::make_pair(sessionID, buffer));
        }
    }
};

namespace
{
StreamMapping loadStreams(const Zstring& filename) //throw FileError
{
    if (!zen::fileExists(filename))
        throw FileErrorDatabaseNotExisting(_("Initial synchronization:") + " \n\n" +
                                           _("One of the FreeFileSync database files is not yet existing:") + " \n" +
                                           "\"" + filename + "\"");

    try
    {
        //read format description (uncompressed)
        FileInputStreamDB uncompressed(filename); //throw FileError

        wxZlibInputStream input(uncompressed, wxZLIB_ZLIB);

        StreamMapping streamList;
        ReadFileStream(input, toWx(filename), streamList);
        return streamList;
    }
    catch (const std::bad_alloc&) //this is most likely caused by a corrupted database file
    {
        throw FileError(_("Error reading from synchronization database:") + " (bad_alloc)");
    }
}


DirInfoPtr parseStream(const std::vector<char>& stream, const Zstring& fileName) //throw FileError -> return value always bound!
{
    try
    {
        //read streams into DirInfo
        auto dirInfo = std::make_shared<DirInformation>();
        wxMemoryInputStream buffer(&stream[0], stream.size()); //convert char-array to inputstream: no copying, ownership not transferred
        ReadDirInfo(buffer, toWx(fileName), *dirInfo); //throw FileError
        return dirInfo;
    }
    catch (const std::bad_alloc&) //this is most likely caused by a corrupted database file
    {
        throw FileError(_("Error reading from synchronization database:") + " (bad_alloc)");
    }
}
}


std::pair<DirInfoPtr, DirInfoPtr> zen::loadFromDisk(const BaseDirMapping& baseMapping) //throw FileError
{
    const Zstring fileNameLeft  = getDBFilename<LEFT_SIDE>(baseMapping);
    const Zstring fileNameRight = getDBFilename<RIGHT_SIDE>(baseMapping);

    //read file data: list of session ID + DirInfo-stream
    const StreamMapping streamListLeft  = ::loadStreams(fileNameLeft);  //throw FileError
    const StreamMapping streamListRight = ::loadStreams(fileNameRight); //throw FileError

    //find associated session: there can be at most one session within intersection of left and right ids
    StreamMapping::const_iterator streamLeft  = streamListLeft .end();
    StreamMapping::const_iterator streamRight = streamListRight.end();
    for (auto iterLeft = streamListLeft.begin(); iterLeft != streamListLeft.end(); ++iterLeft)
    {
        auto iterRight = streamListRight.find(iterLeft->first);
        if (iterRight != streamListRight.end())
        {
            streamLeft  = iterLeft;
            streamRight = iterRight;
            break;
        }
    }

    if (streamLeft  == streamListLeft .end() ||
        streamRight == streamListRight.end() ||
        !streamLeft ->second.get() ||
        !streamRight->second.get())
        throw FileErrorDatabaseNotExisting(_("Initial synchronization:") + " \n\n" +
                                           _("Database files do not share a common synchronization session:") + " \n" +
                                           "\"" + fileNameLeft  + "\"\n" +
                                           "\"" + fileNameRight + "\"");
    //read streams into DirInfo
    DirInfoPtr dirInfoLeft  = parseStream(*streamLeft ->second, fileNameLeft);  //throw FileError
    DirInfoPtr dirInfoRight = parseStream(*streamRight->second, fileNameRight); //throw FileError

    return std::make_pair(dirInfoLeft, dirInfoRight);
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
        std::for_each(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(), boost::bind(&SaveDirInfo::processFile, this, _1, oldDirInfo));
        writeNumberC<bool>(false); //mark last entry
        std::for_each(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(), boost::bind(&SaveDirInfo::processLink, this, _1, oldDirInfo));
        writeNumberC<bool>(false); //mark last entry
        std::for_each(hierObj.refSubDirs ().begin(), hierObj.refSubDirs ().end(), boost::bind(&SaveDirInfo::processDir,  this, _1, oldDirInfo));
        writeNumberC<bool>(false); //mark last entry
    }

    void processFile(const FileMapping& fileMap, const DirContainer* oldParentDir)
    {
        if (fileMap.getCategory() == FILE_EQUAL) //data in sync: write current state
        {
            if (!fileMap.isEmpty<side>())
            {
                writeNumberC<bool>(true); //mark beginning of entry
                writeStringC(fileMap.getShortName<side>()); //save respecting case! (Windows)
                writeNumberC<std:: int64_t>(to<std:: int64_t>(fileMap.getLastWriteTime<side>())); //last modification time
                writeNumberC<std::uint64_t>(to<std::uint64_t>(fileMap.getFileSize<side>())); //filesize
            }
        }
        else //not in sync: reuse last synchronous state
        {
            if (oldParentDir) //no data is also a "synchronous state"!
            {
                auto iter = oldParentDir->files.find(fileMap.getObjShortName());
                if (iter != oldParentDir->files.end())
                {
                    writeNumberC<bool>(true); //mark beginning of entry
                    writeStringC(iter->first); //save respecting case! (Windows)
                    writeNumberC<std:: int64_t>(to<std:: int64_t>(iter->second.lastWriteTimeRaw)); //last modification time
                    writeNumberC<std::uint64_t>(to<std::uint64_t>(iter->second.fileSize)); //filesize
                }
            }
        }
    }

    void processLink(const SymLinkMapping& linkObj, const DirContainer* oldParentDir)
    {
        if (linkObj.getLinkCategory() == SYMLINK_EQUAL) //data in sync: write current state
        {
            if (!linkObj.isEmpty<side>())
            {
                writeNumberC<bool>(true); //mark beginning of entry
                writeStringC(linkObj.getShortName<side>()); //save respecting case! (Windows)
                writeNumberC<std::int64_t>(to<std::int64_t>(linkObj.getLastWriteTime<side>())); //last modification time
                writeStringC(linkObj.getTargetPath<side>());
                writeNumberC<std::int32_t>(linkObj.getLinkType<side>());
            }
        }
        else //not in sync: reuse last synchronous state
        {
            if (oldParentDir) //no data is also a "synchronous state"!
            {
                auto iter = oldParentDir->links.find(linkObj.getObjShortName());
                if (iter != oldParentDir->links.end())
                {
                    writeNumberC<bool>(true); //mark beginning of entry
                    writeStringC(iter->first); //save respecting case! (Windows)
                    writeNumberC<std::int64_t>(to<std::int64_t>(iter->second.lastWriteTimeRaw)); //last modification time
                    writeStringC(iter->second.targetPath);
                    writeNumberC<std::int32_t>(iter->second.type);
                }
            }
        }
    }

    void processDir(const DirMapping& dirMap, const DirContainer* oldParentDir)
    {
        const DirContainer* oldDir     = NULL;
        const Zstring*      oldDirName = NULL;
        if (oldParentDir) //no data is also a "synchronous state"!
        {
            auto iter = oldParentDir->dirs.find(dirMap.getObjShortName());
            if (iter != oldParentDir->dirs.end())
            {
                oldDirName = &iter->first;
                oldDir     = &iter->second;
            }
        }

        CompareDirResult cat = dirMap.getDirCategory();

        if (cat == DIR_EQUAL) //data in sync: write current state
        {
            if (!dirMap.isEmpty<side>())
            {
                writeNumberC<bool>(true); //mark beginning of entry
                writeStringC(dirMap.getShortName<side>()); //save respecting case! (Windows)
                execute(dirMap, oldDir); //recurse
            }
        }
        else //not in sync: reuse last synchronous state
        {
            if (oldDir)
            {
                writeNumberC<bool>(true);  //mark beginning of entry
                writeStringC(*oldDirName); //save respecting case! (Windows)
                execute(dirMap, oldDir); //recurse
                return;
            }
            //no data is also a "synchronous state"!

            //else: not in sync AND no "last synchronous state"
            //we cannot simply skip the whole directory, since sub-items might be in sync
            //Example: directories on left and right differ in case while sub-files are equal
            switch (cat)
            {
                case DIR_LEFT_SIDE_ONLY: //sub-items cannot be in sync
                    break;
                case DIR_RIGHT_SIDE_ONLY: //sub-items cannot be in sync
                    break;
                case DIR_EQUAL:
                    assert(false);
                    break;
                case DIR_DIFFERENT_METADATA:
                    writeNumberC<bool>(true);
                    writeStringC(dirMap.getShortName<side>());
                    //ATTENTION: strictly this is a violation of the principle of reporting last synchronous state!
                    //however in this case this will result in "last sync unsuccessful" for this directory within <automatic> algorithm, which is fine
                    execute(dirMap, oldDir); //recurse and save sub-items which are in sync
                    break;
            }
        }
    }
};


class WriteFileStream : public WriteOutputStream
{
public:
    WriteFileStream(const StreamMapping& streamList, const wxString& filename, wxOutputStream& stream) : WriteOutputStream(filename, stream)
    {
        //save file format version
        writeNumberC<std::int32_t>(FILE_FORMAT_VER);

        writeNumberC<boost::uint32_t>(static_cast<boost::uint32_t>(streamList.size())); //number of database records: one for each sync-pair

        for (StreamMapping::const_iterator i = streamList.begin(); i != streamList.end(); ++i)
        {
            //sync session id
            writeArrayC(std::vector<char>(i->first.begin(), i->first.end()));

            //write DirInformation stream
            writeArrayC(*(i->second));
        }
    }
};


//save/load DirContainer
void saveFile(const StreamMapping& streamList, const Zstring& filename) //throw FileError
{
    {
        //write format description (uncompressed)
        FileOutputStreamDB uncompressed(filename); //throw FileError

        wxZlibOutputStream output(uncompressed, 4, wxZLIB_ZLIB);
        /* 4 - best compromise between speed and compression: (scanning 200.000 objects)
        0 (uncompressed)        8,95 MB -  422 ms
        2                       2,07 MB -  470 ms
        4                       1,87 MB -  500 ms
        6                       1,77 MB -  613 ms
        9 (maximal compression) 1,74 MB - 3330 ms */

        WriteFileStream(streamList, toWx(filename), output);
    }
    //(try to) hide database file
#ifdef FFS_WIN
    ::SetFileAttributes(zen::applyLongPathPrefix(filename).c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
}


bool equalEntry(const MemoryStreamPtr& lhs, const MemoryStreamPtr& rhs)
{
    if (!lhs.get() || !rhs.get())
        return lhs.get() == rhs.get();

    return *lhs == *rhs;
}


void zen::saveToDisk(const BaseDirMapping& baseMapping) //throw FileError
{
    //transactional behaviour! write to tmp files first
    const Zstring dbNameLeftTmp  = getDBFilename<LEFT_SIDE >(baseMapping, true);
    const Zstring dbNameRightTmp = getDBFilename<RIGHT_SIDE>(baseMapping, true);

    const Zstring dbNameLeft  = getDBFilename<LEFT_SIDE >(baseMapping);
    const Zstring dbNameRight = getDBFilename<RIGHT_SIDE>(baseMapping);

    //delete old tmp file, if necessary -> throws if deletion fails!
    removeFile(dbNameLeftTmp);  //
    removeFile(dbNameRightTmp); //throw FileError

    //(try to) load old database files...
    StreamMapping streamListLeft;
    StreamMapping streamListRight;

    try //read file data: list of session ID + DirInfo-stream
    {
        streamListLeft = ::loadStreams(dbNameLeft);
    }
    catch (FileError&) {} //if error occurs: just overwrite old file! User is already informed about issues right after comparing!
    try
    {
        streamListRight = ::loadStreams(dbNameRight);
    }
    catch (FileError&) {}

    //find associated session: there can be at most one session within intersection of left and right ids
    StreamMapping::iterator streamLeft  = streamListLeft .end();
    StreamMapping::iterator streamRight = streamListRight.end();
    for (auto iterLeft = streamListLeft.begin(); iterLeft != streamListLeft.end(); ++iterLeft)
    {
        auto iterRight = streamListRight.find(iterLeft->first);
        if (iterRight != streamListRight.end())
        {
            streamLeft  = iterLeft;
            streamRight = iterRight;
            break;
        }
    }

    //(try to) read old DirInfo
    DirInfoPtr oldDirInfoLeft;
    DirInfoPtr oldDirInfoRight;
    try
    {
        if (streamLeft  != streamListLeft .end() &&
            streamRight != streamListRight.end() &&
            streamLeft ->second.get() &&
            streamRight->second.get())
        {
            oldDirInfoLeft  = parseStream(*streamLeft ->second, dbNameLeft); //throw FileError
            oldDirInfoRight = parseStream(*streamRight->second, dbNameRight); //throw FileError
        }
    }
    catch (FileError&)
    {
        //if error occurs: just overwrite old file! User is already informed about issues right after comparing!
        oldDirInfoLeft .reset(); //read both or none!
        oldDirInfoRight.reset(); //
    }

    //create new database entries
    MemoryStreamPtr newStreamLeft = std::make_shared<std::vector<char>>();
    {
        wxMemoryOutputStream buffer;
        const DirContainer* oldDir = oldDirInfoLeft.get() ? &oldDirInfoLeft->baseDirContainer : NULL;
        SaveDirInfo<LEFT_SIDE>(baseMapping, oldDir, toWx(dbNameLeft), buffer);
        newStreamLeft->resize(buffer.GetSize());               //convert output stream to char-array
        buffer.CopyTo(&(*newStreamLeft)[0], buffer.GetSize()); //
    }

    MemoryStreamPtr newStreamRight = std::make_shared<std::vector<char>>();
    {
        wxMemoryOutputStream buffer;
        const DirContainer* oldDir = oldDirInfoRight.get() ? &oldDirInfoRight->baseDirContainer : NULL;
        SaveDirInfo<RIGHT_SIDE>(baseMapping, oldDir, toWx(dbNameRight), buffer);
        newStreamRight->resize(buffer.GetSize());               //convert output stream to char-array
        buffer.CopyTo(&(*newStreamRight)[0], buffer.GetSize()); //
    }

    //check if there is some work to do at all
    {
        const bool updateRequiredLeft  = streamLeft  == streamListLeft .end() || !equalEntry(newStreamLeft,  streamLeft ->second);
        const bool updateRequiredRight = streamRight == streamListRight.end() || !equalEntry(newStreamRight, streamRight->second);
        //some users monitor the *.ffs_db file with RTS => don't touch the file if it isnt't strictly needed
        if (!updateRequiredLeft && !updateRequiredRight)
            return;
    }

    //create/update DirInfo-streams
    std::string sessionID = zen::generateGUID();

    //erase old session data
    if (streamLeft != streamListLeft.end())
        streamListLeft.erase(streamLeft);
    if (streamRight != streamListRight.end())
        streamListRight.erase(streamRight);

    //fill in new
    streamListLeft .insert(std::make_pair(sessionID, newStreamLeft));
    streamListRight.insert(std::make_pair(sessionID, newStreamRight));

    //write (temp-) files...
    zen::ScopeGuard guardTempFileLeft = zen::makeGuard([&]() {zen::removeFile(dbNameLeftTmp); });
    saveFile(streamListLeft, dbNameLeftTmp);  //throw FileError

    zen::ScopeGuard guardTempFileRight = zen::makeGuard([&]() {zen::removeFile(dbNameRightTmp); });
    saveFile(streamListRight, dbNameRightTmp); //throw FileError

    //operation finished: rename temp files -> this should work transactionally:
    //if there were no write access, creation of temp files would have failed
    removeFile(dbNameLeft);
    removeFile(dbNameRight);
    renameFile(dbNameLeftTmp,  dbNameLeft);  //throw FileError;
    renameFile(dbNameRightTmp, dbNameRight); //throw FileError;

    guardTempFileLeft. dismiss(); //no need to delete temp file anymore
    guardTempFileRight.dismiss(); //
}
