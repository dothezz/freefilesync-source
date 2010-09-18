// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef DEBUG_PERF_HEADER
#define DEBUG_PERF_HEADER

//#define WIN32_LEAN_AND_MEAN -> not in a header
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include <sstream>

#ifdef __MINGW32__
    #define DEPRECATED(x) x __attribute__ ((deprecated))
#elif defined _MSC_VER
    #define DEPRECATED(x) __declspec(deprecated) x
#endif

class Performance
{
public:
    DEPRECATED(Performance()) : resultWasShown(false), startTime(::GetTickCount()) {}

    ~Performance()
    {
        if (!resultWasShown)
            showResult();
    }

    void showResult()
    {
        const DWORD delta = ::GetTickCount() - startTime;
        std::ostringstream ss;
        ss << delta << " ms";
 
        ::MessageBoxA(NULL, ss.str().c_str(), "Timer", 0);
        resultWasShown = true;

        startTime = ::GetTickCount(); //don't include call to MessageBox()!
    }
	
private:
    bool resultWasShown;
    DWORD startTime;
};

//two macros for quick performance measurements
#define PERF_START Performance a;
#define PERF_STOP  a.showResult();

#endif //DEBUG_PERF_HEADER