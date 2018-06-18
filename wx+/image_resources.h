// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef RESOURCES_H_8740257825342532457
#define RESOURCES_H_8740257825342532457

#include <wx/bitmap.h>
#include <wx/animate.h>
#include <zen/zstring.h>

namespace zen
{
void initResourceImages(const Zstring& filename); //pass resources .zip file at application startup

const wxBitmap&    getResourceImage    (const wxString& name);
const wxAnimation& getResourceAnimation(const wxString& name);
}

#endif //RESOURCES_H_8740257825342532457
