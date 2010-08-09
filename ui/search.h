// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

class wxGrid;
class wxWindow;


namespace ffs3
{
void startFind(wxWindow& parentWindow, wxGrid& leftGrid, wxGrid& rightGrid, bool& respectCase); //Strg + F
void findNext( wxWindow& parentWindow, wxGrid& leftGrid, wxGrid& rightGrid, bool& respectCase); //F3
}

#endif // SEARCH_H_INCLUDED
