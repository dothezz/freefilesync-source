// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "db_file.h"
#include <zen/guid.h>
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
const int DB_FORMAT_STREAM    = 2; //since 2015-05-02
//-------------------------------------------------------------------------------------------------------------------------------

typedef std::string UniqueId;
typedef std::map<UniqueId, ByteArray> DbStreams; //list of streams ordered by session UUID

using MemStreamOut = MemoryStreamOut<ByteArray>;
using MemStreamIn  = MemoryStreamIn <ByteArray>;

//-----------------------------------------------------------------------------------
//| ensure 32/64 bit portability: use fixed size data types only e.g. std::uint32_t |
//-----------------------------------------------------------------------------------

template <SelectedSide side> inline
AbstractPathRef getDatabaseFilePath(const BaseDirPair& baseDirObj, bool tempfile = false)
{
    //Linux and Windows builds are binary incompatible: different file id?, problem with case sensitivity? are UTC file times really compatible?
    //what about endianess!?
    //however 32 and 64 bit db files *are* designed to be binary compatible!
    //Give db files different names.
    //make sure they end with ".ffs_db". These files will be excluded from comparison
#ifdef ZEN_WIN
    const Zstring dbname = Zstr("sync");
#elif defined ZEN_LINUX || defined ZEN_MAC
    const Zstring dbname = Zstr(".sync"); //files beginning with dots are hidden e.g. in Nautilus
#endif
    const Zstring dbFileName = dbname + (tempfile ? Zstr(".tmp") : Zstr("")) + SYNC_DB_FILE_ENDING;

    return baseDirObj.getABF<side>().getAbstractPath(dbFileName);
}

//#######################################################################################################################################

void saveStreams(const DbStreams& streamList, const AbstractPathRef& dbPath, const std::function<void(std::int64_t bytesDelta)>& onUpdateStatus) //throw FileError
{
    //perf? instead of writing to a file stream directly, collect data into memory first, then write to file block-wise
    MemStreamOut memStreamOut;

    //write FreeFileSync file identifier
    writeArray(memStreamOut, FILE_FORMAT_DESCR, sizeof(FILE_FORMAT_DESCR));

    //save file format version
    writeNumber<std::int32_t>(memStreamOut, DB_FORMAT_CONTAINER);

    //save stream list
    writeNumber<std::uint32_t>(memStreamOut, static_cast<std::uint32_t>(streamList.size())); //number of streams, one for each sync-pair

    for (const auto& stream : streamList)
    {
        writeContainer<std::string>(memStreamOut, stream.first );
        writeContainer<ByteArray>  (memStreamOut, stream.second);
    }

    assert(!ABF::somethingExists(dbPath)); //orphan tmp files should have been cleaned up at this point!

    //save memory stream to file (as a transaction!)
    {
        MemoryStreamIn<ByteArray> memStreamIn(memStreamOut.ref());
        const std::uint64_t streamSize = memStreamOut.ref().size();
        const std::unique_ptr<ABF::OutputStream> fileStreamOut = ABF::getOutputStream(dbPath, &streamSize, nullptr /*modificationTime*/); //throw FileError, ErrorTargetExisting
        if (onUpdateStatus) onUpdateStatus(0);
        copyStream(memStreamIn, *fileStreamOut, fileStreamOut->optimalBlockSize(), onUpdateStatus); //throw FileError
        fileStreamOut->finalize([&] { if (onUpdateStatus) onUpdateStatus(0); }); //throw FileError
        //commit and close stream
    }

#ifdef ZEN_WIN
    if (Opt<Zstring> nativeFilePath = ABF::getNativeItemPath(dbPath))
        ::SetFileAttributes(applyLongPathPrefix(*nativeFilePath).c_str(), FILE_ATTRIBUTE_HIDDEN); //(try to) hide database file
#endif
}


DbStreams loadStreams(const AbstractPathRef& dbPath, const std::function<void(std::int64_t bytesDelta)>& onUpdateStatus) //throw FileError, FileErrorDatabaseNotExisting
{
    try
    {
        //load memory stream from file
        MemoryStreamOut<ByteArray> memStreamOut;
        {
            const std::unique_ptr<ABF::InputStream> fileStreamIn = ABF::getInputStream(dbPath); //throw FileError, ErrorFileLocked
            if (onUpdateStatus) onUpdateStatus(0);
            copyStream(*fileStreamIn, memStreamOut, fileStreamIn->optimalBlockSize(), onUpdateStatus); //throw FileError
        } //close file handle

        MemStreamIn streamIn(memStreamOut.ref());

        //read FreeFileSync file identifier
        char formatDescr[sizeof(FILE_FORMAT_DESCR)] = {};
        readArray(streamIn, formatDescr, sizeof(formatDescr)); //throw UnexpectedEndOfStreamError

        if (!std::equal(FILE_FORMAT_DESCR, FILE_FORMAT_DESCR + sizeof(FILE_FORMAT_DESCR), formatDescr))
            throw FileError(replaceCpy(_("Database file %x is incompatible."), L"%x", fmtPath(ABF::getDisplayPath(dbPath))));

        const int version = readNumber<std::int32_t>(streamIn); //throw UnexpectedEndOfStreamError
        if (version != DB_FORMAT_CONTAINER) //read file format version number
            throw FileError(replaceCpy(_("Database file %x is incompatible."), L"%x", fmtPath(ABF::getDisplayPath(dbPath))));

        DbStreams output;

        //read stream lists
        size_t dbCount = readNumber<std::uint32_t>(streamIn); //number of streams, one for each sync-pair
        while (dbCount-- != 0)
        {
            //DB id of partner databases
            std::string sessionID = readContainer<std::string>(streamIn); //throw UnexpectedEndOfStreamError
            ByteArray stream      = readContainer<ByteArray>  (streamIn); //

            output[sessionID] = std::move(stream);
        }
        return output;
    }
    catch (FileError&)
    {
        if (!ABF::somethingExists(dbPath)) //a benign(?) race condition with FileError
            throw FileErrorDatabaseNotExisting(_("Initial synchronization:") + L" \n" +
                                               replaceCpy(_("Database file %x does not yet exist."), L"%x", fmtPath(ABF::getDisplayPath(dbPath))));
        throw;
    }
    catch (UnexpectedEndOfStreamError&)
    {
        throw FileError(_("Database file is corrupt:") + L"\n" + fmtPath(ABF::getDisplayPath(dbPath)));
    }
    catch (const std::bad_alloc& e) //still required?
    {
        throw FileError(_("Database file is corrupt:") + L"\n" + fmtPath(ABF::getDisplayPath(dbPath)),
                        _("Out of memory.") + L" " + utfCvrtTo<std::wstring>(e.what()));
    }
}

//#######################################################################################################################################

class StreamGenerator //for db-file back-wards compatibility we stick with two output streams until further
{
public:
    static void execute(const InSyncDir& dir, //throw FileError
                        const std::wstring& displayFilePathL, //used for diagnostics only
                        const std::wstring& displayFilePathR,
                        ByteArray& streamL,
                        ByteArray& streamR)
    {
        StreamGenerator generator;

        //PERF_START
        generator.recurse(dir);
        //PERF_STOP

        auto compStream = [](const ByteArray& stream, const std::wstring& displayFilePath) -> ByteArray //throw FileError
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
                throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtPath(displayFilePath)), L"zlib internal error");
            }
        };

        const ByteArray tmpL = compStream(generator.outputLeft .ref(), displayFilePathL);
        const ByteArray tmpR = compStream(generator.outputRight.ref(), displayFilePathR);
        const ByteArray tmpB = compStream(generator.outputBoth .ref(), displayFilePathL + L"/" + displayFilePathR);

        MemStreamOut outL;
        MemStreamOut outR;
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
        writeContainer<ByteArray>(outL, tmpL);
        writeContainer<ByteArray>(outR, tmpR);

        streamL = outL.ref();
        streamR = outR.ref();
    }

private:
    void recurse(const InSyncDir& container)
    {
        writeNumber<std::uint32_t>(outputBoth, static_cast<std::uint32_t>(container.files.size()));
        for (const auto& dbFile : container.files)
        {
            writeUtf8(outputBoth, dbFile.first);
            writeNumber<std::int32_t >(outputBoth, dbFile.second.cmpVar);
            writeNumber<std::uint64_t>(outputBoth, dbFile.second.fileSize);

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

    static void writeUtf8(MemStreamOut& output, const Zstring& str) { writeContainer(output, utfCvrtTo<Zbase<char>>(str)); }

    static void writeFile(MemStreamOut& output, const InSyncDescrFile& descr)
    {
        writeNumber<std:: int64_t>(output, descr.lastWriteTimeRaw);
        writeContainer(output, descr.fileId);
        static_assert(IsSameType<decltype(descr.fileId), Zbase<char>>::value, "");
    }

    static void writeLink(MemStreamOut& output, const InSyncDescrLink& descr)
    {
        writeNumber<std::int64_t>(output, descr.lastWriteTimeRaw);
    }

    MemStreamOut outputLeft;  //data related to one side only
    MemStreamOut outputRight; //
    MemStreamOut outputBoth;  //data concerning both sides
};


class StreamParser
{
public:
    static std::shared_ptr<InSyncDir> execute(const ByteArray& streamL, //throw FileError
                                              const ByteArray& streamR,
                                              const std::wstring& displayFilePathL, //used for diagnostics only
                                              const std::wstring& displayFilePathR)
    {
        auto decompStream = [](const ByteArray& stream, const std::wstring& displayFilePath) -> ByteArray //throw FileError
        {
            try
            {
                return decompress(stream); //throw ZlibInternalError
            }
            catch (ZlibInternalError&)
            {
                throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtPath(displayFilePath)), L"zlib internal error");
            }
        };

        try
        {
            MemStreamIn inL(streamL);
            MemStreamIn inR(streamR);

            const int streamVersionL = readNumber<std::int32_t>(inL); //throw UnexpectedEndOfStreamError
            const int streamVersionR = readNumber<std::int32_t>(inR); //

            if (streamVersionL != streamVersionR)
                throw FileError(_("Database file is corrupt:") + L"\n" + fmtPath(displayFilePathL) + L"\n" + fmtPath(displayFilePathR), L"different stream formats");

            warn_static("remove check for stream version 1 after migration! 2015-05-02")
            if (streamVersionL != 1 &&
                streamVersionL != DB_FORMAT_STREAM)
                throw FileError(replaceCpy(_("Database file %x is incompatible."), L"%x", fmtPath(displayFilePathL)), L"unknown stream format");

            const bool has1stPartL = readNumber<std::int8_t>(inL) != 0; //throw UnexpectedEndOfStreamError
            const bool has1stPartR = readNumber<std::int8_t>(inR) != 0; //

            if (has1stPartL == has1stPartR)
                throw FileError(_("Database file is corrupt:") + L"\n" + fmtPath(displayFilePathL) + L"\n" + fmtPath(displayFilePathR), L"second part missing");

            MemStreamIn& in1stPart = has1stPartL ? inL : inR;
            MemStreamIn& in2ndPart = has1stPartL ? inR : inL;

            const size_t size1stPart = static_cast<size_t>(readNumber<std::uint64_t>(in1stPart));
            const size_t size2ndPart = static_cast<size_t>(readNumber<std::uint64_t>(in2ndPart));

            ByteArray tmpB;
            tmpB.resize(size1stPart + size2ndPart); //throw bad_alloc
            readArray(in1stPart, &*tmpB.begin(),               size1stPart); //stream always non-empty
            readArray(in2ndPart, &*tmpB.begin() + size1stPart, size2ndPart); //

            const ByteArray tmpL = readContainer<ByteArray>(inL);
            const ByteArray tmpR = readContainer<ByteArray>(inR);

            auto output = std::make_shared<InSyncDir>(InSyncDir::DIR_STATUS_IN_SYNC);
            StreamParser parser(streamVersionL,
                                decompStream(tmpL, displayFilePathL),
                                decompStream(tmpR, displayFilePathR),
                                decompStream(tmpB, displayFilePathL + L"/" + displayFilePathR));
            parser.recurse(*output); //throw UnexpectedEndOfStreamError
            return output;
        }
        catch (const UnexpectedEndOfStreamError&)
        {
            throw FileError(_("Database file is corrupt:") + L"\n" + fmtPath(displayFilePathL) + L"\n" + fmtPath(displayFilePathR));
        }
        catch (const std::bad_alloc& e)
        {
            throw FileError(_("Database file is corrupt:") + L"\n" + fmtPath(displayFilePathL) + L"\n" + fmtPath(displayFilePathR),
                            _("Out of memory.") + L" " + utfCvrtTo<std::wstring>(e.what()));
        }
    }

private:
    StreamParser(int streamVersion,
                 const ByteArray& bufferL,
                 const ByteArray& bufferR,
                 const ByteArray& bufferB) :
        streamVersion_(streamVersion),
        inputLeft (bufferL),
        inputRight(bufferR),
        inputBoth (bufferB) {}

    void recurse(InSyncDir& container)
    {
        size_t fileCount = readNumber<std::uint32_t>(inputBoth);
        while (fileCount-- != 0)
        {
            const Zstring shortName = readUtf8(inputBoth);
            const auto cmpVar = static_cast<CompareVariant>(readNumber<std::int32_t>(inputBoth));
            const std::uint64_t fileSize = readNumber<std::uint64_t>(inputBoth);
            const InSyncDescrFile dataL = readFile(inputLeft);
            const InSyncDescrFile dataR = readFile(inputRight);
            container.addFile(shortName, dataL, dataR, cmpVar, fileSize);
        }

        size_t linkCount = readNumber<std::uint32_t>(inputBoth);
        while (linkCount-- != 0)
        {
            const Zstring shortName = readUtf8(inputBoth);
            const auto cmpVar = static_cast<CompareVariant>(readNumber<std::int32_t>(inputBoth));
            InSyncDescrLink dataL = readLink(inputLeft);
            InSyncDescrLink dataR = readLink(inputRight);
            container.addSymlink(shortName, dataL, dataR, cmpVar);
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

    static Zstring readUtf8(MemStreamIn& input) { return utfCvrtTo<Zstring>(readContainer<Zbase<char>>(input)); } //throw UnexpectedEndOfStreamError

    InSyncDescrFile readFile(MemStreamIn& input) const
    {
        //attention: order of function argument evaluation is undefined! So do it one after the other...
        auto lastWriteTimeRaw = readNumber<std::int64_t>(input); //throw UnexpectedEndOfStreamError

        ABF::FileId fileId;
        warn_static("remove after migration! 2015-05-02")
        if (streamVersion_ == 1)
        {
            auto devId   = static_cast<DeviceId >(readNumber<std::uint64_t>(input)); //
            auto fileIdx = static_cast<FileIndex>(readNumber<std::uint64_t>(input)); //silence "loss of precision" compiler warnings
            if (devId != 0 || fileIdx != 0)
            {
                fileId.append(reinterpret_cast<const char*>(&devId), sizeof(devId));
                fileId.append(reinterpret_cast<const char*>(&fileIdx), sizeof(fileIdx));
            }
        }
        else

            fileId = readContainer<Zbase<char>>(input);

        return InSyncDescrFile(lastWriteTimeRaw, fileId);
    }

    static InSyncDescrLink readLink(MemStreamIn& input)
    {
        auto lastWriteTimeRaw = readNumber<std::int64_t>(input);
        return InSyncDescrLink(lastWriteTimeRaw);
    }

    const int streamVersion_;
    MemStreamIn inputLeft;  //data related to one side only
    MemStreamIn inputRight; //
    MemStreamIn inputBoth;  //data concerning both sides
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
        process(hierObj.refSubFiles(), hierObj.getPairRelativePathPf(), dir.files);
        process(hierObj.refSubLinks(), hierObj.getPairRelativePathPf(), dir.symlinks);
        process(hierObj.refSubDirs (), hierObj.getPairRelativePathPf(), dir.dirs);
    }

    template <class M, class V>
    static V& updateItem(M& map, const Zstring& key, const V& value)
    {
        auto rv = map.emplace(key, value);
        if (!rv.second)
        {
#if defined ZEN_WIN || defined ZEN_MAC //caveat: key must be updated, if there is a change in short name case!!!
            if (rv.first->first != key)
            {
                map.erase(rv.first);
                return map.emplace(key, value).first->second;
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
                return map.emplace(key, value).first->second;
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
        std::unordered_set<const InSyncFile*> toPreserve; //referencing fixed-in-memory std::map elements

        for (const FilePair& fileObj : currentFiles)
            if (!fileObj.isEmpty())
            {
                if (fileObj.getCategory() == FILE_EQUAL) //data in sync: write current state
                {
                    //Caveat: If FILE_EQUAL, we *implicitly* assume equal left and right short names matching case: InSyncDir's mapping tables use short name as a key!
                    //This makes us silently dependent from code in algorithm.h!!!
                    assert(fileObj.getItemName<LEFT_SIDE>() == fileObj.getItemName<RIGHT_SIDE>());
                    //this should be taken for granted:
                    assert(fileObj.getFileSize<LEFT_SIDE>() == fileObj.getFileSize<RIGHT_SIDE>());

                    //create or update new "in-sync" state
                    InSyncFile& file = updateItem(dbFiles, fileObj.getPairShortName(),
                                                  InSyncFile(InSyncDescrFile(fileObj.getLastWriteTime<LEFT_SIDE >(),
                                                                             fileObj.getFileId       <LEFT_SIDE >()),
                                                             InSyncDescrFile(fileObj.getLastWriteTime<RIGHT_SIDE>(),
                                                                             fileObj.getFileId       <RIGHT_SIDE>()),
                                                             activeCmpVar_,
                                                             fileObj.getFileSize<LEFT_SIDE>()));
                    toPreserve.insert(&file);
                }
                else //not in sync: preserve last synchronous state
                {
                    auto it = dbFiles.find(fileObj.getPairShortName());
                    if (it != dbFiles.end())
                        toPreserve.insert(&it->second);
                }
            }

        //delete removed items (= "in-sync") from database
        map_remove_if(dbFiles, [&](const InSyncDir::FileList::value_type& v) -> bool
        {
            if (toPreserve.find(&v.second) != toPreserve.end())
                return false;
            //all items not existing in "currentFiles" have either been deleted meanwhile or been excluded via filter:
            const Zstring& shortName = v.first;
            return filter_.passFileFilter(parentRelativeNamePf + shortName);
            //note: items subject to traveral errors are also excluded by this file filter here! see comparison.cpp, modified file filter for read errors
        });
    }

    void process(const HierarchyObject::SubLinkVec& currentLinks, const Zstring& parentRelativeNamePf, InSyncDir::LinkList& dbLinks)
    {
        std::unordered_set<const InSyncSymlink*> toPreserve;

        for (const SymlinkPair& linkObj : currentLinks)
            if (!linkObj.isEmpty())
            {
                if (linkObj.getLinkCategory() == SYMLINK_EQUAL) //data in sync: write current state
                {
                    assert(linkObj.getItemName<LEFT_SIDE>() == linkObj.getItemName<RIGHT_SIDE>());

                    //create or update new "in-sync" state
                    InSyncSymlink& link = updateItem(dbLinks, linkObj.getPairShortName(),
                                                     InSyncSymlink(InSyncDescrLink(linkObj.getLastWriteTime<LEFT_SIDE>()),
                                                                   InSyncDescrLink(linkObj.getLastWriteTime<RIGHT_SIDE>()),
                                                                   activeCmpVar_));
                    toPreserve.insert(&link);
                }
                else //not in sync: preserve last synchronous state
                {
                    auto it = dbLinks.find(linkObj.getPairShortName());
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
        std::unordered_set<const InSyncDir*> toPreserve;

        for (const DirPair& dirObj : currentDirs)
            if (!dirObj.isEmpty())
                switch (dirObj.getDirCategory())
                {
                    case DIR_EQUAL:
                    {
                        assert(dirObj.getItemName<LEFT_SIDE>() == dirObj.getItemName<RIGHT_SIDE>());

                        //update directory entry only (shallow), but do *not touch* exising child elements!!!
                        const Zstring& key = dirObj.getPairShortName();
                        auto insertResult = dbDirs.emplace(key, InSyncDir(InSyncDir::DIR_STATUS_IN_SYNC)); //get or create
                        auto it = insertResult.first;

#if defined ZEN_WIN || defined ZEN_MAC //caveat: key might need to be updated, too, if there is a change in short name case!!!
                        const bool alreadyExisting = !insertResult.second;
                        if (alreadyExisting && it->first != key)
                        {
                            auto oldValue = std::move(it->second);
                            dbDirs.erase(it); //don't fiddle with decrementing "it"! - you might lose while optimizing pointlessly
                            it = dbDirs.emplace(key, std::move(oldValue)).first;
                        }
#endif
                        InSyncDir& dir = it->second;
                        dir.status = InSyncDir::DIR_STATUS_IN_SYNC; //update immediate directory entry
                        toPreserve.insert(&dir);
                        recurse(dirObj, dir);
                    }
                    break;

                    case DIR_CONFLICT:
                    case DIR_DIFFERENT_METADATA:
                        //if DIR_DIFFERENT_METADATA and no old database entry yet: we have to insert a placeholder database entry:
                        //we cannot simply skip the whole directory, since sub-items might be in sync!
                        //Example: directories on left and right differ in case while sub-files are equal
                    {
                        //reuse last "in-sync" if available or insert strawman entry (do not try to update and thereby remove child elements!!!)
                        InSyncDir& dir = dbDirs.emplace(dirObj.getPairShortName(), InSyncDir(InSyncDir::DIR_STATUS_STRAW_MAN)).first->second;
                        toPreserve.insert(&dir);
                        recurse(dirObj, dir); //unconditional recursion without filter check! => no problem since "subObjMightMatch" is optional!!!
                    }
                    break;

                    //not in sync: reuse last synchronous state:
                    case DIR_LEFT_SIDE_ONLY:
                    case DIR_RIGHT_SIDE_ONLY:
                    {
                        auto it = dbDirs.find(dirObj.getPairShortName());
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
            //or it was excluded via filter and the database entry should be preserved

            warn_static("insufficient for *.txt-include filters! -> e.g. 1. *.txt-include, both sides in sync, txt-fiels in subfolder")
            warn_static("2. delete all subfolders externally ")
            warn_static("3. sync => db should be updated == entries removed for .txt; mabye even for deleted subfolders!?!")
        });
    }

    const HardFilter& filter_; //filter used while scanning directory: generates view on actual files!
    const CompareVariant activeCmpVar_;
};
}

//#######################################################################################################################################

std::shared_ptr<InSyncDir> zen::loadLastSynchronousState(const BaseDirPair& baseDirObj, //throw FileError, FileErrorDatabaseNotExisting -> return value always bound!
                                                         const std::function<void(std::int64_t bytesDelta)>& onUpdateStatus)
{
    const AbstractPathRef dbPathLeft  = getDatabaseFilePath<LEFT_SIDE >(baseDirObj);
    const AbstractPathRef dbPathRight = getDatabaseFilePath<RIGHT_SIDE>(baseDirObj);

    if (!baseDirObj.isExisting<LEFT_SIDE >() ||
        !baseDirObj.isExisting<RIGHT_SIDE>())
    {
        //avoid race condition with directory existence check: reading sync.ffs_db may succeed although first dir check had failed => conflicts!
        //https://sourceforge.net/tracker/?func=detail&atid=1093080&aid=3531351&group_id=234430
        const AbstractPathRef filePath = !baseDirObj.isExisting<LEFT_SIDE>() ? dbPathLeft : dbPathRight;
        throw FileErrorDatabaseNotExisting(_("Initial synchronization:") + L" \n" + //it could be due to a to-be-created target directory not yet existing => FileErrorDatabaseNotExisting
                                           replaceCpy(_("Database file %x does not yet exist."), L"%x", fmtPath(ABF::getDisplayPath(filePath))));
    }

    //read file data: list of session ID + DirInfo-stream
    const DbStreams streamsLeft  = ::loadStreams(dbPathLeft,  onUpdateStatus); //throw FileError, FileErrorDatabaseNotExisting
    const DbStreams streamsRight = ::loadStreams(dbPathRight, onUpdateStatus); //

    //find associated session: there can be at most one session within intersection of left and right ids
    for (const auto& streamLeft : streamsLeft)
    {
        auto itRight = streamsRight.find(streamLeft.first);
        if (itRight != streamsRight.end())
        {
            return StreamParser::execute(streamLeft.second, //throw FileError
                                         itRight->second,
                                         ABF::getDisplayPath(dbPathLeft),
                                         ABF::getDisplayPath(dbPathRight));
        }
    }
    throw FileErrorDatabaseNotExisting(_("Initial synchronization:") + L" \n" +
                                       _("Database files do not share a common session."));
}


void zen::saveLastSynchronousState(const BaseDirPair& baseDirObj, const std::function<void(std::int64_t bytesDelta)>& onUpdateStatus) //throw FileError
{
    //transactional behaviour! write to tmp files first
    const AbstractPathRef dbPathLeft  = getDatabaseFilePath<LEFT_SIDE >(baseDirObj);
    const AbstractPathRef dbPathRight = getDatabaseFilePath<RIGHT_SIDE>(baseDirObj);

    const AbstractPathRef dbPathLeftTmp  = getDatabaseFilePath<LEFT_SIDE >(baseDirObj, true);
    const AbstractPathRef dbPathRightTmp = getDatabaseFilePath<RIGHT_SIDE>(baseDirObj, true);

    //delete old tmp file, if necessary -> throws if deletion fails!
    ABF::removeFile(dbPathLeftTmp);  //
    ABF::removeFile(dbPathRightTmp); //throw FileError

    //(try to) load old database files...
    DbStreams streamsLeft;  //list of session ID + DirInfo-stream
    DbStreams streamsRight;

    //std::function<void(std::int64_t bytesDelta)> onUpdateLoadStatus;
    //if (onUpdateStatus)
    //    onUpdateLoadStatus = [&](std::int64_t bytesDelta) { onUpdateStatus(0); };

    try { streamsLeft  = ::loadStreams(dbPathLeft, onUpdateStatus); }
    catch (FileError&) {}
    try { streamsRight = ::loadStreams(dbPathRight, onUpdateStatus); }
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
                                                  ABF::getDisplayPath(dbPathLeft),
                                                  ABF::getDisplayPath(dbPathRight));
        }
        catch (FileError&) {} //if error occurs: just overwrite old file! User is already informed about issues right after comparing!

    //update last synchrounous state
    UpdateLastSynchronousState::execute(baseDirObj, *lastSyncState);

    //serialize again
    ByteArray updatedStreamLeft;
    ByteArray updatedStreamRight;
    StreamGenerator::execute(*lastSyncState, //throw FileError
                             ABF::getDisplayPath(dbPathLeft),
                             ABF::getDisplayPath(dbPathRight),
                             updatedStreamLeft,
                             updatedStreamRight);

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

    //write (temp-) files as a transaction
    saveStreams(streamsLeft,  dbPathLeftTmp,  onUpdateStatus); //throw FileError
    saveStreams(streamsRight, dbPathRightTmp, onUpdateStatus); //

    //operation finished: rename temp files -> this should work transactionally:
    //if there were no write access, creation of temp files would have failed
    ABF::removeFile(dbPathLeft);                  //throw FileError
    ABF::renameItem(dbPathLeftTmp, dbPathLeft);   //throw FileError, (ErrorTargetExisting, ErrorDifferentVolume)

    ABF::removeFile(dbPathRight);                 //
    ABF::renameItem(dbPathRightTmp, dbPathRight); //
}
