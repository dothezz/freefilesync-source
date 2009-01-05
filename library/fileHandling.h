#ifndef RECYCLER_H_INCLUDED
#define RECYCLER_H_INCLUDED

#include "globalFunctions.h"
#include <wx/dir.h>


class FileError //Exception class used to notify file/directory copy/delete errors
{
public:
    FileError(const wxString& txt) : errorMessage(txt) {}

    wxString show() const
    {
        return errorMessage;
    }

private:
    wxString errorMessage;
};


namespace FreeFileSync
{
    void getAllFilesAndDirs(const wxString& sourceDir, wxArrayString& files, wxArrayString& directories) throw(FileError);

    //recycler
    bool recycleBinExists(); //test existence of Recycle Bin API on current system

    //file handling
    void removeDirectory(const wxString& directory, const bool useRecycleBin);
    void removeFile(const wxString& filename, const bool useRecycleBin);
    void createDirectory(const wxString& directory, int level = 0); //level is used internally only
}


#endif // RECYCLER_H_INCLUDED
