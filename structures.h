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


    enum SyncDirectionCfg
    {
        SYNC_DIR_CFG_LEFT = 0,
        SYNC_DIR_CFG_RIGHT,
        SYNC_DIR_CFG_NONE
    };
    //attention make sure these /|\  \|/ two enums match!!!
    enum SyncDirection
    {
        SYNC_DIR_LEFT  = SYNC_DIR_CFG_LEFT,
        SYNC_DIR_RIGHT = SYNC_DIR_CFG_RIGHT,
        SYNC_DIR_NONE  = SYNC_DIR_CFG_NONE,
    };


    inline
    SyncDirection convertToSyncDirection(SyncDirectionCfg syncCfg)
    {
        return static_cast<SyncDirection>(syncCfg);
    }


    struct SyncConfiguration
    {
        SyncConfiguration() :
                exLeftSideOnly( SYNC_DIR_CFG_RIGHT),
                exRightSideOnly(SYNC_DIR_CFG_LEFT),
                leftNewer(      SYNC_DIR_CFG_RIGHT),
                rightNewer(     SYNC_DIR_CFG_LEFT),
                different(      SYNC_DIR_CFG_NONE),
                conflict(       SYNC_DIR_CFG_NONE) {}

        SyncDirectionCfg exLeftSideOnly;
        SyncDirectionCfg exRightSideOnly;
        SyncDirectionCfg leftNewer;
        SyncDirectionCfg rightNewer;
        SyncDirectionCfg different;
        SyncDirectionCfg conflict;

        bool operator==(const SyncConfiguration& other) const
        {
            return exLeftSideOnly  == other.exLeftSideOnly  &&
                   exRightSideOnly == other.exRightSideOnly &&
                   leftNewer       == other.leftNewer       &&
                   rightNewer      == other.rightNewer      &&
                   different       == other.different       &&
                   conflict        == other.conflict;
        }

        //get/set default configuration variants
        enum Variant
        {
            MIRROR,
            UPDATE,
            TWOWAY,
            CUSTOM
        };
        Variant getVariant() const;
        void setVariant(const Variant var);
    };


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
                traverseDirectorySymlinks(false),
                copyFileSymlinks(true),
                verifyFileCopy(false) {}

        unsigned int fileTimeTolerance; //max. allowed file time deviation
        bool traverseDirectorySymlinks;
        bool copyFileSymlinks; //copy symbolic link instead of target file
        bool verifyFileCopy;   //verify copied files

        bool operator==(const HiddenSettings& other) const
        {
            return fileTimeTolerance         == other.fileTimeTolerance         &&
                   traverseDirectorySymlinks == other.traverseDirectorySymlinks &&
                   copyFileSymlinks          == other.copyFileSymlinks          &&
                   verifyFileCopy            == other.verifyFileCopy;
        }
    };


    bool recycleBinExistsWrap();


    struct AlternateSyncConfig
    {
        AlternateSyncConfig(const SyncConfiguration& syncCfg,
                            const DeletionPolicy     handleDel,
                            const wxString&          customDelDir) :
                syncConfiguration(syncCfg),
                handleDeletion(handleDel),
                customDeletionDirectory(customDelDir) {};

        AlternateSyncConfig() :  //construct with default values
                handleDeletion(FreeFileSync::recycleBinExistsWrap() ? MOVE_TO_RECYCLE_BIN : DELETE_PERMANENTLY) {} //enable if OS supports it; else user will have to activate first and then get an error message

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


    struct AlternateFilter
    {
        AlternateFilter(const wxString& include, const wxString& exclude) :
                includeFilter(include),
                excludeFilter(exclude) {}

        AlternateFilter() : //construct with default values
                includeFilter(wxT("*")),        //include all files/folders
                excludeFilter(wxEmptyString) {} //exclude nothing

        wxString includeFilter;
        wxString excludeFilter;

        bool operator==(const AlternateFilter& other) const
        {
            return includeFilter == other.includeFilter &&
                   excludeFilter == other.excludeFilter;
        }
    };


    struct FolderPair
    {
        FolderPair(const Zstring& leftDir, const Zstring& rightDir) :
                leftDirectory(leftDir),
                rightDirectory(rightDir) {}

        Zstring leftDirectory;
        Zstring rightDirectory;

        bool operator==(const FolderPair& other) const
        {
            return leftDirectory  == other.leftDirectory &&
                   rightDirectory == other.rightDirectory;
        }
    };


    struct FolderPairEnh //enhanced folder pairs with (optional) alternate configuration
    {
        FolderPairEnh() {}

        FolderPairEnh(const Zstring& leftDir,
                      const Zstring& rightDir,
                      const boost::shared_ptr<const AlternateSyncConfig>& syncConfig,
                      const boost::shared_ptr<const AlternateFilter>& filter) :
                leftDirectory(leftDir),
                rightDirectory(rightDir) ,
                altSyncConfig(syncConfig),
                altFilter(filter) {}

        Zstring leftDirectory;
        Zstring rightDirectory;

        boost::shared_ptr<const AlternateSyncConfig> altSyncConfig; //optional
        boost::shared_ptr<const AlternateFilter> altFilter;         //optional

        bool operator==(const FolderPairEnh& other) const
        {
            return leftDirectory  == other.leftDirectory  &&
                   rightDirectory == other.rightDirectory &&

                   (altSyncConfig.get() && other.altSyncConfig.get() ?
                    *altSyncConfig == *other.altSyncConfig :
                    altSyncConfig.get() == other.altSyncConfig.get()) &&

                   (altFilter.get() && other.altFilter.get() ?
                    *altFilter == *other.altFilter :
                    altFilter.get() == other.altFilter.get());
        }
    };


    struct MainConfiguration
    {
        MainConfiguration() :
                mainFolderPair(Zstring(), Zstring()),
                compareVar(CMP_BY_TIME_SIZE),
                filterIsActive(false),        //do not filter by default
                includeFilter(wxT("*")),      //include all files/folders
                excludeFilter(wxEmptyString), //exclude nothing
                handleDeletion(FreeFileSync::recycleBinExistsWrap() ? MOVE_TO_RECYCLE_BIN : DELETE_PERMANENTLY) {} //enable if OS supports it; else user will have to activate first and then get an error message

        FolderPair mainFolderPair;
        std::vector<FolderPairEnh> additionalPairs;

        //Compare setting
        CompareVariant compareVar;

        //Synchronisation settings
        SyncConfiguration syncConfiguration;

        //Filter setting
        bool filterIsActive;
        wxString includeFilter;
        wxString excludeFilter;

        //misc options
        HiddenSettings hidden; //settings not visible on GUI

        DeletionPolicy handleDeletion; //use Recycle, delete permanently or move to user-defined location
        wxString customDeletionDirectory;

        wxString getSyncVariantName();

        bool operator==(const MainConfiguration& other) const
        {
            return mainFolderPair    == other.mainFolderPair    &&
                   additionalPairs   == other.additionalPairs   &&
                   compareVar        == other.compareVar        &&
                   syncConfiguration == other.syncConfiguration &&
                   filterIsActive    == other.filterIsActive    &&
                   includeFilter     == other.includeFilter     &&
                   excludeFilter     == other.excludeFilter     &&
                   hidden            == other.hidden            &&
                   handleDeletion    == other.handleDeletion    &&
                   customDeletionDirectory == other.customDeletionDirectory;
        }
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
        SO_DO_NOTHING,
        SO_UNRESOLVED_CONFLICT
    };

    wxString getDescription(SyncOperation op);
    wxString getSymbol(SyncOperation op);


    //Exception class used to abort the "compare" and "sync" process
    class AbortThisProcess {};
}

#endif // FREEFILESYNC_H_INCLUDED
