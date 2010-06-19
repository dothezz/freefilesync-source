// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "structures.h"
#include <wx/intl.h>
#include <stdexcept>

using namespace FreeFileSync;


Zstring FreeFileSync::standardExcludeFilter()
{
#ifdef FFS_WIN
    static Zstring exclude(wxT("\
\\System Volume Information\\\n\
\\RECYCLER\\\n\
\\RECYCLED\\\n\
\\$Recycle.Bin\\")); //exclude Recycle Bin
#elif defined FFS_LINUX
    static Zstring exclude; //exclude nothing
#endif
    return exclude;
}


wxString FreeFileSync::getVariantName(CompareVariant var)
{
    switch (var)
    {
    case CMP_BY_CONTENT:
        return _("File content");
    case CMP_BY_TIME_SIZE:
        return _("File size and date");
    }

    assert(false);
    return wxEmptyString;
}


wxString FreeFileSync::getVariantName(const SyncConfiguration& syncCfg)
{
    switch (getVariant(syncCfg))
    {
    case SyncConfiguration::AUTOMATIC:
        return _("<Automatic>");
    case SyncConfiguration::MIRROR:
        return _("Mirror ->>");
    case SyncConfiguration::UPDATE:
        return _("Update ->");
    case SyncConfiguration::CUSTOM:
        return _("Custom");
    }
    return _("Error");
}


void FreeFileSync::setTwoWay(SyncConfiguration& syncCfg) //helper method used by <Automatic> mode fallback to overwrite old with newer files
{
    syncCfg.automatic = false;
    syncCfg.exLeftSideOnly  = SYNC_DIR_RIGHT;
    syncCfg.exRightSideOnly = SYNC_DIR_LEFT;
    syncCfg.leftNewer       = SYNC_DIR_RIGHT;
    syncCfg.rightNewer      = SYNC_DIR_LEFT;
    syncCfg.different       = SYNC_DIR_NONE;
    syncCfg.conflict        = SYNC_DIR_NONE;
}


SyncConfiguration::Variant FreeFileSync::getVariant(const SyncConfiguration& syncCfg)
{
    if (syncCfg.automatic == true)
        return SyncConfiguration::AUTOMATIC;  //automatic mode

    if (    syncCfg.exLeftSideOnly  == SYNC_DIR_RIGHT &&
            syncCfg.exRightSideOnly == SYNC_DIR_RIGHT &&
            syncCfg.leftNewer       == SYNC_DIR_RIGHT &&
            syncCfg.rightNewer      == SYNC_DIR_RIGHT &&
            syncCfg.different       == SYNC_DIR_RIGHT &&
            syncCfg.conflict        == SYNC_DIR_RIGHT)
        return SyncConfiguration::MIRROR;    //one way ->

    else if (syncCfg.exLeftSideOnly  == SYNC_DIR_RIGHT &&
             syncCfg.exRightSideOnly == SYNC_DIR_NONE  &&
             syncCfg.leftNewer       == SYNC_DIR_RIGHT &&
             syncCfg.rightNewer      == SYNC_DIR_NONE  &&
             syncCfg.different       == SYNC_DIR_RIGHT  &&
             syncCfg.conflict        == SYNC_DIR_NONE)
        return SyncConfiguration::UPDATE;    //Update ->
    else
        return SyncConfiguration::CUSTOM;    //other
}


void FreeFileSync::setVariant(SyncConfiguration& syncCfg, const SyncConfiguration::Variant var)
{
    switch (var)
    {
    case SyncConfiguration::AUTOMATIC:
        syncCfg.automatic = true;
        break;
    case SyncConfiguration::MIRROR:
        syncCfg.automatic = false;
        syncCfg.exLeftSideOnly  = SYNC_DIR_RIGHT;
        syncCfg.exRightSideOnly = SYNC_DIR_RIGHT;
        syncCfg.leftNewer       = SYNC_DIR_RIGHT;
        syncCfg.rightNewer      = SYNC_DIR_RIGHT;
        syncCfg.different       = SYNC_DIR_RIGHT;
        syncCfg.conflict        = SYNC_DIR_RIGHT;
        break;
    case SyncConfiguration::UPDATE:
        syncCfg.automatic = false;
        syncCfg.exLeftSideOnly  = SYNC_DIR_RIGHT;
        syncCfg.exRightSideOnly = SYNC_DIR_NONE;
        syncCfg.leftNewer       = SYNC_DIR_RIGHT;
        syncCfg.rightNewer      = SYNC_DIR_NONE;
        syncCfg.different       = SYNC_DIR_RIGHT;
        syncCfg.conflict        = SYNC_DIR_NONE;
        break;
    case SyncConfiguration::CUSTOM:
        assert(false);
        break;
    }
}



wxString MainConfiguration::getSyncVariantName()
{
    const SyncConfiguration firstSyncCfg =
        firstPair.altSyncConfig.get() ?
        firstPair.altSyncConfig->syncConfiguration :
        syncConfiguration; //fallback to main sync cfg

    const SyncConfiguration::Variant firstVariant = getVariant(firstSyncCfg);

    //test if there's a deviating variant within the additional folder pairs
    for (std::vector<FolderPairEnh>::const_iterator i = additionalPairs.begin(); i != additionalPairs.end(); ++i)
    {
        const SyncConfiguration::Variant thisVariant =
            i->altSyncConfig.get() ?
            getVariant(i->altSyncConfig->syncConfiguration) :
            getVariant(syncConfiguration);

        if (thisVariant != firstVariant)
            return _("Multiple...");
    }

    //seems to be all in sync...
    return getVariantName(firstSyncCfg);
}


wxString FreeFileSync::getDescription(CompareFilesResult cmpRes)
{
    switch (cmpRes)
    {
    case FILE_LEFT_SIDE_ONLY:
        return _("Files/folders that exist on left side only");
    case FILE_RIGHT_SIDE_ONLY:
        return _("Files/folders that exist on right side only");
    case FILE_LEFT_NEWER:
        return _("Files that exist on both sides, left one is newer");
    case FILE_RIGHT_NEWER:
        return _("Files that exist on both sides, right one is newer");
    case FILE_DIFFERENT:
        return _("Files that have different content");
    case FILE_EQUAL:
        return _("Files that are equal on both sides");
    case FILE_CONFLICT:
        return _("Conflicts/files that cannot be categorized");
    }

    assert(false);
    return wxEmptyString;
}


wxString FreeFileSync::getSymbol(CompareFilesResult cmpRes)
{
    switch (cmpRes)
    {
    case FILE_LEFT_SIDE_ONLY:
        return wxT("<|");
    case FILE_RIGHT_SIDE_ONLY:
        return wxT("|>");
    case FILE_LEFT_NEWER:
        return wxT("<<");
    case FILE_RIGHT_NEWER:
        return wxT(">>");
    case FILE_DIFFERENT:
        return wxT("!=");
    case FILE_EQUAL:
        return wxT("'=="); //added quotation mark to avoid error in Excel cell when exporting to *.cvs
    case FILE_CONFLICT:
        return wxT("\\/\\->");
    }

    assert(false);
    return wxEmptyString;
}


wxString FreeFileSync::getDescription(SyncOperation op)
{
    switch (op)
    {
    case SO_CREATE_NEW_LEFT:
        return _("Copy from right to left");
    case SO_CREATE_NEW_RIGHT:
        return _("Copy from left to right");
    case SO_DELETE_LEFT:
        return _("Delete files/folders existing on left side only");
    case SO_DELETE_RIGHT:
        return _("Delete files/folders existing on right side only");
    case SO_OVERWRITE_LEFT:
        return _("Copy from right to left overwriting");
    case SO_OVERWRITE_RIGHT:
        return _("Copy from left to right overwriting");
    case SO_DO_NOTHING:
        return _("Do nothing");
    case SO_EQUAL:
        return _("Files that are equal on both sides");
    case SO_UNRESOLVED_CONFLICT:
        return _("Conflicts/files that cannot be categorized");
    };

    assert(false);
    return wxEmptyString;
}


wxString FreeFileSync::getSymbol(SyncOperation op)
{
    switch (op)
    {
    case SO_CREATE_NEW_LEFT:
        return wxT("*-");
    case SO_CREATE_NEW_RIGHT:
        return wxT("-*");
    case SO_DELETE_LEFT:
        return wxT("D-");
    case SO_DELETE_RIGHT:
        return wxT("-D");
    case SO_OVERWRITE_LEFT:
        return wxT("<-");
    case SO_OVERWRITE_RIGHT:
        return wxT("->");
    case SO_DO_NOTHING:
        return wxT(" -");
    case SO_EQUAL:
        return wxT("'=="); //added quotation mark to avoid error in Excel cell when exporting to *.cvs
    case SO_UNRESOLVED_CONFLICT:
        return wxT("\\/\\->");
    };

    assert(false);
    return wxEmptyString;
}
