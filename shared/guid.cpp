// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "guid.h"
#include <boost/uuid/uuid.hpp>

//boost really should clean a bit up...
#ifdef __MINGW32__
#pragma GCC diagnostic ignored "-Wshadow"
#endif

#include <boost/uuid/uuid_generators.hpp>


std::string util::generateGUID() //creates a 16 byte GUID
{
    boost::uuids::uuid nativeRep = boost::uuids::random_generator()();
    return std::string(nativeRep.begin(), nativeRep.end());
}

