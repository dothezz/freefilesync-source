#ifndef COMPARISON_H_INCLUDED
#define COMPARISON_H_INCLUDED

#include "FreeFileSync.h"


namespace FreeFileSync
{
    bool foldersAreValidForComparison(const vector<FolderPair>& folderPairs, wxString& errorMessage);
    bool foldersHaveDependencies(const vector<FolderPair>& folderPairs, wxString& warningMessage);

    //class handling comparison process
    class CompareProcess
    {
    public:
        CompareProcess(bool lineBreakOnMessages, StatusHandler* handler);

        void startCompareProcess(const vector<FolderPair>& directoryPairsFormatted,
                                 const CompareVariant cmpVar,
                                 FileCompareResult& output) throw(AbortThisProcess);

    private:
        //create comparison result table and fill relation except for files existing on both sides
        void performBaseComparison(const vector<FolderPair>& directoryPairsFormatted,
                                   FileCompareResult& output);

        StatusHandler* statusUpdater;
        wxString optionalLineBreak; //optional line break for status messages (used by GUI mode only)
    };
}

#endif // COMPARISON_H_INCLUDED
