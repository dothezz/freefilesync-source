#ifndef GUID_H_INCLUDED
#define GUID_H_INCLUDED

#include <wx/stream.h>
#include <boost/shared_ptr.hpp>

namespace Utility
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
