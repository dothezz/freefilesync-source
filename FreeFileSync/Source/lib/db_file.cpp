// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "db_file.h"
#include <zen/file_handling.h>
#include <zen/scope_guard.h>
#include <zen/guid.h>
#include <zen/utf.h>
#include <zen/serialize.h>
#include <wx+/zlib_wrap.h>

#ifdef ZEN_WIN
#include <zen/win.h> //includes "windows.h"
#include <zen/long_path_prefix.h>
#endif

using namespace zen;


namespace
{
//-------------------------------------------------------------------------------------------------------------------------------
const char FILE_FORMAT_DESCR[] = "FreeFileSync";
const int DB_FORMAT_CONTAINER = 9;
const int DB_FORMAT_STREAM    = 1;
//-------------------------------------------------------------------------------------------------------------------------------

typedef std::string UniqueId;
typedef std::map<UniqueId, BinaryStream> DbStreams; //list of streams ordered by session UUID

//-----------------------------------------------------------------------------------
//| ensure 32/64 bit portability: use fixed size data types only e.g. std::uint32_t |
//-----------------------------------------------------------------------------------

template <SelectedSide side> inline
Zstring getDBFilename(const BaseDirPair& baseDirObj, bool tempfile = false)
{
    //Linux and Windows builds are binary incompatible: different file id?, problem with case sensitivity? are UTC file times really compatible?
    //what about endianess!?
    //however 32 and 64 bit db files *are* designed to be binary compatible!
    //Give db files different names.
    //make sure they end with ".ffs_db". These files will be excluded from comparison
#ifdef ZEN_WIN
    Zstring dbname = Zstring(Zstr("sync")) + (tempfile ? Zstr(".tmp") : Zstr("")) + SYNC_DB_FILE_ENDING;
#elif defined ZEN_LINUX || defined ZEN_MAC
    //files beginning with dots are hidden e.g. in Nautilus
    Zstring dbname = Zstring(Zstr(".sync")) + (tempfile ? Zstr(".tmp") : Zstr("")) + SYNC_DB_FILE_ENDING;
#endif
    return baseDirObj.getBaseDirPf<side>() + dbname;
}

//#######################################################################################################################################

void saveStreams(const DbStreams& streamList, const Zstring& filename) //throw FileError
{
    BinStreamOut streamOut;

    //write FreeFileSync file identifier
    writeArray(streamOut, FILE_FORMAT_DESCR, sizeof(FILE_FORMAT_DESCR));

    //save file format version
    writeNumber<std::int32_t>(streamOut, DB_FORMAT_CONTAINER);

    //save stream list
    writeNumber<std::uint32_t>(streamOut, static_cast<std::uint32_t>(streamList.size())); //number of streams, one for each sync-pair

    for (const auto& stream : streamList)
    {
        writeContainer<std::string >(streamOut, stream.first );
        writeContainer<BinaryStream>(streamOut, stream.second);
    }

    assert(!somethingExists(filename)); //orphan tmp files should be cleaned up already at this point!
    saveBinStream(filename, streamOut.get()); //throw FileError

#ifdef ZEN_WIN
    //be careful to avoid CreateFile() + CREATE_ALWAYS on a hidden file -> see file_io.cpp
    ::SetFileAttributes(applyLongPathPrefix(filename).c_str(), FILE_ATTRIBUTE_HIDDEN); //(try to) hide database file
#endif
}


DbStreams loadStreams(const Zstring& filename) //throw FileError, FileErrorDatabaseNotExisting
{
    try
    {
        BinStreamIn streamIn = loadBinStream<BinaryStream>(filename); //throw FileError

        //read FreeFileSync file identifier
        char formatDescr[sizeof(FILE_FORMAT_DESCR)] = {};
        readArray(streamIn, formatDescr, sizeof(formatDescr)); //throw UnexpectedEndOfStreamError

        if (!std::equal(FILE_FORMAT_DESCR, FILE_FORMAT_DESCR + sizeof(FILE_FORMAT_DESCR), formatDescr))
            throw FileError(replaceCpy(_("Database file %x is incompatible."), L"%x", fmtFileName(filename)));

        const int version = readNumber<std::int32_t>(streamIn); //throw UnexpectedEndOfStreamError
        if (version != DB_FORMAT_CONTAINER) //read file format version number
            throw FileError(replaceCpy(_("Database file %x is incompatible."), L"%x", fmtFileName(filename)));

        DbStreams output;

        //read stream lists
        size_t dbCount = readNumber<std::uint32_t>(streamIn); //number of streams, one for each sync-pair
        while (dbCount-- != 0)
        {
            //DB id of partner databases
            std::string sessionID = readContainer<std::string >(streamIn); //throw UnexpectedEndOfStreamError
            BinaryStream stream   = readContainer<BinaryStream>(streamIn); //

            output[sessionID] = std::move(stream);
        }
        return output;
    }
    catch (FileError&)
    {
        if (!somethingExists(filename)) //a benign(?) race condition with FileError
            throw FileErrorDatabaseNotExisting(_("Initial synchronization:") + L" \n" +
                                               replaceCpy(_("Database file %x does not yet exist."), L"%x", fmtFileName(filename)));
        throw;
    }
    catch (UnexpectedEndOfStreamError&)
    {
        throw FileError(_("Database file is corrupt:") + L"\n" + fmtFileName(filename));
    }
    catch (const std::bad_alloc& e) //still required?
    {
        throw FileError(_("Database file is corrupt:") + L"\n" + fmtFileName(filename),
                        _("Out of memory.") + L" " + utfCvrtTo<std::wstring>(e.what()));
    }
}

//#######################################################################################################################################

class StreamGenerator //for db-file back-wards compatibility we stick with two output streams until further
{
public:
    static void execute(const InSyncDir& dir, //throw FileError
                        const Zstring& filenameL, //used for diagnostics only
                        const Zstring& filenameR,
                        BinaryStream& streamL,
                        BinaryStream& streamR)
    {
        StreamGenerator generator;

        //PERF_START
        generator.recurse(dir);
        //PERF_STOP

        auto compStream = [](const BinaryStream& stream, const Zstring& filename) -> BinaryStream //throw FileError
        {
            try
            {
                /* Zlib: optimal level - testcase 1 million files
                level/size [MB]/time [ms]
                  0    49.54      272 (uncompressed)
                  1    14.53     1013
                  2    14.13     1106
                  3    13.76     1288 - best compromise between speed and compression
                  4    13.20     1526
                  5    12.73     1916
                  6    12.58     2765
                  7    12.54     3633
                  8    12.51     9032
                  9    12.50    19698 (maximal compression) */
                return compress(stream, 3); //throw ZlibInternalError
            }
            catch (ZlibInternalError&)
            {
                throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(filename)), L"zlib internal error");
            }
        };

        const BinaryStream tmpL = compStream(generator.outputLeft .get(), filenameL);
        const BinaryStream tmpR = compStream(generator.outputRight.get(), filenameR);
        const BinaryStream tmpB = compStream(generator.outputBoth .get(), filenameL + Zstr("/") + filenameR);

        BinStreamOut outL;
        BinStreamOut outR;
        //save format version
        writeNumber<std::int32_t>(outL, DB_FORMAT_STREAM);
        writeNumber<std::int32_t>(outR, DB_FORMAT_STREAM);

        //distribute "outputBoth" over left and right streams:
        writeNumber<std::int8_t>(outL, true); //this side contains first part of "outputBoth"
        writeNumber<std::int8_t>(outR, false);

        const size_t size1stPart = tmpB.size() / 2;
        const size_t size2ndPart = tmpB.size() - size1stPart;

        writeNumber<std::uint64_t>(outL, size1stPart);
        writeNumber<std::uint64_t>(outR, size2ndPart);

        writeArray(outL, &*tmpB.begin(), size1stPart);
        writeArray(outR, &*tmpB.begin() + size1stPart, size2ndPart);

        //write streams corresponding to one side only
        writeContainer<BinaryStream>(outL, tmpL);
        writeContainer<BinaryStream>(outR, tmpR);

        streamL = outL.get();
        streamR = outR.get();
    }

private:
    void recurse(const InSyncDir& container)
    {
        writeNumber<std::uint32_t>(outputBoth, static_cast<std::uint32_t>(container.files.size()));
        for (const auto& dbFile : container.files)
        {
            writeUtf8(outputBoth, dbFile.first);
            writeNumber<std::int32_t>(outputBoth, dbFile.second.cmpVar);
            writeNumber<std::uint64_t>(outputBoth, to<std::uint64_t>(dbFile.second.fileSize));

            writeFile(outputLeft,  dbFile.second.left);
            writeFile(outputRight, dbFile.second.right);
        }

        writeNumber<std::uint32_t>(outputBoth, static_cast<std::uint32_t>(container.symlinks.size()));
        for (const auto& dbSymlink : container.symlinks)
        {
            writeUtf8(outputBoth, dbSymlink.first);
            writeNumber<std::int32_t>(outputBoth, dbSymlink.second.cmpVar);

            writeLink(outputLeft,  dbSymlink.second.left);
            writeLink(outputRight, dbSymlink.second.right);
        }

        writeNumber<std::uint32_t>(outputBoth, static_cast<std::uint32_t>(container.dirs.size()));
        for (const auto& dbDir : container.dirs)
        {
            writeUtf8(outputBoth, dbDir.first);
            writeNumber<std::int32_t>(outputBoth, dbDir.second.status);

            recurse(dbDir.second);
        }
    }

    static void writeUtf8(BinStreamOut& output, const Zstring& str) { writeContainer(output, utfCvrtTo<Zbase<char>>(str)); }

    static void writeFile(BinStreamOut& output, const InSyncDescrFile& descr)
    {
        writeNumber<std:: int64_t>(output, to<std:: int64_t>(descr.lastWriteTimeRaw));
        writeNumber<std::uint64_t>(output, descr.fileId.first);
        writeNumber<std::uint64_t>(output, descr.fileId.second);
        assert_static(sizeof(descr.fileId.first ) <= sizeof(std::uint64_t));
        assert_static(sizeof(descr.fileId.second) <= sizeof(std::uint64_t));
    }

    static void writeLink(BinStreamOut& output, const InSyncDescrLink& descr)
    {
        writeNumber<std::int64_t>(output, to<std:: int64_t>(descr.lastWriteTimeRaw));
    }

    BinStreamOut outputLeft;  //data related to one side only
    BinStreamOut outputRight; //
    BinStreamOut outputBoth;  //data concerning both sides
};


class StreamParser
{
public:
    static std::shared_ptr<InSyncDir> execute(const BinaryStream& streamL, //throw FileError
                                              const BinaryStream& streamR,
                                              const Zstring& filenameL, //used for diagnostics only
                                              const Zstring& filenameR)
    {
        auto decompStream = [](const BinaryStream& stream, const Zstring& filename) -> BinaryStream //throw FileError
        {
            try
            {
                return decompress(stream); //throw ZlibInternalError
            }
            catch (ZlibInternalError&)
            {
                throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(filename)), L"zlib internal error");
            }
        };

        try
        {
            BinStreamIn inL(streamL);
            BinStreamIn inR(streamR);

            const int streamVersion = readNumber<std::int32_t>(inL); //throw UnexpectedEndOfStreamError
            warn_static("remove this case after migration:")
            bool migrateStreamFromOldFormat = streamVersion != DB_FORMAT_STREAM;
            if (migrateStreamFromOldFormat)
                inL = BinStreamIn(streamL);
            else
            {
                const int streamVersionRef = readNumber<std::int32_t>(inR); //throw UnexpectedEndOfStreamError
                if (streamVersionRef != streamVersion) //throw UnexpectedEndOfStreamError
                    throw FileError(replaceCpy(_("Database file %x is incompatible."), L"%x", fmtFileName(filenameR), L"stream format mismatch"));
                if (streamVersion != DB_FORMAT_STREAM)
                    throw FileError(replaceCpy(_("Database file %x is incompatible."), L"%x", fmtFileName(filenameL), L"stream format"));
            }

            bool has1stPartL = false;
            bool has1stPartR = false;
            if (migrateStreamFromOldFormat)
            {
                has1stPartL = readNumber<bool>(inL) != 0; //throw UnexpectedEndOfStreamError
                has1stPartR = readNumber<bool>(inR) != 0; //
            }
            else
            {
                has1stPartL = readNumber<std::int8_t>(inL) != 0; //throw UnexpectedEndOfStreamError
                has1stPartR = readNumber<std::int8_t>(inR) != 0; //
            }

            if (has1stPartL == has1stPartR)
                throw UnexpectedEndOfStreamError();

            BinStreamIn& in1stPart = has1stPartL ? inL : inR;
            BinStreamIn& in2ndPart = has1stPartL ? inR : inL;

            const size_t size1stPart = static_cast<size_t>(readNumber<std::uint64_t>(in1stPart));
            const size_t size2ndPart = static_cast<size_t>(readNumber<std::uint64_t>(in2ndPart));

            BinaryStream tmpB;
            tmpB.resize(size1stPart + size2ndPart); //throw bad_alloc
            readArray(in1stPart, &*tmpB.begin(), size1stPart);
            readArray(in2ndPart, &*tmpB.begin() + size1stPart, size2ndPart);

            const BinaryStream tmpL = readContainer<BinaryStream>(inL);
            const BinaryStream tmpR = readContainer<BinaryStream>(inR);

            auto output = std::make_shared<InSyncDir>(InSyncDir::DIR_STATUS_IN_SYNC);
            StreamParser parser(decompStream(tmpL, filenameL),
                                decompStream(tmpR, filenameR),
                                decompStream(tmpB, filenameL + Zstr("/") + filenameR), migrateStreamFromOldFormat);
            parser.recurse(*output); //throw UnexpectedEndOfStreamError
            return output;
        }
        catch (const UnexpectedEndOfStreamError&)
        {
            throw FileError(_("Database file is corrupt:") + L"\n" + fmtFileName(filenameL) + L"\n" + fmtFileName(filenameR));
        }
        catch (const std::bad_alloc& e)
        {
            throw FileError(_("Database file is corrupt:") + L"\n" + fmtFileName(filenameL) + L"\n" + fmtFileName(filenameR),
                            _("Out of memory.") + L" " + utfCvrtTo<std::wstring>(e.what()));
        }
    }

private:
    StreamParser(const BinaryStream& bufferL,
                 const BinaryStream& bufferR,
                 const BinaryStream& bufferB,
                 bool migrateStreamFromOldFormat) :
        migrateStreamFromOldFormat_(migrateStreamFromOldFormat),
        inputLeft (bufferL),
        inputRight(bufferR),
        inputBoth (bufferB) {}

    void recurse(InSyncDir& container)
    {
        size_t fileCount = readNumber<std::uint32_t>(inputBoth);
        while (fileCount-- != 0)
        {
            const Zstring shortName = readUtf8(inputBoth);

            if (migrateStreamFromOldFormat_)
            {
                const auto inSyncType  = readNumber<std::int32_t>(inputBoth);
                const CompareVariant cmpVar =  inSyncType == 0 ? CMP_BY_CONTENT : CMP_BY_TIME_SIZE;

                auto lastWriteTimeRawL = readNumber<std::int64_t>(inputLeft); //throw UnexpectedEndOfStreamError
                const UInt64 fileSize         = readNumber<std::uint64_t>(inputLeft);
                auto devIdL            = static_cast<DeviceId >(readNumber<std::uint64_t>(inputLeft)); //
                auto fileIdxL          = static_cast<FileIndex>(readNumber<std::uint64_t>(inputLeft)); //silence "loss of precision" compiler warnings
                const InSyncDescrFile dataL = InSyncDescrFile(lastWriteTimeRawL, FileId(devIdL, fileIdxL));

                auto lastWriteTimeRaw = readNumber<std::int64_t>(inputRight); //throw UnexpectedEndOfStreamError
                readNumber<std::uint64_t>(inputRight);
                auto devId            = static_cast<DeviceId >(readNumber<std::uint64_t>(inputRight)); //
                auto fileIdx          = static_cast<FileIndex>(readNumber<std::uint64_t>(inputRight)); //silence "loss of precision" compiler warnings
                const InSyncDescrFile dataR = InSyncDescrFile(lastWriteTimeRaw, FileId(devId, fileIdx));

                container.addFile(shortName, dataL, dataR, cmpVar, fileSize);
            }
            else
            {
                const auto cmpVar = static_cast<CompareVariant>(readNumber<std::int32_t>(inputBoth));
                const UInt64 fileSize = readNumber<std::uint64_t>(inputBoth);
                const InSyncDescrFile dataL = readFile(inputLeft);
                const InSyncDescrFile dataR = readFile(inputRight);
                container.addFile(shortName, dataL, dataR, cmpVar, fileSize);
            }
        }

        size_t linkCount = readNumber<std::uint32_t>(inputBoth);
        while (linkCount-- != 0)
        {
            const Zstring shortName = readUtf8(inputBoth);

            if (migrateStreamFromOldFormat_)
            {
                const CompareVariant cmpVar = CMP_BY_CONTENT;
                auto lastWriteTimeRaw = readNumber<std::int64_t>(inputLeft);
                readUtf8(inputLeft);//descr.targetPath       = readUtf8(input);
                readNumber<std::int32_t>(inputLeft);//descr.type             = static_cast<LinkDescriptor::LinkType>(readNumber<std::int32_t>(input));
                InSyncDescrLink dataL = InSyncDescrLink(lastWriteTimeRaw);

                auto lastWriteTimeRawR = readNumber<std::int64_t>(inputRight);
                readUtf8(inputRight);//descr.targetPath       = readUtf8(input);
                readNumber<std::int32_t>(inputRight);//descr.type             = static_cast<LinkDescriptor::LinkType>(readNumber<std::int32_t>(input));
                InSyncDescrLink dataR = InSyncDescrLink(lastWriteTimeRawR);

                container.addSymlink(shortName, dataL, dataR, cmpVar);
            }
            else
            {
                const auto cmpVar = static_cast<CompareVariant>(readNumber<std::int32_t>(inputBoth));
                InSyncDescrLink dataL = readLink(inputLeft);
                InSyncDescrLink dataR = readLink(inputRight);
                container.addSymlink(shortName, dataL, dataR, cmpVar);
            }
        }

        size_t dirCount = readNumber<std::uint32_t>(inputBoth);
        while (dirCount-- != 0)
        {
            const Zstring shortName = readUtf8(inputBoth);
            auto status = static_cast<InSyncDir::InSyncStatus>(readNumber<std::int32_t>(inputBoth));

            InSyncDir& subDir = container.addDir(shortName, status);
            recurse(subDir);
        }
    }

    static Zstring readUtf8(BinStreamIn& input) { return utfCvrtTo<Zstring>(readContainer<Zbase<char>>(input)); } //throw UnexpectedEndOfStreamError

    static InSyncDescrFile readFile(BinStreamIn& input)
    {
        //attention: order of function argument evaluation is undefined! So do it one after the other...
        auto lastWriteTimeRaw = readNumber<std::int64_t>(input); //throw UnexpectedEndOfStreamError
        auto devId            = static_cast<DeviceId >(readNumber<std::uint64_t>(input)); //
        auto fileIdx          = static_cast<FileIndex>(readNumber<std::uint64_t>(input)); //silence "loss of precision" compiler warnings
        return InSyncDescrFile(lastWriteTimeRaw, FileId(devId, fileIdx));
    }

    static InSyncDescrLink readLink(BinStreamIn& input)
    {
        auto lastWriteTimeRaw = readNumber<std::int64_t>(input);
        return InSyncDescrLink(lastWriteTimeRaw);
    }

    warn_static("remove after migration")
    bool migrateStreamFromOldFormat_;

    BinStreamIn inputLeft;  //data related to one side only
    BinStreamIn inputRight; //
    BinStreamIn inputBoth;  //data concerning both sides
};

//#######################################################################################################################################

class UpdateLastSynchronousState
{
    /*
    1. filter by file name does *not* create a new hierarchy, but merely gives a different *view* on the existing file hierarchy
        => only update database entries matching this view!
    2. Symlink handling *does* create a new (asymmetric) hierarchy during comparison
        => update all database entries!
    */
public:
    static void execute(const BaseDirPair& baseDirObj, InSyncDir& dir)
    {
        UpdateLastSynchronousState updater(baseDirObj.getCompVariant(), baseDirObj.getFilter());
        updater.recurse(baseDirObj, dir);
    }

private:
    UpdateLastSynchronousState(CompareVariant activeCmpVar, const HardFilter& filter) :
        filter_(filter),
        activeCmpVar_(activeCmpVar) {}

    void recurse(const HierarchyObject& hierObj, InSyncDir& dir)
    {
        process(hierObj.refSubFiles(), hierObj.getObjRelativeNamePf(), dir.files);
        process(hierObj.refSubLinks(), hierObj.getObjRelativeNamePf(), dir.symlinks);
        process(hierObj.refSubDirs (), hierObj.getObjRelativeNamePf(), dir.dirs);
    }

    template <class M, class V>
    static V& updateItem(M& map, const Zstring& key, const V& value)
    {
        auto rv = map.insert(typename M::value_type(key, value));
        if (!rv.second)
        {
#if defined ZEN_WIN || defined ZEN_MAC //caveat: key must be updated, if there is a change in short name case!!!
            if (rv.first->first != key)
            {
                map.erase(rv.first);
                return map.insert(typename M::value_type(key, value)).first->second;
            }
#endif
            rv.first->second = value;
        }
        return rv.first->second;

        //www.cplusplus.com claims that hint position for map<>::insert(iterator position, const value_type& val) changed with C++11 -> standard is unclear in [map.modifiers]
        // => let's use the more generic and potentially less performant version above!

        /*
        //efficient create or update without "default-constructible" requirement (Effective STL, item 24)

        //first check if key already exists (if yes, we're saving a value construction/destruction compared to std::map<>::insert
        auto it = map.lower_bound(key);
        if (it != map.end() && !(map.key_comp()(key, it->first)))
        {
        #if defined ZEN_WIN || defined ZEN_MAC //caveat: key might need to be updated, too, if there is a change in short name case!!!
            if (it->first != key)
            {
                map.erase(it); //don't fiddle with decrementing "it"! - you might lose while optimizing pointlessly
                return map.insert(typename M::value_type(key, value)).first->second;
            }
        #endif
            it->second = value;
            return it->second;
        }
        return map.insert(it, typename M::value_type(key, value))->second;
        */
    }

    void process(const HierarchyObject::SubFileVec& currentFiles, const Zstring& parentRelativeNamePf, InSyncDir::FileList& dbFiles)
    {
        hash_set<const InSyncFile*> toPreserve; //referencing fixed-in-memory std::map elements
        for (const FilePair& fileObj : currentFiles)
            if (!fileObj.isEmpty())
            {
                if (fileObj.getCategory() == FILE_EQUAL) //data in sync: write current state
                {
                    //Caveat: If FILE_EQUAL, we *implicitly* assume equal left and right short names matching case: InSyncDir's mapping tables use short name as a key!
                    //This makes us silently dependent from code in algorithm.h!!!
                    assert(fileObj.getShortName<LEFT_SIDE>() == fileObj.getShortName<RIGHT_SIDE>());
                    //this should be taken for granted:
                    assert(fileObj.getFileSize<LEFT_SIDE>() == fileObj.getFileSize<RIGHT_SIDE>());

                    //create or update new "in-sync" state
                    InSyncFile& file = updateItem(dbFiles, fileObj.getObjShortName(),
                                                  InSyncFile(InSyncDescrFile(fileObj.getLastWriteTime<LEFT_SIDE>(),
                                                                             fileObj.getFileId       <LEFT_SIDE>()),
                                                             InSyncDescrFile(fileObj.getLastWriteTime<RIGHT_SIDE>(),
                                                                             fileObj.getFileId       <RIGHT_SIDE>()),
                                                             activeCmpVar_,
                                                             fileObj.getFileSize<LEFT_SIDE>()));
                    toPreserve.insert(&file);
                }
                else //not in sync: preserve last synchronous state
                {
                    auto it = dbFiles.find(fileObj.getObjShortName());
                    if (it != dbFiles.end())
                        toPreserve.insert(&it->second);
                }
            }

        warn_static("consider temporarily excluded items due to traveral error just like a fixed file filter here!?")
        //delete removed items (= "in-sync") from database
        map_remove_if(dbFiles, [&](const InSyncDir::FileList::value_type& v) -> bool
        {
            if (toPreserve.find(&v.second) != toPreserve.end())
                return false;
            //all items not existing in "currentFiles" have either been deleted meanwhile or been excluded via filter:
            const Zstring& shortName = v.first;
            return filter_.passFileFilter(parentRelativeNamePf + shortName);
        });
    }

    void process(const HierarchyObject::SubLinkVec& currentLinks, const Zstring& parentRelativeNamePf, InSyncDir::LinkList& dbLinks)
    {
        hash_set<const InSyncSymlink*> toPreserve;
        for (const SymlinkPair& linkObj : currentLinks)
            if (!linkObj.isEmpty())
            {
                if (linkObj.getLinkCategory() == SYMLINK_EQUAL) //data in sync: write current state
                {
                    assert(linkObj.getShortName<LEFT_SIDE>() == linkObj.getShortName<RIGHT_SIDE>());

                    //create or update new "in-sync" state
                    InSyncSymlink& link = updateItem(dbLinks, linkObj.getObjShortName(),
                                                     InSyncSymlink(InSyncDescrLink(linkObj.getLastWriteTime<LEFT_SIDE>()),
                                                                   InSyncDescrLink(linkObj.getLastWriteTime<RIGHT_SIDE>()),
                                                                   activeCmpVar_));
                    toPreserve.insert(&link);
                }
                else //not in sync: preserve last synchronous state
                {
                    auto it = dbLinks.find(linkObj.getObjShortName());
                    if (it != dbLinks.end())
                        toPreserve.insert(&it->second);
                }
            }

        //delete removed items (= "in-sync") from database
        map_remove_if(dbLinks, [&](const InSyncDir::LinkList::value_type& v) -> bool
        {
            if (toPreserve.find(&v.second) != toPreserve.end())
                return false;
            //all items not existing in "currentLinks" have either been deleted meanwhile or been excluded via filter:
            const Zstring& shortName = v.first;
            return filter_.passFileFilter(parentRelativeNamePf + shortName);
        });
    }

    void process(const HierarchyObject::SubDirVec& currentDirs, const Zstring& parentRelativeNamePf, InSyncDir::DirList& dbDirs)
    {
        hash_set<const InSyncDir*> toPreserve;
        for (const DirPair& dirObj : currentDirs)
            if (!dirObj.isEmpty())
                switch (dirObj.getDirCategory())
                {
                    case DIR_EQUAL:
                    {
                        assert(dirObj.getShortName<LEFT_SIDE>() == dirObj.getShortName<RIGHT_SIDE>());

                        //update directory entry only (shallow), but do *not touch* exising child elements!!!
                        const Zstring& key = dirObj.getObjShortName();
                        auto insertResult = dbDirs.insert(std::make_pair(key, InSyncDir(InSyncDir::DIR_STATUS_IN_SYNC))); //get or create
                        auto it = insertResult.first;

#if defined ZEN_WIN || defined ZEN_MAC //caveat: key might need to be updated, too, if there is a change in short name case!!!
                        const bool alreadyExisting = !insertResult.second;
                        if (alreadyExisting && it->first != key)
                        {
                            auto oldValue = std::move(it->second);
                            dbDirs.erase(it); //don't fiddle with decrementing "it"! - you might lose while optimizing pointlessly
                            it = dbDirs.insert(InSyncDir::DirList::value_type(key, std::move(oldValue))).first;
                        }
#endif
                        InSyncDir& dir = it->second;
                        dir.status = InSyncDir::DIR_STATUS_IN_SYNC; //update immediate directory entry
                        toPreserve.insert(&dir);
                        recurse(dirObj, dir);
                    }
                    break;

                    case DIR_DIFFERENT_METADATA:
                        //if DIR_DIFFERENT_METADATA and no old database entry yet: we have to insert a new (bogus) database entry:
                        //we cannot simply skip the whole directory, since sub-items might be in sync!
                        //Example: directories on left and right differ in case while sub-files are equal
                    {
                        //reuse last "in-sync" if available or insert strawman entry (do not try to update thereby removing child elements!!!)
                        InSyncDir& dir = dbDirs.insert(std::make_pair(dirObj.getObjShortName(), InSyncDir(InSyncDir::DIR_STATUS_STRAW_MAN))).first->second;
                        toPreserve.insert(&dir);
                        recurse(dirObj, dir);
                    }
                    break;

                    //not in sync: reuse last synchronous state:
                    case DIR_LEFT_SIDE_ONLY:
                    case DIR_RIGHT_SIDE_ONLY:
                    {
                        auto it = dbDirs.find(dirObj.getObjShortName());
                        if (it != dbDirs.end())
                        {
                            toPreserve.insert(&it->second);
                            recurse(dirObj, it->second); //although existing sub-items cannot be in sync, items deleted on both sides *are* in-sync!!!
                        }
                    }
                    break;
                }

        //delete removed items (= "in-sync") from database
        map_remove_if(dbDirs, [&](const InSyncDir::DirList::value_type& v) -> bool
        {
            if (toPreserve.find(&v.second) != toPreserve.end())
                return false;
            const Zstring& shortName = v.first;
            return filter_.passDirFilter(parentRelativeNamePf + shortName, nullptr);
            //if directory is not included in "currentDirs", it is either not existing anymore, in which case it should be deleted from database
            //or it was excluded via filter, in which case the database entry should be preserved:
            //=> all child db elements are also preserved since they are not recursed in the loop above!!!
            //=> no problem with filter logic of excluding complete directory subtrees, if top folder is excluded directly!
        });
    }

    const HardFilter& filter_; //filter used while scanning directory: generates view on actual files!
    const CompareVariant activeCmpVar_;
};
}

//#######################################################################################################################################

std::shared_ptr<InSyncDir> zen::loadLastSynchronousState(const BaseDirPair& baseDirObj) //throw FileError, FileErrorDatabaseNotExisting -> return value always bound!
{
    const Zstring fileNameLeft  = getDBFilename<LEFT_SIDE >(baseDirObj);
    const Zstring fileNameRight = getDBFilename<RIGHT_SIDE>(baseDirObj);

    if (!baseDirObj.isExisting<LEFT_SIDE >() ||
        !baseDirObj.isExisting<RIGHT_SIDE>())
    {
        //avoid race condition with directory existence check: reading sync.ffs_db may succeed although first dir check had failed => conflicts!
        //https://sourceforge.net/tracker/?func=detail&atid=1093080&aid=3531351&group_id=234430
        const Zstring filename = !baseDirObj.isExisting<LEFT_SIDE>() ? fileNameLeft : fileNameRight;
        throw FileErrorDatabaseNotExisting(_("Initial synchronization:") + L" \n" + //it could be due to a to-be-created target directory not yet existing => FileErrorDatabaseNotExisting
                                           replaceCpy(_("Database file %x does not yet exist."), L"%x", fmtFileName(filename)));
    }

    //read file data: list of session ID + DirInfo-stream
    const DbStreams streamsLeft  = ::loadStreams(fileNameLeft);  //throw FileError, FileErrorDatabaseNotExisting
    const DbStreams streamsRight = ::loadStreams(fileNameRight); //

    //find associated session: there can be at most one session within intersection of left and right ids
    for (const auto& streamLeft : streamsLeft)
    {
        auto itRight = streamsRight.find(streamLeft.first);
        if (itRight != streamsRight.end())
        {
            return StreamParser::execute(streamLeft.second, //throw FileError
                                         itRight->second,
                                         fileNameLeft,
                                         fileNameRight);
        }
    }
    throw FileErrorDatabaseNotExisting(_("Initial synchronization:") + L" \n" +
                                       _("Database files do not share a common session."));
}


void zen::saveLastSynchronousState(const BaseDirPair& baseDirObj) //throw FileError
{
    //transactional behaviour! write to tmp files first
    const Zstring dbNameLeftTmp  = getDBFilename<LEFT_SIDE >(baseDirObj, true);
    const Zstring dbNameRightTmp = getDBFilename<RIGHT_SIDE>(baseDirObj, true);

    const Zstring dbNameLeft  = getDBFilename<LEFT_SIDE >(baseDirObj);
    const Zstring dbNameRight = getDBFilename<RIGHT_SIDE>(baseDirObj);

    //delete old tmp file, if necessary -> throws if deletion fails!
    removeFile(dbNameLeftTmp);  //
    removeFile(dbNameRightTmp); //throw FileError

    //(try to) load old database files...
    DbStreams streamsLeft;  //list of session ID + DirInfo-stream
    DbStreams streamsRight;

    try { streamsLeft  = ::loadStreams(dbNameLeft ); }
    catch (FileError&) {}
    try { streamsRight = ::loadStreams(dbNameRight); }
    catch (FileError&) {}
    //if error occurs: just overwrite old file! User is already informed about issues right after comparing!

    //find associated session: there can be at most one session within intersection of left and right ids
    auto itStreamLeftOld  = streamsLeft .cend();
    auto itStreamRightOld = streamsRight.cend();
    for (auto iterLeft = streamsLeft.begin(); iterLeft != streamsLeft.end(); ++iterLeft)
    {
        auto iterRight = streamsRight.find(iterLeft->first);
        if (iterRight != streamsRight.end())
        {
            itStreamLeftOld  = iterLeft;
            itStreamRightOld = iterRight;
            break;
        }
    }

    //load last synchrounous state
    std::shared_ptr<InSyncDir> lastSyncState = std::make_shared<InSyncDir>(InSyncDir::DIR_STATUS_IN_SYNC);
    if (itStreamLeftOld  != streamsLeft .end() &&
        itStreamRightOld != streamsRight.end())
        try
        {
            lastSyncState = StreamParser::execute(itStreamLeftOld ->second, //throw FileError
                                                  itStreamRightOld->second,
                                                  dbNameLeft,
                                                  dbNameRight);
        }
        catch (FileError&) {} //if error occurs: just overwrite old file! User is already informed about issues right after comparing!

    //update last synchrounous state
    UpdateLastSynchronousState::execute(baseDirObj, *lastSyncState);

    //serialize again
    BinaryStream updatedStreamLeft;
    BinaryStream updatedStreamRight;
    StreamGenerator::execute(*lastSyncState,
                             dbNameLeft,
                             dbNameRight,
                             updatedStreamLeft,
                             updatedStreamRight); //throw FileError

    //check if there is some work to do at all
    if (itStreamLeftOld  != streamsLeft .end() && updatedStreamLeft  == itStreamLeftOld ->second &&
        itStreamRightOld != streamsRight.end() && updatedStreamRight == itStreamRightOld->second)
        return; //some users monitor the *.ffs_db file with RTS => don't touch the file if it isnt't strictly needed

    //erase old session data
    if (itStreamLeftOld != streamsLeft.end())
        streamsLeft.erase(itStreamLeftOld);
    if (itStreamRightOld != streamsRight.end())
        streamsRight.erase(itStreamRightOld);

    //create new session data
    const std::string sessionID = zen::generateGUID();

    streamsLeft [sessionID] = std::move(updatedStreamLeft);
    streamsRight[sessionID] = std::move(updatedStreamRight);

    //write (temp-) files...
    zen::ScopeGuard guardTempFileLeft = zen::makeGuard([&] {zen::removeFile(dbNameLeftTmp); });
    saveStreams(streamsLeft, dbNameLeftTmp);  //throw FileError

    zen::ScopeGuard guardTempFileRight = zen::makeGuard([&] {zen::removeFile(dbNameRightTmp); });
    saveStreams(streamsRight, dbNameRightTmp); //throw FileError

    //operation finished: rename temp files -> this should work transactionally:
    //if there were no write access, creation of temp files would have failed
    removeFile(dbNameLeft);                  //
    removeFile(dbNameRight);                 //throw FileError
    renameFile(dbNameLeftTmp,  dbNameLeft);  //
    renameFile(dbNameRightTmp, dbNameRight); //

    guardTempFileLeft. dismiss(); //no need to delete temp files anymore
    guardTempFileRight.dismiss(); //
}
