// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "detect_renaming.h"
#include <map>
#include <vector>
#include <boost/bind.hpp>

using namespace FreeFileSync;

/*detect renamed files:
Example:
 X  ->  |_|      Create right
|_| ->   Y       Delete right

is detected as:

Rename Y to X on right

Algorithm:
----------
DB-file left  ---filename, Metadata(=:MD)--->    DB-file right
  /|\                                                |
   |                                             fileID, MD
 fileID, MD                                          |
   |                                                \|/
   X                                                 Y

*/


class FindDBAssoc
{
    /*
      load and associate db-files by filename and metadata(size, date)
      fileID, MD |-> fileID
    */
public:
    struct AssocKey
    {
        AssocKey(const Utility::FileID& fileId,
                 const wxLongLong&      lastWriteTimeRaw,
                 const wxULongLong&     fileSize);

        bool operator<(const AssocKey& other) const;

        Utility::FileID fileId_;
        wxLongLong      lastWriteTimeRaw_;
        wxULongLong     fileSize_;
    };

    FindDBAssoc(const FreeFileSync::BaseDirMapping& baseMapping,
                std::map<AssocKey, Utility::FileID>& assocDBLeftToRight);

private:
    void recurse(const DirContainer& leftSide, const DirContainer& rightSide);

    std::map<AssocKey, Utility::FileID>& assocDBLeftToRight_; //-->
};


inline
FindDBAssoc::AssocKey::AssocKey(const Utility::FileID& fileId,
                                const wxLongLong&      lastWriteTimeRaw,
                                const wxULongLong&     fileSize) :
    fileId_(fileId),
    lastWriteTimeRaw_(lastWriteTimeRaw),
    fileSize_(fileSize) {}


inline
bool FindDBAssoc::AssocKey::operator<(const AssocKey& other) const
{
    if (fileId_ != other.fileId_)
        return fileId_ < other.fileId_;

    if (lastWriteTimeRaw_ != other.lastWriteTimeRaw_)
        return lastWriteTimeRaw_ < other.lastWriteTimeRaw_;

    return fileSize_ < other.fileSize_;
}


FindDBAssoc::FindDBAssoc(const FreeFileSync::BaseDirMapping& baseMapping,
                         std::map<AssocKey, Utility::FileID>& assocDBLeftToRight) : assocDBLeftToRight_(assocDBLeftToRight)
{
    try
    {
        std::pair<FreeFileSync::DirInfoPtr, FreeFileSync::DirInfoPtr> dbInfo =
            FreeFileSync::loadFromDisk(baseMapping); //throw (FileError)

        recurse(dbInfo.first->baseDirContainer,
                dbInfo.second->baseDirContainer);
    }
    catch (...) {} //swallow...
}


void FindDBAssoc::recurse(const DirContainer& leftSide, const DirContainer& rightSide)
{
    for (DirContainer::SubFileList::const_iterator i = leftSide.getSubFiles().begin(); i != leftSide.getSubFiles().end(); ++i)
    {
        const FileDescriptor& fileDescrI = i->second.getData();
        if (!fileDescrI.fileIdentifier.isNull()) //fileIdentifier may be NULL
        {
            const DirContainer::SubFileList::const_iterator j = rightSide.getSubFiles().find(i->first);

            //find files that exist on left and right
            if (j != rightSide.getSubFiles().end())
            {
                const FileDescriptor& fileDescrJ = j->second.getData();
                if (!fileDescrJ.fileIdentifier.isNull())  //fileIdentifier may be NULL
                {
                    if (    fileDescrI.lastWriteTimeRaw == fileDescrJ.lastWriteTimeRaw &&
                            fileDescrI.fileSize         == fileDescrJ.fileSize)
                    {
                        assocDBLeftToRight_[AssocKey(fileDescrI.fileIdentifier,
                                                     fileDescrI.lastWriteTimeRaw,
                                                     fileDescrI.fileSize)] = fileDescrJ.fileIdentifier;
                    }
                }
            }
        }
    }

    //-----------------------------------------------------------------------------------------------
    for (DirContainer::SubDirList::const_iterator i = leftSide.getSubDirs().begin(); i != leftSide.getSubDirs().end(); ++i)
    {
        const DirContainer::SubDirList::const_iterator j = rightSide.getSubDirs().find(i->first);

        //directories that exist on both sides
        if (j != rightSide.getSubDirs().end())
        {
            recurse(i->second, j->second); //recurse into subdirectories
        }
    }
}



class FindRenameCandidates
{
public:
    FindRenameCandidates(FreeFileSync::BaseDirMapping& baseMapping)
    {
        FindDBAssoc(baseMapping,
                    assocDBLeftToRight);

        if (!assocDBLeftToRight.empty())
            recurse(baseMapping);
    }

    void getRenameCandidates(std::vector<std::pair<FileMapping*, FileMapping*> >& renameOnLeft,
                             std::vector<std::pair<FileMapping*, FileMapping*> >& renameOnRight);

private:
    void recurse(HierarchyObject& hierObj)
    {
        //files
        std::for_each(hierObj.subFiles.begin(), hierObj.subFiles.end(),
                      boost::bind(&FindRenameCandidates::processFile, this, _1));

        //directories
        std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(),
                      boost::bind(&FindRenameCandidates::recurse, this, _1));//recursion
    }

    void processFile(FileMapping& fileObj)
    {
        switch (fileObj.getSyncOperation()) //evaluate comparison result and sync direction
        {
            case SO_CREATE_NEW_LEFT:
                if (!fileObj.getFileID<RIGHT_SIDE>().isNull())  //fileIdentifier may be NULL
                    createLeft[FindDBAssoc::AssocKey(fileObj.getFileID<RIGHT_SIDE>(),
                                                     fileObj.getLastWriteTime<RIGHT_SIDE>(),
                                                     fileObj.getFileSize<RIGHT_SIDE>())] = &fileObj;
                break;

            case SO_CREATE_NEW_RIGHT:
                if (!fileObj.getFileID<LEFT_SIDE>().isNull())  //fileIdentifier may be NULL
                    createRight.push_back(&fileObj);
                break;

            case SO_DELETE_LEFT:
                if (!fileObj.getFileID<LEFT_SIDE>().isNull())  //fileIdentifier may be NULL
                    deleteLeft.push_back(&fileObj);
                break;

            case SO_DELETE_RIGHT:
                if (!fileObj.getFileID<RIGHT_SIDE>().isNull())  //fileIdentifier may be NULL
                    deleteRight[FindDBAssoc::AssocKey(fileObj.getFileID<RIGHT_SIDE>(),
                                                      fileObj.getLastWriteTime<RIGHT_SIDE>(),
                                                      fileObj.getFileSize<RIGHT_SIDE>())] = &fileObj;
                break;

            case SO_OVERWRITE_RIGHT:
            case SO_OVERWRITE_LEFT:
            case SO_DO_NOTHING:
            case SO_UNRESOLVED_CONFLICT:
                break;
        }

    }


    std::vector<FileMapping*> createRight; //pointer always bound!
    std::vector<FileMapping*> deleteLeft;  //
    //         |
    //        \|/
    std::map<FindDBAssoc::AssocKey, Utility::FileID> assocDBLeftToRight;
    //         |
    //        \|/
    std::map<FindDBAssoc::AssocKey, FileMapping*> deleteRight; //pointer always bound!
    std::map<FindDBAssoc::AssocKey, FileMapping*> createLeft;  //

};



void FindRenameCandidates::getRenameCandidates(
    std::vector<std::pair<FileMapping*, FileMapping*> >& renameOnLeft,
    std::vector<std::pair<FileMapping*, FileMapping*> >& renameOnRight)
{
    for (std::vector<FileMapping*>::const_iterator crRightIter = createRight.begin();
         crRightIter != createRight.end();
         ++crRightIter)
    {
        const FindDBAssoc::AssocKey assocDbKey((*crRightIter)->getFileID<LEFT_SIDE>(),
                                               (*crRightIter)->getLastWriteTime<LEFT_SIDE>(),
                                               (*crRightIter)->getFileSize<LEFT_SIDE>());

        const std::map<FindDBAssoc::AssocKey, Utility::FileID>::const_iterator assocDBIter =
            assocDBLeftToRight.find(assocDbKey);

        if (assocDBIter != assocDBLeftToRight.end())
        {
            std::map<FindDBAssoc::AssocKey, FileMapping*>::const_iterator delRightIter =
                deleteRight.find(FindDBAssoc::AssocKey(assocDBIter->second, //FileID of right side
                                                       assocDbKey.lastWriteTimeRaw_,
                                                       assocDbKey.fileSize_));

            if (delRightIter != deleteRight.end())
            {
                renameOnRight.push_back(std::make_pair(*crRightIter, delRightIter->second));
            }
        }
    }
    //------------------------------------------------------------------------------------------------
    for (std::vector<FileMapping*>::const_iterator delLeftIter = deleteLeft.begin();
         delLeftIter != deleteLeft.end();
         ++delLeftIter)
    {
        const FindDBAssoc::AssocKey assocDbKey((*delLeftIter)->getFileID<LEFT_SIDE>(),
                                               (*delLeftIter)->getLastWriteTime<LEFT_SIDE>(),
                                               (*delLeftIter)->getFileSize<LEFT_SIDE>());

        const std::map<FindDBAssoc::AssocKey, Utility::FileID>::const_iterator assocDBIter =
            assocDBLeftToRight.find(assocDbKey);

        if (assocDBIter != assocDBLeftToRight.end())
        {
            std::map<FindDBAssoc::AssocKey, FileMapping*>::const_iterator createLeftIter =
                createLeft.find(FindDBAssoc::AssocKey(assocDBIter->second, //FileID of right side
                                                      assocDbKey.lastWriteTimeRaw_,
                                                      assocDbKey.fileSize_));

            if (createLeftIter != createLeft.end())
            {
                renameOnLeft.push_back(std::make_pair(createLeftIter->second, *delLeftIter));
            }
        }
    }
}


void FreeFileSync::getRenameCandidates(FreeFileSync::BaseDirMapping& baseMapping,                            //in
                                       std::vector<std::pair<CreateOnLeft, DeleteOnLeft> >& renameOnLeft,    //out
                                       std::vector<std::pair<CreateOnRight, DeleteOnRight> >& renameOnRight) //out        throw()!
{
    FindRenameCandidates(baseMapping).getRenameCandidates(renameOnLeft, renameOnRight);
}

