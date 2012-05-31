// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
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
#include <zen/utf.h>

#ifdef FFS_WIN
#include <zen/win.h> //includes "windows.h"
#include <zen/long_path_prefix.h>
#endif

using namespace zen;


namespace
{
//-------------------------------------------------------------------------------------------------------------------------------
const char FILE_FORMAT_DESCR[] = "FreeFileSync";
const int FILE_FORMAT_VER = 8;
//-------------------------------------------------------------------------------------------------------------------------------

typedef std::string UniqueId;
typedef Zbase<char> MemoryStream;                       //ref-counted byte stream representing DirInformation
typedef std::map<UniqueId, MemoryStream> StreamMapping; //list of streams ordered by session UUID


//-----------------------------------------------------------------------------------
//| ensure 32/64 bit portability: use fixed size data types only e.g. std::uint32_t |
//-----------------------------------------------------------------------------------


template <SelectedSide side> inline
Zstring getDBFilename(const BaseDirMapping& baseMap, bool tempfile = false)
{
    //Linux and Windows builds are binary incompatible: different file id?, problem with case sensitivity?
    //however 32 and 64 bit db files *are* designed to be binary compatible!
    //Give db files different names.
    //make sure they end with ".ffs_db". These files will not be included into comparison
#ifdef FFS_WIN
    Zstring dbname = Zstring(Zstr("sync")) + (tempfile ? Zstr(".tmp") : Zstr("")) + SYNC_DB_FILE_ENDING;
#elif defined FFS_LINUX
    //files beginning with dots are hidden e.g. in Nautilus
    Zstring dbname = Zstring(Zstr(".sync")) + (tempfile ? Zstr(".tmp") : Zstr("")) + SYNC_DB_FILE_ENDING;
#endif

    return baseMap.getBaseDirPf<side>() + dbname;
}


class CheckedDbReader : public CheckedReader
{
public:
    CheckedDbReader(wxInputStream& stream, const Zstring& errorObjName) : CheckedReader(stream), errorObjName_(errorObjName) {}

private:
    virtual void throwException() const { throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(errorObjName_))); }

    const Zstring errorObjName_;
};


class CheckedDbWriter : public CheckedWriter
{
public:
    CheckedDbWriter(wxOutputStream& stream, const Zstring& errorObjName) : CheckedWriter(stream), errorObjName_(errorObjName) {}

private:
    virtual void throwException() const { throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(errorObjName_))); }

    const Zstring errorObjName_;
};



StreamMapping loadStreams(const Zstring& filename) //throw FileError
{
    try
    {
        //read format description (uncompressed)
        FileInputStream rawStream(filename); //throw FileError, ErrorNotExisting

        //read FreeFileSync file identifier
        char formatDescr[sizeof(FILE_FORMAT_DESCR)] = {};
        rawStream.Read(formatDescr, sizeof(formatDescr)); //throw FileError

        if (!std::equal(FILE_FORMAT_DESCR, FILE_FORMAT_DESCR + sizeof(FILE_FORMAT_DESCR), formatDescr))
            throw FileError(replaceCpy(_("Database file %x is incompatible."), L"%x", fmtFileName(filename)));

        wxZlibInputStream decompressed(rawStream, wxZLIB_ZLIB);

        CheckedDbReader cr(decompressed, filename);

        std::int32_t version = cr.readPOD<std::int32_t>();
        if (version != FILE_FORMAT_VER) //read file format version#
            throw FileError(replaceCpy(_("Database file %x is incompatible."), L"%x", fmtFileName(filename)));

        //read stream lists
        StreamMapping output;

        std::uint32_t dbCount = cr.readPOD<std::uint32_t>(); //number of databases: one for each sync-pair
        while (dbCount-- != 0)
        {
            //DB id of partner databases
            const std::string sessionID = cr.readString<std::string>();
            const MemoryStream stream = cr.readString<MemoryStream>(); //read db-entry stream (containing DirInformation)

            output.insert(std::make_pair(sessionID, stream));
        }
        return output;
    }
    catch (ErrorNotExisting&)
    {
        throw FileErrorDatabaseNotExisting(_("Initial synchronization:") + L" \n" +
                                           replaceCpy(_("Database file %x does not yet exist."), L"%x", fmtFileName(filename)));
    }
    catch (const std::bad_alloc& e)
    {
        throw FileError(_("Out of memory!") + L" " + utfCvrtTo<std::wstring>(e.what()));
    }
}


class StreamParser : private CheckedDbReader
{
public:
    static DirInfoPtr execute(const MemoryStream& stream, const Zstring& fileName) //throw FileError -> return value always bound!
    {
        try
        {
            //read streams into DirInfo
            auto dirInfo = std::make_shared<DirInformation>();
            wxMemoryInputStream buffer(&*stream.begin(), stream.size()); //convert char-array to inputstream: no copying, ownership not transferred
            StreamParser(buffer, fileName, *dirInfo); //throw FileError
            return dirInfo;
        }
        catch (const std::bad_alloc& e)
        {
            throw FileError(_("Out of memory!") + L" " + utfCvrtTo<std::wstring>(e.what()));
        }
    }

private:
    StreamParser(wxInputStream& stream, const Zstring& errorObjName, DirInformation& dirInfo) : CheckedDbReader(stream, errorObjName)
    {
        recurse(dirInfo.baseDirContainer);
    }

    Zstring readStringUtf8() const
    {
        return utfCvrtTo<Zstring>(readString<Zbase<char>>());
    }

    FileId readFileId() const
    {
        assert_static(sizeof(FileId().first ) <= sizeof(std::uint64_t));
        assert_static(sizeof(FileId().second) <= sizeof(std::uint64_t));

        const auto deviceId = static_cast<decltype(FileId().first )>(readPOD<std::uint64_t>()); //
        const auto fileId   = static_cast<decltype(FileId().second)>(readPOD<std::uint64_t>()); //silence "loss of precision" compiler warnings
        return std::make_pair(deviceId, fileId);
    }

    void recurse(DirContainer& dirCont) const
    {
        while (readPOD<bool>()) //files
        {
            //attention: order of function argument evaluation is undefined! So do it one after the other...
            const Zstring shortName = readStringUtf8(); //file name

            const std::int64_t  modTime  = readPOD<std::int64_t>();
            const std::uint64_t fileSize = readPOD<std::uint64_t>();
            const FileId        fileID   = readFileId();

            dirCont.addSubFile(shortName,
                               FileDescriptor(modTime, fileSize, fileID));
        }

        while (readPOD<bool>()) //symlinks
        {
            //attention: order of function argument evaluation is undefined! So do it one after the other...
            const Zstring      shortName  = readStringUtf8(); //file name
            const std::int64_t modTime    = readPOD<std::int64_t>();
            const Zstring      targetPath = readStringUtf8(); //file name
            const LinkDescriptor::LinkType linkType = static_cast<LinkDescriptor::LinkType>(readPOD<std::int32_t>());

            dirCont.addSubLink(shortName,
                               LinkDescriptor(modTime, targetPath, linkType));
        }

        while (readPOD<bool>()) //directories
        {
            const Zstring shortName = readStringUtf8(); //directory name
            DirContainer& subDir = dirCont.addSubDir(shortName);
            recurse(subDir);
        }
    }
};


//save/load DirContainer
void saveFile(const StreamMapping& streamList, const Zstring& filename) //throw FileError
{
    {
        FileOutputStream rawStream(filename); //throw FileError

        //write FreeFileSync file identifier
        rawStream.Write(FILE_FORMAT_DESCR, sizeof(FILE_FORMAT_DESCR)); //throw FileError

        wxZlibOutputStream compressed(rawStream, 4, wxZLIB_ZLIB);
        /* 4 - best compromise between speed and compression: (scanning 200.000 objects)
        0 (uncompressed)        8,95 MB -  422 ms
        2                       2,07 MB -  470 ms
        4                       1,87 MB -  500 ms
        6                       1,77 MB -  613 ms
        9 (maximal compression) 1,74 MB - 3330 ms */

        CheckedDbWriter cw(compressed, filename);

        //save file format version
        cw.writePOD<std::int32_t>(FILE_FORMAT_VER);

        //save stream list
        cw.writePOD<std::uint32_t>(static_cast<std::uint32_t>(streamList.size())); //number of database records: one for each sync-pair

        for (auto iter = streamList.begin(); iter != streamList.end(); ++iter)
        {
            cw.writeString<std::string >(iter->first ); //sync session id
            cw.writeString<MemoryStream>(iter->second); //DirInformation stream
        }
    }
#ifdef FFS_WIN
    ::SetFileAttributes(applyLongPathPrefix(filename).c_str(), FILE_ATTRIBUTE_HIDDEN); //(try to) hide database file
#endif
}


template <SelectedSide side>
class StreamGenerator : private CheckedDbWriter
{
public:
    static MemoryStream execute(const BaseDirMapping& baseMapping, const DirContainer* oldDirInfo, const Zstring& errorObjName)
    {
        wxMemoryOutputStream buffer;
        StreamGenerator(baseMapping, oldDirInfo, errorObjName, buffer);

        MemoryStream output;
        output.resize(buffer.GetSize());
        buffer.CopyTo(&*output.begin(), buffer.GetSize());
        return output;
    }

private:
    StreamGenerator(const BaseDirMapping& baseMapping, const DirContainer* oldDirInfo, const Zstring& errorObjName, wxOutputStream& stream) : CheckedDbWriter(stream, errorObjName)
    {
        recurse(baseMapping, oldDirInfo);
    }

    void recurse(const HierarchyObject& hierObj, const DirContainer* oldDirInfo)
    {
        // for (const auto& fileMap : hierObj.refSubFiles()) { processFile(fileMap, oldDirInfo); }); !

        std::for_each(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(), [&](const FileMapping&    fileMap) { this->processFile(fileMap, oldDirInfo); });
        writePOD<bool>(false); //mark last entry
        std::for_each(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(), [&](const SymLinkMapping& linkObj) { this->processLink(linkObj, oldDirInfo); });
        writePOD<bool>(false); //mark last entry
        std::for_each(hierObj.refSubDirs ().begin(), hierObj.refSubDirs ().end(), [&](const DirMapping&      dirMap) { this->processDir (dirMap,  oldDirInfo); });
        writePOD<bool>(false); //mark last entry
    }

    void writeStringUtf8(const Zstring& str) { writeString(utfCvrtTo<Zbase<char>>(str)); }

    void writeFileId(const FileId& id)
    {
        writePOD<std::uint64_t>(id.first ); //device id
        writePOD<std::uint64_t>(id.second); //file id
    }

#ifdef _MSC_VER
    warn_static("support multiple folder pairs that differ in hard filter only?")
#endif

    void processFile(const FileMapping& fileMap, const DirContainer* oldParentDir)
    {
        if (fileMap.getCategory() == FILE_EQUAL) //data in sync: write current state
        {
            if (!fileMap.isEmpty<side>())
            {
                writePOD<bool>(true); //mark beginning of entry
                writeStringUtf8(fileMap.getShortName<side>()); //save respecting case! (Windows)
                writePOD<std:: int64_t>(to<std:: int64_t>(fileMap.getLastWriteTime<side>()));
                writePOD<std::uint64_t>(to<std::uint64_t>(fileMap.getFileSize<side>()));
                writeFileId(fileMap.getFileId<side>());
            }
        }
        else //not in sync: reuse last synchronous state
        {
            if (oldParentDir) //no data is also a "synchronous state"!
            {
                auto iter = oldParentDir->files.find(fileMap.getObjShortName());
                if (iter != oldParentDir->files.end())
                {
                    writePOD<bool>(true); //mark beginning of entry
                    writeStringUtf8(iter->first); //save respecting case! (Windows)
                    writePOD<std:: int64_t>(to<std:: int64_t>(iter->second.lastWriteTimeRaw));
                    writePOD<std::uint64_t>(to<std::uint64_t>(iter->second.fileSize));
                    writeFileId(iter->second.id);
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
                writePOD<bool>(true); //mark beginning of entry
                writeStringUtf8(linkObj.getShortName<side>()); //save respecting case! (Windows)
                writePOD<std::int64_t>(to<std::int64_t>(linkObj.getLastWriteTime<side>()));
                writeStringUtf8(linkObj.getTargetPath<side>());
                writePOD<std::int32_t>(linkObj.getLinkType<side>());
            }
        }
        else //not in sync: reuse last synchronous state
        {
            if (oldParentDir) //no data is also a "synchronous state"!
            {
                auto iter = oldParentDir->links.find(linkObj.getObjShortName());
                if (iter != oldParentDir->links.end())
                {
                    writePOD<bool>(true); //mark beginning of entry
                    writeStringUtf8(iter->first); //save respecting case! (Windows)
                    writePOD<std::int64_t>(to<std::int64_t>(iter->second.lastWriteTimeRaw));
                    writeStringUtf8(iter->second.targetPath);
                    writePOD<std::int32_t>(iter->second.type);
                }
            }
        }
    }

    void processDir(const DirMapping& dirMap, const DirContainer* oldParentDir)
    {
        const DirContainer* oldDir     = nullptr;
        const Zstring*      oldDirName = nullptr;
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
                writePOD<bool>(true); //mark beginning of entry
                writeStringUtf8(dirMap.getShortName<side>()); //save respecting case! (Windows)
                recurse(dirMap, oldDir);
            }
        }
        else //not in sync: reuse last synchronous state
        {
            if (oldDir)
            {
                writePOD<bool>(true);  //mark beginning of entry
                writeStringUtf8(*oldDirName); //save respecting case! (Windows)
                recurse(dirMap, oldDir);
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
                    writePOD<bool>(true);
                    writeStringUtf8(dirMap.getShortName<side>());
                    //ATTENTION: strictly this is a violation of the principle of reporting last synchronous state!
                    //however in this case this will result in "last sync unsuccessful" for this directory within <automatic> algorithm, which is fine
                    recurse(dirMap, oldDir); //recurse and save sub-items which are in sync
                    break;
            }
        }
    }
};
}
//#######################################################################################################################################


std::pair<DirInfoPtr, DirInfoPtr> zen::loadFromDisk(const BaseDirMapping& baseMapping) //throw FileError
{
    const Zstring fileNameLeft  = getDBFilename<LEFT_SIDE >(baseMapping);
    const Zstring fileNameRight = getDBFilename<RIGHT_SIDE>(baseMapping);

    //read file data: list of session ID + DirInfo-stream
    const StreamMapping streamListLeft  = ::loadStreams(fileNameLeft);  //throw FileError
    const StreamMapping streamListRight = ::loadStreams(fileNameRight); //throw FileError

    //find associated session: there can be at most one session within intersection of left and right ids
    for (auto iterLeft = streamListLeft.begin(); iterLeft != streamListLeft.end(); ++iterLeft)
    {
        auto iterRight = streamListRight.find(iterLeft->first);
        if (iterRight != streamListRight.end())
        {
            //read streams into DirInfo
            DirInfoPtr dirInfoLeft  = StreamParser::execute(iterLeft ->second, fileNameLeft);  //throw FileError
            DirInfoPtr dirInfoRight = StreamParser::execute(iterRight->second, fileNameRight); //throw FileError

            return std::make_pair(dirInfoLeft, dirInfoRight);
        }
    }

    throw FileErrorDatabaseNotExisting(_("Initial synchronization:") + L" \n" +
                                       _("Database files do not share a common session."));
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

    //read file data: list of session ID + DirInfo-stream
    try { streamListLeft  = ::loadStreams(dbNameLeft ); }
    catch (FileError&) {}
    try { streamListRight = ::loadStreams(dbNameRight); }
    catch (FileError&) {}
    //if error occurs: just overwrite old file! User is already informed about issues right after comparing!

    //find associated session: there can be at most one session within intersection of left and right ids
    auto streamLeftOld  = streamListLeft .cend();
    auto streamRightOld = streamListRight.cend();
    for (auto iterLeft = streamListLeft.begin(); iterLeft != streamListLeft.end(); ++iterLeft)
    {
        auto iterRight = streamListRight.find(iterLeft->first);
        if (iterRight != streamListRight.end())
        {
            streamLeftOld  = iterLeft;
            streamRightOld = iterRight;
            break;
        }
    }

    //(try to) read old DirInfo
    DirInfoPtr dirInfoLeftOld;
    DirInfoPtr dirInfoRightOld;
    if (streamLeftOld  != streamListLeft .end() &&
        streamRightOld != streamListRight.end())
        try
        {
            dirInfoLeftOld  = StreamParser::execute(streamLeftOld ->second, dbNameLeft ); //throw FileError
            dirInfoRightOld = StreamParser::execute(streamRightOld->second, dbNameRight); //throw FileError
        }
        catch (FileError&)
        {
            //if error occurs: just overwrite old file! User is already informed about issues right after comparing!
            dirInfoLeftOld .reset(); //read both or none!
            dirInfoRightOld.reset(); //
        }

    //create new database entries
    MemoryStream rawStreamLeftNew  = StreamGenerator<LEFT_SIDE >::execute(baseMapping, dirInfoLeftOld .get() ? &dirInfoLeftOld ->baseDirContainer : nullptr, dbNameLeft);
    MemoryStream rawStreamRightNew = StreamGenerator<RIGHT_SIDE>::execute(baseMapping, dirInfoRightOld.get() ? &dirInfoRightOld->baseDirContainer : nullptr, dbNameRight);

    //check if there is some work to do at all
    if (streamLeftOld  != streamListLeft .end() && rawStreamLeftNew  == streamLeftOld ->second &&
        streamRightOld != streamListRight.end() && rawStreamRightNew == streamRightOld->second)
        return; //some users monitor the *.ffs_db file with RTS => don't touch the file if it isnt't strictly needed

    //erase old session data
    if (streamLeftOld != streamListLeft.end())
        streamListLeft.erase(streamLeftOld);
    if (streamRightOld != streamListRight.end())
        streamListRight.erase(streamRightOld);

    //create/update DirInfo-streams
    const std::string sessionID = zen::generateGUID();

    //fill in new
    streamListLeft .insert(std::make_pair(sessionID, rawStreamLeftNew));
    streamListRight.insert(std::make_pair(sessionID, rawStreamRightNew));

    //write (temp-) files...
    zen::ScopeGuard guardTempFileLeft = zen::makeGuard([&] {zen::removeFile(dbNameLeftTmp); });
    saveFile(streamListLeft, dbNameLeftTmp);  //throw FileError

    zen::ScopeGuard guardTempFileRight = zen::makeGuard([&] {zen::removeFile(dbNameRightTmp); });
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
