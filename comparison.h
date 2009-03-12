#ifndef COMPARISON_H_INCLUDED
#define COMPARISON_H_INCLUDED

#include "FreeFileSync.h"
#include "library/statusHandler.h"

namespace FreeFileSync
{
    class DirectoryDescrBuffer;

    //class handling comparison process
    class CompareProcess
    {
    public:
        CompareProcess(const bool traverseSymLinks,
                       const bool handleDstOnFat32Drives,
                       bool& warningDependentFolders,
                       StatusHandler* handler);

        ~CompareProcess();

        void startCompareProcess(const std::vector<FolderPair>& directoryPairs,
                                 const CompareVariant cmpVar,
                                 FileCompareResult& output) throw(AbortThisProcess);

    private:
        void compareByTimeSize(const std::vector<FolderPair>& directoryPairsFormatted, FileCompareResult& output);

        void compareByContent(const std::vector<FolderPair>& directoryPairsFormatted, FileCompareResult& output);

        //create comparison result table and fill relation except for files existing on both sides
        void performBaseComparison(const FolderPair& pair, FileCompareResult& output);

        //buffer accesses to the same directories; useful when multiple folder pairs are used
        DirectoryDescrBuffer* descriptionBuffer;

        const bool traverseSymbolicLinks;
        const bool handleDstOnFat32;
        bool& m_warningDependentFolders;

        StatusHandler* statusUpdater;
        Zstring txtComparingContentOfFiles;
    };
}

#endif // COMPARISON_H_INCLUDED
