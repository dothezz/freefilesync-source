// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef STATUSHANDLER_IMPL_H_INCLUDED
#define STATUSHANDLER_IMPL_H_INCLUDED

#include <zen/optional.h>
#include <zen/file_error.h>
#include "process_callback.h"


template <typename Function> inline
zen::Opt<std::wstring> tryReportingError(Function cmd, ProcessCallback& handler) //return ignored error message if available
{
    for (size_t retryNumber = 0;; ++retryNumber)
        try
        {
            cmd(); //throw FileError
            return zen::NoValue();
        }
        catch (zen::FileError& error)
        {
            switch (handler.reportError(error.toString(), retryNumber)) //may throw!
            {
                case ProcessCallback::IGNORE_ERROR:
                    return error.toString();
                case ProcessCallback::RETRY:
                    break; //continue with loop
            }
        }
}


#endif //STATUSHANDLER_IMPL_H_INCLUDED
