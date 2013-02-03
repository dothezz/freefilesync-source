// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef VERSIONING_HEADER_8760247652438056
#define VERSIONING_HEADER_8760247652438056

#include <vector>
#include <functional>
#include <zen/time.h>
#include <zen/zstring.h>
#include <zen/int64.h>
#include <zen/file_error.h>
#include "../structures.h"

namespace zen
{
struct CallbackMoveDir;
struct CallbackMoveFile;

//e.g. move C:\Source\subdir\Sample.txt -> D:\Revisions\subdir\Sample.txt 2012-05-15 131513.txt
//scheme: <revisions directory>\<relpath>\<filename>.<ext> YYYY-MM-DD HHMMSS.<ext>
/*
	- ignores missing source files/dirs
	- creates missing intermediate directories
	- does not create empty directories
	- handles symlinks
	- replaces already existing target files/dirs (supports retry)
		=> (unlikely) risk of data loss for naming convention "versioning":
		race-condition if two FFS instances start at the very same second OR multiple folder pairs process the same filename!!
*/

class FileVersioner
{
public:
    FileVersioner(const Zstring& versioningDirectory, //throw FileError
                  VersioningStyle versioningStyle,
                  const TimeComp& timeStamp) : //max versions per file; < 0 := no limit
        versioningStyle_(versioningStyle),
        versioningDirectory_(versioningDirectory),
        timeStamp_(formatTime<Zstring>(Zstr("%Y-%m-%d %H%M%S"), timeStamp)) //e.g. "2012-05-15 131513"
    {
        if (timeStamp_.size() != 17) //formatTime() returns empty string on error; unexpected length: e.g. problem in year 10000!
            throw FileError(_("Failure to create timestamp for versioning:") + L" \'" + timeStamp_ + L"\'");
    }

    bool revisionFile(const Zstring& sourceFile, const Zstring& relativeName, CallbackMoveFile& callback); //throw FileError; return "false" if file is not existing
    void revisionDir (const Zstring& sourceDir,  const Zstring& relativeName, CallbackMoveDir& callback); //throw FileError

    //void limitVersions(std::function<void()> updateUI); //throw FileError; call when done revisioning!

private:
    bool revisionFileImpl(const Zstring& sourceFile, const Zstring& relativeName, CallbackMoveDir& callback); //throw FileError
    void revisionDirImpl (const Zstring& sourceDir,  const Zstring& relativeName, CallbackMoveDir& callback); //throw FileError

    const VersioningStyle versioningStyle_;
    const Zstring versioningDirectory_;
    const Zstring timeStamp_;

    //std::vector<Zstring> fileRelNames; //store list of revisioned file and symlink relative names for limitVersions()
};


struct CallbackMoveFile //see CallbackCopyFile for limitations when throwing exceptions!
{
    virtual ~CallbackMoveFile() {}
    virtual void updateStatus(Int64 bytesDelta) = 0; //called frequently if move has to revert to copy + delete:
};

struct CallbackMoveDir //see CallbackCopyFile for limitations when throwing exceptions!
{
    virtual ~CallbackMoveDir() {}

    virtual void onBeforeFileMove(const Zstring& fileFrom, const Zstring& fileTo) = 0; //one call for each *existing* object!
    virtual void onBeforeDirMove (const Zstring& dirFrom,  const Zstring& dirTo ) = 0; //
    virtual void updateStatus(Int64 bytesDelta) = 0; //called frequently if move has to revert to copy + delete:
};



namespace impl //declare for unit tests:
{
bool isMatchingVersion(const Zstring& shortname, const Zstring& shortnameVersion);
}
}

#endif //VERSIONING_HEADER_8760247652438056
