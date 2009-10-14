#ifndef RECYCLER_H_INCLUDED
#define RECYCLER_H_INCLUDED

#include "zstring.h"
#include "fileError.h"
#include <wx/longlong.h>


namespace FreeFileSync
{
Zstring getFormattedDirectoryName(const Zstring& dirname);

bool fileExists(const DefaultChar* filename);   //throw()       replaces wxFileExists()!
bool dirExists(const DefaultChar* dirname);     //throw()       replaces wxDirExists(): optional 'cause wxDirExists treats symlinks correctly
bool symlinkExists(const DefaultChar* objname); //throw()       check if a symbolic link exists

//check if files can be moved between two EXISTING paths (without copying)
bool isMovable(const Zstring& pathFrom, const Zstring& pathTo); //throw()

//optionally: copy directory last change date, DOES NOTHING if something fails
void copyDirLastChangeDate(const Zstring& sourceDir, const Zstring& targetDir);

//recycler
bool recycleBinExists(); //test existence of Recycle Bin API on current system

//file handling
void removeFile(const Zstring& filename, const bool useRecycleBin);       //throw (FileError, std::logic_error)
void removeDirectory(const Zstring& directory, const bool useRecycleBin); //throw (FileError)


struct MoveFileCallback //callback functionality
{
    virtual ~MoveFileCallback() {}

    enum Response
    {
        CONTINUE,
        CANCEL
    };
    virtual Response requestUiRefresh() = 0;  //DON'T throw exceptions here, at least in Windows build!
};

//move source to target; expectations: target not existing, all super-directories of target exist
void moveFile(const Zstring& sourceFile, const Zstring& targetFile, MoveFileCallback* callback = NULL); //throw (FileError);

//move source to target including subdirectories
//"ignoreExistingDirs": existing directories will be enhanced as long as this is possible without overwriting files
void moveDirectory(const Zstring& sourceDir, const Zstring& targetDir, bool ignoreExistingDirs, MoveFileCallback* callback = NULL); //throw (FileError);

//creates superdirectories automatically:
void createDirectory(const Zstring& directory, const Zstring& templateDir = Zstring(), const bool copyDirectorySymLinks = false); //throw (FileError);

struct CopyFileCallback //callback functionality
{
    virtual ~CopyFileCallback() {}

    enum Response
    {
        CONTINUE,
        CANCEL
    };
    virtual Response updateCopyStatus(const wxULongLong& totalBytesTransferred) = 0; //DON'T throw exceptions here, at least in Windows build!
};

#ifdef FFS_WIN
class ShadowCopy;

void copyFile(const Zstring& sourceFile,
              const Zstring& targetFile,
              const bool copyFileSymLinks,
              ShadowCopy* shadowCopyHandler = NULL, //supply handler for making shadow copies
              CopyFileCallback* callback = NULL); //throw (FileError);

#elif defined FFS_LINUX
void copyFile(const Zstring& sourceFile,
              const Zstring& targetFile,
              const bool copyFileSymLinks,
              CopyFileCallback* callback = NULL); //throw (FileError);
#endif
}


#endif // RECYCLER_H_INCLUDED
