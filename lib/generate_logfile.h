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
Utf8String generateLogStream(const ErrorLog& log,
                             const std::wstring& jobName, //may be empty
                             const std::wstring& finalStatus,
                             int itemsSynced, Int64 dataSynced,
                             int itemsTotal,  Int64 dataTotal,
                             long totalTime); //unit: [sec]

void saveToLastSyncsLog(const Utf8String& logstream); //throw FileError







//####################### implementation #######################
namespace
{
Utf8String generateLogStream_impl(const ErrorLog& log,
                                  const std::wstring& jobName, //may be empty
                                  const std::wstring& finalStatus,
                                  int itemsSynced, Int64 dataSynced,
                                  int itemsTotal,  Int64 dataTotal,
                                  long totalTime) //unit: [sec]
{
    assert(itemsSynced <= itemsTotal);
    assert(dataSynced  <= dataTotal);

    Utf8String output;

    //write header
    std::wstring headerLine = formatTime<std::wstring>(FORMAT_DATE);
    if (!jobName.empty())
        headerLine += L" - " + jobName;
    headerLine += L": " + finalStatus;

    //assemble results box
    std::vector<std::wstring> results;
    results.push_back(headerLine);
    results.push_back(L"");

	const wchar_t tabSpace[] = L"    ";

    std::wstring itemsProc = tabSpace + _("Items processed:") + L" " + toGuiString(itemsSynced); //show always, even if 0!
    if (itemsSynced != 0 || dataSynced != 0) //[!] don't show 0 bytes processed if 0 items were processed
        itemsProc += + L" (" + filesizeToShortString(dataSynced) + L")";
    results.push_back(itemsProc);

    if (itemsTotal != 0 || dataTotal != 0) //=: sync phase was reached and there were actual items to sync
    {
        if (itemsSynced != itemsTotal ||
            dataSynced  != dataTotal)
            results.push_back(tabSpace + _("Items remaining:") + L" " + toGuiString(itemsTotal - itemsSynced) + L" (" + filesizeToShortString(dataTotal - dataSynced) + L")");
    }

    results.push_back(tabSpace + _("Total time:") + L" " + copyStringTo<std::wstring>(wxTimeSpan::Seconds(totalTime).Format()));

    //calculate max width, this considers UTF-16 only, not true Unicode...
    size_t sepLineLen = 0;
    std::for_each(results.begin(), results.end(), [&](const std::wstring& str) { sepLineLen = std::max(sepLineLen, str.size()); });

    for (size_t i = 0; i < sepLineLen; ++i) output += '_'; //this considers UTF-16 only, not true Unicode!!!
    output += "\n";

    std::for_each(results.begin(), results.end(), [&](const std::wstring& str) { output += utfCvrtTo<Utf8String>(str); output += '\n'; });

    for (size_t i = 0; i < sepLineLen; ++i) output += '_';
    output += "\n\n";

    //write log items
    const auto& entries = log.getEntries();
    for (auto iter = entries.begin(); iter != entries.end(); ++iter)
    {
        output += utfCvrtTo<Utf8String>(formatMessage(*iter));
        output += '\n';
    }

    return replaceCpy(output, '\n', LINE_BREAK); //don't replace line break any earlier
}
}


inline
Utf8String generateLogStream(const ErrorLog& log,
                             const std::wstring& jobName, //may be empty
                             const std::wstring& finalStatus,
                             int itemsSynced, Int64 dataSynced,
                             int itemsTotal,  Int64 dataTotal,
                             long totalTime) //unit: [sec]
{
    return generateLogStream_impl(log, jobName, finalStatus, itemsSynced, dataSynced, itemsTotal, dataTotal, totalTime);
}


inline
void saveToLastSyncsLog(const Utf8String& logstream) //throw FileError
{
    const Zstring filename = getConfigDir() + Zstr("LastSyncs.log");

    Utf8String oldStream;
    try
    {
        oldStream = loadBinStream<Utf8String>(filename); //throw FileError, ErrorNotExisting
    }
    catch (const ErrorNotExisting&) {}

    Utf8String newStream = logstream;
    if (!oldStream.empty())
    {
        newStream += LINE_BREAK;
        newStream += LINE_BREAK;
        newStream += oldStream;
    }

    //limit file size: 128 kB (but do not truncate new log)
    const size_t newSize = std::min(newStream.size(), std::max<size_t>(logstream.size(), 128 * 1024));

    //do not cut in the middle of a row
    auto iter = std::search(newStream.cbegin() + newSize, newStream.cend(), std::begin(LINE_BREAK), std::end(LINE_BREAK) - 1);
    if (iter != newStream.cend())
    {
        newStream.resize(iter - newStream.cbegin());

        newStream += LINE_BREAK;
        newStream += "[...]";
        newStream += LINE_BREAK;
    }

    saveBinStream(filename, newStream); //throw FileError
}
}

#endif //GEN_LOGFILE_H_93172643216748973216458732165415
