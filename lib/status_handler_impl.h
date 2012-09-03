// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef STATUSHANDLER_IMPL_H_INCLUDED
#define STATUSHANDLER_IMPL_H_INCLUDED

#include <zen/file_error.h>
#include "process_callback.h"

template <typename Function> inline
bool tryReportingError(Function cmd, ProcessCallback& handler) //return "true" on success, "false" if error was ignored
{
    for (;;)
        try
        {
            cmd(); //throw FileError
            return true;
        }
        catch (zen::FileError& error)
        {
            switch (handler.reportError(error.toString())) //may throw!
            {
                case ProcessCallback::IGNORE_ERROR:
                    return false;
                case ProcessCallback::RETRY:
                    break; //continue with loop
            }
        }
}

#endif //STATUSHANDLER_IMPL_H_INCLUDED
