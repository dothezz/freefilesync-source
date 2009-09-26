#include "fileHierarchy.h"

using namespace FreeFileSync;


struct LowerID
{
    bool operator()(const FileSystemObject& a, FileSystemObject::ObjectID b) const
    {
        return a.getId() < b;
    }

    bool operator()(const FileSystemObject& a, const FileSystemObject& b) const //used by VC++
    {
        return a.getId() < b.getId();
    }

    bool operator()(FileSystemObject::ObjectID a, const FileSystemObject& b) const
    {
        return a < b.getId();
    }
};


const FileSystemObject* HierarchyObject::retrieveById(FileSystemObject::ObjectID id) const //returns NULL if object is not found
{
    //ATTENTION: HierarchyObject::retrieveById() can only work correctly if the following conditions are fulfilled:
    //1. on each level, files are added first, then directories (=> file id < dir id)
    //2. when a directory is added, all subdirectories must be added immediately (recursion) before the next dir on this level is added
    //3. entries may be deleted but NEVER new ones inserted!!!
    //=> this allows for a quasi-binary search by id!

    //See MergeSides::execute()!


    //search within sub-files
    SubFileMapping::const_iterator i = std::lower_bound(subFiles.begin(), subFiles.end(), id, LowerID()); //binary search!
    if (i != subFiles.end())
    {   //id <= i
        if (LowerID()(id, *i))
            return NULL; // --i < id < i
        else //id found
            return &(*i);
    }
    else //search within sub-directories
    {
        SubDirMapping::const_iterator j = std::lower_bound(subDirs.begin(), subDirs.end(), id, LowerID()); //binary search!
        if (j != subDirs.end()) //id <= j
        {
            if (LowerID()(id, *j)) // --j < id < j
            {
                if (j == subDirs.begin())
                    return NULL;
                else
                    return (--j)->retrieveById(id);
            }
            else //id found
                return &(*j);
        }
        else //subdirs < id
        {
            if (j == subDirs.begin()) //empty vector
                return NULL;
            else // --j < id < j
                return (--j)->retrieveById(id);
        }
    }
}


struct IsInvalid
{
    bool operator()(const FileSystemObject& fsObj) const
    {
        return fsObj.isEmpty();
    }
};


void FileSystemObject::removeEmptyNonRec(HierarchyObject& hierObj)
{
    //remove invalid files
    hierObj.subFiles.erase(std::remove_if(hierObj.subFiles.begin(), hierObj.subFiles.end(), IsInvalid()),
                           hierObj.subFiles.end());

    //remove invalid directories
    hierObj.subDirs.erase(std::remove_if(hierObj.subDirs.begin(), hierObj.subDirs.end(), IsInvalid()),
                          hierObj.subDirs.end());
}


void removeEmptyRec(HierarchyObject& hierObj)
{
    FileSystemObject::removeEmptyNonRec(hierObj);

    //recurse into remaining directories
    std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), removeEmptyRec);
}


void FileSystemObject::removeEmpty(BaseDirMapping& baseDir)
{
    removeEmptyRec(baseDir);
}

