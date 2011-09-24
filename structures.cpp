// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "structures.h"
#include "shared/i18n.h"
#include <iterator>
#include <stdexcept>
#include <ctime>

using namespace zen;


wxString zen::getVariantName(CompareVariant var)
{
    switch (var)
    {
        case CMP_BY_CONTENT:
            return _("File content");
        case CMP_BY_TIME_SIZE:
            return _("File time and size");
    }

    assert(false);
    return wxEmptyString;
}


wxString zen::getVariantName(DirectionConfig::Variant var)
{
    switch (var)
    {
        case DirectionConfig::AUTOMATIC:
            return _("<Automatic>");
        case DirectionConfig::MIRROR:
            return _("Mirror ->>");
        case DirectionConfig::UPDATE:
            return _("Update ->");
        case DirectionConfig::CUSTOM:
            return _("Custom");
    }
    return _("Error");
}


DirectionSet zen::extractDirections(const DirectionConfig& cfg)
{
    DirectionSet output;
    switch (cfg.var)
    {
        case DirectionConfig::AUTOMATIC:
            throw std::logic_error("there are no predefined directions for automatic mode!");

        case DirectionConfig::MIRROR:
            output.exLeftSideOnly  = SYNC_DIR_RIGHT;
            output.exRightSideOnly = SYNC_DIR_RIGHT;
            output.leftNewer       = SYNC_DIR_RIGHT;
            output.rightNewer      = SYNC_DIR_RIGHT;
            output.different       = SYNC_DIR_RIGHT;
            output.conflict        = SYNC_DIR_RIGHT;
            break;

        case DirectionConfig::UPDATE:
            output.exLeftSideOnly  = SYNC_DIR_RIGHT;
            output.exRightSideOnly = SYNC_DIR_NONE;
            output.leftNewer       = SYNC_DIR_RIGHT;
            output.rightNewer      = SYNC_DIR_NONE;
            output.different       = SYNC_DIR_RIGHT;
            output.conflict        = SYNC_DIR_NONE;
            break;

        case DirectionConfig::CUSTOM:
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


wxString MainConfiguration::getCompVariantName() const
{
    const CompareVariant firstVariant = firstPair.altCmpConfig.get() ?
                                        firstPair.altCmpConfig->compareVar :
                                        cmpConfig.compareVar; //fallback to main sync cfg

    //test if there's a deviating variant within the additional folder pairs
    for (auto fp = additionalPairs.begin(); fp != additionalPairs.end(); ++fp)
    {
        const CompareVariant thisVariant = fp->altCmpConfig.get() ?
                                           fp->altCmpConfig->compareVar :
                                           cmpConfig.compareVar; //fallback to main sync cfg
        if (thisVariant != firstVariant)
            return _("Multiple...");
    }

    //seems to be all in sync...
    return getVariantName(firstVariant);
}


wxString MainConfiguration::getSyncVariantName() const
{
    const DirectionConfig::Variant firstVariant = firstPair.altSyncConfig.get() ?
                                                  firstPair.altSyncConfig->directionCfg.var :
                                                  syncCfg.directionCfg.var; //fallback to main sync cfg

    //test if there's a deviating variant within the additional folder pairs
    for (auto fp = additionalPairs.begin(); fp != additionalPairs.end(); ++fp)
    {
        const DirectionConfig::Variant thisVariant = fp->altSyncConfig.get() ?
                                                     fp->altSyncConfig->directionCfg.var :
                                                     syncCfg.directionCfg.var;
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
            return _("File/folder exists on left side only");
        case FILE_RIGHT_SIDE_ONLY:
            return _("File/folder exists on right side only");
        case FILE_LEFT_NEWER:
            return _("Left file is newer");
        case FILE_RIGHT_NEWER:
            return _("Right file is newer");
        case FILE_DIFFERENT:
            return _("Files have different content");
        case FILE_EQUAL:
            return _("Both sides are equal");
        case FILE_DIFFERENT_METADATA:
            return _("Files/folders differ in attributes only");
        case FILE_CONFLICT:
            return _("Conflict/file cannot be categorized");
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
            return _("Copy new file/folder to left");
        case SO_CREATE_NEW_RIGHT:
            return _("Copy new file/folder to right");
        case SO_DELETE_LEFT:
            return _("Delete left file/folder");
        case SO_DELETE_RIGHT:
            return _("Delete right file/folder");
        case SO_OVERWRITE_LEFT:
            return _("Overwrite left file/folder with right one");
        case SO_OVERWRITE_RIGHT:
            return _("Overwrite right file/folder with left one");
        case SO_DO_NOTHING:
            return _("Do nothing");
        case SO_EQUAL:
            return _("Both sides are equal");
        case SO_COPY_METADATA_TO_LEFT:
            return _("Copy file attributes only to left");
        case SO_COPY_METADATA_TO_RIGHT:
            return _("Copy file attributes only to right");
        case SO_UNRESOLVED_CONFLICT: //not used on GUI, but in .csv
            return _("Conflict/file cannot be categorized");
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


int daysSinceBeginOfWeek(int dayOfWeek) //0-6, 0=Monday, 6=Sunday
{
    assert(0 <= dayOfWeek && dayOfWeek <= 6);
#ifdef FFS_WIN
    DWORD firstDayOfWeek = 0;
    if (::GetLocaleInfo(LOCALE_USER_DEFAULT,                 //__in   LCID Locale,
                        LOCALE_IFIRSTDAYOFWEEK |             // first day of week specifier, 0-6, 0=Monday, 6=Sunday
                        LOCALE_RETURN_NUMBER,                //__in   LCTYPE LCType,
                        reinterpret_cast<LPTSTR>(&firstDayOfWeek),    //__out  LPTSTR lpLCData,
                        sizeof(firstDayOfWeek) / sizeof(TCHAR)) != 0) //__in   int cchData
    {
        assert(firstDayOfWeek <= 6);
        return (dayOfWeek + (7 - firstDayOfWeek)) % 7;
    }
    else //default
#endif
        return dayOfWeek; //let all weeks begin with monday
}

zen::Int64 resolve(size_t value, UnitTime unit, zen::Int64 defaultVal)
{
    const time_t utcTimeNow = ::time(NULL);
    struct tm* localTimeFmt = ::localtime (&utcTimeNow); //utc to local

    switch (unit)
    {
        case UTIME_NONE:
            return defaultVal;

            //        case UTIME_LAST_X_HOURS:
            //           return Int64(utcTimeNow) - Int64(value) * 3600;

        case UTIME_TODAY:
            localTimeFmt->tm_sec  = 0; //0-61
            localTimeFmt->tm_min  = 0; //0-59
            localTimeFmt->tm_hour = 0; //0-23
            return Int64(::mktime(localTimeFmt)); //convert local time back to UTC

        case UTIME_THIS_WEEK:
        {
            localTimeFmt->tm_sec  = 0; //0-61
            localTimeFmt->tm_min  = 0; //0-59
            localTimeFmt->tm_hour = 0; //0-23
            size_t timeFrom = ::mktime(localTimeFmt);

            int dayOfWeek = (localTimeFmt->tm_wday + 6) % 7; //tm_wday := days since Sunday	0-6
            // +6 == -1 in Z_7

            return Int64(timeFrom) - daysSinceBeginOfWeek(dayOfWeek) * 24 * 3600;
        }
        case UTIME_THIS_MONTH:
            localTimeFmt->tm_sec  = 0; //0-61
            localTimeFmt->tm_min  = 0; //0-59
            localTimeFmt->tm_hour = 0; //0-23
            localTimeFmt->tm_mday = 1; //1-31
            return Int64(::mktime(localTimeFmt)); //convert local time back to UTC

        case UTIME_THIS_YEAR:
            localTimeFmt->tm_sec  = 0; //0-61
            localTimeFmt->tm_min  = 0; //0-59
            localTimeFmt->tm_hour = 0; //0-23
            localTimeFmt->tm_mday = 1; //1-31
            localTimeFmt->tm_mon  = 0; //0-11
            return Int64(::mktime(localTimeFmt)); //convert local time back to UTC
    }

    assert(false);
    return utcTimeNow;
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
                       zen::Int64&  timeFrom,  //unit: UTC time, seconds
                       zen::UInt64& sizeMinBy, //unit: bytes
                       zen::UInt64& sizeMaxBy) //unit: bytes
{
    timeFrom  = resolve(timeSpan, unitTimeSpan, std::numeric_limits<Int64>::min());
    sizeMinBy = resolve(sizeMin,  unitSizeMin, 0U);
    sizeMaxBy = resolve(sizeMax,  unitSizeMax, std::numeric_limits<UInt64>::max());
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
    Int64  loctimeFrom;
    UInt64 locSizeMinBy;
    UInt64 locSizeMaxBy;
    resolveUnits(out.timeSpan, out.unitTimeSpan,
                 out.sizeMin,  out.unitSizeMin,
                 out.sizeMax,  out.unitSizeMax,
                 loctimeFrom,   //unit: UTC time, seconds
                 locSizeMinBy,  //unit: bytes
                 locSizeMaxBy); //unit: bytes

    //soft filter
    Int64  glotimeFrom;
    UInt64 gloSizeMinBy;
    UInt64 gloSizeMaxBy;
    resolveUnits(global.timeSpan, global.unitTimeSpan,
                 global.sizeMin,  global.unitSizeMin,
                 global.sizeMax,  global.unitSizeMax,
                 glotimeFrom,
                 gloSizeMinBy,
                 gloSizeMaxBy);

    if (glotimeFrom > loctimeFrom)
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


inline
bool isEmpty(const FolderPairEnh& fp)
{
    return fp == FolderPairEnh();
}
}


MainConfiguration zen::merge(const std::vector<MainConfiguration>& mainCfgs)
{
    assert(!mainCfgs.empty());
    if (mainCfgs.empty())
        return MainConfiguration();

    if (mainCfgs.size() == 1) //mergeConfigFilesImpl relies on this!
        return mainCfgs[0];   //

    //merge folder pair config
    std::vector<FolderPairEnh> fpMerged;
    for (std::vector<MainConfiguration>::const_iterator iterMain = mainCfgs.begin(); iterMain != mainCfgs.end(); ++iterMain)
    {
        std::vector<FolderPairEnh> fpTmp;

        //list non-empty local configurations
        if (!isEmpty(iterMain->firstPair))
            fpTmp.push_back(iterMain->firstPair);
        std::copy_if(iterMain->additionalPairs.begin(), iterMain->additionalPairs.end(), std::back_inserter(fpTmp),
        [](const FolderPairEnh& fp) { return !isEmpty(fp); });

        //move all configuration down to item level
        for (std::vector<FolderPairEnh>::iterator fp = fpTmp.begin(); fp != fpTmp.end(); ++fp)
        {
            if (!fp->altCmpConfig.get())
                fp->altCmpConfig = std::make_shared<CompConfig>(iterMain->cmpConfig);

            if (!fp->altSyncConfig.get())
                fp->altSyncConfig = std::make_shared<SyncConfig>(iterMain->syncCfg);

            fp->localFilter = mergeFilterConfig(iterMain->globalFilter, fp->localFilter);
        }

        fpMerged.insert(fpMerged.end(), fpTmp.begin(), fpTmp.end());
    }

    if (fpMerged.empty())
        return MainConfiguration();

    //optimization: remove redundant configuration

    //########################################################################################################################
    //find out which comparison and synchronization setting are used most often and use them as new "header"
    std::vector<std::pair<CompConfig, int>> cmpCfgStat;
    std::vector<std::pair<SyncConfig, int>> syncCfgStat;
    for (auto fp = fpMerged.begin(); fp != fpMerged.end(); ++fp) //rather inefficient algorithm, but it does not require a less-than operator!
    {
        {
            const CompConfig& cmpCfg = *fp->altCmpConfig;

            auto iter = std::find_if(cmpCfgStat.begin(), cmpCfgStat.end(),
            [&](const std::pair<CompConfig, int>& entry) { return entry.first == cmpCfg; });
            if (iter == cmpCfgStat.end())
                cmpCfgStat.push_back(std::make_pair(cmpCfg, 1));
            else
                ++(iter->second);
        }
        {
            const SyncConfig& syncCfg = *fp->altSyncConfig;

            auto iter = std::find_if(syncCfgStat.begin(), syncCfgStat.end(),
            [&](const std::pair<SyncConfig, int>& entry) { return entry.first == syncCfg; });
            if (iter == syncCfgStat.end())
                syncCfgStat.push_back(std::make_pair(syncCfg, 1));
            else
                ++(iter->second);
        }
    }

    //set most-used comparison and synchronization settions as new header options
    const CompConfig cmpCfgHead = cmpCfgStat.empty() ? CompConfig() :
                                  std::max_element(cmpCfgStat.begin(), cmpCfgStat.end(),
    [](const std::pair<CompConfig, int>& lhs, const std::pair<CompConfig, int>& rhs) { return lhs.second < rhs.second; })->first;

    const SyncConfig syncCfgHead = syncCfgStat.empty() ? SyncConfig() :
                                   std::max_element(syncCfgStat.begin(), syncCfgStat.end(),
    [](const std::pair<SyncConfig, int>& lhs, const std::pair<SyncConfig, int>& rhs) { return lhs.second < rhs.second; })->first;
    //########################################################################################################################

    FilterConfig globalFilter;
    const bool equalFilters = sameFilter(fpMerged);
    if (equalFilters)
        globalFilter = fpMerged[0].localFilter;

    //strip redundancy...
    for (std::vector<FolderPairEnh>::iterator fp = fpMerged.begin(); fp != fpMerged.end(); ++fp)
    {
        //if local config matches output global config we don't need local one
        if (fp->altCmpConfig &&
            *fp->altCmpConfig == cmpCfgHead)
            fp->altCmpConfig.reset();

        if (fp->altSyncConfig &&
            *fp->altSyncConfig == syncCfgHead)
            fp->altSyncConfig.reset();

        if (equalFilters) //use global filter in this case
            fp->localFilter = FilterConfig();
    }

    //final assembly
    zen::MainConfiguration cfgOut;
    cfgOut.cmpConfig = cmpCfgHead;
    cfgOut.syncCfg   = syncCfgHead;
    cfgOut.globalFilter = globalFilter;
    cfgOut.firstPair    = fpMerged[0];
    cfgOut.additionalPairs.assign(fpMerged.begin() + 1, fpMerged.end());

    return cfgOut;
}
