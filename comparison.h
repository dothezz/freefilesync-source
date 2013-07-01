// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
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
    FolderPairCfg(const Zstring& leftDir, //must be formatted folder pairs!
                  const Zstring& rightDir,
                  CompareVariant cmpVar,
                  SymLinkHandling handleSymlinksIn,
                  const NormalizedFilter& filterIn,
                  const DirectionConfig& directCfg) :
        leftDirectoryFmt(leftDir),
        rightDirectoryFmt(rightDir),
        compareVar(cmpVar),
        handleSymlinks(handleSymlinksIn),
        filter(filterIn),
        directionCfg(directCfg) {}

    Zstring leftDirectoryFmt;  //resolved directory names!
    Zstring rightDirectoryFmt; //

    CompareVariant compareVar;
    SymLinkHandling handleSymlinks;

    NormalizedFilter filter;

    DirectionConfig directionCfg;
};

std::vector<FolderPairCfg> extractCompareCfg(const MainConfiguration& mainCfg); //fill FolderPairCfg and resolve folder pairs

//FFS core routine:
void compare(size_t fileTimeTolerance, //max allowed file time deviation
             xmlAccess::OptionalDialogs& warnings,
             bool allowUserInteraction,
             bool runWithBackgroundPriority,
             bool createDirLocks,
             std::unique_ptr<LockHolder>& dirLocks, //out
             const std::vector<FolderPairCfg>& cfgList,
             FolderComparison& output, //out
             ProcessCallback& callback);
}

#endif // COMPARISON_H_INCLUDED
