// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#include <wx+/grid.h>

namespace zen
{
void startFind(wxWindow* parent, Grid& grid, size_t compPosLeft, size_t compPosRight, bool& respectCase); //Strg + F
void findNext( wxWindow* parent, Grid& grid, size_t compPosLeft, size_t compPosRight, bool& respectCase); //F3
}

#endif // SEARCH_H_INCLUDED
