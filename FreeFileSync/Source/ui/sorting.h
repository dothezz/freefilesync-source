// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef SORTING_H_INCLUDED
#define SORTING_H_INCLUDED

#include <zen/assert_static.h>
#include <zen/type_tools.h>
#include "../file_hierarchy.h"

namespace zen
{
namespace
{
struct CompileTimeReminder : public FSObjectVisitor
{
    virtual void visit(const FilePair&    fileObj) {}
    virtual void visit(const SymlinkPair& linkObj) {}
    virtual void visit(const DirPair&     dirObj ) {}
} checkDymanicCasts; //just a compile-time reminder to manually check dynamic casts in this file when needed
}

inline
bool isDirectoryPair(const FileSystemObject& fsObj)
{
    return dynamic_cast<const DirPair*>(&fsObj) != nullptr;
}


template <bool ascending, SelectedSide side> inline
bool lessShortFileName(const FileSystemObject& a, const FileSystemObject& b)
{
    //presort types: first files, then directories then empty rows
    if (a.isEmpty<side>())
        return false;  //empty rows always last
    else if (b.isEmpty<side>())
        return true;  //empty rows always last


    if (isDirectoryPair(a)) //sort directories by relative name
    {
        if (isDirectoryPair(b))
            return LessFilename()(a.getRelativeName<side>(), b.getRelativeName<side>());
        else
            return false;
    }
    else
    {
        if (isDirectoryPair(b))
            return true;
        else
            return makeSortDirection(LessFilename(), Int2Type<ascending>())(a.getShortName<side>(), b.getShortName<side>());
    }
}


template <bool ascending> //side currently unused!
bool lessRelativeName(const FileSystemObject& a, const FileSystemObject& b)
{
    const bool isDirectoryA = isDirectoryPair(a);
    const Zstring& relDirNameA = isDirectoryA ?
                                 a.getObjRelativeName() : //directory
                                 beforeLast(a.getObjRelativeName(), FILE_NAME_SEPARATOR); //returns empty string if ch not found

    const bool isDirectoryB = isDirectoryPair(b);
    const Zstring& relDirNameB = isDirectoryB ?
                                 b.getObjRelativeName() : //directory
                                 beforeLast(b.getObjRelativeName(), FILE_NAME_SEPARATOR); //returns empty string if ch not found

    //compare relative names without filenames first
    const int rv = cmpFileName(relDirNameA, relDirNameB);
    if (rv != 0)
        return makeSortDirection(std::less<int>(), Int2Type<ascending>())(rv, 0);
    else //compare the filenames
    {
        if (isDirectoryB) //directories shall appear before files
            return false;
        else if (isDirectoryA)
            return true;

        return LessFilename()(a.getObjShortName(), b.getObjShortName());
    }
}


template <bool ascending, SelectedSide side> inline
bool lessFilesize(const FileSystemObject& a, const FileSystemObject& b)
{
    //empty rows always last
    if (a.isEmpty<side>())
        return false;
    else if (b.isEmpty<side>())
        return true;

    const bool isDirA = dynamic_cast<const DirPair*>(&a) != nullptr;
    const bool isDirB = dynamic_cast<const DirPair*>(&b) != nullptr;

    //directories second last
    if (isDirA)
        return false;
    else if (isDirB)
        return true;

    const FilePair* fileObjA = dynamic_cast<const FilePair*>(&a);
    const FilePair* fileObjB = dynamic_cast<const FilePair*>(&b);

    //then symlinks
    if (!fileObjA)
        return false;
    else if (!fileObjB)
        return true;

    //return list beginning with largest files first
    return makeSortDirection(std::less<UInt64>(), Int2Type<ascending>())(fileObjA->getFileSize<side>(), fileObjB->getFileSize<side>());
}


template <bool ascending, SelectedSide side> inline
bool lessFiletime(const FileSystemObject& a, const FileSystemObject& b)
{
    if (a.isEmpty<side>())
        return false;  //empty rows always last
    else if (b.isEmpty<side>())
        return true;  //empty rows always last


    const FilePair* fileObjA = dynamic_cast<const FilePair*>(&a);
    const FilePair* fileObjB = dynamic_cast<const FilePair*>(&b);

    const SymlinkPair* linkObjA = dynamic_cast<const SymlinkPair*>(&a);
    const SymlinkPair* linkObjB = dynamic_cast<const SymlinkPair*>(&b);

    if (!fileObjA && !linkObjA)
        return false; //directories last
    else if (!fileObjB && !linkObjB)
        return true;  //directories last

    zen::Int64 dateA = fileObjA ? fileObjA->getLastWriteTime<side>() : linkObjA->getLastWriteTime<side>();
    zen::Int64 dateB = fileObjB ? fileObjB->getLastWriteTime<side>() : linkObjB->getLastWriteTime<side>();

    //return list beginning with newest files first
    return makeSortDirection(std::less<Int64>(), Int2Type<ascending>())(dateA, dateB);
}


template <bool ascending, SelectedSide side> inline
bool lessExtension(const FileSystemObject& a, const FileSystemObject& b)
{
    if (a.isEmpty<side>())
        return false;  //empty rows always last
    else if (b.isEmpty<side>())
        return true;  //empty rows always last

    if (dynamic_cast<const DirPair*>(&a))
        return false; //directories last
    else if (dynamic_cast<const DirPair*>(&b))
        return true;  //directories last

    auto getExtension = [&](const FileSystemObject& fsObj) -> Zstring
    {
        const Zstring& shortName = fsObj.getShortName<side>();
        const size_t pos = shortName.rfind(Zchar('.'));
        return pos == Zstring::npos ? Zstring() : Zstring(shortName.c_str() + pos + 1);
    };

    return makeSortDirection(LessFilename(), Int2Type<ascending>())(getExtension(a), getExtension(b));
}


template <bool ascending> inline
bool lessCmpResult(const FileSystemObject& a, const FileSystemObject& b)
{
    //presort result: equal shall appear at end of list
    if (a.getCategory() == FILE_EQUAL)
        return false;
    if (b.getCategory() == FILE_EQUAL)
        return true;

    return makeSortDirection(std::less<CompareFilesResult>(), Int2Type<ascending>())(a.getCategory(), b.getCategory());
}


template <bool ascending> inline
bool lessSyncDirection(const FileSystemObject& a, const FileSystemObject& b)
{
    return makeSortDirection(std::less<SyncOperation>(), Int2Type<ascending>())(a.getSyncOperation(), b.getSyncOperation());
}
}

#endif // SORTING_H_INCLUDED
