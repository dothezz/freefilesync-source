// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef GEN_LOGFILE_H_93172643216748973216458732165415
#define GEN_LOGFILE_H_93172643216748973216458732165415

#include <zen/error_log.h>
#include <zen/file_io.h>
#include <zen/serialize.h>
#include <zen/format_unit.h>
#include "ffs_paths.h"


namespace zen
{
struct SummaryInfo
{
    std::wstring jobName; //may be empty
    std::wstring finalStatus;
    int itemsSynced;
    std::int64_t dataSynced;
    int itemsTotal;
    std::int64_t dataTotal;
    int64_t totalTime; //unit: [sec]
};

void saveLogToFile(const SummaryInfo& summary, //throw FileError
                   const ErrorLog& log,
                   FileOutput& fileOut);

void saveToLastSyncsLog(const SummaryInfo& summary,  //throw FileError
                        const ErrorLog& log,
                        size_t maxBytesToWrite);





//####################### implementation #######################
namespace
{
std::wstring generateLogHeader(const SummaryInfo& s)
{
    assert(s.itemsSynced <= s.itemsTotal);
    assert(s.dataSynced  <= s.dataTotal);

    std::wstring output;

    //write header
    std::wstring headerLine = formatTime<std::wstring>(FORMAT_DATE);
    if (!s.jobName.empty())
        headerLine += L" - " + s.jobName;
    headerLine += L": " + s.finalStatus;

    //assemble results box
    std::vector<std::wstring> results;
    results.push_back(headerLine);
    results.push_back(L"");

    const wchar_t tabSpace[] = L"    ";

    std::wstring itemsProc = tabSpace + _("Items processed:") + L" " + toGuiString(s.itemsSynced); //show always, even if 0!
    if (s.itemsSynced != 0 || s.dataSynced != 0) //[!] don't show 0 bytes processed if 0 items were processed
        itemsProc += + L" (" + filesizeToShortString(s.dataSynced) + L")";
    results.push_back(itemsProc);

    if (s.itemsTotal != 0 || s.dataTotal != 0) //=: sync phase was reached and there were actual items to sync
    {
        if (s.itemsSynced != s.itemsTotal ||
            s.dataSynced  != s.dataTotal)
            results.push_back(tabSpace + _("Items remaining:") + L" " + toGuiString(s.itemsTotal - s.itemsSynced) + L" (" + filesizeToShortString(s.dataTotal - s.dataSynced) + L")");
    }

    results.push_back(tabSpace + _("Total time:") + L" " + copyStringTo<std::wstring>(wxTimeSpan::Seconds(s.totalTime).Format()));

    //calculate max width, this considers UTF-16 only, not true Unicode...but maybe good idea? those 2-char-UTF16 codes are usually wider than fixed width chars anyway!
    size_t sepLineLen = 0;
    std::for_each(results.begin(), results.end(), [&](const std::wstring& str) { sepLineLen = std::max(sepLineLen, str.size()); });

    output.resize(output.size() + sepLineLen + 1, L'_');
    output += L'\n';

    std::for_each(results.begin(), results.end(), [&](const std::wstring& str) { output += L'|'; output += str; output += L'\n'; });

    output += L'|';
    output.resize(output.size() + sepLineLen, L'_');
    output += L'\n';

    return output;
}
}


inline
void saveLogToFile(const SummaryInfo& summary, //throw FileError
                   const ErrorLog& log,
                   FileOutput& fileOut)
{
    Utf8String header = utfCvrtTo<Utf8String>(generateLogHeader(summary));
    replace(header, '\n', LINE_BREAK); //don't replace line break any earlier
    header += LINE_BREAK; //make sure string is not empty!

    fileOut.write(&*header.begin(), header.size()); //throw FileError

    //write log items one after the other instead of creating one big string: memory allocation might fail; think 1 million entries!
    for (const LogEntry& entry : log)
    {
        Utf8String msg = replaceCpy(utfCvrtTo<Utf8String>(formatMessage<std::wstring>(entry)), '\n', LINE_BREAK);
        msg += LINE_BREAK; //make sure string is not empty!

        fileOut.write(&*msg.begin(), msg.size()); //throw FileError
    }
}


inline
void saveToLastSyncsLog(const SummaryInfo& summary,  //throw FileError
                        const ErrorLog& log,
                        size_t maxBytesToWrite) //log may be *huge*, e.g. 1 million items; LastSyncs.log *must not* create performance problems!
{
    const Zstring filepath = getConfigDir() + Zstr("LastSyncs.log");

    Utf8String newStream = utfCvrtTo<Utf8String>(generateLogHeader(summary));
    replace(newStream, '\n', LINE_BREAK); //don't replace line break any earlier
    newStream += LINE_BREAK;

    //check size of "newStream": memory allocation might fail - think 1 million entries!
    for (const LogEntry& entry : log)
    {
        newStream += replaceCpy(utfCvrtTo<Utf8String>(formatMessage<std::wstring>(entry)), '\n', LINE_BREAK);
        newStream += LINE_BREAK;

        if (newStream.size() > maxBytesToWrite)
        {
            newStream += "[...]";
            newStream += LINE_BREAK;
            break;
        }
    }

    //fill up the rest of permitted space by appending old log
    if (newStream.size() < maxBytesToWrite)
    {
        Utf8String oldStream;
        try
        {
            oldStream = loadBinStream<Utf8String>(filepath); //throw FileError
        }
        catch (FileError&) {}

        if (!oldStream.empty())
        {
            newStream += LINE_BREAK;
            newStream += LINE_BREAK;
            newStream += oldStream; //impliticly limited by "maxBytesToWrite"!

            //truncate size if required
            if (newStream.size() > maxBytesToWrite)
            {
                //but do not cut in the middle of a row
                auto iter = std::search(newStream.cbegin() + maxBytesToWrite, newStream.cend(), std::begin(LINE_BREAK), std::end(LINE_BREAK) - 1);
                if (iter != newStream.cend())
                {
                    newStream.resize(iter - newStream.cbegin());
                    newStream += LINE_BREAK;

                    newStream += "[...]";
                    newStream += LINE_BREAK;
                }
            }
        }
    }

    saveBinStream(filepath, newStream); //throw FileError
}
}

#endif //GEN_LOGFILE_H_93172643216748973216458732165415
