// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "guid.h"
#include <stdexcept>

#ifdef FFS_WIN
#include "Objbase.h"

#elif defined FFS_LINUX
#include "ossp_uuid/uuid++.hh"
#endif

using namespace Utility;


#ifdef FFS_WIN
struct UniqueId::IntData
{
    GUID nativeRep;
};
#elif defined FFS_LINUX
struct UniqueId::IntData
{
    uuid nativeRep; //ossp uuid
};
#endif


UniqueId::UniqueId() :
    pData(new IntData)
{
#ifdef FFS_WIN
    if (FAILED(::CoCreateGuid(&pData->nativeRep)))
        throw std::runtime_error("Error creating UUID!");
#elif defined FFS_LINUX
    pData->nativeRep.make(UUID_MAKE_V1);
#endif
}


bool UniqueId::operator==(const UniqueId rhs) const
{
#ifdef FFS_WIN
    //return ::IsEqualGUID(pData->nativeRep, rhs.pData->nativeRep); -> harmonize with operator<

    const GUID& guidL = pData->nativeRep;
    const GUID& guidR = rhs.pData->nativeRep;

    return guidL.Data1 == guidR.Data1 &&
           guidL.Data2 == guidR.Data2 &&
           guidL.Data3 == guidR.Data3 &&
           ::memcmp(guidL.Data4, guidR.Data4, sizeof(guidR.Data4)) == 0;
#elif defined FFS_LINUX
    return pData->nativeRep == rhs.pData->nativeRep;
#endif
}


bool UniqueId::operator<(const UniqueId rhs) const
{
#ifdef FFS_WIN
    const GUID& guidL = pData->nativeRep;
    const GUID& guidR = rhs.pData->nativeRep;

    if (guidL.Data1 != guidR.Data1)
        return guidL.Data1 < guidR.Data1;
    if (guidL.Data2 != guidR.Data2)
        return guidL.Data2 < guidR.Data2;
    if (guidL.Data3 != guidR.Data3)
        return guidL.Data3 < guidR.Data3;

    return ::memcmp(guidL.Data4, guidR.Data4, sizeof(guidR.Data4)) < 0;
#elif defined FFS_LINUX
    return pData->nativeRep < rhs.pData->nativeRep;
#endif
}


UniqueId::UniqueId(wxInputStream& stream) : //read
    pData(new IntData)
{
#ifdef FFS_WIN
    stream.Read(&pData->nativeRep, sizeof(GUID));
#elif defined FFS_LINUX
    char buffer[UUID_LEN_BIN] = {0};
    stream.Read(buffer, sizeof(buffer));

    pData->nativeRep.import(static_cast<void*>(buffer)); //warning: import is overloaded with void*/char*!
#endif
}


#ifdef FFS_LINUX
struct MallocDeleter
{
    void operator() (void* ptr)
    {
        free(ptr);
    }
};
#endif


void UniqueId::toStream(wxOutputStream& stream) const //write
{
#ifdef FFS_WIN
    stream.Write(&pData->nativeRep, sizeof(GUID));
#elif defined FFS_LINUX
    boost::shared_ptr<const void> buffer(pData->nativeRep.binary(), //caller has to "free" memory (of size UUID_LEN_BIN)
                                         MallocDeleter());

    stream.Write(buffer.get(), UUID_LEN_BIN);
#endif
}


