// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef TASKBARPROGRESS_H_INCLUDED
#define TASKBARPROGRESS_H_INCLUDED

#include <wx/toplevel.h>
#include <memory>

/*
Windows 7; show progress in windows superbar via ITaskbarList3 Interface (http://msdn.microsoft.com/en-us/library/dd391692(VS.85).aspx)

Ubuntu: use Unity interface (optional)

Define HAVE_UBUNTU_UNITY and set:
    Compiler flag: `pkg-config --cflags unity`
    Linker   flag: `pkg-config --libs unity`
*/

namespace util
{
class TaskbarNotAvailable {};

class Taskbar
{
public:
    Taskbar(const wxTopLevelWindow& window); //throw TaskbarNotAvailable()
    ~Taskbar();

    enum Status
    {
        STATUS_NOPROGRESS,
        STATUS_INDETERMINATE,
        STATUS_NORMAL,
        STATUS_ERROR,
        STATUS_PAUSED
    };

    void setStatus(Status status);
    void setProgress(double fraction); //between [0, 1]

private:
    class Pimpl;
    std::unique_ptr<Pimpl> pimpl_;
};

}

#endif // TASKBARPROGRESS_H_INCLUDED
