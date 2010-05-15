// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef FREEFILESYNC_H_INCLUDED
#define FREEFILESYNC_H_INCLUDED

#include <wx/string.h>
#include <vector>
#include "shared/zstring.h"
#include "shared/systemConstants.h"
#include "shared/staticAssert.h"
#include <boost/shared_ptr.hpp>

namespace FreeFileSync
{
enum CompareVariant
{
    CMP_BY_TIME_SIZE,
    CMP_BY_CONTENT
};

wxString getVariantName(CompareVariant var);


enum SyncDirection
{
    SYNC_DIR_LEFT  = 0,
    SYNC_DIR_RIGHT,
    SYNC_DIR_NONE //NOTE: align with SyncDirectionIntern before adding anything here!
};


enum CompareFilesResult
{
    FILE_LEFT_SIDE_ONLY = 0,
    FILE_RIGHT_SIDE_ONLY,
    FILE_LEFT_NEWER,
    FILE_RIGHT_NEWER,
    FILE_DIFFERENT,
    FILE_EQUAL,
    FILE_CONFLICT
};
//attention make sure these /|\  \|/ two enums match!!!
enum CompareDirResult
{
    DIR_LEFT_SIDE_ONLY  = FILE_LEFT_SIDE_ONLY,
    DIR_RIGHT_SIDE_ONLY = FILE_RIGHT_SIDE_ONLY,
    DIR_EQUAL           = FILE_EQUAL
};


inline
CompareFilesResult convertToFilesResult(CompareDirResult value)
{
    return static_cast<CompareFilesResult>(value);
}



wxString getDescription(CompareFilesResult cmpRes);
wxString getSymbol(CompareFilesResult cmpRes);


enum SyncOperation
{
    SO_CREATE_NEW_LEFT,
    SO_CREATE_NEW_RIGHT,
    SO_DELETE_LEFT,
    SO_DELETE_RIGHT,
    SO_OVERWRITE_LEFT,
    SO_OVERWRITE_RIGHT,
    SO_DO_NOTHING, //= both sides differ, but nothing will be synced
    SO_EQUAL,      //= both sides are equal, so nothing will be synced
    SO_UNRESOLVED_CONFLICT
};

wxString getDescription(SyncOperation op);
wxString getSymbol(SyncOperation op);


//Exception class used to abort the "compare" and "sync" process
class AbortThisProcess {};


struct SyncConfigCustom //save last used custom config settings
{
    SyncConfigCustom() :
        exLeftSideOnly( SYNC_DIR_NONE),
        exRightSideOnly(SYNC_DIR_NONE),
        leftNewer(      SYNC_DIR_NONE),
        rightNewer(     SYNC_DIR_NONE),
        different(      SYNC_DIR_NONE),
        conflict(       SYNC_DIR_NONE) {}

    SyncDirection exLeftSideOnly;
    SyncDirection exRightSideOnly;
    SyncDirection leftNewer;
    SyncDirection rightNewer;
    SyncDirection different;
    SyncDirection conflict;
};


struct SyncConfiguration //technical representation of sync-config: not to be edited by GUI directly!
{
    SyncConfiguration() :
        automatic(true),
        exLeftSideOnly( SYNC_DIR_RIGHT),
        exRightSideOnly(SYNC_DIR_LEFT),
        leftNewer(      SYNC_DIR_RIGHT),
        rightNewer(     SYNC_DIR_LEFT),
        different(      SYNC_DIR_NONE),
        conflict(       SYNC_DIR_NONE) {}

    enum Variant
    {
        AUTOMATIC,
        MIRROR,
        UPDATE,
        CUSTOM
    };

    bool operator==(const SyncConfiguration& other) const
    {
        return automatic       == other.automatic       &&
               exLeftSideOnly  == other.exLeftSideOnly  &&
               exRightSideOnly == other.exRightSideOnly &&
               leftNewer       == other.leftNewer       &&
               rightNewer      == other.rightNewer      &&
               different       == other.different       &&
               conflict        == other.conflict;
    }

    bool automatic; //use sync-database
    SyncDirection exLeftSideOnly;
    SyncDirection exRightSideOnly;
    SyncDirection leftNewer;
    SyncDirection rightNewer;
    SyncDirection different;
    SyncDirection conflict;
};


SyncConfiguration::Variant getVariant(const SyncConfiguration& syncCfg);
void setVariant(SyncConfiguration& syncCfg, const SyncConfiguration::Variant var);
wxString getVariantName(const SyncConfiguration& syncCfg);
void setTwoWay(SyncConfiguration& syncCfg); //helper method used by <Automatic> mode fallback to overwrite old with newer files


enum DeletionPolicy
{
    DELETE_PERMANENTLY = 5,
    MOVE_TO_RECYCLE_BIN,
    MOVE_TO_CUSTOM_DIRECTORY
};


struct HiddenSettings
{
    HiddenSettings() :
        fileTimeTolerance(2),  //default 2s: FAT vs NTFS
        verifyFileCopy(false) {}

    size_t fileTimeTolerance; //max. allowed file time deviation
    bool verifyFileCopy;   //verify copied files

    bool operator==(const HiddenSettings& other) const
    {
        return fileTimeTolerance == other.fileTimeTolerance &&
               verifyFileCopy    == other.verifyFileCopy;
    }
};


struct AlternateSyncConfig
{
    AlternateSyncConfig(const SyncConfiguration& syncCfg,
                        const DeletionPolicy     handleDel,
                        const wxString&          customDelDir) :
        syncConfiguration(syncCfg),
        handleDeletion(handleDel),
        customDeletionDirectory(customDelDir) {};

    AlternateSyncConfig() :  //construct with default values
        handleDeletion(MOVE_TO_RECYCLE_BIN) {}

    //Synchronisation settings
    SyncConfiguration syncConfiguration;

    //misc options
    DeletionPolicy handleDeletion; //use Recycle, delete permanently or move to user-defined location
    wxString customDeletionDirectory;

    bool operator==(const AlternateSyncConfig& other) const
    {
        return syncConfiguration       == other.syncConfiguration &&
               handleDeletion          == other.handleDeletion    &&
               customDeletionDirectory == other.customDeletionDirectory;
    }
};

//standard exclude filter settings, OS dependent
Zstring standardExcludeFilter();


struct FilterConfig
{
    FilterConfig(const Zstring& include, const Zstring& exclude) :
        includeFilter(include),
        excludeFilter(exclude) {}

    FilterConfig() : //construct with default values
        includeFilter(DefaultStr("*")) {}

    Zstring includeFilter;
    Zstring excludeFilter;

    bool operator==(const FilterConfig& other) const
    {
        return includeFilter == other.includeFilter &&
               excludeFilter == other.excludeFilter;
    }
};


struct FolderPairEnh //enhanced folder pairs with (optional) alternate configuration
{
    FolderPairEnh() {}

    FolderPairEnh(const Zstring& leftDir,
                  const Zstring& rightDir,
                  const boost::shared_ptr<const AlternateSyncConfig>& syncConfig,
                  const FilterConfig& filter) :
        leftDirectory(leftDir),
        rightDirectory(rightDir) ,
        altSyncConfig(syncConfig),
        localFilter(filter) {}

    Zstring leftDirectory;
    Zstring rightDirectory;

    boost::shared_ptr<const AlternateSyncConfig> altSyncConfig; //optional
    FilterConfig localFilter;

    bool operator==(const FolderPairEnh& other) const
    {
        return leftDirectory  == other.leftDirectory  &&
               rightDirectory == other.rightDirectory &&

               (altSyncConfig.get() && other.altSyncConfig.get() ?
                *altSyncConfig == *other.altSyncConfig :
                altSyncConfig.get() == other.altSyncConfig.get()) &&

               localFilter == other.localFilter;
    }
};


struct MainConfiguration
{
    MainConfiguration() :
        compareVar(CMP_BY_TIME_SIZE),
        processSymlinks(false),
        traverseDirectorySymlinks(true),
        copyFileSymlinks(false),
        globalFilter(DefaultStr("*"), standardExcludeFilter()),
        handleDeletion(MOVE_TO_RECYCLE_BIN) {}

    FolderPairEnh firstPair; //there needs to be at least one pair!
    std::vector<FolderPairEnh> additionalPairs;

    //Compare setting
    CompareVariant compareVar;

    bool processSymlinks; //include Symbolic links into file listing at all?
    bool traverseDirectorySymlinks; //traverse dir symlinks <=> not copying symlink
    bool copyFileSymlinks; //copy symbolic link instead of target file

    //Synchronisation settings
    SyncConfiguration syncConfiguration;

    //GLOBAL filter settings
    FilterConfig globalFilter;

    //misc options
    HiddenSettings hidden; //settings not visible on GUI

    DeletionPolicy handleDeletion; //use Recycle, delete permanently or move to user-defined location
    wxString customDeletionDirectory;

    wxString getSyncVariantName();

    bool operator==(const MainConfiguration& other) const
    {
        return firstPair         == other.firstPair         &&
               additionalPairs   == other.additionalPairs   &&
               compareVar        == other.compareVar        &&
               processSymlinks   == other.processSymlinks   &&
               traverseDirectorySymlinks == other.traverseDirectorySymlinks &&
               copyFileSymlinks  == other.copyFileSymlinks  &&
               syncConfiguration == other.syncConfiguration &&
               globalFilter      == other.globalFilter      &&
               hidden            == other.hidden            &&
               handleDeletion    == other.handleDeletion    &&
               customDeletionDirectory == other.customDeletionDirectory;
    }
};
}

#endif // FREEFILESYNC_H_INCLUDED
