// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef STATISTICS_H_INCLUDED
#define STATISTICS_H_INCLUDED

#include <vector>
#include <map>
#include <memory>
#include <wx/defs.h>
#include <wx/string.h>
#include <wx/stopwatch.h>
#include <zen/deprecate.h>

class RetrieveStatistics
{
public:
    ZEN_DEPRECATE ~RetrieveStatistics(); //remove code after measurements!
    void writeEntry(double value, int objects);

private:
    struct StatEntry
    {
        long time;
        int objects;
        double value;
    };
    std::vector<StatEntry> data;
    wxStopWatch timer;
};


class Statistics
{
public:
    Statistics(int totalObjectCount,
               double totalDataAmount,
               unsigned windowSizeRemainingTime,   //time in ms
               unsigned windowSizeBytesPerSecond); //time in ms

    void addMeasurement(int objectsCurrent, double dataCurrent);
    wxString getRemainingTime() const; //returns the remaining time in milliseconds
    wxString getBytesPerSecond() const;

    void pauseTimer();
    void resumeTimer();

private:
    const int      objectsTotal;
    const double   dataTotal;

    const unsigned windowSizeRemTime; //"window width" of statistics used for calculation of remaining time in ms
    const unsigned windowSizeBPS;     //
    const unsigned windowMax;

    struct Record
    {
        int objects; //object count
        double data; //unit: bytes
    };

    typedef std::multimap<long, Record> TimeRecordMap; //time, unit: milliseconds
    TimeRecordMap measurements; //

    wxStopWatch timer;
};

#endif // STATISTICS_H_INCLUDED
