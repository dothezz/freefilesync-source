// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef GUID_H_INCLUDED
#define GUID_H_INCLUDED

#include <wx/stream.h>
#include <boost/shared_ptr.hpp>

namespace util
{
class UniqueId
{
public:
    UniqueId(); //create UUID

    UniqueId(wxInputStream& stream);  //read
    void toStream(wxOutputStream& stream) const; //write

    bool operator==(const UniqueId rhs) const;
    bool operator<(const UniqueId rhs) const;

private:
    struct IntData;
    boost::shared_ptr<IntData> pData;
};
}


#endif // GUID_H_INCLUDED
