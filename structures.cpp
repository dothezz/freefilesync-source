// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "structures.h"
#include <iterator>
#include <stdexcept>
#include <ctime>
#include <zen/i18n.h>
#include <zen/time.h>

using namespace zen;


std::wstring zen::getVariantName(CompareVariant var)
{
    switch (var)
    {
        case CMP_BY_CONTENT:
            return _("File content");
        case CMP_BY_TIME_SIZE:
            return _("File time and size");
    }
    assert(false);
    return _("Error");
}


std::wstring zen::getVariantName(DirectionConfig::Variant var)
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
    assert(false);
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


std::wstring MainConfiguration::getCompVariantName() const
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


std::wstring MainConfiguration::getSyncVariantName() const
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


std::wstring zen::getSymbol(CompareFilesResult cmpRes)
{
    switch (cmpRes)
    {
        case FILE_LEFT_SIDE_ONLY:
            return L"only <-";
        case FILE_RIGHT_SIDE_ONLY:
            return L"only ->";
        case FILE_LEFT_NEWER:
            return L"newer <-";
        case FILE_RIGHT_NEWER:
            return L"newer ->";
        case FILE_DIFFERENT:
            return L"!=";
        case FILE_EQUAL:
        case FILE_DIFFERENT_METADATA: //= sub-category of equal!
            return L"'=="; //added quotation mark to avoid error in Excel cell when exporting to *.cvs
        case FILE_CONFLICT:
            return L"conflict";
    }
    assert(false);
    return std::wstring();
}


std::wstring zen::getSymbol(SyncOperation op)
{
    switch (op)
    {
        case SO_CREATE_NEW_LEFT:
            return L"create <-";
        case SO_CREATE_NEW_RIGHT:
            return L"create ->";
        case SO_DELETE_LEFT:
            return L"delete <-";
        case SO_DELETE_RIGHT:
            return L"delete ->";
        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
            return L"move <-";
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_RIGHT_TARGET:
            return L"move ->";
        case SO_OVERWRITE_LEFT:
        case SO_COPY_METADATA_TO_LEFT:
            return L"update <-";
        case SO_OVERWRITE_RIGHT:
        case SO_COPY_METADATA_TO_RIGHT:
            return L"update ->";
        case SO_DO_NOTHING:
            return L" -";
        case SO_EQUAL:
            return L"'=="; //added quotation mark to avoid error in Excel cell when exporting to *.cvs
        case SO_UNRESOLVED_CONFLICT:
            return L"conflict";
    };
    assert(false);
    return std::wstring();
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
                        sizeof(firstDayOfWeek) / sizeof(TCHAR)) > 0) //__in   int cchData
    {
        assert(firstDayOfWeek <= 6);
        return (dayOfWeek + (7 - firstDayOfWeek)) % 7;
    }
    else //default
#endif
        return dayOfWeek; //let all weeks begin with monday
}


Int64 resolve(size_t value, UnitTime unit, Int64 defaultVal)
{
    TimeComp locTimeStruc = zen::localTime();

    switch (unit)
    {
        case UTIME_NONE:
            return defaultVal;

        case UTIME_TODAY:
            locTimeStruc.second = 0; //0-61
            locTimeStruc.minute = 0; //0-59
            locTimeStruc.hour   = 0; //0-23
            return localToTimeT(locTimeStruc); //convert local time back to UTC

            //case UTIME_THIS_WEEK:
            //{
            //    localTimeFmt->tm_sec  = 0; //0-61
            //    localTimeFmt->tm_min  = 0; //0-59
            //    localTimeFmt->tm_hour = 0; //0-23
            //    const time_t timeFrom = ::mktime(localTimeFmt);

            //    int dayOfWeek = (localTimeFmt->tm_wday + 6) % 7; //tm_wday := days since Sunday	0-6
            //    // +6 == -1 in Z_7

            //    return Int64(timeFrom) - daysSinceBeginOfWeek(dayOfWeek) * 24 * 3600;
            //}

        case UTIME_THIS_MONTH:
            locTimeStruc.second = 0; //0-61
            locTimeStruc.minute = 0; //0-59
            locTimeStruc.hour   = 0; //0-23
            locTimeStruc.day    = 1; //1-31
            return localToTimeT(locTimeStruc);

        case UTIME_THIS_YEAR:
            locTimeStruc.second = 0; //0-61
            locTimeStruc.minute = 0; //0-59
            locTimeStruc.hour   = 0; //0-23
            locTimeStruc.day    = 1; //1-31
            locTimeStruc.month  = 1; //1-12
            return localToTimeT(locTimeStruc);

        case UTIME_LAST_X_DAYS:
            locTimeStruc.second = 0; //0-61
            locTimeStruc.minute = 0; //0-59
            locTimeStruc.hour   = 0; //0-23
            return localToTimeT(locTimeStruc) - Int64(value) * 24 * 3600;
    }

    assert(false);
    return localToTimeT(locTimeStruc);
}


UInt64 resolve(size_t value, UnitSize unit, UInt64 defaultVal)
{
    double out = 0;
    switch (unit)
    {
        case USIZE_NONE:
            return defaultVal;
        case USIZE_BYTE:
            return value;
        case USIZE_KB:
            out = 1024.0 * value;
            break;
        case USIZE_MB:
            out = 1024 * 1024.0 * value;
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

    trim(out.excludeFilter, true, false);
    out.excludeFilter = global.excludeFilter + Zstr("\n") + out.excludeFilter;
    trim(out.excludeFilter, true, false);

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
    for (auto iterMain = mainCfgs.begin(); iterMain != mainCfgs.end(); ++iterMain)
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
    for (auto fp = fpMerged.begin(); fp != fpMerged.end(); ++fp)
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
    cfgOut.cmpConfig    = cmpCfgHead;
    cfgOut.syncCfg      = syncCfgHead;
    cfgOut.globalFilter = globalFilter;
    cfgOut.firstPair    = fpMerged[0];
    cfgOut.additionalPairs.assign(fpMerged.begin() + 1, fpMerged.end());
    cfgOut.onCompletion = mainCfgs[0].onCompletion;

    return cfgOut;
}
