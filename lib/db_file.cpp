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

#ifdef FFS_WIN
#include <zen/win.h> //includes "windows.h"
#include <zen/long_path_prefix.h>
#endif

using namespace zen;


namespace
{
//-------------------------------------------------------------------------------------------------------------------------------
const char FILE_FORMAT_DESCR[] = "FreeFileSync";
const int FILE_FORMAT_VER = 9;
//-------------------------------------------------------------------------------------------------------------------------------

typedef std::string UniqueId;
typedef std::map<UniqueId, BinaryStream> StreamMapping; //list of streams ordered by session UUID

//-----------------------------------------------------------------------------------
//| ensure 32/64 bit portability: use fixed size data types only e.g. std::uint32_t |
//-----------------------------------------------------------------------------------


template <SelectedSide side> inline
Zstring getDBFilename(const BaseDirMapping& baseMap, bool tempfile = false)
{
    //Linux and Windows builds are binary incompatible: different file id?, problem with case sensitivity?
    //however 32 and 64 bit db files *are* designed to be binary compatible!
    //Give db files different names.
    //make sure they end with ".ffs_db". These files will be excluded from comparison
#ifdef FFS_WIN
    Zstring dbname = Zstring(Zstr("sync")) + (tempfile ? Zstr(".tmp") : Zstr("")) + SYNC_DB_FILE_ENDING;
#elif defined FFS_LINUX
    //files beginning with dots are hidden e.g. in Nautilus
    Zstring dbname = Zstring(Zstr(".sync")) + (tempfile ? Zstr(".tmp") : Zstr("")) + SYNC_DB_FILE_ENDING;
#endif

    return baseMap.getBaseDirPf<side>() + dbname;
}

//#######################################################################################################################################

//save/load streams
void saveStreams(const StreamMapping& streamList, const Zstring& filename) //throw FileError
{
    BinStreamOut streamOut;

    //write FreeFileSync file identifier
    writeArray(streamOut, FILE_FORMAT_DESCR, sizeof(FILE_FORMAT_DESCR));

    //save file format version
    writeNumber<std::int32_t>(streamOut, FILE_FORMAT_VER);

    //save stream list
    writeNumber<std::uint32_t>(streamOut, static_cast<std::uint32_t>(streamList.size())); //number of streams, one for each sync-pair

    for (auto iter = streamList.begin(); iter != streamList.end(); ++iter)
    {
        writeContainer<std::string >(streamOut, iter->first );
        writeContainer<BinaryStream>(streamOut, iter->second);
    }

    saveBinStream(filename, streamOut.get()); //throw FileError

#ifdef FFS_WIN
    ::SetFileAttributes(applyLongPathPrefix(filename).c_str(), FILE_ATTRIBUTE_HIDDEN); //(try to) hide database file
#endif
}


StreamMapping loadStreams(const Zstring& filename) //throw FileError, FileErrorDatabaseNotExisting
{
    try
    {
        BinStreamIn streamIn = loadBinStream<BinaryStream>(filename); //throw FileError, ErrorNotExisting

        //read FreeFileSync file identifier
        char formatDescr[sizeof(FILE_FORMAT_DESCR)] = {};
        readArray(streamIn, formatDescr, sizeof(formatDescr)); //throw UnexpectedEndOfStreamError

        if (!std::equal(FILE_FORMAT_DESCR, FILE_FORMAT_DESCR + sizeof(FILE_FORMAT_DESCR), formatDescr))
            throw FileError(replaceCpy(_("Database file %x is incompatible."), L"%x", fmtFileName(filename)));

        const int version = readNumber<std::int32_t>(streamIn); //throw UnexpectedEndOfStreamError
        if (version != FILE_FORMAT_VER) //read file format version#
            throw FileError(replaceCpy(_("Database file %x is incompatible."), L"%x", fmtFileName(filename)));

        //read stream lists
        StreamMapping output;

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
    catch (ErrorNotExisting&)
    {
        throw FileErrorDatabaseNotExisting(_("Initial synchronization:") + L" \n" +
                                           replaceCpy(_("Database file %x does not yet exist."), L"%x", fmtFileName(filename)));
    }
    catch (UnexpectedEndOfStreamError&)
    {
        throw FileError(_("Database file is corrupt:") + L"\n" + fmtFileName(filename));
    }
    catch (const std::bad_alloc& e) //still required?
    {
        throw FileError(_("Database file is corrupt:") + L"\n" + fmtFileName(filename) + L"\n\n" +
                        _("Out of memory!") + L" " + utfCvrtTo<std::wstring>(e.what()));
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
                  0	49.54	  272 (uncompressed)
                  1	14.53	 1013
                  2	14.13	 1106
                  3	13.76	 1288 - best compromise between speed and compression
                  4	13.20	 1526
                  5	12.73	 1916
                  6	12.58	 2765
                  7	12.54	 3633
                  8	12.51	 9032
                  9	12.50	19698 (maximal compression) */
                return compress(stream, 3); //throw ZlibInternalError
            }
            catch (ZlibInternalError&)
            {
                throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(filename)) + L" (zlib error)");
            }
        };

        const BinaryStream tmpL = compStream(generator.outputLeft .get(), filenameL);
        const BinaryStream tmpR = compStream(generator.outputRight.get(), filenameR);
        const BinaryStream tmpB = compStream(generator.outputBoth .get(), filenameL + Zstr("/") + filenameR);

        //distribute "outputBoth" over left and right streams:
        BinStreamOut outL;
        BinStreamOut outR;
        writeNumber<bool>(outL, true); //this side contains first part of "outputBoth"
        writeNumber<bool>(outR, false);

        size_t size1stPart = tmpB.size() / 2;
        size_t size2ndPart = tmpB.size() - size1stPart;

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
        // for (const auto& filePair : container.files) { processFile(filePair); }); !

        writeNumber<std::uint32_t>(outputBoth, static_cast<std::uint32_t>(container.files.size()));
        std::for_each(container.files.begin(), container.files.end(), [&](const std::pair<Zstring, InSyncFile>& filePair) { this->process(filePair); });

        writeNumber<std::uint32_t>(outputBoth, static_cast<std::uint32_t>(container.symlinks.size()));
        std::for_each(container.symlinks.begin(), container.symlinks.end(), [&](const std::pair<Zstring, InSyncSymlink>& symlinkPair) { this->process(symlinkPair); });

        writeNumber<std::uint32_t>(outputBoth, static_cast<std::uint32_t>(container.dirs.size()));
        std::for_each(container.dirs.begin(), container.dirs.end(), [&](const std::pair<Zstring, InSyncDir>& dirPair) { this->process(dirPair); });
    }

    static void writeUtf8(BinStreamOut& output, const Zstring& str) { writeContainer(output, utfCvrtTo<Zbase<char>>(str)); }

    static void write(BinStreamOut& output, const FileDescriptor& descr)
    {
        writeNumber<std:: int64_t>(output, to<std:: int64_t>(descr.lastWriteTimeRaw));
        writeNumber<std::uint64_t>(output, to<std::uint64_t>(descr.fileSize));
        writeNumber<std::uint64_t>(output, descr.devId);
        writeNumber<std::uint64_t>(output, descr.fileIdx);
        assert_static(sizeof(descr.devId  ) <= sizeof(std::uint64_t));
        assert_static(sizeof(descr.fileIdx) <= sizeof(std::uint64_t));
    }

    static void write(BinStreamOut& output, const LinkDescriptor& descr)
    {
        writeNumber<std::int64_t>(output, to<std:: int64_t>(descr.lastWriteTimeRaw));
        writeUtf8(output, descr.targetPath);
        writeNumber<std::int32_t>(output, descr.type);
    }

    static void write(BinStreamOut& output, const InSyncDir::InSyncStatus& status)
    {
        writeNumber<std::int32_t>(output, status);
    }

    void process(const std::pair<Zstring, InSyncFile>& filePair)
    {
        writeUtf8(outputBoth, filePair.first);
        writeNumber<std::int32_t>(outputBoth, filePair.second.inSyncType);

        write(outputLeft,  filePair.second.left);
        write(outputRight, filePair.second.right);
    }

    void process(const std::pair<Zstring, InSyncSymlink>& symlinkPair)
    {
        writeUtf8(outputBoth, symlinkPair.first);
        write(outputLeft,  symlinkPair.second.left);
        write(outputRight, symlinkPair.second.right);
    }

    void process(const std::pair<Zstring, InSyncDir>& dirPair)
    {
        writeUtf8(outputBoth, dirPair.first);
        write(outputBoth, dirPair.second.status);

        recurse(dirPair.second);
    }

    BinStreamOut outputLeft;  //data related to one side only
    BinStreamOut outputRight; //
    BinStreamOut outputBoth;  //data concerning both sides
};


class StreamParser //for db-file back-wards compatibility we stick with two output streams until further
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
                throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(filename)) + L" (zlib error)");
            }
        };

        try
        {
            BinStreamIn inL(streamL);
            BinStreamIn inR(streamR);

            bool has1stPartL = readNumber<bool>(inL); //throw UnexpectedEndOfStreamError
            bool has1stPartR = readNumber<bool>(inR); //
            if (has1stPartL == has1stPartR)
                throw UnexpectedEndOfStreamError();

            BinStreamIn& in1stPart = has1stPartL ? inL : inR;
            BinStreamIn& in2ndPart = has1stPartL ? inR : inL;

            const size_t size1stPart = static_cast<size_t>(readNumber<std::uint64_t>(in1stPart));
            const size_t size2ndPart = static_cast<size_t>(readNumber<std::uint64_t>(in2ndPart));

            BinaryStream tmpB;
            tmpB.resize(size1stPart + size2ndPart);
            readArray(in1stPart, &*tmpB.begin(), size1stPart);
            readArray(in2ndPart, &*tmpB.begin() + size1stPart, size2ndPart);

            const BinaryStream tmpL = readContainer<BinaryStream>(inL);
            const BinaryStream tmpR = readContainer<BinaryStream>(inR);

            auto output = std::make_shared<InSyncDir>(InSyncDir::STATUS_IN_SYNC);
            StreamParser(decompStream(tmpL, filenameL),
                         decompStream(tmpR, filenameR),
                         decompStream(tmpB, filenameL + Zstr("/") + filenameR),
                         *output); //throw UnexpectedEndOfStreamError
            return output;
        }
        catch (const UnexpectedEndOfStreamError&)
        {
            throw FileError(_("Database file is corrupt:") + L"\n" + fmtFileName(filenameL) + L"\n" + fmtFileName(filenameR));
        }
        catch (const std::bad_alloc& e) //still required?
        {
            throw FileError(_("Database file is corrupt:") + L"\n" + fmtFileName(filenameL) + L"\n" + fmtFileName(filenameR) + L"\n\n" +
                            _("Out of memory!") + L" " + utfCvrtTo<std::wstring>(e.what()));
        }
    }

private:
    StreamParser(const BinaryStream& bufferL,
                 const BinaryStream& bufferR,
                 const BinaryStream& bufferB,
                 InSyncDir& container) :
        inputLeft (bufferL),
        inputRight(bufferR),
        inputBoth (bufferB) { recurse(container); }

    static Zstring readUtf8(BinStreamIn& input) { return utfCvrtTo<Zstring>(readContainer<Zbase<char>>(input)); } //throw UnexpectedEndOfStreamError

    static void read(BinStreamIn& input, FileDescriptor& descr)
    {
        //attention: order of function argument evaluation is undefined! So do it one after the other...
        descr.lastWriteTimeRaw = readNumber<std::int64_t>(input); //throw UnexpectedEndOfStreamError
        descr.fileSize         = readNumber<std::uint64_t>(input);
        descr.devId            = static_cast<DeviceId >(readNumber<std::uint64_t>(input)); //
        descr.fileIdx          = static_cast<FileIndex>(readNumber<std::uint64_t>(input)); //silence "loss of precision" compiler warnings
    }

    static void read(BinStreamIn& input, LinkDescriptor& descr)
    {
        descr.lastWriteTimeRaw = readNumber<std::int64_t>(input);
        descr.targetPath       = readUtf8(input); //file name
        descr.type             = static_cast<LinkDescriptor::LinkType>(readNumber<std::int32_t>(input));
    }

    static void read(BinStreamIn& input, InSyncDir::InSyncStatus& status)
    {
        status = static_cast<InSyncDir::InSyncStatus>(readNumber<std::int32_t>(input));
    }

    void recurse(InSyncDir& container)
    {
        size_t fileCount = readNumber<std::uint32_t>(inputBoth);
        while (fileCount-- != 0) //files
        {
            const Zstring shortName = readUtf8(inputBoth);
            const auto inSyncType = static_cast<InSyncFile::InSyncType>(readNumber<std::int32_t>(inputBoth));

            FileDescriptor dataL;
            FileDescriptor dataR;
            read(inputLeft,  dataL);
            read(inputRight, dataR);

            container.addFile(shortName, dataL, dataR, inSyncType);
        }

        size_t linkCount = readNumber<std::uint32_t>(inputBoth);
        while (linkCount-- != 0) //files
        {
            const Zstring shortName = readUtf8(inputBoth);

            LinkDescriptor dataL;
            LinkDescriptor dataR;
            read(inputLeft,  dataL);
            read(inputRight, dataR);

            container.addSymlink(shortName, dataL, dataR);
        }

        size_t dirCount = readNumber<std::uint32_t>(inputBoth);
        while (dirCount-- != 0) //files
        {
            const Zstring shortName = readUtf8(inputBoth);

            InSyncDir::InSyncStatus status = InSyncDir::STATUS_STRAW_MAN;
            read(inputBoth, status);

            InSyncDir& subDir = container.addDir(shortName, status);
            recurse(subDir);
        }
    }

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
    static void execute(const BaseDirMapping& baseMapping, InSyncDir& dir)
    {
        bool binaryComparison = false;
        switch (baseMapping.getCompVariant())
        {
            case CMP_BY_TIME_SIZE:
                break;
            case CMP_BY_CONTENT:
                binaryComparison = true;
                break;
        }

        UpdateLastSynchronousState updater(baseMapping.getFilter(), binaryComparison);
        updater.recurse(baseMapping, dir);
    }

private:
    UpdateLastSynchronousState(const HardFilter& filter, bool binaryComparison) :
        filter_(filter),
        binaryComparison_(binaryComparison) {}

    void recurse(const HierarchyObject& hierObj, InSyncDir& dir)
    {
        process(hierObj.refSubFiles(), hierObj.getObjRelativeNamePf(), dir.files);
        process(hierObj.refSubLinks(), hierObj.getObjRelativeNamePf(), dir.symlinks);
        process(hierObj.refSubDirs (), hierObj.getObjRelativeNamePf(), dir.dirs);
    }

    template <class M, class V>
    static V& updateItem(M& map, const Zstring& key, const V& value) //efficient create or update without "default-constructible" requirement (Effective STL, item 24)
    {
        auto iter = map.lower_bound(key);
        if (iter != map.end() && !(map.key_comp()(key, iter->first)))
        {
#ifdef FFS_WIN //caveat: key might need to be updated, too, if there is a change in short name case!!!
            if (iter->first != key)
            {
                map.erase(iter); //don't fiddle with decrementing "iter"! - you might lose while optimizing pointlessly
                return map.insert(typename M::value_type(key, value)).first->second;
            }
            else
#endif
            {
                iter->second = value;
                return iter->second;
            }
        }
        return map.insert(iter, typename M::value_type(key, value))->second;
    }

    void process(const HierarchyObject::SubFileVec& currentFiles, const Zstring& parentRelativeNamePf, InSyncDir::FileList& dbFiles)
    {
        hash_set<const InSyncFile*> toPreserve; //referencing fixed-in-memory std::map elements
        std::for_each(currentFiles.begin(), currentFiles.end(), [&](const FileMapping& fileMap)
        {
            if (!fileMap.isEmpty())
            {
                if (fileMap.getCategory() == FILE_EQUAL) //data in sync: write current state
                {
                    //create or update new "in-sync" state
                    InSyncFile& file = updateItem(dbFiles, fileMap.getObjShortName(),
                                                  InSyncFile(FileDescriptor(fileMap.getLastWriteTime<LEFT_SIDE>(),
                                                                            fileMap.getFileSize     <LEFT_SIDE>(),
                                                                            fileMap.getFileId       <LEFT_SIDE>()),
                                                             FileDescriptor(fileMap.getLastWriteTime<RIGHT_SIDE>(),
                                                                            fileMap.getFileSize     <RIGHT_SIDE>(),
                                                                            fileMap.getFileId       <RIGHT_SIDE>()),
                                                             binaryComparison_ ?
                                                             InSyncFile::IN_SYNC_BINARY_EQUAL :
                                                             InSyncFile::IN_SYNC_ATTRIBUTES_EQUAL)); //efficient add or update (Effective STL, item 24)
                    //Caveat: If FILE_EQUAL, we *implicitly* assume equal left and right short names matching case: InSyncDir's mapping tables use short name as a key!
                    //This makes us silently dependent from code in algorithm.h!!!
                    toPreserve.insert(&file);
                }
                else //not in sync: preserve last synchronous state
                {
                    auto iter = dbFiles.find(fileMap.getObjShortName());
                    if (iter != dbFiles.end())
                        toPreserve.insert(&iter->second);
                }
            }
        });

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
        std::for_each(currentLinks.begin(), currentLinks.end(), [&](const SymLinkMapping& linkMap)
        {
            if (!linkMap.isEmpty())
            {
                if (linkMap.getLinkCategory() == SYMLINK_EQUAL) //data in sync: write current state
                {
                    //create or update new "in-sync" state
                    InSyncSymlink& link = updateItem(dbLinks, linkMap.getObjShortName(),
                                                     InSyncSymlink(LinkDescriptor(linkMap.getLastWriteTime<LEFT_SIDE>(),
                                                                                  linkMap.getTargetPath   <LEFT_SIDE>(),
                                                                                  linkMap.getLinkType     <LEFT_SIDE>()),
                                                                   LinkDescriptor(linkMap.getLastWriteTime<RIGHT_SIDE>(),
                                                                                  linkMap.getTargetPath   <RIGHT_SIDE>(),
                                                                                  linkMap.getLinkType     <RIGHT_SIDE>()))); //efficient add or update (Effective STL, item 24)
                    toPreserve.insert(&link);
                }
                else //not in sync: preserve last synchronous state
                {
                    auto iter = dbLinks.find(linkMap.getObjShortName());
                    if (iter != dbLinks.end())
                        toPreserve.insert(&iter->second);
                }
            }
        });

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
        std::for_each(currentDirs.begin(), currentDirs.end(), [&](const DirMapping& dirMap)
        {
            if (!dirMap.isEmpty())
            {
                switch (dirMap.getDirCategory())
                {
                    case DIR_EQUAL:
                    {
                        //update directory entry only (shallow), but do *not touch* exising child elements!!!
                        const Zstring& key = dirMap.getObjShortName();
                        auto insertResult = dbDirs.insert(std::make_pair(key, InSyncDir(InSyncDir::STATUS_IN_SYNC))); //get or create
                        auto iter = insertResult.first;

#ifdef FFS_WIN //caveat: key might need to be updated, too, if there is a change in short name case!!!
                        const bool alreadyExisting = !insertResult.second;
                        if (alreadyExisting && iter->first != key)
                        {
                            auto oldValue = std::move(iter->second);
                            dbDirs.erase(iter); //don't fiddle with decrementing "iter"! - you might lose while optimizing pointlessly
                            iter = dbDirs.insert(InSyncDir::DirList::value_type(key, std::move(oldValue))).first;
                        }
#endif
                        InSyncDir& dir = iter->second;
                        dir.status = InSyncDir::STATUS_IN_SYNC; //update immediate directory entry
                        toPreserve.insert(&dir);
                        recurse(dirMap, dir);
                    }
                    break;

                    case DIR_DIFFERENT_METADATA:
                        //if DIR_DIFFERENT_METADATA and no old database entry yet: we have to insert a new (bogus) database entry:
                        //we cannot simply skip the whole directory, since sub-items might be in sync!
                        //Example: directories on left and right differ in case while sub-files are equal
                    {
                        //reuse last "in-sync" if available or insert strawman entry (do not try to update thereby removing child elements!!!)
                        InSyncDir& dir = dbDirs.insert(std::make_pair(dirMap.getObjShortName(), InSyncDir(InSyncDir::STATUS_STRAW_MAN))).first->second;
                        toPreserve.insert(&dir);
                        recurse(dirMap, dir);
                    }
                    break;

                    //not in sync: reuse last synchronous state:
                    case DIR_LEFT_SIDE_ONLY:
                    case DIR_RIGHT_SIDE_ONLY:
                    {
                        auto iter = dbDirs.find(dirMap.getObjShortName());
                        if (iter != dbDirs.end())
                        {
                            toPreserve.insert(&iter->second);
                            recurse(dirMap, iter->second); //although existing sub-items cannot be in sync, items deleted on both sides *are* in-sync!!!
                        }
                    }
                    break;
                }
            }
        });

        //delete removed items (= "in-sync") from database
        map_remove_if(dbDirs, [&](const InSyncDir::DirList::value_type& v) -> bool
        {
            if (toPreserve.find(&v.second) != toPreserve.end())
                return false;
            //all items not existing in "currentDirs" have either been deleted meanwhile or been excluded via filter:
            const Zstring& shortName = v.first;
            return filter_.passDirFilter(parentRelativeNamePf + shortName, nullptr);
            //if directory is not included in "currentDirs", it is either not existing anymore, in which case it should be deleted from database
            //or it was excluded via filter, in which case the database entry should be preserved -> we can't tell and need to preserve the old db entry
        });
    }

    const HardFilter& filter_; //filter used while scanning directory: generates view on actual files!
    const bool binaryComparison_;
};
}

//#######################################################################################################################################

std::shared_ptr<InSyncDir> zen::loadLastSynchronousState(const BaseDirMapping& baseMapping) //throw FileError, FileErrorDatabaseNotExisting -> return value always bound!
{
    const Zstring fileNameLeft  = getDBFilename<LEFT_SIDE >(baseMapping);
    const Zstring fileNameRight = getDBFilename<RIGHT_SIDE>(baseMapping);

    if (!baseMapping.isExisting<LEFT_SIDE >() ||
        !baseMapping.isExisting<RIGHT_SIDE>())
    {
        //avoid race condition with directory existence check: reading sync.ffs_db may succeed although first dir check had failed => conflicts!
        //https://sourceforge.net/tracker/?func=detail&atid=1093080&aid=3531351&group_id=234430
        const Zstring filename = !baseMapping.isExisting<LEFT_SIDE>() ? fileNameLeft : fileNameRight;
        throw FileErrorDatabaseNotExisting(_("Initial synchronization:") + L" \n" + //it could be due to a to-be-created target directory not yet existing => FileErrorDatabaseNotExisting
                                           replaceCpy(_("Database file %x does not yet exist."), L"%x", fmtFileName(filename)));
    }

    //read file data: list of session ID + DirInfo-stream
    const StreamMapping streamListLeft  = ::loadStreams(fileNameLeft);  //throw FileError, FileErrorDatabaseNotExisting
    const StreamMapping streamListRight = ::loadStreams(fileNameRight); //

    //find associated session: there can be at most one session within intersection of left and right ids
    for (auto iterLeft = streamListLeft.begin(); iterLeft != streamListLeft.end(); ++iterLeft)
    {
        auto iterRight = streamListRight.find(iterLeft->first);
        if (iterRight != streamListRight.end())
        {
            return StreamParser::execute(iterLeft ->second, //throw FileError
                                         iterRight->second,
                                         fileNameLeft,
                                         fileNameRight);
        }
    }
    throw FileErrorDatabaseNotExisting(_("Initial synchronization:") + L" \n" +
                                       _("Database files do not share a common session."));
}


void zen::saveLastSynchronousState(const BaseDirMapping& baseMapping) //throw FileError
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
    auto streamIterLeftOld  = streamListLeft .cend();
    auto streamIterRightOld = streamListRight.cend();
    for (auto iterLeft = streamListLeft.begin(); iterLeft != streamListLeft.end(); ++iterLeft)
    {
        auto iterRight = streamListRight.find(iterLeft->first);
        if (iterRight != streamListRight.end())
        {
            streamIterLeftOld  = iterLeft;
            streamIterRightOld = iterRight;
            break;
        }
    }

    //load last synchrounous state
    std::shared_ptr<InSyncDir> lastSyncState = std::make_shared<InSyncDir>(InSyncDir::STATUS_IN_SYNC);
    if (streamIterLeftOld  != streamListLeft .end() &&
        streamIterRightOld != streamListRight.end())
        try
        {
            lastSyncState = StreamParser::execute(streamIterLeftOld ->second, //throw FileError
                                                  streamIterRightOld->second,
                                                  dbNameLeft,
                                                  dbNameRight);
        }
        catch (FileError&) {} //if error occurs: just overwrite old file! User is already informed about issues right after comparing!

    //update last synchrounous state
    UpdateLastSynchronousState::execute(baseMapping, *lastSyncState);

    //serialize again
    BinaryStream updatedStreamLeft;
    BinaryStream updatedStreamRight;
    StreamGenerator::execute(*lastSyncState,
                             dbNameLeft,
                             dbNameRight,
                             updatedStreamLeft,
                             updatedStreamRight); //throw FileError

    //check if there is some work to do at all
    if (streamIterLeftOld  != streamListLeft .end() && updatedStreamLeft  == streamIterLeftOld ->second &&
        streamIterRightOld != streamListRight.end() && updatedStreamRight == streamIterRightOld->second)
        return; //some users monitor the *.ffs_db file with RTS => don't touch the file if it isnt't strictly needed

    //erase old session data
    if (streamIterLeftOld != streamListLeft.end())
        streamListLeft.erase(streamIterLeftOld);
    if (streamIterRightOld != streamListRight.end())
        streamListRight.erase(streamIterRightOld);

    //create new session data
    const std::string sessionID = zen::generateGUID();

    streamListLeft [sessionID] = std::move(updatedStreamLeft);
    streamListRight[sessionID] = std::move(updatedStreamRight);

    //write (temp-) files...
    zen::ScopeGuard guardTempFileLeft = zen::makeGuard([&] {zen::removeFile(dbNameLeftTmp); });
    saveStreams(streamListLeft, dbNameLeftTmp);  //throw FileError

    zen::ScopeGuard guardTempFileRight = zen::makeGuard([&] {zen::removeFile(dbNameRightTmp); });
    saveStreams(streamListRight, dbNameRightTmp); //throw FileError

    //operation finished: rename temp files -> this should work transactionally:
    //if there were no write access, creation of temp files would have failed
    removeFile(dbNameLeft);                  //
    removeFile(dbNameRight);                 //throw FileError
    renameFile(dbNameLeftTmp,  dbNameLeft);  //
    renameFile(dbNameRightTmp, dbNameRight); //

    guardTempFileLeft. dismiss(); //no need to delete temp files anymore
    guardTempFileRight.dismiss(); //
}
