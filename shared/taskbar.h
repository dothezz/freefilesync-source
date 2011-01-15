// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef TASKBARPROGRESS_H_INCLUDED
#define TASKBARPROGRESS_H_INCLUDED

#ifndef FFS_WIN
use in windows build only!
#endif

#include <wx/toplevel.h>
#include <memory>


namespace util
{
class TaskbarNotAvailable {};

//show progress in windows superbar via ITaskbarList3 Interface (http://msdn.microsoft.com/en-us/library/dd391692(VS.85).aspx)
class TaskbarProgress
{
public:
    TaskbarProgress(const wxTopLevelWindow& window); //throw TaskbarNotAvailable()
    ~TaskbarProgress();

    enum Status
    {
        STATUS_NOPROGRESS,
        STATUS_INDETERMINATE,
        STATUS_NORMAL,
        STATUS_ERROR,
        STATUS_PAUSED
    };

    void setStatus(Status status);
    void setProgress(size_t current, size_t total);

private:
    class Pimpl;
    std::auto_ptr<Pimpl> pimpl_;
};

}

#endif // TASKBARPROGRESS_H_INCLUDED
