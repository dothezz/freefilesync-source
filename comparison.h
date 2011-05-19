// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef COMPARISON_H_INCLUDED
#define COMPARISON_H_INCLUDED

#include "file_hierarchy.h"
#include "library/process_xml.h"
#include "library/status_handler.h"
#include "structures.h"
#include "shared/disable_standby.h"
#include "library/norm_filter.h"


namespace zen
{
struct FolderPairCfg
{
    FolderPairCfg(const Zstring& leftDir, //must be formatted folder pairs!
                  const Zstring& rightDir,
                  const NormalizedFilter& filterIn,
                  const SyncConfig& syncCfg) :
        leftDirectoryFmt(leftDir),
        rightDirectoryFmt(rightDir),
        filter(filterIn),
        syncConfiguration(syncCfg) {}

    Zstring leftDirectoryFmt;  //resolved folder pairs!!!
    Zstring rightDirectoryFmt; //
    NormalizedFilter filter;
    SyncConfig syncConfiguration;
};

std::vector<FolderPairCfg> extractCompareCfg(const MainConfiguration& mainCfg); //fill FolderPairCfg and resolve folder pairs


//class handling comparison process
class CompareProcess
{
public:
    CompareProcess(SymLinkHandling handleSymlinks,
                   size_t fileTimeTol,
                   xmlAccess::OptionalDialogs& warnings,
                   ProcessCallback& handler);

    void startCompareProcess(const std::vector<FolderPairCfg>& directoryPairs,
                             const CompareVariant cmpVar,
                             FolderComparison& output);

private:
    void compareByTimeSize(const std::vector<FolderPairCfg>& directoryPairsFormatted, FolderComparison& output) const;
    void compareByContent( const std::vector<FolderPairCfg>& directoryPairsFormatted, FolderComparison& output) const;

    //create comparison result table and fill category except for files existing on both sides: undefinedFiles and undefinedLinks are appended!
    void performBaseComparison(BaseDirMapping& output, std::vector<FileMapping*>& undefinedFiles, std::vector<SymLinkMapping*>& undefinedLinks) const;

    void categorizeSymlinkByTime(SymLinkMapping* linkObj) const;
    void categorizeSymlinkByContent(SymLinkMapping* linkObj) const;

    //buffer accesses to the same directories; useful when multiple folder pairs are used
    class DirectoryBuffer;
    boost::shared_ptr<DirectoryBuffer> directoryBuffer; //std::auto_ptr does not work with forward declarations (Or we need a non-inline ~CompareProcess())!

    const size_t fileTimeTolerance; //max allowed file time deviation

    xmlAccess::OptionalDialogs& m_warnings;

    ProcessCallback& procCallback;
    const Zstring txtComparingContentOfFiles;
};
}

#endif // COMPARISON_H_INCLUDED
