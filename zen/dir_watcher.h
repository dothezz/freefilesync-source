// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef DIR_WATCHER_348577025748023458
#define DIR_WATCHER_348577025748023458

#include <vector>
#include <memory>
#include "file_error.h"

namespace zen
{
//Windows: ReadDirectoryChangesW http://msdn.microsoft.com/en-us/library/aa365465(v=vs.85).aspx
//Linux:   inotify               http://linux.die.net/man/7/inotify

//watch directory including subdirectories
/*
!Note handling of directories!:
    Linux: newly added subdirectories are reported but not automatically added for watching! -> reset Dirwatcher!
	       removal of top watched directory is NOT notified!
    Windows: removal of top watched directory also NOT notified (e.g. brute force usb stick removal)
	         however manual unmount IS notified (e.g. usb stick removal, then re-insert), but watching is stopped!
			 Renaming of top watched directory handled incorrectly: Not notified(!) + changes in subfolders
			 report FILE_ACTION_MODIFIED for directory (check that should prevent this fails!)

	Overcome all issues portably: check existence of watched directory externally + reinstall watch after changes in directory structure (added directories) are possible
*/
class DirWatcher
{
public:
    DirWatcher(const Zstring& directory); //throw FileError, ErrorNotExisting
    ~DirWatcher();

    //extract accumulated changes since last call
    std::vector<Zstring> getChanges(); //throw FileError

private:
    DirWatcher(const DirWatcher&);
    DirWatcher& operator=(const DirWatcher&);

    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl_;
};

}

#endif
