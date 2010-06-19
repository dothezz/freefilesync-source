// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef COMPARISON_H_INCLUDED
#define COMPARISON_H_INCLUDED

#include "fileHierarchy.h"
#include "library/processXml.h"
#include "library/statusHandler.h"
#include "structures.h"


namespace FreeFileSync
{

struct FolderPairCfg
{
    FolderPairCfg(const Zstring& leftDir,
                  const Zstring& rightDir,
                  const BaseFilter::FilterRef& filterIn,
                  const SyncConfiguration& syncCfg) :
        leftDirectory(leftDir),
        rightDirectory(rightDir),
        filter(filterIn),
        syncConfiguration(syncCfg) {}

    Zstring leftDirectory;
    Zstring rightDirectory;

    BaseFilter::FilterRef filter; //filter interface: always bound by design!
    SyncConfiguration syncConfiguration;
};

std::vector<FolderPairCfg> extractCompareCfg(const MainConfiguration& mainCfg);


//class handling comparison process
class CompareProcess
{
public:
    CompareProcess(SymLinkHandling handleSymlinks,
                   size_t fileTimeTol,
                   bool ignoreOneHourDiff,
                   xmlAccess::OptionalDialogs& warnings,
                   StatusHandler* handler);

    void startCompareProcess(const std::vector<FolderPairCfg>& directoryPairs,
                             const CompareVariant cmpVar,
                             FolderComparison& output);

private:
    void compareByTimeSize(const std::vector<FolderPairCfg>& directoryPairsFormatted, FolderComparison& output) const;
    void compareByContent( const std::vector<FolderPairCfg>& directoryPairsFormatted, FolderComparison& output) const;

    //create comparison result table and fill category except for files existing on both sides
    void performBaseComparison(BaseDirMapping& output, std::vector<FileMapping*>& appendUndefined) const;

    void categorizeSymlink(SymLinkMapping* linkObj) const;

    //buffer accesses to the same directories; useful when multiple folder pairs are used
    class DirectoryBuffer;
    boost::shared_ptr<DirectoryBuffer> directoryBuffer; //std::auto_ptr does not work with forward declarations (Or we need a non-inline ~CompareProcess())!

    const size_t fileTimeTolerance; //max allowed file time deviation
    const bool ignoreOneHourDifference;

    xmlAccess::OptionalDialogs& m_warnings;

    StatusHandler* const statusUpdater;
    const Zstring txtComparingContentOfFiles;
};
}

#endif // COMPARISON_H_INCLUDED
