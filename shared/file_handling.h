// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef FILE_HANDLING_H_INCLUDED
#define FILE_HANDLING_H_INCLUDED

#include "zstring.h"
#include "file_error.h"
#include <wx/longlong.h>

#ifdef FFS_WIN
#include "shadow.h"
#endif


namespace ffs3
{
struct RemoveDirCallback;
struct MoveFileCallback;
struct CopyFileCallback;


Zstring getFormattedDirectoryName(const Zstring& dirname);

bool fileExists(     const Zstring& filename); //throw()       replaces wxFileExists()!
bool dirExists(      const Zstring& dirname);  //throw()       replaces wxDirExists(): optional 'cause wxDirExists treats symlinks correctly
bool symlinkExists(  const Zstring& objname);  //throw()       check whether a symbolic link exists
bool somethingExists(const Zstring& objname);  //throw()       check whether any object with this name exists

//check whether two folders are located on the same (logical) volume
//left and right directories NEED NOT yet exist! volume prefix is sufficient! path may end with PATH_SEPARATOR
enum ResponseSameVol
{
    VOLUME_SAME,
    VOLUME_DIFFERENT,
    VOLUME_CANT_SAY
};
ResponseSameVol onSameVolume(const Zstring& folderLeft, const Zstring& folderRight); //throw()

//copy file or directory create/last change date,
void copyFileTimes(const Zstring& sourceDir, const Zstring& targetDir, bool derefSymlinks); //throw (FileError)

//copy filesystem permissions: probably requires admin rights
void copyObjectPermissions(const Zstring& source, const Zstring& target, bool derefSymlinks); //throw FileError();

//symlink handling: always evaluate target
wxULongLong getFilesize(const Zstring& filename); //throw (FileError)


//file handling
void removeFile(const Zstring& filename);       //throw (FileError)
void removeDirectory(const Zstring& directory, RemoveDirCallback* callback = NULL); //throw (FileError)


//rename file or directory: no copying!!!
void renameFile(const Zstring& oldName, const Zstring& newName); //throw (FileError);

//move source to target; expectations: target not existing, all super-directories of target exist
void moveFile(const Zstring& sourceFile, const Zstring& targetFile, MoveFileCallback* callback = NULL); //throw (FileError);

//move source to target including subdirectories
//"ignoreExistingDirs": existing directories will be enhanced as long as this is possible without overwriting files
void moveDirectory(const Zstring& sourceDir, const Zstring& targetDir, bool ignoreExistingDirs, MoveFileCallback* callback = NULL); //throw (FileError);

//creates superdirectories automatically:
void createDirectory(const Zstring& directory, const Zstring& templateDir, bool copyDirectorySymLinks, bool copyFilePermissions); //throw (FileError);
void createDirectory(const Zstring& directory); //throw (FileError); -> function overload avoids default parameter ambiguity issues!


void copyFile(const Zstring& sourceFile, //throw (FileError);
              const Zstring& targetFile,
              bool copyFileSymLinks,
              bool copyFilePermissions,
#ifdef FFS_WIN
              shadow::ShadowCopy* shadowCopyHandler, //supply handler for making shadow copies, may be NULL
#endif
              CopyFileCallback* callback);  //may be NULL
//Note: it MAY happen that copyFile() leaves temp files behind, e.g. temporary network drop.
// => clean them up at an appropriate time (automatically set sync directions to delete them). They have the following ending:
const Zstring TEMP_FILE_ENDING = Zstr(".ffs_tmp");






//----------- callbacks ---------------
struct RemoveDirCallback
{
    virtual ~RemoveDirCallback() {}
    virtual void requestUiRefresh(const Zstring& currentObject) = 0;
};


struct MoveFileCallback //callback functionality
{
    virtual ~MoveFileCallback() {}

    enum Response
    {
        CONTINUE,
        CANCEL
    };
    virtual Response requestUiRefresh(const Zstring& currentObject) = 0;  //DON'T throw exceptions here, at least in Windows build!
};


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
}

#endif //FILE_HANDLING_H_INCLUDED
