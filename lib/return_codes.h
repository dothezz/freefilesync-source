// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef RETURN_CODES_H_INCLUDED
#define RETURN_CODES_H_INCLUDED

namespace zen
{
enum FfsReturnCode
{
    FFS_RC_SUCCESS = 0,
    FFS_RC_FINISHED_WITH_ERRORS,
    FFS_RC_ABORTED,
    FFS_RC_EXCEPTION,
};
}


#endif // RETURN_CODES_H_INCLUDED
