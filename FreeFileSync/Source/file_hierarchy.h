// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0           *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#ifndef FILE_HIERARCHY_H_257235289645296
#define FILE_HIERARCHY_H_257235289645296

#include <map>
#include <cstddef> //required by GCC 4.8.1 to find ptrdiff_t
#include <string>
#include <memory>
#include <functional>
#include <unordered_set>
#include <zen/zstring.h>
#include <zen/fixed_list.h>
#include <zen/stl_tools.h>
#include <zen/file_id_def.h>
#include "structures.h"
#include "lib/hard_filter.h"
#include "fs/abstract.h"


namespace zen
{
using AFS = AbstractFileSystem;

struct FileDescriptor
{
    FileDescriptor() {}
    FileDescriptor(int64_t lastWriteTimeRawIn,
                   uint64_t fileSizeIn,
                   const AFS::FileId& idIn,
                   bool isSymlink) :
        lastWriteTimeRaw(lastWriteTimeRawIn),
        fileSize(fileSizeIn),
        fileId(idIn),
        isFollowedSymlink(isSymlink) {}

    int64_t lastWriteTimeRaw = 0; //number of seconds since Jan. 1st 1970 UTC, same semantics like time_t (== signed long)
    uint64_t fileSize = 0;
    AFS::FileId fileId {}; // optional!
    bool isFollowedSymlink = false;
};


struct LinkDescriptor
{
    LinkDescriptor() {}
    explicit LinkDescriptor(int64_t lastWriteTimeRawIn) : lastWriteTimeRaw(lastWriteTimeRawIn) {}

    int64_t lastWriteTimeRaw = 0; //number of seconds since Jan. 1st 1970 UTC, same semantics like time_t (== signed long)
};


struct FolderDescriptor
{
    FolderDescriptor() {}
    FolderDescriptor(bool isSymlink) :
        isFollowedSymlink(isSymlink) {}

    bool isFollowedSymlink = false;
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


template <SelectedSide side>
struct SelectParam;

template <>
struct SelectParam<LEFT_SIDE>
{
    template <class T>
    static T& ref(T& left, T& right) { return left; }
};

template <>
struct SelectParam<RIGHT_SIDE>
{
    template <class T>
    static T& ref(T& left, T& right) { return right; }
};

//------------------------------------------------------------------

struct FolderContainer
{
    //------------------------------------------------------------------
    using FolderList  = std::map<Zstring, std::pair<FolderDescriptor, FolderContainer>, LessFilePath>; //
    using FileList    = std::map<Zstring, FileDescriptor,  LessFilePath>; //key: file name
    using SymlinkList = std::map<Zstring, LinkDescriptor,  LessFilePath>; //
    //------------------------------------------------------------------

    FolderContainer() = default;
    FolderContainer           (const FolderContainer&) = delete; //catch accidental (and unnecessary) copying
    FolderContainer& operator=(const FolderContainer&) = delete; //

    FileList    files;
    SymlinkList symlinks; //non-followed symlinks
    FolderList  folders;

    void addSubFile(const Zstring& itemName, const FileDescriptor& descr)
    {
        auto rv = files.emplace(itemName, descr);
        if (!rv.second) //update entry if already existing (e.g. during folder traverser "retry") => does not handle different item name case (irrelvant!..)
            rv.first->second = descr;
    }

    void addSubLink(const Zstring& itemName, const LinkDescriptor& descr)
    {
        auto rv = symlinks.emplace(itemName, descr);
        if (!rv.second)
            rv.first->second = descr;
    }

    FolderContainer& addSubFolder(const Zstring& itemName, const FolderDescriptor& descr)
    {
        auto& p = folders[itemName]; //value default-construction is okay here
        p.first = descr;
        return p.second;

        //auto rv = folders.emplace(itemName, std::pair<FolderDescriptor, FolderContainer>(descr, FolderContainer()));
        //if (!rv.second)
        //  rv.first->second.first = descr;
        //return rv.first->second.second;
    }
};

class BaseFolderPair;
class FolderPair;
class FilePair;
class SymlinkPair;
class FileSystemObject;

/*------------------------------------------------------------------
    inheritance diagram:

                  ObjectMgr
                     /|\
                      |
               FileSystemObject         HierarchyObject
                     /|\                      /|\
       _______________|_______________   ______|______
      |               |               | |             |
 SymlinkPair       FilePair        FolderPair   BaseFolderPair

------------------------------------------------------------------*/

class HierarchyObject
{
    friend class FolderPair;
    friend class FileSystemObject;

public:
    using FileList    = FixedList<FilePair>;    //MergeSides::execute() requires a structure that doesn't invalidate pointers after push_back()
    using SymlinkList = FixedList<SymlinkPair>; //
    using FolderList  = FixedList<FolderPair>;

    FolderPair& addSubFolder(const Zstring& itemNameLeft,
                             const FolderDescriptor& left,          //file exists on both sides
                             CompareDirResult defaultCmpResult,
                             const Zstring& itemNameRight,
                             const FolderDescriptor& right);

    template <SelectedSide side>
    FolderPair& addSubFolder(const Zstring& itemName, //dir exists on one side only
                             const FolderDescriptor& descr);

    FilePair& addSubFile(const Zstring&        itemNameLeft,
                         const FileDescriptor& left,          //file exists on both sides
                         CompareFilesResult    defaultCmpResult,
                         const Zstring&        itemNameRight,
                         const FileDescriptor& right);

    template <SelectedSide side>
    FilePair& addSubFile(const Zstring&          itemName, //file exists on one side only
                         const FileDescriptor&   descr);

    SymlinkPair& addSubLink(const Zstring&        itemNameLeft,
                            const LinkDescriptor& left,  //link exists on both sides
                            CompareSymlinkResult  defaultCmpResult,
                            const Zstring&        itemNameRight,
                            const LinkDescriptor& right);

    template <SelectedSide side>
    SymlinkPair& addSubLink(const Zstring&        itemName, //link exists on one side only
                            const LinkDescriptor& descr);

    const FileList& refSubFiles() const { return subFiles_; }
    /**/  FileList& refSubFiles()       { return subFiles_; }

    const SymlinkList& refSubLinks() const { return subLinks_; }
    /**/  SymlinkList& refSubLinks()       { return subLinks_; }

    const FolderList& refSubFolders() const { return subFolders_; }
    /**/  FolderList& refSubFolders()       { return subFolders_; }

    BaseFolderPair& getBase() { return base_; }

    const Zstring& getPairRelativePathPf() const { return pairRelPathPf_; } //postfixed or empty!

protected:
    HierarchyObject(const Zstring& relPathPf,
                    BaseFolderPair& baseFolder) :
        pairRelPathPf_(relPathPf),
        base_(baseFolder) {}

    virtual ~HierarchyObject() {} //don't need polymorphic deletion, but we have a vtable anyway

    virtual void flip();

    void removeEmptyRec();

private:
    virtual void notifySyncCfgChanged() {}

    HierarchyObject           (const HierarchyObject&) = delete; //this class is referenced by it's child elements => make it non-copyable/movable!
    HierarchyObject& operator=(const HierarchyObject&) = delete;

    FileList    subFiles_;   //contained file maps
    SymlinkList subLinks_;   //contained symbolic link maps
    FolderList  subFolders_; //contained directory maps

    Zstring pairRelPathPf_; //postfixed or empty
    BaseFolderPair& base_;
};

//------------------------------------------------------------------

class BaseFolderPair : public HierarchyObject //synchronization base directory
{
public:
    BaseFolderPair(const AbstractPath& folderPathLeft,
                   bool folderAvailableLeft,
                   const AbstractPath& folderPathRight,
                   bool folderAvailableRight,
                   const HardFilter::FilterRef& filter,
                   CompareVariant cmpVar,
                   int fileTimeTolerance,
                   const std::vector<unsigned int>& ignoreTimeShiftMinutes) :
        HierarchyObject(Zstring(), *this),
        filter_(filter), cmpVar_(cmpVar), fileTimeTolerance_(fileTimeTolerance), ignoreTimeShiftMinutes_(ignoreTimeShiftMinutes),
        folderAvailableLeft_ (folderAvailableLeft),
        folderAvailableRight_(folderAvailableRight),
        folderPathLeft_(folderPathLeft),
        folderPathRight_(folderPathRight) {}

    template <SelectedSide side> const AbstractPath& getAbstractPath() const;

    static void removeEmpty(BaseFolderPair& baseFolder) { baseFolder.removeEmptyRec(); } //physically remove all invalid entries (where both sides are empty) recursively

    template <SelectedSide side> bool isAvailable() const; //base folder status at the time of comparison!
    template <SelectedSide side> void setAvailable(bool value); //update after creating the directory in FFS

    //get settings which were used while creating BaseFolderPair
    const HardFilter&   getFilter() const { return *filter_; }
    CompareVariant getCompVariant() const { return cmpVar_; }
    int  getFileTimeTolerance() const { return fileTimeTolerance_; }
    const std::vector<unsigned int>& getIgnoredTimeShift() const { return ignoreTimeShiftMinutes_; }

    void flip() override;

private:
    const HardFilter::FilterRef filter_; //filter used while scanning directory: represents sub-view of actual files!
    const CompareVariant cmpVar_;
    const int fileTimeTolerance_;
    const std::vector<unsigned int> ignoreTimeShiftMinutes_;

    bool folderAvailableLeft_;
    bool folderAvailableRight_;

    AbstractPath folderPathLeft_;
    AbstractPath folderPathRight_;
};


template <> inline
const AbstractPath& BaseFolderPair::getAbstractPath<LEFT_SIDE>() const { return folderPathLeft_; }

template <> inline
const AbstractPath& BaseFolderPair::getAbstractPath<RIGHT_SIDE>() const { return folderPathRight_; }


//get rid of shared_ptr indirection
template <class IterImpl, //underlying iterator type
          class V>        //target value type
class DerefIter : public std::iterator<std::bidirectional_iterator_tag, V>
{
public:
    DerefIter() {}
    DerefIter(IterImpl it) : it_(it) {}
    DerefIter(const DerefIter& other) : it_(other.it_) {}
    DerefIter& operator++() { ++it_; return *this; }
    DerefIter& operator--() { --it_; return *this; }
    inline friend DerefIter operator++(DerefIter& it, int) { return it++; }
    inline friend DerefIter operator--(DerefIter& it, int) { return it--; }
    inline friend ptrdiff_t operator-(const DerefIter& lhs, const DerefIter& rhs) { return lhs.it_ - rhs.it_; }
    inline friend bool operator==(const DerefIter& lhs, const DerefIter& rhs) { return lhs.it_ == rhs.it_; }
    inline friend bool operator!=(const DerefIter& lhs, const DerefIter& rhs) { return !(lhs == rhs); }
    V& operator* () const { return  **it_; }
    V* operator->() const { return &** it_; }
private:
    IterImpl it_;
};

using FolderComparison = std::vector<std::shared_ptr<BaseFolderPair>>; //make sure pointers to sub-elements remain valid
//don't change this back to std::vector<BaseFolderPair> too easily: comparison uses push_back to add entries which may result in a full copy!

DerefIter<typename FolderComparison::iterator,             BaseFolderPair> inline begin(      FolderComparison& vect) { return vect.begin(); }
DerefIter<typename FolderComparison::iterator,             BaseFolderPair> inline end  (      FolderComparison& vect) { return vect.end  (); }
DerefIter<typename FolderComparison::const_iterator, const BaseFolderPair> inline begin(const FolderComparison& vect) { return vect.begin(); }
DerefIter<typename FolderComparison::const_iterator, const BaseFolderPair> inline end  (const FolderComparison& vect) { return vect.end  (); }

//------------------------------------------------------------------
struct FSObjectVisitor
{
    virtual ~FSObjectVisitor() {}
    virtual void visit(const FilePair&    file   ) = 0;
    virtual void visit(const SymlinkPair& symlink) = 0;
    virtual void visit(const FolderPair&  folder ) = 0;
};


//inherit from this class to allow safe random access by id instead of unsafe raw pointer
//allow for similar semantics like std::weak_ptr without having to use std::shared_ptr
template <class T>
class ObjectMgr
{
public:
    using ObjectId      = ObjectMgr* ;
    using ObjectIdConst = const ObjectMgr*;

    ObjectIdConst  getId() const { return this; }
    /**/  ObjectId getId()       { return this; }

    static const T* retrieve(ObjectIdConst id) //returns nullptr if object is not valid anymore
    {
        const auto& aObj = activeObjects();
        return static_cast<const T*>(aObj.find(id) == aObj.end() ? nullptr : id);
    }
    static T* retrieve(ObjectId id) { return const_cast<T*>(retrieve(static_cast<ObjectIdConst>(id))); }

protected:
    ObjectMgr () { activeObjects().insert(this); }
    ~ObjectMgr() { activeObjects().erase (this); }

private:
    ObjectMgr           (const ObjectMgr& rhs) = delete;
    ObjectMgr& operator=(const ObjectMgr& rhs) = delete; //it's not well-defined what copying an objects means regarding object-identity in this context

    static std::unordered_set<const ObjectMgr*>& activeObjects()
    {
        static std::unordered_set<const ObjectMgr*> inst;
        return inst; //external linkage (even in header file!)
    }

};

//------------------------------------------------------------------

class FileSystemObject : public ObjectMgr<FileSystemObject>
{
public:
    virtual void accept(FSObjectVisitor& visitor) const = 0;

    Zstring getPairItemName    () const; //like getItemName() but also returns value if either side is empty
    Zstring getPairRelativePath() const; //like getRelativePath() but also returns value if either side is empty
    template <SelectedSide side>           bool isEmpty()         const;
    template <SelectedSide side> const Zstring& getItemName()     const; //case sensitive!
    template <SelectedSide side>       Zstring  getRelativePath() const; //get path relative to base sync dir without FILE_NAME_SEPARATOR prefix

public:
    template <SelectedSide side> AbstractPath getAbstractPath() const; //precondition: !isEmpty<side>()

    //comparison result
    CompareFilesResult getCategory() const { return cmpResult_; }
    std::wstring getCatExtraDescription() const; //only filled if getCategory() == FILE_CONFLICT or FILE_DIFFERENT_METADATA

    //sync settings
    SyncDirection getSyncDir() const { return syncDir_; }
    void setSyncDir(SyncDirection newDir);
    void setSyncDirConflict(const std::wstring& description); //set syncDir = SyncDirection::NONE + fill conflict description

    bool isActive() const { return selectedForSync_; }
    void setActive(bool active);

    //sync operation
    virtual SyncOperation testSyncOperation(SyncDirection testSyncDir) const; //semantics: "what if"! assumes "active, no conflict, no recursion (directory)!
    virtual SyncOperation getSyncOperation() const;
    std::wstring getSyncOpConflict() const; //return conflict when determining sync direction or (still unresolved) conflict during categorization

    template <SelectedSide side> void removeObject();    //removes file or directory (recursively!) without physically removing the element: used by manual deletion

    bool isEmpty() const; //true, if both sides are empty

    const HierarchyObject& parent() const { return parent_; }
    /**/  HierarchyObject& parent()       { return parent_; }
    const BaseFolderPair& base() const  { return parent_.getBase(); }
    /**/  BaseFolderPair& base()        { return parent_.getBase(); }

    //for use during init in "CompareProcess" only:
    template <CompareFilesResult res> void setCategory();
    void setCategoryConflict    (const std::wstring& description);
    void setCategoryDiffMetadata(const std::wstring& description);

protected:
    FileSystemObject(const Zstring& itemNameLeft,
                     const Zstring& itemNameRight,
                     HierarchyObject& parentObj,
                     CompareFilesResult defaultCmpResult) :
        cmpResult_(defaultCmpResult),
        itemNameLeft_(itemNameLeft),
        itemNameRight_(itemNameRight),
        //itemNameRight_(itemNameRight == itemNameLeft ? itemNameLeft : itemNameRight), -> strangely doesn't seem to shrink peak memory consumption at all!
        parent_(parentObj)
    {
        parent_.notifySyncCfgChanged();
    }

    virtual ~FileSystemObject() {} //don't need polymorphic deletion, but we have a vtable anyway
    //mustn't call parent here, it is already partially destroyed and nothing more than a pure HierarchyObject!

    virtual void flip();
    virtual void notifySyncCfgChanged() { parent().notifySyncCfgChanged(); /*propagate!*/ }

    void setSynced(const Zstring& itemName);

private:
    FileSystemObject           (const FileSystemObject&) = delete;
    FileSystemObject& operator=(const FileSystemObject&) = delete;

    virtual void removeObjectL() = 0;
    virtual void removeObjectR() = 0;

    //categorization
    std::unique_ptr<std::wstring> cmpResultDescr_; //only filled if getCategory() == FILE_CONFLICT or FILE_DIFFERENT_METADATA
    CompareFilesResult cmpResult_; //although this uses 4 bytes there is currently *no* space wasted in class layout!

    bool selectedForSync_ = true;

    //Note: we model *four* states with following two variables => "syncDirectionConflict is empty or syncDir == NONE" is a class invariant!!!
    SyncDirection syncDir_ = SyncDirection::NONE; //1 byte: optimize memory layout!
    std::unique_ptr<std::wstring> syncDirectionConflict_; //non-empty if we have a conflict setting sync-direction
    //get rid of std::wstring small string optimization (consumes 32/48 byte on VS2010 x86/x64!)

    Zstring itemNameLeft_;  //slightly redundant under linux, but on windows the "same" filepaths can differ in case
    Zstring itemNameRight_; //use as indicator: an empty name means: not existing!

    HierarchyObject& parent_;
};

//------------------------------------------------------------------

class FolderPair : public FileSystemObject, public HierarchyObject
{
    friend class HierarchyObject;

public:
    void accept(FSObjectVisitor& visitor) const override;

    CompareDirResult getDirCategory() const; //returns actually used subset of CompareFilesResult

    FolderPair(const Zstring& itemNameLeft,  //use empty itemName if "not existing"
               const FolderDescriptor& left,
               CompareDirResult defaultCmpResult,
               const Zstring& itemNameRight,
               const FolderDescriptor& right,
               HierarchyObject& parentObj) :
        FileSystemObject(itemNameLeft, itemNameRight, parentObj, static_cast<CompareFilesResult>(defaultCmpResult)),
        HierarchyObject(getPairRelativePath() + FILE_NAME_SEPARATOR, parentObj.getBase()),
        dataLeft_(left),
        dataRight_(right) {}

    template <SelectedSide side> bool isFollowedSymlink() const;

    SyncOperation getSyncOperation() const override;

    template <SelectedSide sideTrg>
    void setSyncedTo(const Zstring& itemName, bool isSymlinkTrg, bool isSymlinkSrc); //call after sync, sets DIR_EQUAL

private:
    void flip         () override;
    void removeObjectL() override;
    void removeObjectR() override;
    void notifySyncCfgChanged() override { haveBufferedSyncOp_ = false; FileSystemObject::notifySyncCfgChanged(); HierarchyObject::notifySyncCfgChanged(); }

    mutable SyncOperation syncOpBuffered_ = SO_DO_NOTHING; //determining sync-op for directory may be expensive as it depends on child-objects -> buffer it
    mutable bool haveBufferedSyncOp_      = false;         //

    FolderDescriptor dataLeft_;
    FolderDescriptor dataRight_;
};

//------------------------------------------------------------------

class FilePair : public FileSystemObject
{
    friend class HierarchyObject; //construction

public:
    void accept(FSObjectVisitor& visitor) const override;

    FilePair(const Zstring&        itemNameLeft, //use empty string if "not existing"
             const FileDescriptor& left,
             CompareFilesResult    defaultCmpResult,
             const Zstring&        itemNameRight, //
             const FileDescriptor& right,
             HierarchyObject& parentObj) :
        FileSystemObject(itemNameLeft, itemNameRight, parentObj, defaultCmpResult),
        dataLeft_(left),
        dataRight_(right) {}

    template <SelectedSide side> int64_t getLastWriteTime() const;
    template <SelectedSide side> uint64_t     getFileSize() const;
    template <SelectedSide side> AFS::FileId  getFileId  () const;
    template <SelectedSide side> bool   isFollowedSymlink() const;

    void setMoveRef(ObjectId refId) { moveFileRef_ = refId; } //reference to corresponding renamed file
    ObjectId getMoveRef() const { return moveFileRef_; } //may be nullptr

    CompareFilesResult getFileCategory() const;

    SyncOperation testSyncOperation(SyncDirection testSyncDir) const override; //semantics: "what if"! assumes "active, no conflict, no recursion (directory)!
    SyncOperation getSyncOperation() const override;

    template <SelectedSide sideTrg>
    void setSyncedTo(const Zstring& itemName, //call after sync, sets FILE_EQUAL
                     uint64_t fileSize,
                     int64_t lastWriteTimeTrg,
                     int64_t lastWriteTimeSrc,
                     const AFS::FileId& fileIdTrg,
                     const AFS::FileId& fileIdSrc,
                     bool isSymlinkTrg,
                     bool isSymlinkSrc);

private:
    SyncOperation applyMoveOptimization(SyncOperation op) const;

    void flip         () override;
    void removeObjectL() override { dataLeft_  = FileDescriptor(); }
    void removeObjectR() override { dataRight_ = FileDescriptor(); }

    FileDescriptor dataLeft_;
    FileDescriptor dataRight_;

    ObjectId moveFileRef_ = nullptr; //optional, filled by redetermineSyncDirection()
};

//------------------------------------------------------------------

class SymlinkPair : public FileSystemObject //this class models a TRUE symbolic link, i.e. one that is NEVER dereferenced: deref-links should be directly placed in class File/FolderPair
{
    friend class HierarchyObject; //construction

public:
    void accept(FSObjectVisitor& visitor) const override;

    template <SelectedSide side> int64_t getLastWriteTime() const; //write time of the link, NOT target!

    CompareSymlinkResult getLinkCategory()   const; //returns actually used subset of CompareFilesResult

    SymlinkPair(const Zstring&         itemNameLeft, //use empty string if "not existing"
                const LinkDescriptor&  left,
                CompareSymlinkResult   defaultCmpResult,
                const Zstring&         itemNameRight, //use empty string if "not existing"
                const LinkDescriptor&  right,
                HierarchyObject& parentObj) :
        FileSystemObject(itemNameLeft, itemNameRight, parentObj, static_cast<CompareFilesResult>(defaultCmpResult)),
        dataLeft_(left),
        dataRight_(right) {}

    template <SelectedSide sideTrg>
    void setSyncedTo(const Zstring& itemName, //call after sync, sets SYMLINK_EQUAL
                     int64_t lastWriteTimeTrg,
                     int64_t lastWriteTimeSrc);

private:
    void flip()          override;
    void removeObjectL() override { dataLeft_  = LinkDescriptor(); }
    void removeObjectR() override { dataRight_ = LinkDescriptor(); }

    LinkDescriptor dataLeft_;
    LinkDescriptor dataRight_;
};

//------------------------------------------------------------------

//generic type descriptions (usecase CSV legend, sync config)
std::wstring getCategoryDescription(CompareFilesResult cmpRes);
std::wstring getSyncOpDescription  (SyncOperation op);

//item-specific type descriptions
std::wstring getCategoryDescription(const FileSystemObject& fsObj);
std::wstring getSyncOpDescription  (const FileSystemObject& fsObj);

//------------------------------------------------------------------

template <class Function1, class Function2, class Function3>
struct FSObjectLambdaVisitor : public FSObjectVisitor
{
    FSObjectLambdaVisitor(Function1 onFolder,
                          Function2 onFile,
                          Function3 onSymlink) : onFolder_(onFolder), onFile_(onFile), onSymlink_(onSymlink) {}
private:
    void visit(const FolderPair&  folder) override { onFolder_ (folder); }
    void visit(const FilePair&    file  ) override { onFile_   (file); }
    void visit(const SymlinkPair& link  ) override { onSymlink_(link); }

    Function1 onFolder_;
    Function2 onFile_;
    Function3 onSymlink_;
};

template <class Function1, class Function2, class Function3> inline
void visitFSObject(const FileSystemObject& fsObj, Function1 onFolder, Function2 onFile, Function3 onSymlink)
{
    FSObjectLambdaVisitor<Function1, Function2, Function3> visitor(onFolder, onFile, onSymlink);
    fsObj.accept(visitor);
}




















//--------------------- implementation ------------------------------------------

//inline virtual... admittedly its use may be limited
inline void FilePair   ::accept(FSObjectVisitor& visitor) const { visitor.visit(*this); }
inline void FolderPair ::accept(FSObjectVisitor& visitor) const { visitor.visit(*this); }
inline void SymlinkPair::accept(FSObjectVisitor& visitor) const { visitor.visit(*this); }


inline
CompareFilesResult FilePair::getFileCategory() const
{
    return getCategory();
}


inline
CompareDirResult FolderPair::getDirCategory() const
{
    return static_cast<CompareDirResult>(getCategory());
}


inline
std::wstring FileSystemObject::getCatExtraDescription() const
{
    assert(getCategory() == FILE_CONFLICT || getCategory() == FILE_DIFFERENT_METADATA);
    if (cmpResultDescr_) //avoid ternary-WTF! (implicit copy-constructor call!!!!!!)
        return *cmpResultDescr_;
    return std::wstring();
}


inline
void FileSystemObject::setSyncDir(SyncDirection newDir)
{
    syncDir_ = newDir;
    syncDirectionConflict_.reset();

    notifySyncCfgChanged();
}


inline
void FileSystemObject::setSyncDirConflict(const std::wstring& description)
{
    syncDir_ = SyncDirection::NONE;
    syncDirectionConflict_ = std::make_unique<std::wstring>(description);

    notifySyncCfgChanged();
}


inline
std::wstring FileSystemObject::getSyncOpConflict() const
{
    assert(getSyncOperation() == SO_UNRESOLVED_CONFLICT);
    if (syncDirectionConflict_) //avoid ternary-WTF! (implicit copy-constructor call!!!!!!)
        return *syncDirectionConflict_;
    return std::wstring();
}


inline
void FileSystemObject::setActive(bool active)
{
    selectedForSync_ = active;
    notifySyncCfgChanged();
}


template <SelectedSide side> inline
bool FileSystemObject::isEmpty() const
{
    return SelectParam<side>::ref(itemNameLeft_, itemNameRight_).empty();
}


inline
bool FileSystemObject::isEmpty() const
{
    return isEmpty<LEFT_SIDE>() && isEmpty<RIGHT_SIDE>();
}


template <SelectedSide side> inline
const Zstring& FileSystemObject::getItemName() const
{
    return SelectParam<side>::ref(itemNameLeft_, itemNameRight_); //empty if not existing
}


template <SelectedSide side> inline
Zstring FileSystemObject::getRelativePath() const
{
    if (isEmpty<side>()) //avoid ternary-WTF! (implicit copy-constructor call!!!!!!)
        return Zstring();
    return parent_.getPairRelativePathPf() + getItemName<side>();
}


inline
Zstring FileSystemObject::getPairRelativePath() const
{
    return parent_.getPairRelativePathPf() + getPairItemName();
}


inline
Zstring FileSystemObject::getPairItemName() const
{
    assert(!isEmpty<LEFT_SIDE>() || !isEmpty<RIGHT_SIDE>());
    return isEmpty<LEFT_SIDE>() ? getItemName<RIGHT_SIDE>() : getItemName<LEFT_SIDE>();
}


template <SelectedSide side> inline
AbstractPath FileSystemObject::getAbstractPath() const
{
    assert(!isEmpty<side>());
    const Zstring& itemName = isEmpty<side>() ? getItemName<OtherSide<side>::result>() : getItemName<side>();
    return AFS::appendRelPath(base().getAbstractPath<side>(), parent_.getPairRelativePathPf() + itemName);
}


template <> inline
void FileSystemObject::removeObject<LEFT_SIDE>()
{
    cmpResult_ = isEmpty<RIGHT_SIDE>() ? FILE_EQUAL : FILE_RIGHT_SIDE_ONLY;
    itemNameLeft_.clear();
    removeObjectL();

    setSyncDir(SyncDirection::NONE); //calls notifySyncCfgChanged()
}


template <> inline
void FileSystemObject::removeObject<RIGHT_SIDE>()
{
    cmpResult_ = isEmpty<LEFT_SIDE>() ? FILE_EQUAL : FILE_LEFT_SIDE_ONLY;
    itemNameRight_.clear();
    removeObjectR();

    setSyncDir(SyncDirection::NONE); //calls notifySyncCfgChanged()
}


inline
void FileSystemObject::setSynced(const Zstring& itemName)
{
    assert(!isEmpty());
    itemNameRight_ = itemNameLeft_ = itemName;
    cmpResult_ = FILE_EQUAL;
    setSyncDir(SyncDirection::NONE);
}


template <CompareFilesResult res> inline
void FileSystemObject::setCategory()
{
    cmpResult_ = res;
}
template <> void FileSystemObject::setCategory<FILE_CONFLICT>          () = delete; //
template <> void FileSystemObject::setCategory<FILE_DIFFERENT_METADATA>() = delete; //deny use
template <> void FileSystemObject::setCategory<FILE_LEFT_SIDE_ONLY>    () = delete; //
template <> void FileSystemObject::setCategory<FILE_RIGHT_SIDE_ONLY>   () = delete; //

inline
void FileSystemObject::setCategoryConflict(const std::wstring& description)
{
    cmpResult_ = FILE_CONFLICT;
    cmpResultDescr_ = std::make_unique<std::wstring>(description);
}

inline
void FileSystemObject::setCategoryDiffMetadata(const std::wstring& description)
{
    cmpResult_ = FILE_DIFFERENT_METADATA;
    cmpResultDescr_ = std::make_unique<std::wstring>(description);
}

inline
void FileSystemObject::flip()
{
    std::swap(itemNameLeft_, itemNameRight_);

    switch (cmpResult_)
    {
        case FILE_LEFT_SIDE_ONLY:
            cmpResult_ = FILE_RIGHT_SIDE_ONLY;
            break;
        case FILE_RIGHT_SIDE_ONLY:
            cmpResult_ = FILE_LEFT_SIDE_ONLY;
            break;
        case FILE_LEFT_NEWER:
            cmpResult_ = FILE_RIGHT_NEWER;
            break;
        case FILE_RIGHT_NEWER:
            cmpResult_ = FILE_LEFT_NEWER;
            break;
        case FILE_DIFFERENT_CONTENT:
        case FILE_EQUAL:
        case FILE_DIFFERENT_METADATA:
        case FILE_CONFLICT:
            break;
    }

    notifySyncCfgChanged();
}


inline
void HierarchyObject::flip()
{
    for (FilePair& file : refSubFiles())
        file.flip();
    for (SymlinkPair& link : refSubLinks())
        link.flip();
    for (FolderPair& folder : refSubFolders())
        folder.flip();
}


inline
FolderPair& HierarchyObject::addSubFolder(const Zstring& itemNameLeft,
                                          const FolderDescriptor& left,
                                          CompareDirResult defaultCmpResult,
                                          const Zstring& itemNameRight,
                                          const FolderDescriptor& right)
{
    subFolders_.emplace_back(itemNameLeft, left, defaultCmpResult, itemNameRight, right, *this);
    return subFolders_.back();
}


template <> inline
FolderPair& HierarchyObject::addSubFolder<LEFT_SIDE>(const Zstring& itemName, const FolderDescriptor& descr)
{
    subFolders_.emplace_back(itemName, descr, DIR_LEFT_SIDE_ONLY, Zstring(), FolderDescriptor(), *this);
    return subFolders_.back();
}


template <> inline
FolderPair& HierarchyObject::addSubFolder<RIGHT_SIDE>(const Zstring& itemName, const FolderDescriptor& descr)
{
    subFolders_.emplace_back(Zstring(), FolderDescriptor(), DIR_RIGHT_SIDE_ONLY, itemName, descr, *this);
    return subFolders_.back();
}


inline
FilePair& HierarchyObject::addSubFile(const Zstring&        itemNameLeft,
                                      const FileDescriptor& left,          //file exists on both sides
                                      CompareFilesResult    defaultCmpResult,
                                      const Zstring&        itemNameRight,
                                      const FileDescriptor& right)
{
    subFiles_.emplace_back(itemNameLeft, left, defaultCmpResult, itemNameRight, right, *this);
    return subFiles_.back();
}


template <> inline
FilePair& HierarchyObject::addSubFile<LEFT_SIDE>(const Zstring& itemName, const FileDescriptor& descr)
{
    subFiles_.emplace_back(itemName, descr, FILE_LEFT_SIDE_ONLY, Zstring(), FileDescriptor(), *this);
    return subFiles_.back();
}


template <> inline
FilePair& HierarchyObject::addSubFile<RIGHT_SIDE>(const Zstring& itemName, const FileDescriptor& descr)
{
    subFiles_.emplace_back(Zstring(), FileDescriptor(), FILE_RIGHT_SIDE_ONLY, itemName, descr, *this);
    return subFiles_.back();
}


inline
SymlinkPair& HierarchyObject::addSubLink(const Zstring&        itemNameLeft,
                                         const LinkDescriptor& left,  //link exists on both sides
                                         CompareSymlinkResult  defaultCmpResult,
                                         const Zstring&        itemNameRight,
                                         const LinkDescriptor& right)
{
    subLinks_.emplace_back(itemNameLeft, left, defaultCmpResult, itemNameRight, right, *this);
    return subLinks_.back();
}


template <> inline
SymlinkPair& HierarchyObject::addSubLink<LEFT_SIDE>(const Zstring& itemName, const LinkDescriptor& descr)
{
    subLinks_.emplace_back(itemName, descr, SYMLINK_LEFT_SIDE_ONLY, Zstring(), LinkDescriptor(), *this);
    return subLinks_.back();
}


template <> inline
SymlinkPair& HierarchyObject::addSubLink<RIGHT_SIDE>(const Zstring& itemName, const LinkDescriptor& descr)
{
    subLinks_.emplace_back(Zstring(), LinkDescriptor(), SYMLINK_RIGHT_SIDE_ONLY, itemName, descr, *this);
    return subLinks_.back();
}


inline
void BaseFolderPair::flip()
{
    HierarchyObject::flip();
    std::swap(folderAvailableLeft_, folderAvailableRight_);
    std::swap(folderPathLeft_,      folderPathRight_);
}


inline
void FolderPair::flip()
{
    HierarchyObject ::flip(); //call base class versions
    FileSystemObject::flip(); //
    std::swap(dataLeft_, dataRight_);
}


inline
void FolderPair::removeObjectL()
{
    for (FilePair& file : refSubFiles())
        file.removeObject<LEFT_SIDE>();
    for (SymlinkPair& link : refSubLinks())
        link.removeObject<LEFT_SIDE>();
    for (FolderPair& folder : refSubFolders())
        folder.removeObject<LEFT_SIDE>();

    dataLeft_ = FolderDescriptor();
}


inline
void FolderPair::removeObjectR()
{
    for (FilePair& file : refSubFiles())
        file.removeObject<RIGHT_SIDE>();
    for (SymlinkPair& link : refSubLinks())
        link.removeObject<RIGHT_SIDE>();
    for (FolderPair& folder : refSubFolders())
        folder.removeObject<RIGHT_SIDE>();

    dataRight_ = FolderDescriptor();
}


template <SelectedSide side> inline
bool BaseFolderPair::isAvailable() const
{
    return SelectParam<side>::ref(folderAvailableLeft_, folderAvailableRight_);
}


template <SelectedSide side> inline
void BaseFolderPair::setAvailable(bool value)
{
    SelectParam<side>::ref(folderAvailableLeft_, folderAvailableRight_) = value;
}


inline
void FilePair::flip()
{
    FileSystemObject::flip(); //call base class version
    std::swap(dataLeft_, dataRight_);
}


template <SelectedSide side> inline
int64_t FilePair::getLastWriteTime() const
{
    return SelectParam<side>::ref(dataLeft_, dataRight_).lastWriteTimeRaw;
}


template <SelectedSide side> inline
uint64_t FilePair::getFileSize() const
{
    return SelectParam<side>::ref(dataLeft_, dataRight_).fileSize;
}


template <SelectedSide side> inline
AFS::FileId FilePair::getFileId() const
{
    return SelectParam<side>::ref(dataLeft_, dataRight_).fileId;
}


template <SelectedSide side> inline
bool FilePair::isFollowedSymlink() const
{
    return SelectParam<side>::ref(dataLeft_, dataRight_).isFollowedSymlink;
}


template <SelectedSide side> inline
bool FolderPair::isFollowedSymlink() const
{
    return SelectParam<side>::ref(dataLeft_, dataRight_).isFollowedSymlink;
}


template <SelectedSide sideTrg> inline
void FilePair::setSyncedTo(const Zstring& itemName,
                           uint64_t fileSize,
                           int64_t lastWriteTimeTrg,
                           int64_t lastWriteTimeSrc,
                           const AFS::FileId& fileIdTrg,
                           const AFS::FileId& fileIdSrc,
                           bool isSymlinkTrg,
                           bool isSymlinkSrc)
{
    //FILE_EQUAL is only allowed for same short name and file size: enforced by this method!
    static const SelectedSide sideSrc = OtherSide<sideTrg>::result;

    SelectParam<sideTrg>::ref(dataLeft_, dataRight_) = FileDescriptor(lastWriteTimeTrg, fileSize, fileIdTrg, isSymlinkTrg);
    SelectParam<sideSrc>::ref(dataLeft_, dataRight_) = FileDescriptor(lastWriteTimeSrc, fileSize, fileIdSrc, isSymlinkSrc);

    moveFileRef_ = nullptr;
    FileSystemObject::setSynced(itemName); //set FileSystemObject specific part
}


template <SelectedSide sideTrg> inline
void SymlinkPair::setSyncedTo(const Zstring& itemName,
                              int64_t lastWriteTimeTrg,
                              int64_t lastWriteTimeSrc)
{
    static const SelectedSide sideSrc = OtherSide<sideTrg>::result;

    SelectParam<sideTrg>::ref(dataLeft_, dataRight_) = LinkDescriptor(lastWriteTimeTrg);
    SelectParam<sideSrc>::ref(dataLeft_, dataRight_) = LinkDescriptor(lastWriteTimeSrc);

    FileSystemObject::setSynced(itemName); //set FileSystemObject specific part
}


template <SelectedSide sideTrg> inline
void FolderPair::setSyncedTo(const Zstring& itemName,
                             bool isSymlinkTrg,
                             bool isSymlinkSrc)
{
    static const SelectedSide sideSrc = OtherSide<sideTrg>::result;

    SelectParam<sideTrg>::ref(dataLeft_, dataRight_) = FolderDescriptor(isSymlinkTrg);
    SelectParam<sideSrc>::ref(dataLeft_, dataRight_) = FolderDescriptor(isSymlinkSrc);

    FileSystemObject::setSynced(itemName); //set FileSystemObject specific part
}


template <SelectedSide side> inline
int64_t SymlinkPair::getLastWriteTime() const
{
    return SelectParam<side>::ref(dataLeft_, dataRight_).lastWriteTimeRaw;
}


inline
CompareSymlinkResult SymlinkPair::getLinkCategory() const
{
    return static_cast<CompareSymlinkResult>(getCategory());
}


inline
void SymlinkPair::flip()
{
    FileSystemObject::flip(); //call base class versions
    std::swap(dataLeft_, dataRight_);
}
}

#endif //FILE_HIERARCHY_H_257235289645296
