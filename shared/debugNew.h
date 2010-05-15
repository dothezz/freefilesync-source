// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef DEBUGNEW_H_INCLUDED
#define DEBUGNEW_H_INCLUDED

#include <string>
#include <sstream>
#include <cstdlib> //malloc(), free()


/*all this header does is to globally overwrite "operator new" to give some more detailed error messages and write memory dumps
Usage:
	 - Include everywhere before any other file: $(ProjectDir)\shared\debugNew.h
	 For Minidumps:
	 - Compile "debugNew.cpp"
	 - Include library "Dbghelp.lib"
	 - Compile in Debug build (need Symbols and less restrictive Optimization)
*/

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
        std::stringstream ss;
        ss << number;
        return ss.str();
    }

    std::string errorMsg;
};

#ifdef _MSC_VER
namespace MemoryDump
{
void writeMinidump();
}
#endif

inline
void* operator new(size_t allocSize)
{
    void* newMem = ::malloc(allocSize);
    if (!newMem)
    {
#ifdef _MSC_VER
        MemoryDump::writeMinidump();
#endif
        throw BadAllocDetailed(allocSize);
    }
    return newMem;
}


inline
void* operator new[](size_t allocSize)
{
    return operator new(allocSize);
}


inline
void operator delete(void* memory)
{
    ::free(memory);
}


inline
void operator delete[](void* memory)
{
    operator delete(memory);
}

#endif // DEBUGNEW_H_INCLUDED

