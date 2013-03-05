// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************
#ifndef PREVENTSTANDBY_H_INCLUDED
#define PREVENTSTANDBY_H_INCLUDED

#include <memory>
#include <zen/file_error.h>

namespace zen
{
//signal a "busy" state to the operating system
class PreventStandby
{
public:
    PreventStandby(); //throw FileError
    ~PreventStandby();
private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

//lower CPU and file I/O priorities
class ScheduleForBackgroundProcessing
{
public:
    ScheduleForBackgroundProcessing(); //throw FileError
    ~ScheduleForBackgroundProcessing();
private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};
}

#endif // PREVENTSTANDBY_H_INCLUDED
