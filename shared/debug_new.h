// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef DEBUGNEW_H_INCLUDED
#define DEBUGNEW_H_INCLUDED

#include <string>
#include <sstream>
#include <cstdlib> //malloc(), free()


#ifndef _MSC_VER
#error currently for use with MSC only
#endif

/*overwrite "operator new" to get more detailed error messages on bad_alloc, detect memory leaks and write memory dumps
Usage:
- Include everywhere before any other file: $(ProjectDir)\shared\debug_new.h
For Minidumps:
- Compile "debug_new.cpp"
- Compile in Debug build (need Symbols and less restrictive Optimization)
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
