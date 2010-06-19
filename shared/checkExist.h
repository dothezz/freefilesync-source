#ifndef CHECKEXIST_H_INCLUDED
#define CHECKEXIST_H_INCLUDED

#include "zstring.h"

namespace Utility
{
enum ResultExist
{
    EXISTING_TRUE,
    EXISTING_FALSE,
    EXISTING_TIMEOUT
};

ResultExist fileExists(const Zstring& filename, size_t timeout); //timeout in ms
ResultExist dirExists( const Zstring& dirname,  size_t timeout); //timeout in ms
}

#endif // CHECKEXIST_H_INCLUDED
