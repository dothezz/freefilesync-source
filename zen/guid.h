// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef GUID_H_80425780237502345
#define GUID_H_80425780237502345

#include <string>
#include <boost/uuid/uuid.hpp>

    #pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <boost/uuid/uuid_generators.hpp>

    #pragma GCC diagnostic pop


namespace zen
{
inline
std::string generateGUID() //creates a 16 byte GUID
{
    boost::uuids::uuid nativeRep = boost::uuids::random_generator()();
    //generator is only thread-safe like an int, so we keep it local until we need to optimize perf
    //perf: generator: 0.22ms per call; retrieve GUID: 0.12µs per call
    return std::string(nativeRep.begin(), nativeRep.end());
}
}

#endif //GUID_H_80425780237502345
