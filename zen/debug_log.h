// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef DEBUG_LOG_HEADER_017324601673246392184621895740256342
#define DEBUG_LOG_HEADER_017324601673246392184621895740256342

#include <string>
#include <cstdio>
#include <memory>
#include "deprecate.h"
#include "string_tools.h"
#include "time.h"


//small macro for writing debug information into a logfile
#define WRITE_LOG(x) globalLogFile().write(__FILE__, __LINE__, x);

//speed alternative: wxLogDebug(wxT("text")) + DebugView


namespace zen
{
#ifdef FFS_WIN
const char ZEN_FILE_NAME_SEPARATOR = '\\';

#elif defined FFS_LINUX
const char ZEN_FILE_NAME_SEPARATOR = '/';

#else
#error specify platform!
#endif


class DebugLog
{
public:
    class LogError {};

    ZEN_DEPRECATE
    DebugLog(const std::string& filePrefix = std::string()) :
        filename(filePrefix + "DEBUG_" + formatTime<std::string>("%Y-%m-%d %H%M%S") + ".log"),
        rowCount(0),
        handle(std::fopen(filename.c_str(), "w")) //Windows: non binary mode: automatically convert "\n" to "\r\n"; Linux: binary is default!
    {
        if (!handle)
            throw LogError();
    }

    ~DebugLog() { std::fclose(handle); }

    void write(const std::string& sourceFile,
               int sourceRow,
               const std::string& message)
    {
        const std::string logEntry = "[" + formatTime<std::string>(FORMAT_TIME) + "] " + afterLast(sourceFile, ZEN_FILE_NAME_SEPARATOR) +
                                     " (" + numberTo<std::string>(sourceRow) + "): " + message + "\n";

        const size_t bytesWritten = ::fwrite(logEntry.c_str(), 1, logEntry.size(), handle);
        if (std::ferror(handle) != 0 || bytesWritten != logEntry.size())
            throw LogError();

        if (std::fflush(handle) != 0)
            throw LogError();

        ++rowCount;
    }

    size_t getRows() const { return rowCount; }

    std::string getFileName() const { return filename; }

private:
    std::string filename;
    size_t rowCount;
    FILE* handle;
};


inline
DebugLog& globalLogFile()
{
    static std::unique_ptr<DebugLog> inst(new DebugLog); //external linkage despite header definition!

    if (inst->getRows() > 50000) //prevent logfile from becoming too big
    {
        const std::string oldName = inst->getFileName();
        inst.reset();
        std::remove(oldName.c_str()); //unchecked deletion!
        inst.reset(new DebugLog);
    }

    return *inst;
}
}

#endif //DEBUG_LOG_HEADER_017324601673246392184621895740256342
