// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef FREEFILESYNC_H_INCLUDED
#define FREEFILESYNC_H_INCLUDED

#include <vector>
#include <memory>
#include <zen/zstring.h>
#include <zen/assert_static.h>
#include <zen/int64.h>

namespace zen
{
enum CompareVariant
{
    CMP_BY_TIME_SIZE,
    CMP_BY_CONTENT
};

std::wstring getVariantName(CompareVariant var);

enum SymLinkHandling
{
    SYMLINK_IGNORE,
    SYMLINK_USE_DIRECTLY,
    SYMLINK_FOLLOW_LINK
};


enum SyncDirection
{
    SYNC_DIR_LEFT = 0,
    SYNC_DIR_RIGHT,
    SYNC_DIR_NONE
};


enum CompareFilesResult
{
    FILE_LEFT_SIDE_ONLY = 0,
    FILE_RIGHT_SIDE_ONLY,
    FILE_LEFT_NEWER,
    FILE_RIGHT_NEWER,
    FILE_DIFFERENT,
    FILE_EQUAL,
    FILE_DIFFERENT_METADATA, //both sides equal, but different metadata only: short name case, modification time
    FILE_CONFLICT
};
//attention make sure these /|\  \|/ three enums match!!!
enum CompareDirResult
{
    DIR_LEFT_SIDE_ONLY     = FILE_LEFT_SIDE_ONLY,
    DIR_RIGHT_SIDE_ONLY    = FILE_RIGHT_SIDE_ONLY,
    DIR_EQUAL              = FILE_EQUAL,
    DIR_DIFFERENT_METADATA = FILE_DIFFERENT_METADATA //both sides equal, but different metadata only: short name case
};

enum CompareSymlinkResult
{
    SYMLINK_LEFT_SIDE_ONLY  = FILE_LEFT_SIDE_ONLY,
    SYMLINK_RIGHT_SIDE_ONLY = FILE_RIGHT_SIDE_ONLY,
    SYMLINK_LEFT_NEWER      = FILE_LEFT_NEWER,
    SYMLINK_RIGHT_NEWER     = FILE_RIGHT_NEWER,
    SYMLINK_DIFFERENT       = FILE_DIFFERENT,
    SYMLINK_EQUAL           = FILE_EQUAL,
    SYMLINK_DIFFERENT_METADATA = FILE_DIFFERENT_METADATA, //both sides equal, but different metadata only: short name case
    SYMLINK_CONFLICT        = FILE_CONFLICT
};


std::wstring getSymbol(CompareFilesResult cmpRes);


enum SyncOperation
{
    SO_CREATE_NEW_LEFT,
    SO_CREATE_NEW_RIGHT,
    SO_DELETE_LEFT,
    SO_DELETE_RIGHT,

    SO_MOVE_LEFT_SOURCE, //SO_DELETE_LEFT    - optimization!
    SO_MOVE_LEFT_TARGET, //SO_CREATE_NEW_LEFT

    SO_MOVE_RIGHT_SOURCE, //SO_DELETE_RIGHT    - optimization!
    SO_MOVE_RIGHT_TARGET, //SO_CREATE_NEW_RIGHT

    SO_OVERWRITE_LEFT,
    SO_OVERWRITE_RIGHT,
    SO_COPY_METADATA_TO_LEFT,  //objects are already equal: transfer metadata only - optimization
    SO_COPY_METADATA_TO_RIGHT, //

    SO_DO_NOTHING, //= both sides differ, but nothing will be synced
    SO_EQUAL,      //= both sides are equal, so nothing will be synced
    SO_UNRESOLVED_CONFLICT
};

std::wstring getSymbol     (SyncOperation op); //method used for exporting .csv file only!


struct DirectionSet
{
    DirectionSet() :
        exLeftSideOnly( SYNC_DIR_RIGHT),
        exRightSideOnly(SYNC_DIR_LEFT),
        leftNewer(      SYNC_DIR_RIGHT),
        rightNewer(     SYNC_DIR_LEFT),
        different(      SYNC_DIR_NONE),
        conflict(       SYNC_DIR_NONE) {}

    SyncDirection exLeftSideOnly;
    SyncDirection exRightSideOnly;
    SyncDirection leftNewer;
    SyncDirection rightNewer;
    SyncDirection different;
    SyncDirection conflict;
};

DirectionSet getTwoWaySet();

inline
bool operator==(const DirectionSet& lhs, const DirectionSet& rhs)
{
    return lhs.exLeftSideOnly  == rhs.exLeftSideOnly  &&
           lhs.exRightSideOnly == rhs.exRightSideOnly &&
           lhs.leftNewer       == rhs.leftNewer       &&
           lhs.rightNewer      == rhs.rightNewer      &&
           lhs.different       == rhs.different       &&
           lhs.conflict        == rhs.conflict;
}

struct DirectionConfig //technical representation of sync-config
{
    enum Variant
    {
        AUTOMATIC, //use sync-database to determine directions
        MIRROR,    //predefined
        UPDATE,    //
        CUSTOM     //use custom directions
    };

    DirectionConfig() : var(AUTOMATIC) {}

    Variant var;

    //custom sync directions
    DirectionSet custom;
};

inline
bool operator==(const DirectionConfig& lhs, const DirectionConfig& rhs)
{
    return lhs.var == rhs.var &&
           (lhs.var != DirectionConfig::CUSTOM ||  lhs.custom == rhs.custom); //directions are only relevant if variant "custom" is active
}

//get sync directions: DON'T call for variant AUTOMATIC!
DirectionSet extractDirections(const DirectionConfig& cfg);

std::wstring getVariantName(DirectionConfig::Variant var);



struct CompConfig
{
    //CompConfig(CompareVariant cmpVar,
    //           SymLinkHandling handleSyml) :
    //    compareVar(cmpVar),
    //    handleSymlinks(handleSyml) {}

    CompConfig() :
        compareVar(CMP_BY_TIME_SIZE),
        handleSymlinks(SYMLINK_IGNORE) {}

    CompareVariant compareVar;
    SymLinkHandling handleSymlinks;
};

inline
bool operator==(const CompConfig& lhs, const CompConfig& rhs)
{
    return lhs.compareVar     == rhs.compareVar &&
           lhs.handleSymlinks == rhs.handleSymlinks;
}


enum DeletionPolicy
{
    DELETE_PERMANENTLY,
    DELETE_TO_RECYCLER,
    DELETE_TO_VERSIONING
};


struct SyncConfig
{
    //SyncConfig(const DirectionConfig& directCfg,
    //           const DeletionPolicy   handleDel,
    //           const Zstring&         versionDir) :
    //    directionCfg(directCfg),
    //    handleDeletion(handleDel),
    //    versioningDirectory(versionDir) {}

    SyncConfig() :  //construct with default values
        handleDeletion(DELETE_TO_RECYCLER),
        versionCountLimit(-1) {}

    //sync direction settings
    DirectionConfig directionCfg;

    //misc options
    DeletionPolicy handleDeletion; //use Recycle, delete permanently or move to user-defined location
    Zstring versioningDirectory;
    int versionCountLimit; //max versions per file (DELETE_TO_VERSIONING); < 0 := no limit
};

inline
bool operator==(const SyncConfig& lhs, const SyncConfig& rhs)
{
    return lhs.directionCfg   == rhs.directionCfg   &&
           lhs.handleDeletion == rhs.handleDeletion &&
           (lhs.handleDeletion != DELETE_TO_VERSIONING || //only compare deletion directory if required!
            (lhs.versioningDirectory == rhs.versioningDirectory &&
             lhs.versionCountLimit   == rhs.versionCountLimit));
}


enum UnitSize
{
    USIZE_NONE,
    USIZE_BYTE,
    USIZE_KB,
    USIZE_MB
};

enum UnitTime
{
    UTIME_NONE,
    UTIME_TODAY,
    //    UTIME_THIS_WEEK,
    UTIME_THIS_MONTH,
    UTIME_THIS_YEAR,
    UTIME_LAST_X_DAYS
};

struct FilterConfig
{
    FilterConfig(const Zstring& include  = Zstr("*"),
                 const Zstring& exclude  = Zstring(),
                 size_t   timeSpanIn     = 0,
                 UnitTime unitTimeSpanIn = UTIME_NONE,
                 size_t   sizeMinIn      = 0,
                 UnitSize unitSizeMinIn  = USIZE_NONE,
                 size_t   sizeMaxIn      = 0,
                 UnitSize unitSizeMaxIn  = USIZE_NONE) :
        includeFilter(include),
        excludeFilter(exclude),
        timeSpan     (timeSpanIn),
        unitTimeSpan (unitTimeSpanIn),
        sizeMin      (sizeMinIn),
        unitSizeMin  (unitSizeMinIn),
        sizeMax      (sizeMaxIn),
        unitSizeMax  (unitSizeMaxIn) {}

    /*
    Semantics of HardFilter:
    1. using it creates a NEW folder hierarchy! -> must be considered by <Automatic>-mode! (fortunately it turns out, doing nothing already has perfect semantics :)
    2. it applies equally to both sides => it always matches either both sides or none! => can be used while traversing a single folder!
    */
    Zstring includeFilter;
    Zstring excludeFilter;

    /*
    Semantics of SoftFilter:
    1. It potentially may match only one side => it MUST NOT be applied while traversing a single folder to avoid mismatches
    2. => it is applied after traversing and just marks rows, (NO deletions after comparison are allowed)
    3. => equivalent to a user temporarily (de-)selecting rows -> not relevant for <Automatic>-mode! ;)
    */
    size_t timeSpan;
    UnitTime unitTimeSpan;

    size_t sizeMin;
    UnitSize unitSizeMin;

    size_t sizeMax;
    UnitSize unitSizeMax;
};

inline
bool operator==(const FilterConfig& lhs, const FilterConfig& rhs)
{
    return lhs.includeFilter == rhs.includeFilter &&
           lhs.excludeFilter == rhs.excludeFilter &&
           lhs.timeSpan      == rhs.timeSpan      &&
           lhs.unitTimeSpan  == rhs.unitTimeSpan  &&
           lhs.sizeMin       == rhs.sizeMin       &&
           lhs.unitSizeMin   == rhs.unitSizeMin   &&
           lhs.sizeMax       == rhs.sizeMax       &&
           lhs.unitSizeMax   == rhs.unitSizeMax;
}

void resolveUnits(size_t timeSpan, UnitTime unitTimeSpan,
                  size_t sizeMin,  UnitSize unitSizeMin,
                  size_t sizeMax,  UnitSize unitSizeMax,
                  zen::Int64&  timeFrom,    //unit: UTC time, seconds
                  zen::UInt64& sizeMinBy,   //unit: bytes
                  zen::UInt64& sizeMaxBy);  //unit: bytes


struct FolderPairEnh //enhanced folder pairs with (optional) alternate configuration
{
    FolderPairEnh() {}

    FolderPairEnh(const Zstring& leftDir,
                  const Zstring& rightDir,
                  const std::shared_ptr<const CompConfig>& cmpConfig,
                  const std::shared_ptr<const SyncConfig>& syncConfig,
                  const FilterConfig& filter) :
        leftDirectory(leftDir),
        rightDirectory(rightDir) ,
        altCmpConfig(cmpConfig),
        altSyncConfig(syncConfig),
        localFilter(filter) {}

    Zstring leftDirectory;
    Zstring rightDirectory;

    std::shared_ptr<const CompConfig> altCmpConfig;  //optional
    std::shared_ptr<const SyncConfig> altSyncConfig; //
    FilterConfig localFilter;
};


inline
bool operator==(const FolderPairEnh& lhs, const FolderPairEnh& rhs)
{
    return lhs.leftDirectory  == rhs.leftDirectory  &&
           lhs.rightDirectory == rhs.rightDirectory &&

           (lhs.altCmpConfig.get() && rhs.altCmpConfig.get() ?
            *lhs.altCmpConfig == *rhs.altCmpConfig :
            lhs.altCmpConfig.get() == rhs.altCmpConfig.get()) &&

           (lhs.altSyncConfig.get() && rhs.altSyncConfig.get() ?
            *lhs.altSyncConfig == *rhs.altSyncConfig :
            lhs.altSyncConfig.get() == rhs.altSyncConfig.get()) &&

           lhs.localFilter == rhs.localFilter;
}


struct MainConfiguration
{
    MainConfiguration() :
#ifdef FFS_WIN
        globalFilter(Zstr("*"),
                     Zstr("\\System Volume Information\\\n")
                     Zstr("\\$Recycle.Bin\\\n")
                     Zstr("\\RECYCLER\\\n")
                     Zstr("\\RECYCLED\\\n")) {}
#elif defined FFS_LINUX
        globalFilter(Zstr("*"),
                     Zstr("/.Trash-*/\n")
                     Zstr("/.recycle/\n")) {}
#endif

    CompConfig   cmpConfig;    //global compare settings:         may be overwritten by folder pair settings
    SyncConfig   syncCfg;      //global synchronisation settings: may be overwritten by folder pair settings
    FilterConfig globalFilter; //global filter settings:          combined with folder pair settings

    FolderPairEnh firstPair; //there needs to be at least one pair!
    std::vector<FolderPairEnh> additionalPairs;

    std::wstring onCompletion; //user-defined command line

    std::wstring getCompVariantName() const;
    std::wstring getSyncVariantName() const;
};


inline
bool operator==(const MainConfiguration& lhs, const MainConfiguration& rhs)
{
    return lhs.cmpConfig        == rhs.cmpConfig       &&
           lhs.globalFilter     == rhs.globalFilter    &&
           lhs.syncCfg          == rhs.syncCfg         &&
           lhs.firstPair        == rhs.firstPair       &&
           lhs.additionalPairs  == rhs.additionalPairs &&
           lhs.onCompletion     == rhs.onCompletion;
}

//facilitate drag & drop config merge:
MainConfiguration merge(const std::vector<MainConfiguration>& mainCfgs);
}

#endif // FREEFILESYNC_H_INCLUDED
