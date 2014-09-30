// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "osx_dock.h"
#include <Cocoa/Cocoa.h>
#include <zen/osx_throw_exception.h>


void osx::dockIconSetText(const char* str) //throw SysError
{
    @try
    {
		NSString* label = [NSString stringWithCString:str encoding:NSUTF8StringEncoding]; 
		//stringWithCString returns string which is already set to autorelease!
		[[NSApp dockTile] setBadgeLabel:label]; //label may be nil
    }
    @catch (NSException* e)
    {
		throwSysError(e); //throw SysError
    }
}
