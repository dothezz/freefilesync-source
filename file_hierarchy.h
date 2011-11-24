// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef FILEHIERARCHY_H_INCLUDED
#define FILEHIERARCHY_H_INCLUDED

#include <map>
#include <string>
#include <unordered_set>
#include <memory>
#include <zen/zstring.h>
#include <zen/fixed_list.h>
#include "structures.h"
#include <zen/int64.h>
#include <zen/file_id_def.h>
#include "structures.h"
#include "lib/hard_filter.h"


namespace zen
{
struct FileDescriptor
{
    FileDescriptor() {}
    FileDescriptor(Int64  lastWriteTimeRawIn,
                   UInt64 fileSizeIn,
                   const FileId& idIn) :
        lastWriteTimeRaw(lastWriteTimeRawIn),
        fileSize(fileSizeIn),
        id(idIn) {}

    Int64  lastWriteTimeRaw;   //number of seconds since Jan. 1st 1970 UTC, same semantics like time_t (== signed long)
    UInt64 fileSize;
    FileId id; //optional! (however, always set on Linux, and *generally* available on Windows)
};


struct LinkDescriptor
{
    enum LinkType
    {
        TYPE_DIR, //Windows: dir  symlink; Linux: dir symlink
        TYPE_FILE //Windows: file symlink; Linux: file symlink or broken link (or other symlink, pathological)
    };

    LinkDescriptor() : type(TYPE_FILE) {}
    LinkDescriptor(Int64 lastWriteTimeRawIn,
                   const Zstring& targetPathIn,
                   LinkType lt) :
        lastWriteTimeRaw(lastWriteTimeRawIn),
        targetPath(targetPathIn),
        type(lt) {}

    Int64    lastWriteTimeRaw; //number of seconds since Jan. 1st 1970 UTC, same semantics like time_t (== signed long)
    Zstring  targetPath;       //symlink "content", may be empty if determination failed
    LinkType type;             //type is required for Windows only! On Linux there is no such thing => consider this when comparing Symbolic Links!
};


enum SelectedSide
{
    LEFT_SIDE,
    RIGHT_SIDE
};

template <SelectedSide side>
struct OtherSide;

template <>
struct OtherSide<LEFT_SIDE> { static const SelectedSide result = RIGHT_SIDE; };

template <>
struct OtherSide<RIGHT_SIDE> { static const SelectedSide result = LEFT_SIDE; };


class BaseDirMapping;
class DirMapping;
class FileMapping;
class SymLinkMapping;
class FileSystemObject;

//------------------------------------------------------------------
/*
ERD:
	DirContainer 1 -----> 0..n DirContainer
	DirContainer 1 -----> 0..n FileDescriptor
	DirContainer 1 -----> 0..n LinkDescriptor
*/

struct DirContainer
{
    //------------------------------------------------------------------
    typedef std::map<Zstring, DirContainer,   LessFilename> DirList;  //
    typedef std::map<Zstring, FileDescriptor, LessFilename> FileList; //key: shortName
    typedef std::map<Zstring, LinkDescriptor, LessFilename> LinkList; //
    //------------------------------------------------------------------

    DirList  dirs;  //contained directories
    FileList files; //contained files
    LinkList links; //contained symlinks (note: only symlinks that are not treated as their target are placed here!)

    //convenience
    DirContainer& addSubDir(const Zstring& shortName)
    {
        //use C++11 emplace when available
        return dirs.insert(std::make_pair(shortName, DirContainer())).first->second;
    }

    void addSubFile(const Zstring& shortName, const FileDescriptor& fileData)
    {
        files.insert(std::make_pair(shortName, fileData));
    }

    void addSubLink(const Zstring& shortName, const LinkDescriptor& linkData)
    {
        links.insert(std::make_pair(shortName, linkData));
    }
};

//------------------------------------------------------------------
/*    inheritance diagram:

                  ObjectMgr
                     /|\
                      |
               FileSystemObject         HierarchyObject
                     /|\                      /|\
       _______________|______________    ______|______
      |               |              |  |             |
SymLinkMapping    FileMapping     DirMapping    BaseDirMapping
*/

//------------------------------------------------------------------
class HierarchyObject
{
    friend class DirMapping;
    friend class FileSystemObject;

    typedef zen::FixedList<FileMapping>    SubFileVec; //MergeSides::execute() requires a structure that doesn't invalidate pointers after push_back()
    typedef zen::FixedList<SymLinkMapping> SubLinkVec; //Note: deque<> has circular dependency in VCPP!
    typedef zen::FixedList<DirMapping>     SubDirVec;

public:
    DirMapping& addSubDir(const Zstring& shortNameLeft,
                          const Zstring& shortNameRight);

    template <SelectedSide side>
    DirMapping& addSubDir(const Zstring& shortName); //dir exists on one side only


    FileMapping& addSubFile(const Zstring&        shortNameLeft,
                            const FileDescriptor& left,          //file exists on both sides
                            CompareFilesResult    defaultCmpResult,
                            const Zstring&        shortNameRight,
                            const FileDescriptor& right);

    template <SelectedSide side>
    FileMapping& addSubFile(const Zstring&          shortNameRight, //file exists on one side only
                            const FileDescriptor&   right);

    SymLinkMapping& addSubLink(const Zstring&        shortNameLeft,
                               const LinkDescriptor& left,  //link exists on both sides
                               CompareSymlinkResult  defaultCmpResult,
                               const Zstring&        shortNameRight,
                               const LinkDescriptor& right);

    template <SelectedSide side>
    SymLinkMapping& addSubLink(const Zstring&        shortName, //link exists on one side only
                               const LinkDescriptor& descr);

    const SubFileVec& refSubFiles() const { return subFiles; }
    /**/  SubFileVec& refSubFiles()       { return subFiles; }

    const SubLinkVec& refSubLinks() const { return subLinks; }
    /**/  SubLinkVec& refSubLinks()       { return subLinks; }

    const SubDirVec& refSubDirs() const { return subDirs; }
    /**/  SubDirVec& refSubDirs()       { return subDirs; }

    BaseDirMapping& getRoot() { return root_; }

protected:
    HierarchyObject(const Zstring& relativeNamePf,
                    BaseDirMapping& baseMap) :
        objRelNamePf(relativeNamePf),
        root_(baseMap) {}

    ~HierarchyObject() {} //don't need polymorphic deletion

    virtual void flip();

    void removeEmptyRec();

private:
    virtual void notifySyncCfgChanged() {}

    HierarchyObject(const HierarchyObject&);            //this class is referenced by it's child elements => make it non-copyable/movable!
    HierarchyObject& operator=(const HierarchyObject&); //

    const Zstring& getObjRelativeNamePf() const { return objRelNamePf; }

    SubFileVec subFiles; //contained file maps
    SubLinkVec subLinks; //contained symbolic link maps
    SubDirVec  subDirs;  //contained directory maps

    Zstring objRelNamePf;
    BaseDirMapping& root_;
};


//------------------------------------------------------------------
class BaseDirMapping : public HierarchyObject //synchronization base directory
{
public:
    BaseDirMapping(const Zstring& dirPostfixedLeft,
                   bool dirExistsLeft,
                   const Zstring& dirPostfixedRight,
                   bool dirExistsRight,
                   const HardFilter::FilterRef& filterIn) :
        HierarchyObject(Zstring(), *this),
        filter(filterIn),
        baseDirPfL(dirPostfixedLeft),
        baseDirPfR(dirPostfixedRight),
        dirExistsLeft_(dirExistsLeft),
        dirExistsRight_(dirExistsRight) {}

    template <SelectedSide side> const Zstring& getBaseDirPf() const; //base sync directory postfixed with FILE_NAME_SEPARATOR
    static void removeEmpty(BaseDirMapping& baseDir) { baseDir.removeEmptyRec(); }; //physically remove all invalid entries (where both sides are empty) recursively

    const HardFilter::FilterRef& getFilter() const;
    template <SelectedSide side> bool wasExisting() const; //status of directory existence at the time of comparison!

    virtual void flip();

private:
    BaseDirMapping(const BaseDirMapping&);            //this class is referenced by HierarchyObject => make it non-copyable/movable!
    BaseDirMapping& operator=(const BaseDirMapping&); //

    //this member is currently not used by the business logic -> may be removed!
    HardFilter::FilterRef filter;

    Zstring baseDirPfL; //base sync dir postfixed
    Zstring baseDirPfR; //

    bool dirExistsLeft_;
    bool dirExistsRight_;
};


template <> inline
const Zstring& BaseDirMapping::getBaseDirPf<LEFT_SIDE>() const { return baseDirPfL; }

template <> inline
const Zstring& BaseDirMapping::getBaseDirPf<RIGHT_SIDE>() const { return baseDirPfR; }


//get rid of shared_ptr indirection
template < class IterTy,     //underlying iterator type
         class U >           //target object type
class DerefIter : public std::iterator<std::bidirectional_iterator_tag, U>
{
public:
    DerefIter() {}
    DerefIter(IterTy it) : iter(it) {}
    DerefIter(const DerefIter& other) : iter(other.iter) {}
    DerefIter& operator++() { ++iter; return *this; }
    DerefIter& operator--() { --iter; return *this; }
    DerefIter operator++(int) { DerefIter tmp(*this); operator++(); return tmp; }
    DerefIter operator--(int) { DerefIter tmp(*this); operator--(); return tmp; }
    inline friend ptrdiff_t operator-(const DerefIter& lhs, const DerefIter& rhs) { return lhs.iter - rhs.iter; }
    inline friend bool operator==(const DerefIter& lhs, const DerefIter& rhs) { return lhs.iter == rhs.iter; }
    inline friend bool operator!=(const DerefIter& lhs, const DerefIter& rhs) { return !(lhs == rhs); }
    U& operator* () { return  **iter; }
    U* operator->() { return &** iter; }
private:
    IterTy iter;
};

typedef std::vector<std::shared_ptr<BaseDirMapping>> FolderComparison; //make sure pointers to sub-elements remain valid
//don't change this back to std::vector<BaseDirMapping> too easily: comparison uses push_back to add entries which may result in a full copy!

DerefIter<typename FolderComparison::iterator, BaseDirMapping> inline begin(FolderComparison& vect) { return vect.begin(); }
DerefIter<typename FolderComparison::iterator, BaseDirMapping> inline end  (FolderComparison& vect) { return vect.end  (); }
DerefIter<typename FolderComparison::const_iterator, const BaseDirMapping> inline begin(const FolderComparison& vect) { return vect.begin(); }
DerefIter<typename FolderComparison::const_iterator, const BaseDirMapping> inline end  (const FolderComparison& vect) { return vect.end  (); }

//------------------------------------------------------------------
class FSObjectVisitor
{
public:
    virtual ~FSObjectVisitor() {}
    virtual void visit(const FileMapping&    fileObj) = 0;
    virtual void visit(const SymLinkMapping& linkObj) = 0;
    virtual void visit(const DirMapping&     dirObj)  = 0;
};

//inherit from this class to allow safe random access by id instead of unsafe raw pointer
//allow for similar semantics like std::weak_ptr without having to use std::shared_ptr
template <class T>
class ObjectMgr
{
public:
    typedef const ObjectMgr* ObjectID;

    ObjectID getId() { activeObjects().insert(this); return this; }
    //unfortunately we need to keep this method non-const to get non-const "this" pointer
    //we could instead put this into the constructor, but temporaries created by STL could lead to some overhead

    static T* retrieve(ObjectID id) //returns NULL if object is not valid anymore
    {
        auto iter = activeObjects().find(const_cast<ObjectMgr*>(id));
        return static_cast<T*>(iter == activeObjects().end() ? NULL : *iter); //static down-cast
    }

protected:
    ObjectMgr() {}
    ~ObjectMgr() { activeObjects().erase(this); }

private:
    ObjectMgr(const ObjectMgr& rhs);            //
    ObjectMgr& operator=(const ObjectMgr& rhs); //it's not well-defined what coping an objects means regarding object-identity in this context

    static std::unordered_set<ObjectMgr*>& activeObjects()
    {
        static std::unordered_set<ObjectMgr*> inst; //external linkage (even if in header file!)
        return inst;
    }
};
//------------------------------------------------------------------

class FileSystemObject : public ObjectMgr<FileSystemObject>
{
public:
    virtual void accept(FSObjectVisitor& visitor) const = 0;

    Zstring getObjShortName   () const; //same as getShortName() but also returns value if either side is empty
    Zstring getObjRelativeName() const; //same as getRelativeName() but also returns value if either side is empty
    template <SelectedSide side>           bool isEmpty()         const;
    template <SelectedSide side> const Zstring& getShortName()    const;
    template <SelectedSide side>       Zstring  getRelativeName() const; //get name relative to base sync dir without FILE_NAME_SEPARATOR prefix
    template <SelectedSide side> const Zstring& getBaseDirPf()    const; //base sync directory postfixed with FILE_NAME_SEPARATOR
    template <SelectedSide side>       Zstring  getFullName()     const; //getFullName() == getBaseDirPf() + getRelativeName()

    //comparison result
    virtual CompareFilesResult getCategory() const = 0;
    virtual std::wstring getCatConflict() const = 0; //only filled if getCategory() == FILE_CONFLICT
    //sync operation
    virtual SyncOperation testSyncOperation(SyncDirection testSyncDir, bool active) const;
    virtual SyncOperation getSyncOperation() const;
    std::wstring getSyncOpConflict() const; //return conflict when determining sync direction or during categorization

    //sync settings
    void setSyncDir(SyncDirection newDir);
    void setSyncDirConflict(const std::wstring& description); //set syncDir = SYNC_DIR_NONE + fill conflict description
    SyncDirection getSyncDir() const { return syncDir; }

    bool isActive() const;
    void setActive(bool active);

    template <SelectedSide side> void removeObject();    //removes file or directory (recursively!) without physically removing the element: used by manual deletion

    bool isEmpty() const; //true, if both sides are empty

    const HierarchyObject& parent() const { return parent_; }
    /**/  HierarchyObject& parent()       { return parent_; }
    const BaseDirMapping& root() const  { return parent_.getRoot(); }
    /**/  BaseDirMapping& root()        { return parent_.getRoot(); }

protected:
    FileSystemObject(const Zstring& shortNameLeft, const Zstring& shortNameRight, HierarchyObject& parentObj) :
        selectedForSynchronization(true),
        syncDir(SYNC_DIR_NONE),
        shortNameLeft_(shortNameLeft),
        shortNameRight_(shortNameRight),
        //shortNameRight_(shortNameRight == shortNameLeft ? shortNameLeft : shortNameRight), -> strangely doesn't seem to shrink peak memory consumption at all!
        parent_(parentObj)
    {
        parent_.notifySyncCfgChanged();
    }

    ~FileSystemObject() {} //don't need polymorphic deletion
    //mustn't call parent here, it is already partially destroyed and nothing more than a pure HierarchyObject!


    virtual void flip();
    virtual void notifySyncCfgChanged() { parent().notifySyncCfgChanged(); /*propagate!*/ }

    void copyToL();
    void copyToR();

private:
    virtual void removeObjectL() = 0;
    virtual void removeObjectR() = 0;

    bool selectedForSynchronization;
    SyncDirection syncDir;
    std::wstring syncDirConflict; //non-empty if we have a conflict setting sync-direction

    Zstring shortNameLeft_;   //slightly redundant under linux, but on windows the "same" filenames can differ in case
    Zstring shortNameRight_;  //use as indicator: an empty name means: not existing!

    HierarchyObject& parent_;
};

//------------------------------------------------------------------
class DirMapping : public FileSystemObject, public HierarchyObject
{
    friend class CompareProcess; //only CompareProcess shall be allowed to change cmpResult
    friend class HierarchyObject;

public:
    virtual void accept(FSObjectVisitor& visitor) const;

    virtual CompareFilesResult getCategory() const;
    CompareDirResult getDirCategory() const; //returns actually used subset of CompareFilesResult
    virtual std::wstring getCatConflict() const;

    DirMapping(const Zstring& shortNameLeft,  //use empty shortname if "not existing"
               const Zstring& shortNameRight, //
               HierarchyObject& parentObj) :
        FileSystemObject(shortNameLeft, shortNameRight, parentObj),
        HierarchyObject(getObjRelativeName() + FILE_NAME_SEPARATOR, parentObj.getRoot()),
        syncOpBuffered(SO_DO_NOTHING),
        syncOpUpToDate(false)
    {
        assert(!shortNameLeft.empty() || !shortNameRight.empty());

        if (shortNameRight.empty())
            cmpResult = DIR_LEFT_SIDE_ONLY;
        else if (shortNameLeft.empty())
            cmpResult = DIR_RIGHT_SIDE_ONLY;
        else
        {
            if (shortNameLeft == shortNameRight)
                cmpResult = DIR_EQUAL;
            else
                cmpResult = DIR_DIFFERENT_METADATA;
        }
    }

    virtual SyncOperation getSyncOperation() const;

    template <SelectedSide side> void copyTo(); //copy dir

private:
    virtual void flip();
    virtual void removeObjectL();
    virtual void removeObjectR();
    virtual void notifySyncCfgChanged() { syncOpUpToDate = false; FileSystemObject::notifySyncCfgChanged(); HierarchyObject::notifySyncCfgChanged(); }
    //------------------------------------------------------------------

    //categorization
    CompareDirResult cmpResult;

    mutable SyncOperation syncOpBuffered; //determining sync-op for directory may be expensive as it depends on child-objects -> buffer it
    mutable bool syncOpUpToDate;         //
};

//------------------------------------------------------------------
class FileMapping : public FileSystemObject
{
    friend class CompareProcess;  //only CompareProcess shall be allowed to change cmpResult
    friend class HierarchyObject; //construction

public:
    virtual void accept(FSObjectVisitor& visitor) const;

    FileMapping(const Zstring&        shortNameLeft, //use empty string if "not existing"
                const FileDescriptor& left,
                CompareFilesResult    defaultCmpResult,
                const Zstring&        shortNameRight, //
                const FileDescriptor& right,
                HierarchyObject& parentObj) :
        FileSystemObject(shortNameLeft, shortNameRight, parentObj),
        cmpResult(defaultCmpResult),
        dataLeft(left),
        dataRight(right),
        moveFileRef(NULL) {}

    template <SelectedSide side> Int64  getLastWriteTime() const;
    template <SelectedSide side> UInt64 getFileSize     () const;
    template <SelectedSide side> FileId getFileId       () const;
    template <SelectedSide side> const Zstring getExtension() const;

    void setMoveRef(ObjectID refId) { moveFileRef = refId; } //reference to corresponding renamed file
    ObjectID getMoveRef() const { return moveFileRef; } //may be NULL

    virtual CompareFilesResult getCategory() const;
    virtual std::wstring getCatConflict() const;

    virtual SyncOperation testSyncOperation(SyncDirection testSyncDir, bool active) const;
    virtual SyncOperation getSyncOperation() const;

    template <SelectedSide side> void syncTo(const FileDescriptor& descrTarget, const FileDescriptor* descrSource = NULL); //copy + update file attributes (optional)

private:
    template <CompareFilesResult res>
    void setCategory();
    void setCategoryConflict(const std::wstring& description);

    virtual void flip();
    virtual void removeObjectL();
    virtual void removeObjectR();
    //------------------------------------------------------------------

    //categorization
    CompareFilesResult cmpResult;
    std::wstring cmpConflictDescr; //only filled if cmpResult == FILE_CONFLICT

    FileDescriptor dataLeft;
    FileDescriptor dataRight;

    ObjectID moveFileRef;
};

//------------------------------------------------------------------
class SymLinkMapping : public FileSystemObject //this class models a TRUE symbolic link, i.e. one that is NEVER dereferenced: deref-links should be directly placed in class File/DirMapping
{
    friend class CompareProcess;  //only CompareProcess shall be allowed to change cmpResult
    friend class HierarchyObject; //construction

public:
    virtual void accept(FSObjectVisitor& visitor) const;

    template <SelectedSide side> zen::Int64 getLastWriteTime() const; //write time of the link, NOT target!
    template <SelectedSide side> LinkDescriptor::LinkType getLinkType() const;
    template <SelectedSide side> const Zstring& getTargetPath() const;

    virtual CompareFilesResult getCategory() const;
    CompareSymlinkResult getLinkCategory()   const; //returns actually used subset of CompareFilesResult
    virtual std::wstring getCatConflict() const;

    SymLinkMapping(const Zstring&         shortNameLeft, //use empty string if "not existing"
                   const LinkDescriptor&  left,
                   CompareSymlinkResult   defaultCmpResult,
                   const Zstring&         shortNameRight, //use empty string if "not existing"
                   const LinkDescriptor&  right,
                   HierarchyObject& parentObj) :
        FileSystemObject(shortNameLeft, shortNameRight, parentObj),
        cmpResult(defaultCmpResult),
        dataLeft(left),
        dataRight(right) {}

    template <SelectedSide side> void copyTo(); //copy

private:
    virtual void flip();
    virtual void removeObjectL();
    virtual void removeObjectR();

    template <CompareSymlinkResult res>
    void setCategory();
    void setCategoryConflict(const std::wstring& description);
    //------------------------------------------------------------------

    //categorization
    CompareSymlinkResult cmpResult;
    std::wstring cmpConflictDescr; //only filled if cmpResult == SYMLINK_CONFLICT

    LinkDescriptor dataLeft;
    LinkDescriptor dataRight;
};

//------------------------------------------------------------------

//generic type descriptions (usecase CSV legend, sync config)
std::wstring getCategoryDescription(CompareFilesResult cmpRes);
std::wstring getSyncOpDescription  (SyncOperation op);

//item-specific type descriptions
std::wstring getCategoryDescription(const FileSystemObject& fsObj);
std::wstring getSyncOpDescription  (const FileSystemObject& fsObj);

//------------------------------------------------------------------































//---------------Inline Implementation---------------------------------------------------
inline //inline virtual... admittedly its use may be limited
void FileMapping::accept(FSObjectVisitor& visitor) const
{
    visitor.visit(*this);
}


inline
void DirMapping::accept(FSObjectVisitor& visitor) const
{
    visitor.visit(*this);
}


inline
void SymLinkMapping::accept(FSObjectVisitor& visitor) const
{
    visitor.visit(*this);
}


inline
CompareFilesResult FileMapping::getCategory() const
{
    return cmpResult;
}


inline
std::wstring FileMapping::getCatConflict() const
{
    return cmpConflictDescr;
}


inline
CompareFilesResult DirMapping::getCategory() const
{
    return convertToFilesResult(cmpResult);
}


inline
CompareDirResult DirMapping::getDirCategory() const
{
    return cmpResult;
}


inline
std::wstring DirMapping::getCatConflict() const
{
    return std::wstring();
}


inline
void FileSystemObject::setSyncDir(SyncDirection newDir)
{
    syncDir = newDir; //should be safe by design
    syncDirConflict.clear();

    notifySyncCfgChanged();
}


inline
void FileSystemObject::setSyncDirConflict(const std::wstring& description)
{
    syncDir = SYNC_DIR_NONE;
    syncDirConflict = description;

    notifySyncCfgChanged();
}


inline
std::wstring FileSystemObject::getSyncOpConflict() const
{
    return syncDirConflict;
}


inline
bool FileSystemObject::isActive() const
{
    return selectedForSynchronization;
}


inline
void FileSystemObject::setActive(bool active)
{
    selectedForSynchronization = active;

    notifySyncCfgChanged();
}


template <> inline
bool FileSystemObject::isEmpty<LEFT_SIDE>() const
{
    return shortNameLeft_.empty();
}


template <> inline
bool FileSystemObject::isEmpty<RIGHT_SIDE>() const
{
    return shortNameRight_.empty();
}


inline
bool FileSystemObject::isEmpty() const
{
    return isEmpty<LEFT_SIDE>() && isEmpty<RIGHT_SIDE>();
}


template <> inline
const Zstring& FileSystemObject::getShortName<LEFT_SIDE>() const
{
    return shortNameLeft_; //empty if not existing
}


template <> inline
const Zstring& FileSystemObject::getShortName<RIGHT_SIDE>() const
{
    return shortNameRight_; //empty if not existing
}


template <SelectedSide side>
inline
Zstring FileSystemObject::getRelativeName() const
{
    return isEmpty<side>() ? Zstring() : parent_.getObjRelativeNamePf() + getShortName<side>();
}


inline
Zstring FileSystemObject::getObjRelativeName() const
{
    return parent_.getObjRelativeNamePf() + getObjShortName();
}


inline
Zstring FileSystemObject::getObjShortName() const
{
    return isEmpty<LEFT_SIDE>() ? getShortName<RIGHT_SIDE>() : getShortName<LEFT_SIDE>();
}


template <SelectedSide side>
inline
Zstring FileSystemObject::getFullName() const
{
    return isEmpty<side>() ? Zstring() : getBaseDirPf<side>() + parent_.getObjRelativeNamePf() + getShortName<side>();
}


template <> inline
const Zstring& FileSystemObject::getBaseDirPf<LEFT_SIDE>() const
{
    return root().getBaseDirPf<LEFT_SIDE>();
}


template <> inline
const Zstring& FileSystemObject::getBaseDirPf<RIGHT_SIDE>() const
{
    return root().getBaseDirPf<RIGHT_SIDE>();
}


template <> inline
void FileSystemObject::removeObject<LEFT_SIDE>()
{
    shortNameLeft_.clear();
    removeObjectL();

    setSyncDir(SYNC_DIR_NONE); //calls notifySyncCfgChanged()
}


template <> inline
void FileSystemObject::removeObject<RIGHT_SIDE>()
{
    shortNameRight_.clear();
    removeObjectR();

    setSyncDir(SYNC_DIR_NONE); //calls notifySyncCfgChanged()
}


inline
void FileSystemObject::copyToL()
{
    assert(!isEmpty());
    shortNameLeft_ = shortNameRight_;
    setSyncDir(SYNC_DIR_NONE);
}


inline
void FileSystemObject::copyToR()
{
    assert(!isEmpty());
    shortNameRight_ = shortNameLeft_;
    setSyncDir(SYNC_DIR_NONE);
}


inline
void FileSystemObject::flip()
{
    std::swap(shortNameLeft_, shortNameRight_);

    notifySyncCfgChanged();
}


inline
void HierarchyObject::flip()
{
    std::for_each(refSubFiles().begin(), refSubFiles().end(), std::mem_fun_ref(&FileMapping   ::flip));
    std::for_each(refSubDirs ().begin(), refSubDirs ().end(), std::mem_fun_ref(&DirMapping    ::flip));
    std::for_each(refSubLinks().begin(), refSubLinks().end(), std::mem_fun_ref(&SymLinkMapping::flip));
}


inline
DirMapping& HierarchyObject::addSubDir(const Zstring& shortNameLeft,
                                       const Zstring& shortNameRight)
{
    subDirs.emplace_back(shortNameLeft, shortNameRight, *this);
    return subDirs.back();
}


template <> inline
DirMapping& HierarchyObject::addSubDir<LEFT_SIDE>(const Zstring& shortName)
{
    subDirs.emplace_back(shortName, Zstring(), *this);
    return subDirs.back();
}


template <> inline
DirMapping& HierarchyObject::addSubDir<RIGHT_SIDE>(const Zstring& shortName)
{
    subDirs.emplace_back(Zstring(), shortName, *this);
    return subDirs.back();
}


inline
FileMapping& HierarchyObject::addSubFile(
    const Zstring&        shortNameLeft,
    const FileDescriptor& left,          //file exists on both sides
    CompareFilesResult    defaultCmpResult,
    const Zstring&        shortNameRight,
    const FileDescriptor& right)
{
    subFiles.emplace_back(shortNameLeft, left, defaultCmpResult, shortNameRight, right, *this);
    return subFiles.back();
}


template <> inline
FileMapping& HierarchyObject::addSubFile<LEFT_SIDE>(const Zstring& shortName, const FileDescriptor& descr)
{
    subFiles.emplace_back(shortName, descr, FILE_LEFT_SIDE_ONLY, Zstring(), FileDescriptor(), *this);
    return subFiles.back();
}


template <> inline
FileMapping& HierarchyObject::addSubFile<RIGHT_SIDE>(const Zstring& shortName, const FileDescriptor& descr)
{
    subFiles.emplace_back(Zstring(), FileDescriptor(), FILE_RIGHT_SIDE_ONLY, shortName, descr, *this);
    return subFiles.back();
}


inline
SymLinkMapping& HierarchyObject::addSubLink(
    const Zstring&        shortNameLeft,
    const LinkDescriptor& left,  //link exists on both sides
    CompareSymlinkResult  defaultCmpResult,
    const Zstring&        shortNameRight,
    const LinkDescriptor& right)
{
    subLinks.emplace_back(shortNameLeft, left, defaultCmpResult, shortNameRight, right, *this);
    return subLinks.back();
}


template <> inline
SymLinkMapping& HierarchyObject::addSubLink<LEFT_SIDE>(const Zstring& shortName, const LinkDescriptor& descr)
{
    subLinks.emplace_back(shortName, descr, SYMLINK_LEFT_SIDE_ONLY, Zstring(), LinkDescriptor(), *this);
    return subLinks.back();
}


template <> inline
SymLinkMapping& HierarchyObject::addSubLink<RIGHT_SIDE>(const Zstring& shortName, const LinkDescriptor& descr)
{
    subLinks.emplace_back(Zstring(), LinkDescriptor(), SYMLINK_RIGHT_SIDE_ONLY, shortName, descr, *this);
    return subLinks.back();
}


inline
void BaseDirMapping::flip()
{
    HierarchyObject::flip();
    std::swap(baseDirPfL, baseDirPfR);
}


inline
void DirMapping::flip()
{

    HierarchyObject ::flip(); //call base class versions
    FileSystemObject::flip(); //

    //swap compare result
    switch (cmpResult)
    {
        case DIR_LEFT_SIDE_ONLY:
            cmpResult = DIR_RIGHT_SIDE_ONLY;
            break;
        case DIR_RIGHT_SIDE_ONLY:
            cmpResult = DIR_LEFT_SIDE_ONLY;
            break;
        case DIR_EQUAL:
        case DIR_DIFFERENT_METADATA:
            break;
    }
}


inline
void DirMapping::removeObjectL()
{
    cmpResult = isEmpty<RIGHT_SIDE>() ? DIR_EQUAL : DIR_RIGHT_SIDE_ONLY;
    std::for_each(refSubFiles().begin(), refSubFiles().end(), std::mem_fun_ref(&FileSystemObject::removeObject<LEFT_SIDE>));
    std::for_each(refSubLinks().begin(), refSubLinks().end(), std::mem_fun_ref(&FileSystemObject::removeObject<LEFT_SIDE>));
    std::for_each(refSubDirs(). begin(), refSubDirs() .end(), std::mem_fun_ref(&FileSystemObject::removeObject<LEFT_SIDE>));
}


inline
void DirMapping::removeObjectR()
{
    cmpResult = isEmpty<LEFT_SIDE>() ? DIR_EQUAL : DIR_LEFT_SIDE_ONLY;
    std::for_each(refSubFiles().begin(), refSubFiles().end(), std::mem_fun_ref(&FileSystemObject::removeObject<RIGHT_SIDE>));
    std::for_each(refSubLinks().begin(), refSubLinks().end(), std::mem_fun_ref(&FileSystemObject::removeObject<RIGHT_SIDE>));
    std::for_each(refSubDirs(). begin(), refSubDirs(). end(), std::mem_fun_ref(&FileSystemObject::removeObject<RIGHT_SIDE>));
}


inline
const HardFilter::FilterRef& BaseDirMapping::getFilter() const
{
    return filter;
}


template <> inline
bool BaseDirMapping::wasExisting<LEFT_SIDE>() const
{
    return dirExistsLeft_;
}


template <> inline
bool BaseDirMapping::wasExisting<RIGHT_SIDE>() const
{
    return dirExistsRight_;
}


inline
void FileMapping::flip()
{
    FileSystemObject::flip(); //call base class version

    //swap compare result
    switch (cmpResult)
    {
        case FILE_LEFT_SIDE_ONLY:
            cmpResult = FILE_RIGHT_SIDE_ONLY;
            break;
        case FILE_RIGHT_SIDE_ONLY:
            cmpResult = FILE_LEFT_SIDE_ONLY;
            break;
        case FILE_LEFT_NEWER:
            cmpResult = FILE_RIGHT_NEWER;
            break;
        case FILE_RIGHT_NEWER:
            cmpResult = FILE_LEFT_NEWER;
            break;
        case FILE_DIFFERENT:
        case FILE_EQUAL:
        case FILE_DIFFERENT_METADATA:
        case FILE_CONFLICT:
            break;
    }

    std::swap(dataLeft, dataRight);
}


template <CompareFilesResult res> inline
void FileMapping::setCategory()
{
    cmpResult = res;
}

template <> inline
void FileMapping::setCategory<FILE_CONFLICT>(); //if conflict is detected, use setCategoryConflict! => method is not defined!


inline
void FileMapping::setCategoryConflict(const std::wstring& description)
{
    cmpResult = FILE_CONFLICT;
    cmpConflictDescr = description;
}


inline
void FileMapping::removeObjectL()
{
    cmpResult = isEmpty<RIGHT_SIDE>() ? FILE_EQUAL : FILE_RIGHT_SIDE_ONLY;
    dataLeft  = FileDescriptor();
}


inline
void FileMapping::removeObjectR()
{
    cmpResult = isEmpty<LEFT_SIDE>() ? FILE_EQUAL : FILE_LEFT_SIDE_ONLY;
    dataRight = FileDescriptor();
}


template <> inline
zen::Int64 FileMapping::getLastWriteTime<LEFT_SIDE>() const
{
    return dataLeft.lastWriteTimeRaw;
}


template <> inline
zen::Int64 FileMapping::getLastWriteTime<RIGHT_SIDE>() const
{
    return dataRight.lastWriteTimeRaw;
}


template <> inline
zen::UInt64 FileMapping::getFileSize<LEFT_SIDE>() const
{
    return dataLeft.fileSize;
}


template <> inline
zen::UInt64 FileMapping::getFileSize<RIGHT_SIDE>() const
{
    return dataRight.fileSize;
}


template <> inline
FileId FileMapping::getFileId<LEFT_SIDE>() const
{
    return dataLeft.id;
}


template <> inline
FileId FileMapping::getFileId<RIGHT_SIDE>() const
{
    return dataRight.id;
}


template <SelectedSide side> inline
const Zstring FileMapping::getExtension() const
{
    //attention: Zstring::AfterLast() returns whole string if char not found! -> don't use
    const Zstring& shortName = getShortName<side>();

    const size_t pos = shortName.rfind(Zchar('.'));
    return pos == Zstring::npos ?
           Zstring() :
           Zstring(shortName.c_str() + pos + 1);
}


template <> inline
void FileMapping::syncTo<LEFT_SIDE>(const FileDescriptor& descrTarget, const FileDescriptor* descrSource) //copy + update file attributes
{
    dataLeft = descrTarget;
    if (descrSource)
        dataRight = *descrSource;

    moveFileRef = NULL;
    cmpResult = FILE_EQUAL;
    copyToL(); //copy FileSystemObject specific part
}


template <> inline
void FileMapping::syncTo<RIGHT_SIDE>(const FileDescriptor& descrTarget, const FileDescriptor* descrSource) //copy + update file attributes
{
    dataRight = descrTarget;
    if (descrSource)
        dataLeft = *descrSource;

    moveFileRef = NULL;
    cmpResult = FILE_EQUAL;
    copyToR(); //copy FileSystemObject specific part
}


template <> inline
void SymLinkMapping::copyTo<LEFT_SIDE>() //copy + update link attributes
{
    dataLeft  = dataRight;
    cmpResult = SYMLINK_EQUAL;
    copyToL(); //copy FileSystemObject specific part
}


template <> inline
void SymLinkMapping::copyTo<RIGHT_SIDE>() //copy + update link attributes
{
    dataRight = dataLeft;
    cmpResult = SYMLINK_EQUAL;
    copyToR(); //copy FileSystemObject specific part
}


template <> inline
void DirMapping::copyTo<LEFT_SIDE>()
{
    cmpResult = DIR_EQUAL;
    copyToL(); //copy FileSystemObject specific part
}


template <> inline
void DirMapping::copyTo<RIGHT_SIDE>()
{
    cmpResult = DIR_EQUAL;
    copyToR(); //copy FileSystemObject specific part
}


template <> inline
zen::Int64 SymLinkMapping::getLastWriteTime<LEFT_SIDE>() const
{
    return dataLeft.lastWriteTimeRaw;
}


template <> inline
zen::Int64 SymLinkMapping::getLastWriteTime<RIGHT_SIDE>() const
{
    return dataRight.lastWriteTimeRaw;
}


template <> inline
LinkDescriptor::LinkType SymLinkMapping::getLinkType<LEFT_SIDE>() const
{
    return dataLeft.type;
}


template <> inline
LinkDescriptor::LinkType SymLinkMapping::getLinkType<RIGHT_SIDE>() const
{
    return dataRight.type;
}


template <> inline
const Zstring& SymLinkMapping::getTargetPath<LEFT_SIDE>() const
{
    return dataLeft.targetPath;
}


template <> inline
const Zstring& SymLinkMapping::getTargetPath<RIGHT_SIDE>() const
{
    return dataRight.targetPath;
}


inline
CompareFilesResult SymLinkMapping::getCategory() const
{
    return convertToFilesResult(cmpResult);
}


inline
CompareSymlinkResult SymLinkMapping::getLinkCategory() const
{
    return cmpResult;
}


inline
std::wstring SymLinkMapping::getCatConflict() const
{
    return cmpConflictDescr;
}


inline
void SymLinkMapping::flip()
{
    FileSystemObject::flip(); //call base class versions

    switch (cmpResult)
    {
        case SYMLINK_LEFT_SIDE_ONLY:
            cmpResult = SYMLINK_RIGHT_SIDE_ONLY;
            break;
        case SYMLINK_RIGHT_SIDE_ONLY:
            cmpResult = SYMLINK_LEFT_SIDE_ONLY;
            break;
        case SYMLINK_LEFT_NEWER:
            cmpResult = SYMLINK_RIGHT_NEWER;
            break;
        case SYMLINK_RIGHT_NEWER:
            cmpResult = SYMLINK_LEFT_NEWER;
            break;
        case SYMLINK_EQUAL:
        case SYMLINK_DIFFERENT_METADATA:
        case SYMLINK_DIFFERENT:
        case SYMLINK_CONFLICT:
            break;
    }

    std::swap(dataLeft, dataRight);
}


inline
void SymLinkMapping::removeObjectL()
{
    cmpResult = isEmpty<RIGHT_SIDE>() ? SYMLINK_EQUAL : SYMLINK_RIGHT_SIDE_ONLY;
    dataLeft  = LinkDescriptor();
}


inline
void SymLinkMapping::removeObjectR()
{
    cmpResult = isEmpty<LEFT_SIDE>() ? SYMLINK_EQUAL : SYMLINK_LEFT_SIDE_ONLY;
    dataRight = LinkDescriptor();
}


template <CompareSymlinkResult res> inline
void SymLinkMapping::setCategory()
{
    cmpResult = res;
}


template <>
void SymLinkMapping::setCategory<SYMLINK_CONFLICT>(); //if conflict is detected, use setCategoryConflict! => method is not defined!


inline
void SymLinkMapping::setCategoryConflict(const std::wstring& description)
{
    cmpResult = SYMLINK_CONFLICT;
    cmpConflictDescr = description;
}
}

#endif // FILEHIERARCHY_H_INCLUDED
