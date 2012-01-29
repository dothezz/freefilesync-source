// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef COMPARISON_H_INCLUDED
#define COMPARISON_H_INCLUDED

#include <zen/process_status.h>
#include "file_hierarchy.h"
#include "lib/process_xml.h"
#include "lib/status_handler.h"
#include "structures.h"
#include "lib/norm_filter.h"
#include "lib/parallel_scan.h"


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

    Zstring leftDirectoryFmt;  //resolved folder pairs!!!
    Zstring rightDirectoryFmt; //

    CompareVariant compareVar;
    SymLinkHandling handleSymlinks;

    NormalizedFilter filter;

    DirectionConfig directionCfg;
};

std::vector<FolderPairCfg> extractCompareCfg(const MainConfiguration& mainCfg); //fill FolderPairCfg and resolve folder pairs


//runComparison

//class handling comparison process
class CompareProcess
{
public:
    CompareProcess(size_t fileTimeTol,
                   xmlAccess::OptionalDialogs& warnings,
                   bool allowUserInteraction,
                   bool runWithBackgroundPriority,
                   ProcessCallback& handler);

    void startCompareProcess(const std::vector<FolderPairCfg>& cfgList, FolderComparison& output);

private:
    CompareProcess(const CompareProcess&);
    CompareProcess& operator=(const CompareProcess&);

    //create comparison result table and fill category except for files existing on both sides: undefinedFiles and undefinedLinks are appended!
    void categorizeSymlinkByTime(SymLinkMapping& linkObj) const;
    void categorizeSymlinkByContent(SymLinkMapping& linkObj) const;

    void compareByTimeSize(const FolderPairCfg& fpConfig, BaseDirMapping& output);
    void compareByContent(std::vector<std::pair<FolderPairCfg, BaseDirMapping*>>& workLoad);

    void performComparison(const FolderPairCfg& fpCfg,
                           BaseDirMapping& output,
                           std::vector<FileMapping*>& undefinedFiles,
                           std::vector<SymLinkMapping*>& undefinedLinks);

    std::map<DirectoryKey, DirectoryValue> directoryBuffer; //contains only *existing* directories

    const size_t fileTimeTolerance; //max allowed file time deviation

    xmlAccess::OptionalDialogs& m_warnings;

    const bool allowUserInteraction_;
    ProcessCallback& procCallback;
    std::unique_ptr<ScheduleForBackgroundProcessing> procBackground;
};
}

#endif // COMPARISON_H_INCLUDED
