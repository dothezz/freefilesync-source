#ifndef COMPARISON_H_INCLUDED
#define COMPARISON_H_INCLUDED

#include "structures.h"
#include "library/processXml.h"

class StatusHandler;
class DirectoryDescrBuffer;

namespace FreeFileSync
{
    class FilterProcess;

    //class handling comparison process
    class CompareProcess
    {
    public:
        CompareProcess(const bool traverseSymLinks,
                       const unsigned fileTimeTol,
                       const bool ignoreOneHourDiff,
                       xmlAccess::WarningMessages& warnings,
                       const FilterProcess* filter, //may be NULL
                       StatusHandler* handler);

        ~CompareProcess();

        void startCompareProcess(const std::vector<FolderPair>& directoryPairs,
                                 const CompareVariant cmpVar,
                                 const SyncConfiguration& config,
                                 FolderComparison& output);

    private:
        void compareByTimeSize(const std::vector<FolderPair>& directoryPairsFormatted, FolderComparison& output);

        void compareByContent(const std::vector<FolderPair>& directoryPairsFormatted, FolderComparison& output);

        //create comparison result table and fill relation except for files existing on both sides
        void performBaseComparison(const FolderPair& pair, FileComparison& output);


        void issueWarningInvalidDate(const Zstring& fileNameFull, const wxLongLong& utcTime);
        void issueWarningSameDateDiffSize(const FileCompareLine& cmpLine);
        void issueWarningChangeWithinHour(const FileCompareLine& cmpLine);


        //buffer accesses to the same directories; useful when multiple folder pairs are used
        DirectoryDescrBuffer* descriptionBuffer;

        const unsigned int fileTimeTolerance; //max allowed file time deviation
        const bool ignoreOneHourDifference;

        xmlAccess::WarningMessages& m_warnings;

        StatusHandler* statusUpdater;
        const Zstring txtComparingContentOfFiles;
    };
}

#endif // COMPARISON_H_INCLUDED
