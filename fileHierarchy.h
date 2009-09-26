#ifndef FILEHIERARCHY_H_INCLUDED
#define FILEHIERARCHY_H_INCLUDED

#include "shared/zstring.h"
#include "shared/systemConstants.h"
#include <wx/longlong.h>
#include <set>
#include <vector>
#include "structures.h"

class DirectoryBuffer;


namespace FreeFileSync
{
    struct FileDescriptor
    {
        FileDescriptor(    const Zstring&     shortNameIn,
                           const wxLongLong&  lastWriteTimeRawIn,
                           const wxULongLong& fileSizeIn) :
                shortName(shortNameIn),
                lastWriteTimeRaw(lastWriteTimeRawIn),
                fileSize(fileSizeIn) {}

        //fullname == baseDirectoryPf + parentRelNamePf + shortName
        Zstring     shortName;
        wxLongLong  lastWriteTimeRaw; //number of seconds since Jan. 1st 1970 UTC, same semantics like time_t (== signed long)
        wxULongLong fileSize;
    };


    struct DirDescriptor
    {
        DirDescriptor(const Zstring& shortNameIn) :
                shortName(shortNameIn) {}

        //fullname == baseDirectoryPf + parentRelNamePf + shortName
        Zstring shortName;
    };


    class FileContainer;
    class FileMapping;
    class DirMapping;
    class CompareProcess;
    class HierarchyObject;
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
        void addSubFile(const FileDescriptor& fileData, const Zstring& parentRelativeNamePf);
        DirContainer& addSubDir(const DirDescriptor& dirData, const Zstring& parentRelativeNamePf);

        //------------------------------------------------------------------
        template <class T>
        struct CmpLess : public std::binary_function<T, T, bool>
        {
            bool operator()(const T& a, const T& b) const;
        };

        typedef std::set<DirContainer,  CmpLess<DirContainer> >  SubDirList;
        typedef std::set<FileContainer, CmpLess<FileContainer> > SubFileList;
        //------------------------------------------------------------------

        const DirDescriptor& getData() const;
        const Zstring& getParentRelNamePf() const;

        const SubDirList& getSubDirs() const;
        const SubFileList& getSubFiles() const;

    private:
        friend class CompareProcess; //only DirectoryBuffer is allowed to create (Base-)DirContainers

        DirContainer(const DirDescriptor& dirData, const Zstring& parentRelativeNamePf) :
                data(dirData),
                parentRelNamePf(parentRelativeNamePf) {}

        DirContainer() : data(Zstring()) {} //default constructor used by DirectoryBuffer only!

        const DirDescriptor data;
        const Zstring parentRelNamePf; //buffer some redundant data:

        SubDirList  subDirs;  //contained directories
        SubFileList subFiles; //contained files
    };

//------------------------------------------------------------------
    class FileContainer
    {
    public:
        const FileDescriptor& getData() const;
        const Zstring& getParentRelNamePf() const;

    private:
        friend class DirContainer;

        FileContainer(const FileDescriptor& fileData, const Zstring& parentRelativeNamePf) :
                data(fileData),
                parentRelNamePf(parentRelativeNamePf) {}

        const FileDescriptor data;
        const Zstring parentRelNamePf; //buffer some redundant data:
    };


//------------------------------------------------------------------
    /*    class hierarchy:

          FileSystemObject  HierarchyObject
                /|\              /|\
           ______|______    ______|______
          |             |  |             |
    FileMapping      DirMapping    BaseDirMapping
    */

//------------------------------------------------------------------
    enum SelectedSide
    {
        LEFT_SIDE,
        RIGHT_SIDE
    };

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
        typedef unsigned int ObjectID;
        template <SelectedSide side>           bool     isEmpty()         const;
        template <SelectedSide side> const Zstring&     getShortName()    const;
        template <SelectedSide side> const Zstring      getRelativeName() const; //get name relative to base sync dir without FILE_NAME_SEPARATOR prefix
        template <SelectedSide side> const Zstring      getParentRelativeName() const; //get name relative to base sync dir without FILE_NAME_SEPARATOR postfix
        template <SelectedSide side> const Zstring      getFullName()     const; //getFullName() == getBaseDirPf() + getRelativeName()
        template <SelectedSide side> const Zstring&     getBaseDirPf()    const; //base sync directory postfixed with FILE_NAME_SEPARATOR
        ObjectID getId() const; //get unique id; ^= logical key

        void swap();

        template <SelectedSide side> void copyTo();          //copy one side to the other (NOT recursive!!!)
        template <SelectedSide side> void removeObject();    //removes file or directory (recursively!): used by manual deletion
        bool isEmpty() const; //true, if both sides are empty
        static void removeEmpty(BaseDirMapping& baseDir);  //remove all invalid entries (where both sides are empty) recursively
        static void removeEmptyNonRec(HierarchyObject& hierObj); //remove all invalid entries (where both sides are empty) non-recursively

        //sync settings:
        SyncDirection syncDir;
        bool selectedForSynchronization;

        virtual CompareFilesResult getCategory() const = 0;
        virtual const wxString& getConflictDescription() const = 0;

    protected:
        FileSystemObject(const RelNamesBuffered& relNameBuff) :
                syncDir(SYNC_DIR_NONE),
                selectedForSynchronization(true),
                nameBuffer(relNameBuff),
                uniqueId(getUniqueId()) {}

        ~FileSystemObject() {} //don't need polymorphic deletion

    private:
        virtual void swapDescriptors() = 0;

        virtual           bool isEmptyL() const = 0;
        virtual           bool isEmptyR() const = 0;
        virtual           void removeObjectL() = 0;
        virtual           void removeObjectR() = 0;
        virtual           void copyToL() = 0;
        virtual           void copyToR() = 0;
        virtual const Zstring& getShortNameL() const = 0;
        virtual const Zstring& getShortNameR() const = 0;
        static ObjectID getUniqueId();

        //buffer some redundant data:
        RelNamesBuffered nameBuffer; //base sync dirs + relative parent name: this does NOT belong into FileDescriptor/DirDescriptor

        ObjectID uniqueId;
    };

//------------------------------------------------------------------
    class HierarchyObject
    {
    public:
        FileSystemObject* retrieveById(FileSystemObject::ObjectID id);             //returns NULL if object is not found; logarithmic complexity
        const FileSystemObject* retrieveById(FileSystemObject::ObjectID id) const; //

        DirMapping& addSubDir(const DirDescriptor&    left,
                              CompareDirResult        defaultCmpResult,
                              const DirDescriptor&    right,
                              const RelNamesBuffered& relNameBuff);
        FileMapping& addSubFile(const FileDescriptor&   left,
                                CompareFilesResult      defaultCmpResult,
                                const FileDescriptor&   right,
                                const RelNamesBuffered& relNameBuff);

        typedef std::vector<FileMapping> SubFileMapping;
        typedef std::vector<DirMapping>  SubDirMapping;

        SubFileMapping subFiles; //contained file maps
        SubDirMapping  subDirs;  //contained directory maps

    protected:
        HierarchyObject() {}
        ~HierarchyObject() {} //don't need polymorphic deletion
    };

//------------------------------------------------------------------
    class DirMapping : public FileSystemObject, public HierarchyObject
    {
    public:
        static const DirDescriptor& nullData();

        virtual CompareFilesResult getCategory() const;
        virtual const wxString& getConflictDescription() const;

    private:
        friend class CompareProcess; //only CompareProcess shall be allowed to change cmpResult
        friend class HierarchyObject;
        virtual           void swapDescriptors();
        virtual           bool isEmptyL() const;
        virtual           bool isEmptyR() const;
        virtual           void removeObjectL();
        virtual           void removeObjectR();
        virtual           void copyToL();
        virtual           void copyToR();
        virtual const Zstring& getShortNameL() const;
        virtual const Zstring& getShortNameR() const;
        //------------------------------------------------------------------

        DirMapping(const DirDescriptor& left, CompareDirResult defaultCmpResult, const DirDescriptor& right, const RelNamesBuffered& relNameBuff) :
                FileSystemObject(relNameBuff),
                cmpResult(defaultCmpResult),
                dataLeft(left),
                dataRight(right) {}

        //categorization
        CompareDirResult cmpResult;

        DirDescriptor dataLeft;
        DirDescriptor dataRight;
    };

//------------------------------------------------------------------
    class FileMapping : public FileSystemObject
    {
    public:
        template <SelectedSide side> const wxLongLong&  getLastWriteTime() const;
        template <SelectedSide side> const wxULongLong& getFileSize() const;

        static const FileDescriptor& nullData();

        virtual CompareFilesResult getCategory() const;
        virtual const wxString& getConflictDescription() const;

    private:
        friend class CompareProcess; //only CompareProcess shall be allowed to change cmpResult
        friend class HierarchyObject;

        FileMapping(const FileDescriptor& left, CompareFilesResult defaultCmpResult, const FileDescriptor& right, const RelNamesBuffered& relNameBuff) :
                FileSystemObject(relNameBuff),
                cmpResult(defaultCmpResult),
                dataLeft(left),
                dataRight(right) {}

        virtual           void swapDescriptors();
        virtual           bool isEmptyL() const;
        virtual           bool isEmptyR() const;
        virtual           void removeObjectL();
        virtual           void removeObjectR();
        virtual           void copyToL();
        virtual           void copyToR();
        virtual const Zstring& getShortNameL() const;
        virtual const Zstring& getShortNameR() const;
        //------------------------------------------------------------------

        //categorization
        CompareFilesResult cmpResult;
        wxString conflictDescription; //only filled if cmpResult == FILE_CONFLICT

        FileDescriptor dataLeft;
        FileDescriptor dataRight;
    };

//------------------------------------------------------------------
    class BaseDirMapping : public HierarchyObject //synchronization base directory
    {
    public:
        BaseDirMapping(const Zstring& dirPostfixedLeft, const Zstring& dirPostfixedRight) :
                baseDirLeft(dirPostfixedLeft),
                baseDirRight(dirPostfixedRight) {}

        template <SelectedSide side> const Zstring& getBaseDir() const;

    private:
        Zstring baseDirLeft;  //directory name ending with FILE_NAME_SEPARATOR
        Zstring baseDirRight; //directory name ending with FILE_NAME_SEPARATOR
    };


    typedef std::vector<BaseDirMapping> FolderComparison;

//------------------------------------------------------------------



















//---------------Inline Implementation---------------------------------------------------
    inline
    FileSystemObject* HierarchyObject::retrieveById(FileSystemObject::ObjectID id) //returns NULL if object is not found
    {
        //code re-use of const method: see Meyers Effective C++
        return const_cast<FileSystemObject*>(static_cast<const HierarchyObject&>(*this).retrieveById(id));
    }


    inline
    FileSystemObject::ObjectID FileSystemObject::getId() const
    {
        return uniqueId;
    }


    inline
    FileSystemObject::ObjectID FileSystemObject::getUniqueId()
    {
        static unsigned int id = 0;
        return ++id;
    }


    template <class T>
    inline
    bool DirContainer::CmpLess<T>::operator()(const T& a, const T& b) const
    {
//        //quick check based on string length
//        const size_t aLength = a.data.shortName.length();
//        const size_t bLength = b.data.shortName.length();
//        if (aLength != bLength)
//            return aLength < bLength;

#ifdef FFS_WIN //Windows does NOT distinguish between upper/lower-case
        return a.data.shortName.CmpNoCase(b.data.shortName) < 0;
#elif defined FFS_LINUX //Linux DOES distinguish between upper/lower-case
        return a.data.shortName.Cmp(b.data.shortName) < 0;
#endif
    }


    inline
    const DirDescriptor& DirContainer::getData() const
    {
        return data;
    }


    inline
    const Zstring& DirContainer::getParentRelNamePf() const
    {
        return parentRelNamePf;
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
    const Zstring& FileContainer::getParentRelNamePf() const
    {
        return parentRelNamePf;
    }


    inline
    DirContainer& DirContainer::addSubDir(const DirDescriptor& dirData, const Zstring& parentRelativeNamePf)
    {
        //const cast: another crime in the name of unproven performance optimization ;) however key part cannot be changed, so this should be safe
        return const_cast<DirContainer&>(*subDirs.insert(DirContainer(dirData, parentRelativeNamePf)).first);
    }


    inline
    void DirContainer::addSubFile(const FileDescriptor& fileData, const Zstring& parentRelativeNamePf)
    {
        subFiles.insert(FileContainer(fileData, parentRelativeNamePf));
    }


    inline
    CompareFilesResult FileMapping::getCategory() const
    {
        return cmpResult;
    }


    inline
    const wxString& FileMapping::getConflictDescription() const
    {
        return conflictDescription;
    }


    inline
    CompareFilesResult DirMapping::getCategory() const
    {
        return convertToFilesResult(cmpResult);
    }


    inline
    const wxString& DirMapping::getConflictDescription() const
    {
        static wxString empty;
        return empty;
    }


    template <SelectedSide side>
    inline
    bool FileSystemObject::isEmpty() const
    {
        return side == LEFT_SIDE ? isEmptyL() : isEmptyR();
    }


    inline
    bool FileSystemObject::isEmpty() const
    {
        return isEmptyL() && isEmptyR();
    }


    template <SelectedSide side>
    inline
    const Zstring& FileSystemObject::getShortName() const
    {
        return side == LEFT_SIDE ? getShortNameL() : getShortNameR();
    }


    template <SelectedSide side>
    inline
    const Zstring FileSystemObject::getRelativeName() const
    {
        return isEmpty<side>() ? Zstring() : nameBuffer.parentRelNamePf + getShortName<side>();
    }


    template <SelectedSide side>
    inline
    const Zstring FileSystemObject::getParentRelativeName() const
    {
        return nameBuffer.parentRelNamePf.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR);  //returns empty string if char not found
    }


    template <SelectedSide side>
    inline
    const Zstring FileSystemObject::getFullName() const
    {
        return isEmpty<side>() ? Zstring() : getBaseDirPf<side>() + nameBuffer.parentRelNamePf + getShortName<side>();
    }


    template <SelectedSide side>
    inline
    const Zstring& FileSystemObject::getBaseDirPf() const
    {
        return side == LEFT_SIDE ? nameBuffer.baseDirPfL : nameBuffer.baseDirPfR;
    }


    template <SelectedSide side>
    inline
    void FileSystemObject::removeObject()
    {
        if (side == LEFT_SIDE)
            removeObjectL();
        else
            removeObjectR();
    }


    template <SelectedSide side>
    inline
    void FileSystemObject::copyTo()
    {
        syncDir   = SYNC_DIR_NONE;

        if (side == LEFT_SIDE)
            copyToL();
        else
            copyToR();
    }


    inline
    void FileSystemObject::swap()
    {
        //swap file descriptors
        swapDescriptors();
    }


    inline
    DirMapping& HierarchyObject::addSubDir(const DirDescriptor& left,
                                           CompareDirResult defaultCmpResult,
                                           const DirDescriptor& right,
                                           const RelNamesBuffered& relNameBuff)
    {
        subDirs.push_back(DirMapping(left, defaultCmpResult, right, relNameBuff));
        return subDirs.back();
    }


    inline
    FileMapping& HierarchyObject::addSubFile(const FileDescriptor& left,
            CompareFilesResult defaultCmpResult,
            const FileDescriptor& right,
            const RelNamesBuffered& relNameBuff)
    {
        subFiles.push_back(FileMapping(left, defaultCmpResult, right, relNameBuff));
        return subFiles.back();
    }


    inline
    void DirMapping::swapDescriptors()
    {
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

        std::swap(dataLeft, dataRight);
    }


    inline
    const DirDescriptor& DirMapping::nullData()
    {
        static DirDescriptor output( static_cast<Zstring>(Zstring()));
        return output;
    }


    inline
    void DirMapping::removeObjectL()
    {
        cmpResult = DIR_RIGHT_SIDE_ONLY;
        dataLeft = nullData();
        std::for_each(subFiles.begin(), subFiles.end(), std::mem_fun_ref(&FileSystemObject::removeObject<LEFT_SIDE>));
        std::for_each(subDirs.begin(),  subDirs.end(),  std::mem_fun_ref(&FileSystemObject::removeObject<LEFT_SIDE>));
    }


    inline
    void DirMapping::removeObjectR()
    {
        cmpResult = DIR_LEFT_SIDE_ONLY;
        dataRight = nullData();
        std::for_each(subFiles.begin(), subFiles.end(), std::mem_fun_ref(&FileSystemObject::removeObject<RIGHT_SIDE>));
        std::for_each(subDirs.begin(),  subDirs.end(),  std::mem_fun_ref(&FileSystemObject::removeObject<RIGHT_SIDE>));
    }


    inline
    void DirMapping::copyToL()
    {
        cmpResult = DIR_EQUAL;
        dataLeft = dataRight;
    }


    inline
    void DirMapping::copyToR()
    {
        cmpResult = DIR_EQUAL;
        dataRight = dataLeft;
    }


    inline
    const Zstring& DirMapping::getShortNameL() const
    {
        return dataLeft.shortName;
    }


    inline
    const Zstring& DirMapping::getShortNameR() const
    {
        return dataRight.shortName;
    }


    inline
    bool DirMapping::isEmptyL() const
    {
        return dataLeft.shortName.empty();
    }


    inline
    bool DirMapping::isEmptyR() const
    {
        return dataRight.shortName.empty();
    }


    template <SelectedSide side>
    inline
    const Zstring& BaseDirMapping::getBaseDir() const
    {
        return side == LEFT_SIDE ? baseDirLeft : baseDirRight;
    }


    inline
    void FileMapping::swapDescriptors()
    {
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


    inline
    void FileMapping::removeObjectL()
    {
        cmpResult = FILE_RIGHT_SIDE_ONLY;
        dataLeft = nullData();
    }


    inline
    void FileMapping::removeObjectR()
    {
        cmpResult = FILE_LEFT_SIDE_ONLY;
        dataRight = nullData();
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


    inline
    const FileDescriptor& FileMapping::nullData()
    {
        static FileDescriptor output(Zstring(), 0, 0);
        return output;
    }


    inline
    const Zstring& FileMapping::getShortNameL() const
    {
        return dataLeft.shortName;
    }


    inline
    const Zstring& FileMapping::getShortNameR() const
    {
        return dataRight.shortName;
    }


    inline
    bool FileMapping::isEmptyL() const
    {
        return dataLeft.shortName.empty();
    }


    inline
    bool FileMapping::isEmptyR() const
    {
        return dataRight.shortName.empty();
    }


    template <SelectedSide side>
    inline
    const wxLongLong& FileMapping::getLastWriteTime() const
    {
        return side == LEFT_SIDE ? dataLeft.lastWriteTimeRaw : dataRight.lastWriteTimeRaw;
    }


    template <SelectedSide side>
    inline
    const wxULongLong& FileMapping::getFileSize() const
    {
        return side == LEFT_SIDE ? dataLeft.fileSize : dataRight.fileSize;
    }
}

#endif // FILEHIERARCHY_H_INCLUDED
