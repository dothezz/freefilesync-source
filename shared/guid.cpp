// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "guid.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <cassert>
#include <algorithm> 
#include <vector> 

using namespace Utility;
 
   
struct UniqueId::IntData
{  
    boost::uuids::uuid nativeRep;  
};
  

UniqueId::UniqueId() : pData(new IntData)
{
    pData->nativeRep = boost::uuids::random_generator()();
}


bool UniqueId::operator==(const UniqueId rhs) const
{
    return pData->nativeRep == rhs.pData->nativeRep;
}


bool UniqueId::operator<(const UniqueId rhs) const
{
    return pData->nativeRep < rhs.pData->nativeRep;
} 


UniqueId::UniqueId(wxInputStream& stream) : //read
    pData(new IntData)
{
    std::vector<char> rawData(boost::uuids::uuid::static_size());
    stream.Read(&rawData[0], rawData.size());

    std::copy(rawData.begin(), rawData.end(), pData->nativeRep.begin());
}


void UniqueId::toStream(wxOutputStream& stream) const //write
{
    std::vector<char> rawData;
    std::copy(pData->nativeRep.begin(), pData->nativeRep.end(), std::back_inserter(rawData));

    assert(boost::uuids::uuid::static_size() == rawData.size());

    stream.Write(&rawData[0], rawData.size());
}

