// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef COMPARISON_H_INCLUDED
#define COMPARISON_H_INCLUDED

#include "file_hierarchy.h"
#include "lib/process_xml.h"
#include "process_callback.h"
#include "lib/norm_filter.h"
#include "lib/lock_holder.h"


namespace zen
{
struct FolderPairCfg
{
    FolderPairCfg(const Zstring& folderPathPhraseLeft,
                  const Zstring& folderPathPhraseRight,
                  CompareVariant cmpVar,
                  SymLinkHandling handleSymlinksIn,
                  int fileTimeToleranceIn,
                  unsigned int optTimeShiftHoursIn,
                  const NormalizedFilter& filterIn,
                  const DirectionConfig& directCfg) :
        folderPathPhraseLeft_ (folderPathPhraseLeft),
        folderPathPhraseRight_(folderPathPhraseRight),
        compareVar(cmpVar),
        handleSymlinks(handleSymlinksIn),
        fileTimeTolerance(fileTimeToleranceIn),
        optTimeShiftHours(optTimeShiftHoursIn),
        filter(filterIn),
        directionCfg(directCfg) {}

    Zstring folderPathPhraseLeft_;  //unresolved directory names as entered by user!
    Zstring folderPathPhraseRight_; //

    CompareVariant compareVar;
    SymLinkHandling handleSymlinks;
    int fileTimeTolerance;
    unsigned int optTimeShiftHours;

    NormalizedFilter filter;

    DirectionConfig directionCfg;
};

std::vector<FolderPairCfg> extractCompareCfg(const MainConfiguration& mainCfg, int fileTimeTolerance); //fill FolderPairCfg and resolve folder pairs

//FFS core routine:
FolderComparison compare(xmlAccess::OptionalDialogs& warnings,
                         bool allowUserInteraction,
                         bool runWithBackgroundPriority,
                         bool createDirLocks,
                         std::unique_ptr<LockHolder>& dirLocks, //out
                         const std::vector<FolderPairCfg>& cfgList,
                         ProcessCallback& callback);
}

#endif // COMPARISON_H_INCLUDED
