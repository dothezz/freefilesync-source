#ifndef COMPARISON_H_INCLUDED
#define COMPARISON_H_INCLUDED

#include "fileHierarchy.h"
#include "library/processXml.h"

class StatusHandler;

namespace FreeFileSync
{
    class FilterProcess;

    struct FolderPairCfg
    {
        FolderPairCfg(const Zstring& leftDir,
                      const Zstring& rightDir,
                      bool filterAct,
                      const wxString& include,
                      const wxString& exclude,
                      const SyncConfiguration& syncCfg) :
                leftDirectory(leftDir),
                rightDirectory(rightDir),
                filterIsActive(filterAct),
                includeFilter(include),
                excludeFilter(exclude),
                syncConfiguration(syncCfg) {}

        Zstring leftDirectory;
        Zstring rightDirectory;

        bool filterIsActive;
        wxString includeFilter;
        wxString excludeFilter;

        SyncConfiguration syncConfiguration;
    };
    std::vector<FolderPairCfg> extractCompareCfg(const MainConfiguration& mainCfg);


    //class handling comparison process
    class CompareProcess
    {
    public:
        CompareProcess(const bool traverseSymLinks,
                       const unsigned int fileTimeTol,
                       const bool ignoreOneHourDiff,
                       xmlAccess::OptionalDialogs& warnings,
                       StatusHandler* handler);

        void startCompareProcess(const std::vector<FolderPairCfg>& directoryPairs,
                                 const CompareVariant cmpVar,
                                 FolderComparison& output);

    private:
        void compareByTimeSize(const std::vector<FolderPairCfg>& directoryPairsFormatted, FolderComparison& output);

        void compareByContent(const std::vector<FolderPairCfg>& directoryPairsFormatted, FolderComparison& output);

        //create comparison result table and fill relation except for files existing on both sides
        void performBaseComparison(const FolderPairCfg& pair, BaseDirMapping& output, std::vector<FileMapping*>& appendUndefined);

        //buffer accesses to the same directories; useful when multiple folder pairs are used
        class DirectoryBuffer;
        boost::shared_ptr<DirectoryBuffer> directoryBuffer; //std::auto_ptr does not work with forward declarations!

        const unsigned int fileTimeTolerance; //max allowed file time deviation
        const bool ignoreOneHourDifference;

        xmlAccess::OptionalDialogs& m_warnings;

        StatusHandler* const statusUpdater;
        const Zstring txtComparingContentOfFiles;
    };
}

#endif // COMPARISON_H_INCLUDED
