// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "binary.h"
#include <boost/scoped_array.hpp>
#include "../shared/fileIO.h"


bool FreeFileSync::filesHaveSameContent(const Zstring& filename1, const Zstring& filename2, CompareCallback& callback)
{
    const size_t BUFFER_SIZE = 512 * 1024; //512 kb seems to be the perfect buffer size
    static boost::scoped_array<unsigned char> memory1(new unsigned char[BUFFER_SIZE]);
    static boost::scoped_array<unsigned char> memory2(new unsigned char[BUFFER_SIZE]);

    FileInput file1(filename1); //throw FileError()
    FileInput file2(filename2); //throw FileError()

    wxLongLong bytesCompared;
    do
    {
        const size_t length1 = file1.read(memory1.get(), BUFFER_SIZE); //returns actual number of bytes read; throw FileError()
        const size_t length2 = file2.read(memory2.get(), BUFFER_SIZE); //

        if (length1 != length2 || ::memcmp(memory1.get(), memory2.get(), length1) != 0)
            return false;

        bytesCompared += length1 * 2;

        //send progress updates
        callback.updateCompareStatus(bytesCompared);
    }
    while (!file1.eof());

    if (!file1.eof()) //highly unlikely, but theoretically possible! (but then again, not in this context where both files have same size...)
        return false;

    return true;
}
