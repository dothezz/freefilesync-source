#ifndef COMPARISON_H_INCLUDED
#define COMPARISON_H_INCLUDED

#include "FreeFileSync.h"
#include "library/statusHandler.h"

class DirectoryDescrBuffer;

namespace FreeFileSync
{
    bool foldersAreValidForComparison(const std::vector<FolderPair>& folderPairs, wxString& errorMessage);
    bool foldersHaveDependencies(     const std::vector<FolderPair>& folderPairs, wxString& warningMessage);

    //class handling comparison process
    class CompareProcess
    {
    public:
        CompareProcess(bool lineBreakOnMessages, bool handleDstOnFat32Drives, StatusHandler* handler);

        void startCompareProcess(const std::vector<FolderPair>& directoryPairsFormatted,
                                 const CompareVariant cmpVar,
                                 FileCompareResult& output) throw(AbortThisProcess);

    private:
        void compareByTimeSize(const std::vector<FolderPair>& directoryPairsFormatted, FileCompareResult& output);

        void compareByContent(const std::vector<FolderPair>& directoryPairsFormatted, FileCompareResult& output);

        //create comparison result table and fill relation except for files existing on both sides
        void performBaseComparison(const FolderPair& pair,
                                   DirectoryDescrBuffer& descriptionBuffer,
                                   FileCompareResult& output);

        const bool includeLineBreak; //optional line break for status messages (used by GUI mode only)
        const bool handleDstOnFat32;
        StatusHandler* statusUpdater;
        Zstring txtComparingContentOfFiles;
    };
}

#endif // COMPARISON_H_INCLUDED
