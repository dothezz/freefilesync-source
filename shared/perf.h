// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include <windows.h>
#include <sstream>

class Performance
{
public:
    Performance() : resultWasShown(false), startTime(::GetTickCount()) {}

    ~Performance()
    {
        if (!resultWasShown)
            showResult();
    }

    void showResult()
    {
        resultWasShown = true;

        const DWORD currentTime = ::GetTickCount();
        const DWORD delta = currentTime - startTime;
        startTime = currentTime;

        std::ostringstream ss;
        ss << delta << " ms";

        ::MessageBoxA(NULL, ss.str().c_str(), "Timer", 0);
    }

private:
    bool resultWasShown;
    DWORD startTime;
};

//two macros for quick performance measurements
#define PERF_START Performance a;
#define PERF_STOP  a.showResult();
