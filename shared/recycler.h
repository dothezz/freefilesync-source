#ifndef RECYCLER_H_INCLUDED
#define RECYCLER_H_INCLUDED

#include "fileError.h"
#include "zstring.h"
#include <vector>

#ifndef FFS_WIN
use in windows build only!
#endif


namespace FreeFileSync
{
//single-file processing
void moveToWindowsRecycler(const Zstring& fileToDelete);  //throw (FileError)
//multi-file processing: about a factor of 15 faster than single-file
void moveToWindowsRecycler(const std::vector<Zstring>& filesToDelete);  //throw (FileError) -> on error reports about first file only!
}

#endif // RECYCLER_H_INCLUDED
