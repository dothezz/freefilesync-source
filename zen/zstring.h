// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef ZSTRING_H_INCLUDED
#define ZSTRING_H_INCLUDED

#include "string_base.h"
#ifdef ZEN_LINUX
#include <cstring> //strcmp
#elif defined ZEN_MAC
//#include <strings.h> //strcasecmp
#endif


#ifndef NDEBUG
namespace z_impl
{
void leakCheckerInsert(const void* ptr, size_t size);
void leakCheckerRemove(const void* ptr);
}
#endif //NDEBUG

class AllocatorFreeStoreChecked
{
public:
    static void* allocate(size_t size) //throw std::bad_alloc
    {
        void* newMem = ::operator new(size);
#ifndef NDEBUG
        z_impl::leakCheckerInsert(newMem, size); //test Zbase for memory leaks
#endif
        return newMem;
    }

    static void deallocate(void* ptr)
    {
#ifndef NDEBUG
        z_impl::leakCheckerRemove(ptr); //check for memory leaks
#endif
        ::operator delete(ptr);
    }

    static size_t calcCapacity(size_t length) { return std::max<size_t>(16, length + length / 2); } //exponential growth + min size
};


//############################## helper functions #############################################

#ifdef ZEN_WIN //Windows encodes Unicode as UTF-16 wchar_t
typedef wchar_t Zchar;
#define Zstr(x) L ## x
const Zchar FILE_NAME_SEPARATOR = L'\\';

#elif defined ZEN_LINUX || defined ZEN_MAC //Linux uses UTF-8
typedef char Zchar;
#define Zstr(x) x
const Zchar FILE_NAME_SEPARATOR = '/';
#endif

//"The reason for all the fuss above" - Loki/SmartPtr
//a high-performance string for interfacing with native OS APIs and multithreaded contexts
typedef zen::Zbase<Zchar, zen::StorageRefCountThreadSafe, AllocatorFreeStoreChecked> Zstring;



//Compare filenames: Windows does NOT distinguish between upper/lower-case, while Linux DOES
template <template <class, class> class SP, class AP>
int cmpFileName(const zen::Zbase<Zchar, SP, AP>& lhs, const zen::Zbase<Zchar, SP, AP>& rhs);

struct LessFilename //case-insensitive on Windows, case-sensitive on Linux
{
    template <template <class, class> class SP, class AP>
    bool operator()(const zen::Zbase<Zchar, SP, AP>& lhs, const zen::Zbase<Zchar, SP, AP>& rhs) const { return cmpFileName(lhs, rhs) < 0; }
};

struct EqualFilename //case-insensitive on Windows, case-sensitive on Linux
{
    template <template <class, class> class SP, class AP>
    bool operator()(const zen::Zbase<Zchar, SP, AP>& lhs, const zen::Zbase<Zchar, SP, AP>& rhs) const { return cmpFileName(lhs, rhs) == 0; }
};

#if defined ZEN_WIN || defined ZEN_MAC
template <template <class, class> class SP, class AP>
void makeUpper(zen::Zbase<Zchar, SP, AP>& str);
#endif

inline
Zstring appendSeparator(Zstring path) //support rvalue references!
{
    return endsWith(path, FILE_NAME_SEPARATOR) ? path : (path += FILE_NAME_SEPARATOR);
}






//################################# inline implementation ########################################
namespace z_impl
{
#if defined ZEN_WIN || defined ZEN_MAC
int compareFilenamesNoCase(const Zchar* lhs, const Zchar* rhs, size_t sizeLhs, size_t sizeRhs);
void makeFilenameUpperCase(Zchar* str, size_t size);
#endif
}


template <template <class, class> class SP, class AP> inline
int cmpFileName(const zen::Zbase<Zchar, SP, AP>& lhs, const zen::Zbase<Zchar, SP, AP>& rhs)
{
#if defined ZEN_WIN || defined ZEN_MAC
    return z_impl::compareFilenamesNoCase(lhs.data(), rhs.data(), lhs.length(), rhs.length());
#elif defined ZEN_LINUX
    return std::strcmp(lhs.c_str(), rhs.c_str()); //POSIX filenames don't have embedded 0
    //#elif defined ZEN_MAC
    //  return ::strcasecmp(lhs.c_str(), rhs.c_str()); //locale-dependent!
#endif
}


#if defined ZEN_WIN || defined ZEN_MAC
template <template <class, class> class SP, class AP> inline
void makeUpper(zen::Zbase<Zchar, SP, AP>& str)
{
    z_impl::makeFilenameUpperCase(str.begin(), str.length());
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
