// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef FILEID_H_INCLUDED_32q87634289562345
#define FILEID_H_INCLUDED_32q87634289562345

#include "file_id_def.h"
#include "zstring.h"

//unique file identifier

namespace zen
{
//get unique file id (symbolic link handling: opens the link!!!)
//returns initial FileId() on error!
FileId getFileID(const Zstring& filename);
}

#endif //FILEID_H_INCLUDED_32q87634289562345
