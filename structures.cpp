// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "structures.h"
#include "shared/i18n.h"
#include <iterator>
#include <stdexcept>

using namespace zen;


wxString zen::getVariantName(CompareVariant var)
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


wxString zen::getVariantName(SyncConfig::Variant var)
{
    switch (var)
    {
        case SyncConfig::AUTOMATIC:
            return _("<Automatic>");
        case SyncConfig::MIRROR:
            return _("Mirror ->>");
        case SyncConfig::UPDATE:
            return _("Update ->");
        case SyncConfig::CUSTOM:
            return _("Custom");
    }
    return _("Error");
}


DirectionSet zen::extractDirections(const SyncConfig& cfg)
{
    DirectionSet output;
    switch (cfg.var)
    {
        case SyncConfig::AUTOMATIC:
            throw std::logic_error("there are no predefined directions for automatic mode!");

        case SyncConfig::MIRROR:
            output.exLeftSideOnly  = SYNC_DIR_RIGHT;
            output.exRightSideOnly = SYNC_DIR_RIGHT;
            output.leftNewer       = SYNC_DIR_RIGHT;
            output.rightNewer      = SYNC_DIR_RIGHT;
            output.different       = SYNC_DIR_RIGHT;
            output.conflict        = SYNC_DIR_RIGHT;
            break;

        case SyncConfig::UPDATE:
            output.exLeftSideOnly  = SYNC_DIR_RIGHT;
            output.exRightSideOnly = SYNC_DIR_NONE;
            output.leftNewer       = SYNC_DIR_RIGHT;
            output.rightNewer      = SYNC_DIR_NONE;
            output.different       = SYNC_DIR_RIGHT;
            output.conflict        = SYNC_DIR_NONE;
            break;

        case SyncConfig::CUSTOM:
            output = cfg.custom;
            break;
    }
    return output;
}


DirectionSet zen::getTwoWaySet()
{
    DirectionSet output;
    output.exLeftSideOnly  = SYNC_DIR_RIGHT;
    output.exRightSideOnly = SYNC_DIR_LEFT;
    output.leftNewer       = SYNC_DIR_RIGHT;
    output.rightNewer      = SYNC_DIR_LEFT;
    output.different       = SYNC_DIR_NONE;
    output.conflict        = SYNC_DIR_NONE;
    return output;
}


wxString MainConfiguration::getSyncVariantName()
{
    const SyncConfig::Variant firstVariant = firstPair.altSyncConfig.get() ?
                                             firstPair.altSyncConfig->syncConfiguration.var :
                                             syncConfiguration.var; //fallback to main sync cfg

    //test if there's a deviating variant within the additional folder pairs
    for (std::vector<FolderPairEnh>::const_iterator i = additionalPairs.begin(); i != additionalPairs.end(); ++i)
    {
        const SyncConfig::Variant thisVariant = i->altSyncConfig.get() ?
                                                i->altSyncConfig->syncConfiguration.var :
                                                syncConfiguration.var;

        if (thisVariant != firstVariant)
            return _("Multiple...");
    }

    //seems to be all in sync...
    return getVariantName(firstVariant);
}


wxString zen::getDescription(CompareFilesResult cmpRes)
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
        case FILE_DIFFERENT_METADATA:
            return _("Equal files/folders that differ in attributes only");
        case FILE_CONFLICT:
            return _("Conflicts/files that cannot be categorized");
    }

    assert(false);
    return wxEmptyString;
}


wxString zen::getSymbol(CompareFilesResult cmpRes)
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
        case FILE_DIFFERENT_METADATA:
            return wxT("\\/\\->");
    }

    assert(false);
    return wxEmptyString;
}


wxString zen::getDescription(SyncOperation op)
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
        case SO_COPY_METADATA_TO_LEFT:
            return _("Copy attributes only from right to left");
        case SO_COPY_METADATA_TO_RIGHT:
            return _("Copy attributes only from left to right");
        case SO_UNRESOLVED_CONFLICT: //not used on GUI, but in .csv
            return _("Conflicts/files that cannot be categorized");
    };

    assert(false);
    return wxEmptyString;
}


wxString zen::getSymbol(SyncOperation op)
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
        case SO_COPY_METADATA_TO_LEFT:
            return wxT("<-");
        case SO_OVERWRITE_RIGHT:
        case SO_COPY_METADATA_TO_RIGHT:
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


namespace
{
assert_static(std::numeric_limits<zen:: Int64>::is_specialized);
assert_static(std::numeric_limits<zen::UInt64>::is_specialized);

zen::Int64 resolve(size_t value, UnitTime unit, zen::Int64 defaultVal)
{
    double out = value;
    switch (unit)
    {
        case UTIME_NONE:
            return defaultVal;
        case UTIME_SEC:
            return zen::Int64(value);
        case UTIME_MIN:
            out *= 60;
            break;
        case UTIME_HOUR:
            out *= 3600;
            break;
        case UTIME_DAY:
            out *= 24 * 3600;
            break;
    }
    return out >= to<double>(std::numeric_limits<zen::Int64>::max()) ? //prevent overflow!!!
           std::numeric_limits<zen::Int64>::max() :
           zen::Int64(out);
}

zen::UInt64 resolve(size_t value, UnitSize unit, zen::UInt64 defaultVal)
{
    double out = value;
    switch (unit)
    {
        case USIZE_NONE:
            return defaultVal;
        case USIZE_BYTE:
            return value;
        case USIZE_KB:
            out *= 1024;
            break;
        case USIZE_MB:
            out *= 1024 * 1024;
            break;
    }
    return out >= to<double>(std::numeric_limits<zen::UInt64>::max()) ? //prevent overflow!!!
           std::numeric_limits<zen::UInt64>::max() :
           zen::UInt64(out);
}
}

void zen::resolveUnits(size_t timeSpan, UnitTime unitTimeSpan,
                       size_t sizeMin,  UnitSize unitSizeMin,
                       size_t sizeMax,  UnitSize unitSizeMax,
                       zen::Int64&  timeSpanSec, //unit: seconds
                       zen::UInt64& sizeMinBy,   //unit: bytes
                       zen::UInt64& sizeMaxBy)   //unit: bytes
{
    timeSpanSec = resolve(timeSpan, unitTimeSpan, std::numeric_limits<zen::Int64>::max());
    sizeMinBy   = resolve(sizeMin,  unitSizeMin, 0U);
    sizeMaxBy   = resolve(sizeMax,  unitSizeMax, std::numeric_limits<zen::UInt64>::max());
}


namespace
{
bool sameFilter(const std::vector<FolderPairEnh>& folderPairs)
{
    if (folderPairs.empty())
        return true;

    for (std::vector<FolderPairEnh>::const_iterator fp = folderPairs.begin(); fp != folderPairs.end(); ++fp)
        if (!(fp->localFilter == folderPairs[0].localFilter))
            return false;

    return true;
}


bool isEmpty(const FolderPairEnh& fp)
{
    return fp == FolderPairEnh();
}


FilterConfig mergeFilterConfig(const FilterConfig& global, const FilterConfig& local)
{
    FilterConfig out = local;

    //hard filter

    //pragmatism: if both global and local include filter contain data, only local filter is preserved
    if (out.includeFilter == FilterConfig().includeFilter)
        out.includeFilter = global.includeFilter;

    out.excludeFilter.Trim(true, false);
    out.excludeFilter = global.excludeFilter + Zstr("\n") + out.excludeFilter;
    out.excludeFilter.Trim(true, false);

    //soft filter
    zen::Int64  locTimeSpanSec;
    zen::UInt64 locSizeMinBy;
    zen::UInt64 locSizeMaxBy;
    zen::resolveUnits(out.timeSpan, out.unitTimeSpan,
                      out.sizeMin,  out.unitSizeMin,
                      out.sizeMax,  out.unitSizeMax,
                      locTimeSpanSec, //unit: seconds
                      locSizeMinBy,   //unit: bytes
                      locSizeMaxBy);   //unit: bytes

    //soft filter
    zen::Int64  gloTimeSpanSec;
    zen::UInt64 gloSizeMinBy;
    zen::UInt64 gloSizeMaxBy;
    zen::resolveUnits(global.timeSpan, global.unitTimeSpan,
                      global.sizeMin,  global.unitSizeMin,
                      global.sizeMax,  global.unitSizeMax,
                      gloTimeSpanSec, //unit: seconds
                      gloSizeMinBy,   //unit: bytes
                      gloSizeMaxBy);   //unit: bytes

    if (gloTimeSpanSec < locTimeSpanSec)
    {
        out.timeSpan     = global.timeSpan;
        out.unitTimeSpan = global.unitTimeSpan;
    }
    if (gloSizeMinBy > locSizeMinBy)
    {
        out.sizeMin     = global.sizeMin;
        out.unitSizeMin = global.unitSizeMin;
    }
    if (gloSizeMaxBy < locSizeMaxBy)
    {
        out.sizeMax     = global.sizeMax;
        out.unitSizeMax = global.unitSizeMax;
    }
    return out;
}
}


zen::MainConfiguration zen::merge(const std::vector<MainConfiguration>& mainCfgs)
{
    assert(!mainCfgs.empty());
    if (mainCfgs.empty())
        return zen::MainConfiguration();

    if (mainCfgs.size() == 1) //mergeConfigFilesImpl relies on this!
        return mainCfgs[0];   //

    //merge folder pair config
    std::vector<FolderPairEnh> fpMerged;
    for (std::vector<MainConfiguration>::const_iterator i = mainCfgs.begin(); i != mainCfgs.end(); ++i)
    {
        std::vector<FolderPairEnh> fpTmp;

        //list non-empty local configurations
        if (!isEmpty(i->firstPair)) fpTmp.push_back(i->firstPair);
        std::remove_copy_if(i->additionalPairs.begin(), i->additionalPairs.end(), std::back_inserter(fpTmp), &isEmpty);

        //move all configuration down to item level
        for (std::vector<FolderPairEnh>::iterator fp = fpTmp.begin(); fp != fpTmp.end(); ++fp)
        {
            if (!fp->altSyncConfig.get())
                fp->altSyncConfig.reset(
                    new AlternateSyncConfig(i->syncConfiguration,
                                            i->handleDeletion,
                                            i->customDeletionDirectory));

            fp->localFilter = mergeFilterConfig(i->globalFilter, fp->localFilter);
        }

        fpMerged.insert(fpMerged.end(), fpTmp.begin(), fpTmp.end());
    }

    if (fpMerged.empty())
        return zen::MainConfiguration();

    //optimization: remove redundant configuration
    FilterConfig newGlobalFilter;

    const bool equalFilters = sameFilter(fpMerged);
    if (equalFilters)
        newGlobalFilter = fpMerged[0].localFilter;

    for (std::vector<FolderPairEnh>::iterator fp = fpMerged.begin(); fp != fpMerged.end(); ++fp)
    {
        //if local config matches output global config we don't need local one
        if (fp->altSyncConfig &&
            *fp->altSyncConfig == AlternateSyncConfig(mainCfgs[0].syncConfiguration,
                                                      mainCfgs[0].handleDeletion,
                                                      mainCfgs[0].customDeletionDirectory))
            fp->altSyncConfig.reset();

        if (equalFilters) //use global filter in this case
            fp->localFilter = FilterConfig();
    }

    //final assembly
    zen::MainConfiguration cfgOut = mainCfgs[0];
    cfgOut.globalFilter            = newGlobalFilter;
    cfgOut.firstPair               = fpMerged[0];
    cfgOut.additionalPairs.assign(fpMerged.begin() + 1, fpMerged.end());

    return cfgOut;
}
