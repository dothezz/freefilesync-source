// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef CRC_H_23489275827847235
#define CRC_H_23489275827847235

#include <boost/crc.hpp>


namespace zen
{
template <class ByteIterator> inline
uint16_t getCrc16(ByteIterator first, ByteIterator last)
{
    static_assert(sizeof(typename std::iterator_traits<ByteIterator>::value_type) == 1, "");
    boost::crc_16_type result;
    if (first != last)
        result.process_bytes(&*first, last - first);
    auto rv = result.checksum();
    static_assert(sizeof(rv) == sizeof(uint16_t), "");
    return rv;
}


template <class ByteIterator> inline
uint32_t getCrc32(ByteIterator first, ByteIterator last)
{
    static_assert(sizeof(typename std::iterator_traits<ByteIterator>::value_type) == 1, "");
    boost::crc_32_type result;
    if (first != last)
		result.process_bytes(&*first, last - first);
    auto rv = result.checksum();
    static_assert(sizeof(rv) == sizeof(uint32_t), "");
    return rv;
}
}

#endif //CRC_H_23489275827847235
