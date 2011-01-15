// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef XMLFREEFILESYNC_H_INCLUDED
#define XMLFREEFILESYNC_H_INCLUDED

#include "xml_proc.h"


//reuse (some of) FreeFileSync's xml files

namespace rts
{
void readRealOrBatchConfig(const wxString& filename, xmlAccess::XmlRealConfig& config);  //throw (xmlAccess::XmlError);

int getProgramLanguage();
}

#endif // XMLFREEFILESYNC_H_INCLUDED
