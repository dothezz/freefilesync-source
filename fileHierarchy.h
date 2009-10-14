#ifndef FILEHIERARCHY_H_INCLUDED
#define FILEHIERARCHY_H_INCLUDED

#include "shared/zstring.h"
#include "shared/systemConstants.h"
#include <wx/longlong.h>
#include <map>
#include <set>
#include <vector>
#include "structures.h"
#include <boost/shared_ptr.hpp>

class DirectoryBuffer;


namespace FreeFileSync
{
struct FileDescriptor
{
    FileDescriptor(const wxLongLong&  lastWriteTimeRawIn, const wxULongLong& fileSizeIn) :
        lastWriteTimeRaw(lastWriteTimeRawIn),
        fileSize(fileSizeIn) {}
    wxLongLong  lastWriteTimeRaw; //number of seconds since Jan. 1st 1970 UTC, same semantics like time_t (== signed long)
    wxULongLong fileSize;
};


enum SelectedSide
{
    LEFT_SIDE,
    RIGHT_SIDE
};


class FileContainer;
class FileMapping;
class DirMapping;
class CompareProcess;
class FileSystemObject;
class BaseDirMapping;
//------------------------------------------------------------------
/*
  DirContainer    FileContainer

ERD:
DirContainer 1 -----> 0..n DirContainer
DirContainer 1 -----> 0..n FileContainer
*/

//------------------------------------------------------------------
class DirContainer
{
public:
    void addSubFile(const Zstring& shortName, const FileDescriptor& fileData);
    DirContainer& addSubDir(const Zstring& shortName);

    //------------------------------------------------------------------
    struct CmpFilename
    {
        bool operator()(const Zstring& a, const Zstring& b) const;
    };

    typedef std::map<Zstring, DirContainer,  CmpFilename> SubDirList;  //key: shortName
    typedef std::map<Zstring, FileContainer, CmpFilename> SubFileList; //
    //------------------------------------------------------------------

    const SubDirList&  getSubDirs() const;
    const SubFileList& getSubFiles() const;

    DirContainer() {} //default constructor use for base directory only!

private:
    SubDirList  subDirs;  //contained directories
    SubFileList subFiles; //contained files
};

//------------------------------------------------------------------
class FileContainer
{
public:
    const FileDescriptor& getData() const;

private:
    friend class DirContainer;

    FileContainer(const FileDescriptor& fileData) :
        data(fileData) {}

    const FileDescriptor data;
};


//------------------------------------------------------------------
struct DirInformation
{
    //filter settings (used when retrieving directory data)
    bool filterActive;
    Zstring includeFilter;
    Zstring excludeFilter;
    //hierarchical directory information
    DirContainer baseDirContainer;
};


//save/load full directory information
const Zstring& getSyncDBFilename(); //get short filename of database file
void saveToDisk(const BaseDirMapping& baseMapping, SelectedSide side, const Zstring& filename); //throw (FileError) -> return value always bound!
boost::shared_ptr<const DirInformation> loadFromDisk(const Zstring& filename);                  //throw (FileError)

//------------------------------------------------------------------
/*    class hierarchy:

      FileSystemObject  HierarchyObject
            /|\              /|\
       ______|______    ______|______
      |             |  |             |
FileMapping      DirMapping    BaseDirMapping
*/

//------------------------------------------------------------------
class HierarchyObject
{
public:
    typedef unsigned int ObjectID;
    FileSystemObject* retrieveById(ObjectID id);             //returns NULL if object is not found; logarithmic complexity
    const FileSystemObject* retrieveById(ObjectID id) const; //

    DirMapping& addSubDir(bool existsLeft,
                          const Zstring& dirNameShort,
                          bool existsRight);

    FileMapping& addSubFile(const FileDescriptor&   left,          //file exists on both sides
                            const Zstring&          fileNameShort,
                            CompareFilesResult      defaultCmpResult,
                            const FileDescriptor&   right);
    void addSubFile(const FileDescriptor&   left,          //file exists on left side only
                    const Zstring&          fileNameShort);
    void addSubFile(const Zstring&          fileNameShort, //file exists on right side only
                    const FileDescriptor&   right);

    const Zstring& getRelativeNamePf() const; //get name relative to base sync dir with FILE_NAME_SEPARATOR postfix: "blah\"
    template <SelectedSide side> const Zstring& getBaseDir() const //postfixed!
    {
        return side == LEFT_SIDE ? baseDirLeft : baseDirRight;
    }

    typedef std::vector<FileMapping> SubFileMapping;
    typedef std::vector<DirMapping>  SubDirMapping;

    SubFileMapping subFiles; //contained file maps
    SubDirMapping  subDirs;  //contained directory maps

protected:
    //constructor used by DirMapping
    HierarchyObject(const HierarchyObject& parent, const Zstring& shortName) :
        relNamePf(parent.getRelativeNamePf() + shortName + globalFunctions::FILE_NAME_SEPARATOR),
        baseDirLeft(parent.getBaseDir<LEFT_SIDE>()),
        baseDirRight(parent.getBaseDir<RIGHT_SIDE>()) {}

    //constructor used by BaseDirMapping
    HierarchyObject(const Zstring& dirPostfixedLeft,
                    const Zstring& dirPostfixedRight) :
        baseDirLeft(dirPostfixedLeft),
        baseDirRight(dirPostfixedRight) {}
    ~HierarchyObject() {} //don't need polymorphic deletion

    virtual void swap();

private:
    Zstring relNamePf;
    Zstring baseDirLeft;  //directory name ending with FILE_NAME_SEPARATOR
    Zstring baseDirRight; //directory name ending with FILE_NAME_SEPARATOR
};


//------------------------------------------------------------------
struct RelNamesBuffered
{
    RelNamesBuffered(const Zstring& baseDirPfLIn, //base sync dir postfixed
                     const Zstring& baseDirPfRIn,
                     const Zstring& parentRelNamePfIn) : //relative parent name postfixed
        baseDirPfL(baseDirPfLIn),
        baseDirPfR(baseDirPfRIn),
        parentRelNamePf(parentRelNamePfIn) {}

    Zstring baseDirPfL;
    Zstring baseDirPfR;
    Zstring parentRelNamePf;
};


//------------------------------------------------------------------
class FileSystemObject
{
public:
    const Zstring getParentRelativeName() const; //get name relative to base sync dir without FILE_NAME_SEPARATOR postfix
    const Zstring getObjRelativeName() const;    //same as getRelativeName() but also returns value if either side is empty
    const Zstring& getObjShortName() const;       //same as getShortName() but also returns value if either side is empty
    template <SelectedSide side>           bool     isEmpty()         const;
    template <SelectedSide side> const Zstring&     getShortName()    const;
    template <SelectedSide side> const Zstring      getRelativeName() const; //get name relative to base sync dir without FILE_NAME_SEPARATOR prefix
    template <SelectedSide side> const Zstring      getFullName()     const; //getFullName() == getBaseDirPf() + getRelativeName()
    template <SelectedSide side> const Zstring&     getBaseDirPf()    const; //base sync directory postfixed with FILE_NAME_SEPARATOR

    HierarchyObject::ObjectID getId() const; //get unique id; ^= logical key

    //comparison result
    virtual CompareFilesResult getCategory() const = 0;
    virtual const wxString& getCatConflict() const = 0; //only filled if cmpResult == FILE_CONFLICT
    //sync operation
    SyncOperation getSyncOperation() const;
    const wxString& getSyncOpConflict() const; //only filled if syncDir == SYNC_DIR_INT_CONFLICT
    SyncOperation testSyncOperation(bool selected, SyncDirection syncDir) const; //get syncOp with provided settings

    //sync settings
    void setSyncDir(SyncDirection newDir);
    void setSyncDirConflict(const wxString& description);   //set syncDir = SYNC_DIR_INT_CONFLICT

    bool isActive() const;
    void setActive(bool active);

    void synchronizeSides();          //copy one side to the other (NOT recursive!!!)
    template <SelectedSide side> void removeObject();    //removes file or directory (recursively!): used by manual deletion
    bool isEmpty() const; //true, if both sides are empty
    static void removeEmpty(BaseDirMapping& baseDir);  //remove all invalid entries (where both sides are empty) recursively
    static void removeEmptyNonRec(HierarchyObject& hierObj); //remove all invalid entries (where both sides are empty) non-recursively

protected:
    FileSystemObject(bool existsLeft, bool existsRight, const Zstring& shortName, const HierarchyObject& parent) :
        selectedForSynchronization(true),
        syncDir(SYNC_DIR_INT_NONE),
        nameBuffer(parent.getBaseDir<LEFT_SIDE>(), parent.getBaseDir<RIGHT_SIDE>(), parent.getRelativeNamePf()),
        existsLeft_(existsLeft),
        existsRight_(existsRight),
        shortName_(shortName),
        uniqueId(getUniqueId()) {}

    ~FileSystemObject() {} //don't need polymorphic deletion

    virtual void swap();

private:
    virtual           void removeObjectL()  = 0;
    virtual           void removeObjectR()  = 0;
    virtual           void copyToL() = 0;
    virtual           void copyToR() = 0;
    static HierarchyObject::ObjectID getUniqueId();

    enum SyncDirectionIntern //same as SyncDirection, but one additional conflict type
    {
        SYNC_DIR_INT_LEFT  = SYNC_DIR_LEFT,
        SYNC_DIR_INT_RIGHT = SYNC_DIR_RIGHT,
        SYNC_DIR_INT_NONE  = SYNC_DIR_NONE,
        SYNC_DIR_INT_CONFLICT //set if automatic synchronization cannot determine a direction
    };
    static SyncOperation getSyncOperation(const CompareFilesResult cmpResult,
                                          const bool selectedForSynchronization,
                                          const SyncDirectionIntern syncDir); //evaluate comparison result and sync direction

    bool selectedForSynchronization;
    SyncDirectionIntern syncDir;
    wxString syncOpConflictDescr; //only filled if syncDir == SYNC_DIR_INT_CONFLICT


    //buffer some redundant data:
    RelNamesBuffered nameBuffer; //base sync dirs + relative parent name: this does NOT belong into FileDescriptor/DirDescriptor

    bool existsLeft_;
    bool existsRight_;
    Zstring shortName_;
    HierarchyObject::ObjectID uniqueId;
};

//------------------------------------------------------------------
class DirMapping : public FileSystemObject, public HierarchyObject
{
public:
    virtual CompareFilesResult getCategory() const;
    CompareDirResult getDirCategory() const; //returns actually used subsed of CompareFilesResult
    virtual const wxString& getCatConflict() const;

private:
    friend class CompareProcess; //only CompareProcess shall be allowed to change cmpResult
    friend class HierarchyObject;
    virtual void swap();
    virtual void removeObjectL();
    virtual void removeObjectR();
    virtual void copyToL();
    virtual void copyToR();
    //------------------------------------------------------------------

    DirMapping(bool existsLeft,
               const Zstring& dirNameShort,
               bool existsRight,
               const HierarchyObject& parent) :
        FileSystemObject(existsLeft, existsRight, dirNameShort, parent),
        HierarchyObject(parent, dirNameShort),
        cmpResult(!existsRight ? DIR_LEFT_SIDE_ONLY : existsLeft ? DIR_EQUAL : DIR_RIGHT_SIDE_ONLY) {}

    //categorization
    CompareDirResult cmpResult;
};

//------------------------------------------------------------------
class FileMapping : public FileSystemObject
{
public:
    template <SelectedSide side> const wxLongLong&  getLastWriteTime() const;
    template <SelectedSide side> const wxULongLong& getFileSize() const;

    virtual CompareFilesResult getCategory() const;
    virtual const wxString& getCatConflict() const;

private:
    friend class CompareProcess;  //only CompareProcess shall be allowed to change cmpResult
    friend class HierarchyObject; //construction

    template <CompareFilesResult res>
    void setCategory();
    void setCategoryConflict(const wxString& description);

    FileMapping(const FileDescriptor&  left, //file exists on both sides
                const Zstring&         fileNameShort,
                CompareFilesResult     defaultCmpResult,
                const FileDescriptor&  right,
                const HierarchyObject& parent) :
        FileSystemObject(true, true, fileNameShort, parent),
        cmpResult(defaultCmpResult),
        dataLeft(left),
        dataRight(right) {}
    FileMapping(const FileDescriptor&  left, //file exists on left side only
                const Zstring&         fileNameShort,
                const HierarchyObject& parent) :
        FileSystemObject(true, false, fileNameShort, parent),
        cmpResult(FILE_LEFT_SIDE_ONLY),
        dataLeft(left),
        dataRight(0, 0) {}
    FileMapping(const Zstring&         fileNameShort, //file exists on right side only
                const FileDescriptor&  right,
                const HierarchyObject& parent) :
        FileSystemObject(false, true, fileNameShort, parent),
        cmpResult(FILE_RIGHT_SIDE_ONLY),
        dataLeft(0, 0),
        dataRight(right) {}

    virtual void swap();
    virtual void removeObjectL();
    virtual void removeObjectR();
    virtual void copyToL();
    virtual void copyToR();
    //------------------------------------------------------------------

    //categorization
    CompareFilesResult cmpResult;
    wxString cmpConflictDescr; //only filled if cmpResult == FILE_CONFLICT

    FileDescriptor dataLeft;
    FileDescriptor dataRight;
};

//------------------------------------------------------------------
class BaseDirMapping : public HierarchyObject //synchronization base directory
{
public:
    BaseDirMapping(const Zstring& dirPostfixedLeft,
                   const Zstring& dirPostfixedRight,
                   bool filterActive,
                   const Zstring& includeFilter,
                   const Zstring& excludeFilter) :
        HierarchyObject(dirPostfixedLeft, dirPostfixedRight),
        filter(filterActive, includeFilter, excludeFilter) {}

    struct FilterSettings
    {
        FilterSettings(bool active,
                       const Zstring& include,
                       const Zstring& exclude) :
            filterActive(active),
            includeFilter(include),
            excludeFilter(exclude) {}
        bool filterActive;
        Zstring includeFilter;
        Zstring excludeFilter;
    };

    const FilterSettings& getFilter() const;
    template <SelectedSide side> Zstring getDBFilename() const;
    virtual void swap();

private:
    FilterSettings filter;
};


typedef std::vector<BaseDirMapping> FolderComparison;

//------------------------------------------------------------------



















//---------------Inline Implementation---------------------------------------------------
inline
FileSystemObject* HierarchyObject::retrieveById(ObjectID id) //returns NULL if object is not found
{
    //code re-use of const method: see Meyers Effective C++
    return const_cast<FileSystemObject*>(static_cast<const HierarchyObject&>(*this).retrieveById(id));
}


inline
HierarchyObject::ObjectID FileSystemObject::getId() const
{
    return uniqueId;
}


inline
HierarchyObject::ObjectID FileSystemObject::getUniqueId()
{
    static unsigned int id = 0;
    return ++id;
}


inline
bool DirContainer::CmpFilename::operator()(const Zstring& a, const Zstring& b) const
{
//        //quick check based on string length
//        const size_t aLength = a.data.shortName.length();
//        const size_t bLength = b.data.shortName.length();
//        if (aLength != bLength)
//            return aLength < bLength;

#ifdef FFS_WIN //Windows does NOT distinguish between upper/lower-case
    return a.CmpNoCase(b) < 0;
#elif defined FFS_LINUX //Linux DOES distinguish between upper/lower-case
    return a.Cmp(b) < 0;
#endif
}

inline
const DirContainer::SubDirList& DirContainer::getSubDirs() const
{
    return subDirs;
}


inline
const DirContainer::SubFileList& DirContainer::getSubFiles() const
{
    return subFiles;
}


inline
const FileDescriptor& FileContainer::getData() const
{
    return data;
}


inline
DirContainer& DirContainer::addSubDir(const Zstring& shortName)
{
    return subDirs.insert(std::make_pair(shortName, DirContainer())).first->second;
}


inline
void DirContainer::addSubFile(const Zstring& shortName, const FileDescriptor& fileData)
{
    subFiles.insert(std::make_pair(shortName, FileContainer(fileData)));
}


inline
CompareFilesResult FileMapping::getCategory() const
{
    return cmpResult;
}


inline
const wxString& FileMapping::getCatConflict() const
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
const wxString& DirMapping::getCatConflict() const
{
    static wxString empty;
    return empty;
}


inline
void FileSystemObject::setSyncDir(SyncDirection newDir)
{
    syncDir = static_cast<SyncDirectionIntern>(newDir); //should be safe by design
}


inline
const wxString& FileSystemObject::getSyncOpConflict() const
{
    //a sync operation conflict can occur when:
    //1. category-conflict and syncDir == NONE -> problem finding category
    //2. syncDir == SYNC_DIR_INT_CONFLICT -> problem finding sync direction
    return syncDir == SYNC_DIR_INT_CONFLICT ? syncOpConflictDescr : getCatConflict();
}


inline
void FileSystemObject::setSyncDirConflict(const wxString& description) //set syncDir = SYNC_DIR_INT_CONFLICT
{
    syncDir = SYNC_DIR_INT_CONFLICT;
    syncOpConflictDescr = description;
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
}


inline
SyncOperation FileSystemObject::getSyncOperation() const
{
    return getSyncOperation(getCategory(), selectedForSynchronization, syncDir);
}


inline
SyncOperation FileSystemObject::testSyncOperation(bool selected, SyncDirection proposedDir) const
{
    return getSyncOperation(getCategory(), selected, static_cast<SyncDirectionIntern>(proposedDir)); //should be safe by design
}


template <>
inline
bool FileSystemObject::isEmpty<LEFT_SIDE>() const
{
    return !existsLeft_;
}


template <>
inline
bool FileSystemObject::isEmpty<RIGHT_SIDE>() const
{
    return !existsRight_;
}


inline
bool FileSystemObject::isEmpty() const
{
    return isEmpty<LEFT_SIDE>() && isEmpty<RIGHT_SIDE>();
}


template <SelectedSide side>
inline
const Zstring& FileSystemObject::getShortName() const
{
    static Zstring null;
    return isEmpty<side>() ? null : shortName_;
}


template <SelectedSide side>
inline
const Zstring FileSystemObject::getRelativeName() const
{
    return isEmpty<side>() ? Zstring() : nameBuffer.parentRelNamePf + shortName_;
}


inline
const Zstring FileSystemObject::getObjRelativeName() const
{
    return nameBuffer.parentRelNamePf + shortName_;
}


inline
const Zstring& FileSystemObject::getObjShortName() const
{
    return shortName_;
}


inline
const Zstring FileSystemObject::getParentRelativeName() const
{
    return nameBuffer.parentRelNamePf.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR);  //returns empty string if char not found
}


template <SelectedSide side>
inline
const Zstring FileSystemObject::getFullName() const
{
    return isEmpty<side>() ? Zstring() : getBaseDirPf<side>() + nameBuffer.parentRelNamePf + shortName_;
}


template <>
inline
const Zstring& FileSystemObject::getBaseDirPf<LEFT_SIDE>() const
{
    return nameBuffer.baseDirPfL;
}


template <>
inline
const Zstring& FileSystemObject::getBaseDirPf<RIGHT_SIDE>() const
{
    return nameBuffer.baseDirPfR;
}


template <>
inline
void FileSystemObject::removeObject<LEFT_SIDE>()
{
    existsLeft_ = false;
    removeObjectL();
}


template <>
inline
void FileSystemObject::removeObject<RIGHT_SIDE>()
{
    existsRight_ = false;
    removeObjectR();
}


inline
void FileSystemObject::synchronizeSides()
{
    switch (syncDir)
    {
    case SYNC_DIR_INT_LEFT:
        existsLeft_ = existsRight_;
        copyToL();
        break;
    case SYNC_DIR_INT_RIGHT:
        existsRight_ = existsLeft_;
        copyToR();
        break;
    case SYNC_DIR_INT_NONE:
    case SYNC_DIR_INT_CONFLICT:
        assert(!"if nothing's todo then why arrive here?");
        break;
    }

    syncDir = SYNC_DIR_INT_NONE;
}


inline
void FileSystemObject::swap()
{
    std::swap(nameBuffer.baseDirPfL, nameBuffer.baseDirPfR);
    std::swap(existsLeft_, existsRight_);
}


inline
void HierarchyObject::swap()
{
    std::swap(baseDirLeft, baseDirRight);

    //files
    std::for_each(subFiles.begin(), subFiles.end(), std::mem_fun_ref(&FileMapping::swap));
    //directories
    std::for_each(subDirs.begin(), subDirs.end(), std::mem_fun_ref(&DirMapping::swap));
}


inline
const Zstring& HierarchyObject::getRelativeNamePf() const
{
    return relNamePf;
}


inline
DirMapping& HierarchyObject::addSubDir(bool existsLeft,
                                       const Zstring& dirNameShort,
                                       bool existsRight)
{
    subDirs.push_back(DirMapping(existsLeft, dirNameShort, existsRight, *this));
    return subDirs.back();
}


inline
FileMapping& HierarchyObject::addSubFile(const FileDescriptor&   left,          //file exists on both sides
        const Zstring&          fileNameShort,
        CompareFilesResult      defaultCmpResult,
        const FileDescriptor&   right)
{
    subFiles.push_back(FileMapping(left, fileNameShort, defaultCmpResult, right, *this));
    return subFiles.back();
}


inline
void HierarchyObject::addSubFile(const FileDescriptor&   left,          //file exists on left side only
                                 const Zstring&          fileNameShort)
{
    subFiles.push_back(FileMapping(left, fileNameShort, *this));
}


inline
void HierarchyObject::addSubFile(const Zstring&          fileNameShort, //file exists on right side only
                                 const FileDescriptor&   right)
{
    subFiles.push_back(FileMapping(fileNameShort, right, *this));
}


inline
void BaseDirMapping::swap()
{
    //call base class versions
    HierarchyObject::swap();
}


inline
void DirMapping::swap()
{
    //call base class versions
    HierarchyObject::swap();
    FileSystemObject::swap();

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
        break;
    }
}


inline
void DirMapping::removeObjectL()
{
    cmpResult = DIR_RIGHT_SIDE_ONLY;
    std::for_each(subFiles.begin(), subFiles.end(), std::mem_fun_ref(&FileSystemObject::removeObject<LEFT_SIDE>));
    std::for_each(subDirs.begin(),  subDirs.end(),  std::mem_fun_ref(&FileSystemObject::removeObject<LEFT_SIDE>));
}


inline
void DirMapping::removeObjectR()
{
    cmpResult = DIR_LEFT_SIDE_ONLY;
    std::for_each(subFiles.begin(), subFiles.end(), std::mem_fun_ref(&FileSystemObject::removeObject<RIGHT_SIDE>));
    std::for_each(subDirs.begin(),  subDirs.end(),  std::mem_fun_ref(&FileSystemObject::removeObject<RIGHT_SIDE>));
}


inline
void DirMapping::copyToL()
{
    cmpResult = DIR_EQUAL;
}


inline
void DirMapping::copyToR()
{
    cmpResult = DIR_EQUAL;
}


inline
const BaseDirMapping::FilterSettings& BaseDirMapping::getFilter() const
{
    return filter;
}


template <SelectedSide side>
inline
Zstring BaseDirMapping::getDBFilename() const
{
    return getBaseDir<side>() + getSyncDBFilename();
}


inline
void FileMapping::swap()
{
    //call base class version
    FileSystemObject::swap();

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
    case FILE_CONFLICT:
        break;
    }

    std::swap(dataLeft, dataRight);
}


template <CompareFilesResult res>
inline
void FileMapping::setCategory()
{
    cmpResult = res;
}

template <>
inline
void FileMapping::setCategory<FILE_CONFLICT>(); //if conflict is detected, use setCategoryConflict! => method is not defined!


inline
void FileMapping::setCategoryConflict(const wxString& description)
{
    cmpResult = FILE_CONFLICT;
    cmpConflictDescr = description;
}


inline
void FileMapping::removeObjectL()
{
    cmpResult = FILE_RIGHT_SIDE_ONLY;
    dataLeft = FileDescriptor(0, 0);
}


inline
void FileMapping::removeObjectR()
{
    cmpResult = FILE_LEFT_SIDE_ONLY;
    dataRight = FileDescriptor(0, 0);
}


inline
void FileMapping::copyToL()
{
    cmpResult = FILE_EQUAL;
    dataLeft = dataRight;
}


inline
void FileMapping::copyToR()
{
    cmpResult = FILE_EQUAL;
    dataRight = dataLeft;
}


template <>
inline
const wxLongLong& FileMapping::getLastWriteTime<LEFT_SIDE>() const
{
    return dataLeft.lastWriteTimeRaw;
}


template <>
inline
const wxLongLong& FileMapping::getLastWriteTime<RIGHT_SIDE>() const
{
    return dataRight.lastWriteTimeRaw;
}


template <>
inline
const wxULongLong& FileMapping::getFileSize<LEFT_SIDE>() const
{
    return dataLeft.fileSize;
}


template <>
inline
const wxULongLong& FileMapping::getFileSize<RIGHT_SIDE>() const
{
    return dataRight.fileSize;
}
}

#endif // FILEHIERARCHY_H_INCLUDED
