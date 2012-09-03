// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef GUID_H_INCLUDED
#define GUID_H_INCLUDED

#include <string>
#include <boost/uuid/uuid.hpp>

#ifdef __MINGW32__  //boost should start cleaning this mess up
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

#include <boost/uuid/uuid_generators.hpp>

#ifdef __MINGW32__
#pragma GCC diagnostic pop
#endif


namespace zen
{
inline
std::string generateGUID() //creates a 16 byte GUID
{
    boost::uuids::uuid nativeRep = boost::uuids::random_generator()(); //generator is thread-safe like an int
    //perf: generator: 0.22ms per call; retrieve GUID: 0.12µs per call
    return std::string(nativeRep.begin(), nativeRep.end());
}
}

#endif // GUID_H_INCLUDED
