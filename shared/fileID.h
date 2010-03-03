// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef FILEID_H_INCLUDED
#define FILEID_H_INCLUDED

#include <wx/stream.h>
#include "zstring.h"

#ifdef FFS_WIN
#elif defined FFS_LINUX
#include <sys/stat.h>
#endif


//unique file identifier
namespace Utility
{
class FileID
{
public:
    //standard copy constructor and assignment operator are okay!

    FileID(wxInputStream& stream);  //read
    void toStream(wxOutputStream& stream) const; //write

    bool isNull() const;
    bool operator==(const FileID& rhs) const;
    bool operator!=(const FileID& rhs) const;
    bool operator<(const FileID& rhs) const;

    FileID();
#ifdef FFS_WIN
    typedef unsigned long DWORD_FFS; //we don't want do include "windows.h" or "<wx/msw/wrapwin.h>" here, do we?

    FileID(DWORD_FFS dwVolumeSN,
           DWORD_FFS fileIndexHi,
           DWORD_FFS fileIndexLo);
#elif defined FFS_LINUX
    FileID(dev_t devId,
           ino_t inId);
#endif
private:
#ifdef FFS_WIN
    DWORD_FFS dwVolumeSerialNumber;
    DWORD_FFS nFileIndexHigh;
    DWORD_FFS nFileIndexLow;
#elif defined FFS_LINUX
    dev_t deviceId;
    ino_t inodeId;
#endif
};

//get unique file id (symbolic link handling: opens the link!!!)
//error condition: returns FileID ()
FileID retrieveFileID(const Zstring& filename);

//test whether two distinct paths point to the same file or directory:
//      true: paths point to same files/dirs
//      false: error occured OR point to different files/dirs
bool sameFileSpecified(const Zstring& file1, const Zstring& file2);
}



















//---------------Inline Implementation---------------------------------------------------
#ifdef FFS_WIN
inline
Utility::FileID::FileID() :
    dwVolumeSerialNumber(0),
    nFileIndexHigh(0),
    nFileIndexLow(0) {}

inline
Utility::FileID::FileID(DWORD_FFS dwVolumeSN,
                        DWORD_FFS fileIndexHi,
                        DWORD_FFS fileIndexLo) :
    dwVolumeSerialNumber(dwVolumeSN),
    nFileIndexHigh(fileIndexHi),
    nFileIndexLow(fileIndexLo) {}

inline
bool Utility::FileID::isNull() const
{
    return dwVolumeSerialNumber == 0 &&
           nFileIndexHigh       == 0 &&
           nFileIndexLow        == 0;
}

inline
bool Utility::FileID::operator==(const FileID& rhs) const
{
    return dwVolumeSerialNumber == rhs.dwVolumeSerialNumber &&
           nFileIndexHigh       == rhs.nFileIndexHigh       &&
           nFileIndexLow        == rhs.nFileIndexLow;
}

inline
bool Utility::FileID::operator<(const FileID& rhs) const
{
    if (dwVolumeSerialNumber != rhs.dwVolumeSerialNumber)
        return dwVolumeSerialNumber < rhs.dwVolumeSerialNumber;

    if (nFileIndexHigh != rhs.nFileIndexHigh)
        return nFileIndexHigh < rhs.nFileIndexHigh;

    return nFileIndexLow < rhs.nFileIndexLow;
}

inline
Utility::FileID::FileID(wxInputStream& stream)  //read
{
    stream.Read(&dwVolumeSerialNumber, sizeof(dwVolumeSerialNumber));
    stream.Read(&nFileIndexHigh,       sizeof(nFileIndexHigh));
    stream.Read(&nFileIndexLow,        sizeof(nFileIndexLow));
}

inline
void Utility::FileID::toStream(wxOutputStream& stream) const //write
{
    stream.Write(&dwVolumeSerialNumber, sizeof(dwVolumeSerialNumber));
    stream.Write(&nFileIndexHigh,       sizeof(nFileIndexHigh));
    stream.Write(&nFileIndexLow,        sizeof(nFileIndexLow));
}

#elif defined FFS_LINUX
inline
Utility::FileID::FileID() :
    deviceId(0),
    inodeId(0) {}

inline
Utility::FileID::FileID(dev_t devId,
                        ino_t inId) :
    deviceId(devId),
    inodeId(inId) {}

inline
bool Utility::FileID::isNull() const
{
    return deviceId == 0 &&
           inodeId  == 0;
}

inline
bool Utility::FileID::operator==(const FileID& rhs) const
{
    return deviceId == rhs.deviceId &&
           inodeId  == rhs.inodeId;
}

inline
bool Utility::FileID::operator<(const FileID& rhs) const
{
    if (deviceId != rhs.deviceId)
        return deviceId < rhs.deviceId;

    return inodeId < rhs.inodeId;
}

inline
Utility::FileID::FileID(wxInputStream& stream)  //read
{
    stream.Read(&deviceId, sizeof(deviceId));
    stream.Read(&inodeId,  sizeof(inodeId));
}

inline
void Utility::FileID::toStream(wxOutputStream& stream) const //write
{
    stream.Write(&deviceId, sizeof(deviceId));
    stream.Write(&inodeId,  sizeof(inodeId));
}
#endif
inline
bool Utility::FileID::operator!=(const FileID& rhs) const
{
    return !(*this == rhs);
}

#endif // FILEID_H_INCLUDED
