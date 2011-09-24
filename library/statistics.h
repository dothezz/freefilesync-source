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

class wxStopWatch;


class RetrieveStatistics
{
public:
    wxDEPRECATED( RetrieveStatistics() ); //generate compiler warnings as a reminder to remove code after measurements
    ~RetrieveStatistics();

    void writeEntry(const double value, const int objects);

private:
    struct StatEntry
    {
        long time;
        int objects;
        double value;
    };

    std::vector<StatEntry> data;
    std::unique_ptr<wxStopWatch> timer;
};


class Statistics
{
public:
    Statistics(int totalObjectCount,
               double totalDataAmount,
               unsigned windowSizeRemainingTime,   //time in ms
               unsigned windowSizeBytesPerSecond); //time in ms

    ~Statistics();

    void addMeasurement(int objectsCurrent, double dataCurrent);
    wxString getRemainingTime() const; //returns the remaining time in milliseconds
    wxString getBytesPerSecond() const;

    void pauseTimer();
    void resumeTimer();

private:
    wxString formatRemainingTime(double timeInMs) const;

    const int      objectsTotal;
    const double   dataTotal;

    const unsigned windowSizeRemTime; //"window width" of statistics used for calculation of remaining time in ms
    const unsigned windowSizeBPS;     //
    const unsigned windowMax;

    mutable int remainingTimeLast; //used for "smoothening" remaining time

    struct Record
    {
        int objects; //object count
        double data; //unit: bytes
    };

    typedef std::multimap<long, Record> TimeRecordMap; //time, unit: milliseconds
    TimeRecordMap measurements; //

    wxStopWatch* timer;
};

#endif // STATISTICS_H_INCLUDED
