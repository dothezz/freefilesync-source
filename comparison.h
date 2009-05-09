#ifndef COMPARISON_H_INCLUDED
#define COMPARISON_H_INCLUDED

#include "structures.h"

class StatusHandler;
class DirectoryDescrBuffer;


namespace FreeFileSync
{
    //class handling comparison process
    class CompareProcess
    {
    public:
        CompareProcess(const bool traverseSymLinks,
                       const unsigned fileTimeTol,
                       bool& warningDependentFolders,
                       StatusHandler* handler);

        ~CompareProcess();

        void startCompareProcess(const std::vector<FolderPair>& directoryPairs,
                                 const CompareVariant cmpVar,
                                 FolderComparison& output);

    private:
        void compareByTimeSize(const std::vector<FolderPair>& directoryPairsFormatted, FolderComparison& output);

        void compareByContent(const std::vector<FolderPair>& directoryPairsFormatted, FolderComparison& output);

        //create comparison result table and fill relation except for files existing on both sides
        void performBaseComparison(const FolderPair& pair, FileComparison& output);

        //buffer accesses to the same directories; useful when multiple folder pairs are used
        DirectoryDescrBuffer* descriptionBuffer;

        const bool traverseDirectorySymlinks;
        const unsigned fileTimeTolerance; //max allowed file time deviation
        bool& m_warningDependentFolders;

        StatusHandler* statusUpdater;
        const Zstring txtComparingContentOfFiles;
    };
}

#endif // COMPARISON_H_INCLUDED
