// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef SORTING_H_INCLUDED
#define SORTING_H_INCLUDED

#include "../file_hierarchy.h"
#include "../shared/system_constants.h"
#include "../synchronization.h"
#include "../shared/assert_static.h"


namespace zen
{
namespace
{
struct CompileTimeReminder : public FSObjectVisitor
{
    virtual void visit(const FileMapping& fileObj) {}
    virtual void visit(const SymLinkMapping& linkObj) {}
    virtual void visit(const DirMapping& dirObj) {}
} checkDymanicCasts; //just a compile-time reminder to check dynamic casts in this file
}

inline
bool isDirectoryMapping(const FileSystemObject& fsObj)
{
    return dynamic_cast<const DirMapping*>(&fsObj) != NULL;
}


template <bool ascending>
struct Compare
{
    template <class T>
    bool isSmallerThan(T a, T b)
    {
        assert_static(sizeof(T) <= 2 * sizeof(int)); //use for comparing (small) INTEGRAL types only!
        return a < b;
    }
};
template <>
struct Compare<false>
{
    template <class T>
    bool isSmallerThan(T a, T b)
    {
        assert_static(sizeof(T) <= 2 * sizeof(int)); //use for comparing (small) INTEGRAL types only!
        return a > b;
    }
};


template <bool ascending, SelectedSide side>
inline
bool sortByFileName(const FileSystemObject& a, const FileSystemObject& b)
{
    //presort types: first files, then directories then empty rows
    if (a.isEmpty<side>())
        return false;  //empty rows always last
    else if (b.isEmpty<side>())
        return true;  //empty rows always last


    if (isDirectoryMapping(a)) //sort directories by relative name
    {
        if (isDirectoryMapping(b))
            return LessFilename()(a.getRelativeName<side>(), b.getRelativeName<side>());
        else
            return false;
    }
    else
    {
        if (isDirectoryMapping(b))
            return true;
        else
            return Compare<ascending>().isSmallerThan(
                       cmpFileName(a.getShortName<side>(), b.getShortName<side>()), 0);
    }
}


template <bool ascending, SelectedSide side>
bool sortByRelativeName(const FileSystemObject& a, const FileSystemObject& b)
{
    if (a.isEmpty<side>())
        return false;  //empty rows always last
    else if (b.isEmpty<side>())
        return true;  //empty rows always last

    const bool isDirectoryA = isDirectoryMapping(a);
    const Zstring& relDirNameA = isDirectoryA ?
                                 a.getRelativeName<side>() : //directory
                                 a.getParentRelativeName();  //file or symlink

    const bool isDirectoryB = isDirectoryMapping(b);
    const Zstring& relDirNameB = isDirectoryB ?
                                 b.getRelativeName<side>() : //directory
                                 b.getParentRelativeName();  //file or symlink


    //compare relative names without filenames first
    const int rv = cmpFileName(relDirNameA, relDirNameB);
    if (rv != 0)
        return Compare<ascending>().isSmallerThan(rv, 0);
    else //compare the filenames
    {
        if (isDirectoryB) //directories shall appear before files
            return false;
        else if (isDirectoryA)
            return true;

        return LessFilename()(a.getShortName<side>(), b.getShortName<side>());
    }
}


template <bool ascending, SelectedSide side>
inline
bool sortByFileSize(const FileSystemObject& a, const FileSystemObject& b)
{
    //empty rows always last
    if (a.isEmpty<side>())
        return false;
    else if (b.isEmpty<side>())
        return true;

    const bool isDirA = dynamic_cast<const DirMapping*>(&a) != NULL;
    const bool isDirB = dynamic_cast<const DirMapping*>(&b) != NULL;

    //directories second last
    if (isDirA)
        return false;
    else if (isDirB)
        return true;

    const FileMapping* fileObjA = dynamic_cast<const FileMapping*>(&a);
    const FileMapping* fileObjB = dynamic_cast<const FileMapping*>(&b);

    //then symlinks
    if (fileObjA == NULL)
        return false;
    else if (fileObjB == NULL)
        return true;

    //return list beginning with largest files first
    return Compare<ascending>().isSmallerThan(fileObjA->getFileSize<side>(), fileObjB->getFileSize<side>());
}


template <bool ascending, SelectedSide side>
inline
bool sortByDate(const FileSystemObject& a, const FileSystemObject& b)
{
    if (a.isEmpty<side>())
        return false;  //empty rows always last
    else if (b.isEmpty<side>())
        return true;  //empty rows always last


    const FileMapping* fileObjA = dynamic_cast<const FileMapping*>(&a);
    const FileMapping* fileObjB = dynamic_cast<const FileMapping*>(&b);

    const SymLinkMapping* linkObjA = dynamic_cast<const SymLinkMapping*>(&a);
    const SymLinkMapping* linkObjB = dynamic_cast<const SymLinkMapping*>(&b);

    if (!fileObjA && !linkObjA)
        return false; //directories last
    else if (!fileObjB && !linkObjB)
        return true;  //directories last

    zen::Int64 dateA = fileObjA ? fileObjA->getLastWriteTime<side>() : linkObjA->getLastWriteTime<side>();
    zen::Int64 dateB = fileObjB ? fileObjB->getLastWriteTime<side>() : linkObjB->getLastWriteTime<side>();

    //return list beginning with newest files first
    return Compare<ascending>().isSmallerThan(dateA, dateB);
}


template <bool ascending, SelectedSide side>
inline
bool sortByExtension(const FileSystemObject& a, const FileSystemObject& b)
{
    if (a.isEmpty<side>())
        return false;  //empty rows always last
    else if (b.isEmpty<side>())
        return true;  //empty rows always last


    const FileMapping* fileObjA = dynamic_cast<const FileMapping*>(&a);
    const FileMapping* fileObjB = dynamic_cast<const FileMapping*>(&b);

    if (fileObjA == NULL)
        return false; //directories last
    else if (fileObjB == NULL)
        return true;  //directories last

    return Compare<ascending>().isSmallerThan(fileObjA->getExtension<side>(), fileObjB->getExtension<side>());
}


template <bool ascending>
inline
bool sortByCmpResult(const FileSystemObject& a, const FileSystemObject& b)
{
    //presort result: equal shall appear at end of list
    if (a.getCategory() == FILE_EQUAL)
        return false;
    if (b.getCategory() == FILE_EQUAL)
        return true;

    return Compare<ascending>().isSmallerThan(a.getCategory(), b.getCategory());
}


template <bool ascending>
inline
bool sortBySyncDirection(const FileSystemObject& a, const FileSystemObject& b)
{
    return Compare<ascending>().isSmallerThan(a.getSyncOperation(), b.getSyncOperation());
}
}

#endif // SORTING_H_INCLUDED
