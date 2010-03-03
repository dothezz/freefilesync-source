// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef DETECTRENAMING_H_INCLUDED
#define DETECTRENAMING_H_INCLUDED

#include "../fileHierarchy.h"


//identify a file "create and delete"-operation as a file renaming!

namespace FreeFileSync
{
typedef FileMapping* CreateOnLeft;
typedef FileMapping* DeleteOnLeft;
typedef FileMapping* CreateOnRight;
typedef FileMapping* DeleteOnRight;
void getRenameCandidates(FreeFileSync::BaseDirMapping& baseMapping,                             //in
                         std::vector<std::pair<CreateOnLeft, DeleteOnLeft> >&   renameOnLeft,   //out
                         std::vector<std::pair<CreateOnRight, DeleteOnRight> >& renameOnRight); //out        throw()!
}

#endif // DETECTRENAMING_H_INCLUDED
