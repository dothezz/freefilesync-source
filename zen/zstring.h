// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef ZSTRING_H_INCLUDED
#define ZSTRING_H_INCLUDED

#include "string_base.h"
#include <cstring> //strcmp()

#ifndef NDEBUG
#include "thread.h" //includes <boost/thread.hpp>
#include <map>
#endif


#ifndef NDEBUG
class LeakChecker //small test for memory leaks
{
public:
    void insert(const void* ptr, size_t size)
    {
        boost::lock_guard<boost::mutex> dummy(lockActStrings);
        if (activeStrings.find(ptr) != activeStrings.end())
            reportProblem("Fatal Error: New memory points into occupied space: " + rawMemToString(ptr, size));

        activeStrings[ptr] = size;
    }

    void remove(const void* ptr)
    {
        boost::lock_guard<boost::mutex> dummy(lockActStrings);
        if (activeStrings.find(ptr) == activeStrings.end())
            reportProblem("Fatal Error: No memory available for deallocation at this location!");

        activeStrings.erase(ptr);
    }

    static LeakChecker& instance();

private:
    LeakChecker() {}
    LeakChecker(const LeakChecker&);
    LeakChecker& operator=(const LeakChecker&);
    ~LeakChecker();

    static std::string rawMemToString(const void* ptr, size_t size);
    void reportProblem(const std::string& message); //throw std::logic_error

    boost::mutex lockActStrings;
    zen::hash_map<const void*, size_t> activeStrings;
};
#endif //NDEBUG


class AllocatorFreeStoreChecked
{
public:
    static void* allocate(size_t size) //throw std::bad_alloc
    {
#ifndef NDEBUG
        void* newMem = ::operator new(size);
        LeakChecker::instance().insert(newMem, size); //test Zbase for memory leaks
        return newMem;
#else
        return ::operator new(size);
#endif
    }

    static void deallocate(void* ptr)
    {
#ifndef NDEBUG
        LeakChecker::instance().remove(ptr); //check for memory leaks
#endif
        ::operator delete(ptr);
    }

    static size_t calcCapacity(size_t length) { return std::max<size_t>(16, length + length / 2); } //exponential growth + min size
};


//############################## helper functions #############################################
#if defined(FFS_WIN) || defined(FFS_LINUX)
//Compare filenames: Windows does NOT distinguish between upper/lower-case, while Linux DOES
template <class T, template <class, class> class SP, class AP> int cmpFileName(const zen::Zbase<T, SP, AP>& lhs, const zen::Zbase<T, SP, AP>& rhs);

struct LessFilename //case-insensitive on Windows, case-sensitive on Linux
{
    template <class T, template <class, class> class SP, class AP>
    bool operator()(const zen::Zbase<T, SP, AP>& lhs, const zen::Zbase<T, SP, AP>& rhs) const;
};

struct EqualFilename //case-insensitive on Windows, case-sensitive on Linux
{
    template <class T, template <class, class> class SP, class AP>
    bool operator()(const zen::Zbase<T, SP, AP>& lhs, const zen::Zbase<T, SP, AP>& rhs) const;
};
#endif

#ifdef FFS_WIN
template <template <class, class> class SP, class AP>
void makeUpper(zen::Zbase<wchar_t, SP, AP>& str);
#endif

#ifdef FFS_WIN //Windows encodes Unicode as UTF-16 wchar_t
typedef wchar_t Zchar;
#define Zstr(x) L ## x
const Zchar FILE_NAME_SEPARATOR = L'\\';

#elif defined FFS_LINUX //Linux uses UTF-8
typedef char Zchar;
#define Zstr(x) x
const Zchar FILE_NAME_SEPARATOR = '/';

#else
#error define your platform: FFS_WIN or FFS_LINUX
#endif

//"The reason for all the fuss above" - Loki/SmartPtr
//a high-performance string for use as file name in multithreaded contexts
typedef zen::Zbase<Zchar, zen::StorageRefCountThreadSafe, AllocatorFreeStoreChecked> Zstring;


inline
Zstring appendSeparator(Zstring path) //support rvalue references!
{
    return endsWith(path, FILE_NAME_SEPARATOR) ? path : (path += FILE_NAME_SEPARATOR);
}




















//################################# inline implementation ########################################
#if defined(FFS_WIN) || defined(FFS_LINUX)
namespace z_impl
{
int compareFilenamesWin(const wchar_t* lhs, const wchar_t* rhs, size_t sizeLhs, size_t sizeRhs);
void makeUpperCaseWin(wchar_t* str, size_t size);
}


template <class T, template <class, class> class SP, class AP> inline
int cmpFileName(const zen::Zbase<T, SP, AP>& lhs, const zen::Zbase<T, SP, AP>& rhs)
{
#ifdef FFS_WIN
    return z_impl::compareFilenamesWin(lhs.data(), rhs.data(), lhs.length(), rhs.length());
#elif defined FFS_LINUX
    return ::strcmp(lhs.c_str(), rhs.c_str()); //POSIX filenames don't have embedded 0
#endif
}


template <class T, template <class, class> class SP, class AP> inline
bool LessFilename::operator()(const zen::Zbase<T, SP, AP>& lhs, const zen::Zbase<T, SP, AP>& rhs) const
{
#ifdef FFS_WIN
    return z_impl::compareFilenamesWin(lhs.data(), rhs.data(), lhs.length(), rhs.length()) < 0;
#elif defined FFS_LINUX
    return ::strcmp(lhs.c_str(), rhs.c_str()) < 0; //POSIX filenames don't have embedded 0
#endif
}


template <class T, template <class, class> class SP, class AP> inline
bool EqualFilename::operator()(const zen::Zbase<T, SP, AP>& lhs, const zen::Zbase<T, SP, AP>& rhs) const
{
#ifdef FFS_WIN
    return z_impl::compareFilenamesWin(lhs.data(), rhs.data(), lhs.length(), rhs.length()) == 0;
#elif defined FFS_LINUX
    return ::strcmp(lhs.c_str(), rhs.c_str()) == 0; //POSIX filenames don't have embedded 0
#endif
}
#endif //defined(FFS_WIN) || defined(FFS_LINUX)


#ifdef FFS_WIN
template <template <class, class> class SP, class AP> inline
void makeUpper(zen::Zbase<wchar_t, SP, AP>& str)
{
    z_impl::makeUpperCaseWin(str.begin(), str.length());
}
#endif


namespace std
{
template<> inline
void swap(Zstring& rhs, Zstring& lhs)
{
    rhs.swap(lhs);
}
}

#endif //ZSTRING_H_INCLUDED
