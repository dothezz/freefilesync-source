// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef FILE_HANDLING_H_INCLUDED
#define FILE_HANDLING_H_INCLUDED

#include "zstring.h"
#include "fileError.h"
#include <wx/longlong.h>

#ifdef FFS_WIN
#include "shadow.h"
#endif


namespace FreeFileSync
{
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

//optionally: copy creation/last change date, DOES NOTHING if something fails
void copyFileTimes(const Zstring& sourceDir, const Zstring& targetDir); //throw()

//symlink handling: always evaluate target
wxULongLong getFilesize(const Zstring& filename); //throw (FileError)


//file handling
void removeFile(const Zstring& filename);       //throw (FileError)
void removeDirectory(const Zstring& directory); //throw (FileError)


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

//rename file or directory: no copying!!!
void renameFile(const Zstring& oldName, const Zstring& newName); //throw (FileError);

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

void copyFile(const Zstring& sourceFile,
              const Zstring& targetFile,
              bool copyFileSymLinks,
#ifdef FFS_WIN
              ShadowCopy* shadowCopyHandler = NULL, //supply handler for making shadow copies
#endif
              CopyFileCallback* callback = NULL);   //throw (FileError);
//Note: it MAY happen that copyFile() leaves temp files behind, e.g. temporary network drop.
// => clean them up at an appropriate time (automatically set sync directions to delete them). They have the following ending:
const Zstring TEMP_FILE_ENDING = DefaultStr(".ffs_tmp");
}


#endif //FILE_HANDLING_H_INCLUDED
