// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "statistics.h"

#include <limits>
#include <wx/ffile.h>
#include <wx/stopwatch.h>
#include <zen/basic_math.h>
#include <zen/assert_static.h>
#include <zen/i18n.h>
#include <wx+/format_unit.h>


using namespace zen;


RetrieveStatistics::~RetrieveStatistics()
{
    //write statistics to a file
    wxFFile outputFile(wxT("statistics.dat"), wxT("w"));

    outputFile.Write(wxT("Time(ms);Objects;Data\n"));

    std::for_each(data.begin(), data.end(),
                  [&](const StatEntry& entry)
    {
        outputFile.Write(numberTo<wxString>(entry.time));
        outputFile.Write(wxT(";"));
        outputFile.Write(numberTo<wxString>(entry.objects));
        outputFile.Write(wxT(";"));
        outputFile.Write(numberTo<wxString>(entry.value));
        outputFile.Write(wxT("\n"));
    });
}


void RetrieveStatistics::writeEntry(double value, int objects)
{
    StatEntry newEntry;
    newEntry.value   = value;
    newEntry.objects = objects;
    newEntry.time    = timer.Time();
    data.push_back(newEntry);
}


//########################################################################################
Statistics::Statistics(int totalObjectCount,
                       double totalDataAmount,
                       unsigned windowSizeRemainingTime,
                       unsigned windowSizeBytesPerSecond) :
    objectsTotal(totalObjectCount),
    dataTotal(totalDataAmount),
    windowSizeRemTime(windowSizeRemainingTime),
    windowSizeBPS(windowSizeBytesPerSecond),
    windowMax(std::max(windowSizeRemainingTime, windowSizeBytesPerSecond)) {}


void Statistics::addMeasurement(int objectsCurrent, double dataCurrent)
{
    Record newRecord;
    newRecord.objects = objectsCurrent;
    newRecord.data    = dataCurrent;

    const long now = timer.Time();

    measurements.insert(measurements.end(), std::make_pair(now, newRecord)); //use fact that time is monotonously ascending

    //remove all records earlier than "now - windowMax"
    const long newBegin = now - windowMax;
    TimeRecordMap::iterator windowBegin = measurements.upper_bound(newBegin);
    if (windowBegin != measurements.begin())
        measurements.erase(measurements.begin(), --windowBegin); //retain one point before newBegin in order to handle "measurement holes"
}


void Statistics::setNewTotal(int totalObjectCount, double totalDataAmount)
{
    objectsTotal = totalObjectCount;
    dataTotal    = totalDataAmount;
}


wxString Statistics::getRemainingTime() const
{
    if (!measurements.empty())
    {
        const TimeRecordMap::value_type& backRecord = *measurements.rbegin();
        //find start of records "window"
        const long frontTime = backRecord.first - windowSizeRemTime;
        TimeRecordMap::const_iterator windowBegin = measurements.upper_bound(frontTime);
        if (windowBegin != measurements.begin())
            --windowBegin; //one point before window begin in order to handle "measurement holes"

        const TimeRecordMap::value_type& frontRecord = *windowBegin;
        //-----------------------------------------------------------------------------------------------
        const double timeDelta = backRecord.first - frontRecord.first;
        const double dataDelta = backRecord.second.data - frontRecord.second.data;

        const double dataRemaining = dataTotal - backRecord.second.data;
        //objects do *NOT* correspond to disk accesses, so we better play safe and use "bytes" only!
        //https://sourceforge.net/tracker/index.php?func=detail&aid=3452469&group_id=234430&atid=1093083

        if (!numeric::isNull(dataDelta)) //sign(dataRemaining) != sign(dataDelta) usually an error, so show it!
        {
            int remTimeSec = dataRemaining * timeDelta / (1000.0 * dataDelta);
            return zen::remainingTimeToShortString(remTimeSec);
        }
    }

    return wxT("-"); //fallback
}


wxString Statistics::getBytesPerSecond() const
{
    if (!measurements.empty())
    {
        const TimeRecordMap::value_type& backRecord = *measurements.rbegin();
        //find start of records "window"
        const long frontTime = backRecord.first - windowSizeBPS;
        TimeRecordMap::const_iterator windowBegin = measurements.upper_bound(frontTime);
        if (windowBegin != measurements.begin())
            --windowBegin; //one point before window begin in order to handle "measurement holes"

        const TimeRecordMap::value_type& frontRecord = *windowBegin;
        //-----------------------------------------------------------------------------------------------
        const double timeDelta = backRecord.first       - frontRecord.first;
        const double dataDelta = backRecord.second.data - frontRecord.second.data;

        if (!numeric::isNull(timeDelta))
            if (dataDelta > 0) //may be negative if user cancels copying
                return zen::filesizeToShortString(zen::Int64(dataDelta * 1000 / timeDelta)) + _("/sec");
    }

    return wxT("-"); //fallback
}


void Statistics::pauseTimer()
{
    timer.Pause();
}


void Statistics::resumeTimer()
{
    timer.Resume();
}

/*
class for calculation of remaining time:
----------------------------------------
"filesize |-> time" is an affine linear function f(x) = z_1 + z_2 x

For given n measurements, sizes x_0, ..., x_n and times f_0, ..., f_n, the function f (as a polynom of degree 1) can be lineary approximated by

z_1 = (r - s * q / p) / ((n + 1) - s * s / p)
z_2 = (q - s * z_1) / p = (r - (n + 1) z_1) / s

with
p := x_0^2 + ... + x_n^2
q := f_0 x_0 + ... + f_n x_n
r := f_0 + ... + f_n
s := x_0 + ... + x_n

=> the time to process N files with amount of data D is:    N * z_1 + D * z_2

Problem:
--------
Times f_0, ..., f_n can be very small so that precision of the PC clock is poor.
=> Times have to be accumulated to enhance precision:
Copying of m files with sizes x_i and times f_i (i = 1, ..., m) takes sum_i f(x_i) := m * z_1 + z_2 * sum x_i = sum f_i
With X defined as the accumulated sizes and F the accumulated times this gives: (in theory...)
m * z_1 + z_2 * X = F   <=>
z_1 + z_2 * X / m = F / m

=> we obtain a new (artificial) measurement with size X / m and time F / m to be used in the linear approximation above


Statistics::Statistics(const int totalObjectCount, const double totalDataAmount, const unsigned recordCount) :
        objectsTotal(totalObjectCount),
        dataTotal(totalDataAmount),
        recordsMax(recordCount),
        objectsLast(0),
        dataLast(0),
        timeLast(wxGetLocalTimeMillis()),
        z1_current(0),
        z2_current(0),
        dummyRecordPresent(false) {}


wxString Statistics::getRemainingTime(const int objectsCurrent, const double dataCurrent)
{
    //add new measurement point
    const int m = objectsCurrent - objectsLast;
    if (m != 0)
    {
        objectsLast = objectsCurrent;

        const double X = dataCurrent - dataLast;
        dataLast = dataCurrent;

        const zen::Int64 timeCurrent = wxGetLocalTimeMillis();
        const double F = (timeCurrent - timeLast).ToDouble();
        timeLast = timeCurrent;

        record newEntry;
        newEntry.x_i = X / m;
        newEntry.f_i = F / m;

        //remove dummy record
        if (dummyRecordPresent)
        {
            measurements.pop_back();
            dummyRecordPresent = false;
        }

        //insert new record
        measurements.push_back(newEntry);
        if (measurements.size() > recordsMax)
            measurements.pop_front();
    }
    else //dataCurrent increased without processing new objects:
    {    //modify last measurement until m != 0
        const double X = dataCurrent - dataLast; //do not set dataLast, timeLast variables here, but write dummy record instead
        if (!isNull(X))
        {
            const zen::Int64 timeCurrent = wxGetLocalTimeMillis();
            const double F = (timeCurrent - timeLast).ToDouble();

            record modifyEntry;
            modifyEntry.x_i = X;
            modifyEntry.f_i = F;

            //insert dummy record
            if (!dummyRecordPresent)
            {
                measurements.push_back(modifyEntry);
                if (measurements.size() > recordsMax)
                    measurements.pop_front();
                dummyRecordPresent = true;
            }
            else //modify dummy record
                measurements.back() = modifyEntry;
        }
    }

    //calculate remaining time based on stored measurement points
    double p = 0;
    double q = 0;
    double r = 0;
    double s = 0;
    for (std::list<record>::const_iterator i = measurements.begin(); i != measurements.end(); ++i)
    {
        const double x_i = i->x_i;
        const double f_i = i->f_i;
        p += x_i * x_i;
        q += f_i * x_i;
        r += f_i;
        s += x_i;
    }

    if (!isNull(p))
    {
        const double n   = measurements.size();
        const double tmp = (n - s * s / p);

        if (!isNull(tmp) && !isNull(s))
        {
            const double z1 = (r - s * q / p) / tmp;
            const double z2 = (r - n * z1) / s;    //not (n + 1) here, since n already is the number of measurements

            //refresh current values for z1, z2
            z1_current = z1;
            z2_current = z2;
        }
    }

    return formatRemainingTime((objectsTotal - objectsCurrent) * z1_current + (dataTotal - dataCurrent) * z2_current);
}

*/
