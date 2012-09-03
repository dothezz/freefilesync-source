// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef DEBUGNEW_H_INCLUDED
#define DEBUGNEW_H_INCLUDED

#include <string>
#include <sstream>
#include <cstdlib> //malloc(), free()

#ifndef _MSC_VER
#error currently for use with MSC only
#endif

/*
Better std::bad_alloc
---------------------
overwrite "operator new" to automatically write mini dump and get info about bytes requested

1. Compile "debug_new.cpp"
2. C/C++ -> Advanced: Forced Include File: zen/debug_new.h

Minidumps http://msdn.microsoft.com/en-us/library/windows/desktop/ee416349(v=vs.85).aspx
---------
1. Compile "debug_new.cpp"
2. Compile "release" build with:
	- C/C++ -> General: Debug Information Format: "Program Database" (/Zi).
	- C/C++ -> Optimization: Omit Frame Pointers: No (/Oy-) - avoid call stack mess up!
	- Linker -> Debugging: Generate Debug Info: Yes (/DEBUG)
	- Linker -> Optimization: References: Yes (/OPT:REF).
	- Linker -> Optimization: Enable COMDAT Folding: Yes (/OPT:ICF).
Optional:
	- C/C++ -> Optimization: Disabled (/Od)
*/

namespace mem_check
{
class BadAllocDetailed : public std::bad_alloc
{
public:
    explicit BadAllocDetailed(size_t allocSize)
    {
        errorMsg = "Memory allocation failed: ";
        errorMsg += numberToString(allocSize);
    }

    ~BadAllocDetailed() throw() {}

    virtual const char* what() const throw()
    {
        return errorMsg.c_str();
    }

private:
    template <class T>
    static std::string numberToString(const T& number) //convert number to string the C++ way
    {
        std::ostringstream ss;
        ss << number;
        return ss.str();
    }

    std::string errorMsg;
};

#ifdef _MSC_VER
void writeMinidump();
#endif
}

inline
void* operator new(size_t size)
{
    void* newMem = ::malloc(size);
    if (!newMem)
    {
#ifdef _MSC_VER
        mem_check::writeMinidump();
#endif
        throw mem_check::BadAllocDetailed(size);
    }
    return newMem;
}


inline
void operator delete(void* ptr)
{
    ::free(ptr);
}


inline
void* operator new[](size_t size)
{
    return operator new(size);
}


inline
void operator delete[](void* ptr)
{
    operator delete(ptr);
}

#endif // DEBUGNEW_H_INCLUDED
