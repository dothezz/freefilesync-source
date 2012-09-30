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

namespace zen
{
struct CallbackMoveFile;

//e.g. move C:\Source\subdir\Sample.txt -> D:\Revisions\subdir\Sample.txt 2012-05-15 131513.txt
//scheme: <revisions directory>\<relpath>\<filename>.<ext> YYYY-MM-DD HHMMSS.<ext>
/*
	- ignores missing source files/dirs
	- creates missing intermediate directories
	- does not create empty directories
	- handles symlinks
	- ignores already existing target files/dirs (support retry)
		=> (unlikely) risk of data loss: race-condition if two FFS instances start at the very same second and process the same filename!!
*/

class FileVersioner
{
public:
    FileVersioner(const Zstring& versioningDirectory, //throw FileError
                  const TimeComp& timeStamp,
                  int versionCountLimit) : //max versions per file; < 0 := no limit
        versioningDirectory_(versioningDirectory),
        timeStamp_(formatTime<Zstring>(Zstr("%Y-%m-%d %H%M%S"), timeStamp)), //e.g. "2012-05-15 131513"
        versionCountLimit_(versionCountLimit)
    {
        if (timeStamp_.size() != 17) //formatTime() returns empty string on error; unexpected length: e.g. problem in year 10000!
            throw FileError(_("Failure to create time stamp for versioning:") + L" \'" + timeStamp_ + L"\'");
    }

    void revisionFile(const Zstring& sourceFile, const Zstring& relativeName, CallbackMoveFile& callback); //throw FileError
    void revisionDir (const Zstring& sourceDir,  const Zstring& relativeName, CallbackMoveFile& callback); //throw FileError

    void limitVersions(std::function<void()> updateUI); //throw FileError; call when done revisioning!

private:
    const Zstring versioningDirectory_;
    const Zstring timeStamp_;
    const int versionCountLimit_;

    std::vector<Zstring> fileRelNames; //store list of revisioned file and symlink relative names for limitVersions()
};


struct CallbackMoveFile
{
    virtual ~CallbackMoveFile() {} //see CallbackCopyFile for limitations when throwing exceptions!

    virtual void onBeforeFileMove(const Zstring& fileFrom, const Zstring& fileTo) = 0; //one call before each (planned) move
    virtual void onBeforeDirMove (const Zstring& dirFrom,  const Zstring& dirTo ) = 0; //
    virtual void objectProcessed() = 0; //one call after each completed move (count objects total)

    //called frequently if move has to revert to copy + delete:
    virtual void updateStatus(Int64 bytesDelta) = 0;
};








namespace impl //declare for unit tests:
{
bool isMatchingVersion(const Zstring& shortname, const Zstring& shortnameVersion);
}
}

#endif //VERSIONING_HEADER_8760247652438056
