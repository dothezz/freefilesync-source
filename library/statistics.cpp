// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "statistics.h"

#include <wx/ffile.h>
#include "../shared/global_func.h"
#include "status_handler.h"
#include "../shared/util.h"
#include "../shared/i18n.h"
#include <limits>
#include <wx/stopwatch.h>
#include "../shared/assert_static.h"


using namespace zen;

RetrieveStatistics::RetrieveStatistics() :
    timer(new wxStopWatch) {}


RetrieveStatistics::~RetrieveStatistics()
{
    //keep non-inline destructor for std::auto_ptr to work with forward declaration

    //write statistics to a file
    wxFFile outputFile(wxT("statistics.dat"), wxT("w"));

    outputFile.Write(wxT("Time(ms);Objects;Data\n"));

    for (std::vector<StatEntry>::const_iterator i = data.begin(); i != data.end(); ++i)
    {
        outputFile.Write(toString<wxString>(i->time));
        outputFile.Write(wxT(";"));
        outputFile.Write(toString<wxString>(i->objects));
        outputFile.Write(wxT(";"));
        outputFile.Write(toString<wxString>(i->value));
        outputFile.Write(wxT("\n"));
    }
}


void RetrieveStatistics::writeEntry(const double value, const int objects)
{
    StatEntry newEntry;
    newEntry.value   = value;
    newEntry.objects = objects;
    newEntry.time    = timer->Time();
    data.push_back(newEntry);
}


//########################################################################################
namespace
{
template <class T>
inline
bool isNull(T number)
{
    return common::abs(number) <= std::numeric_limits<T>::epsilon(); //epsilon == 0 for integer types, therefore less-equal(!)
}
}


enum UnitRemTime
{
    URT_SEC,
    URT_MIN,
    URT_HOUR,
    URT_DAY
};


inline
wxString Statistics::formatRemainingTime(double timeInMs) const
{
    double remainingTime = timeInMs / 1000;

    //determine preferred unit
    UnitRemTime unit = URT_SEC;
    if (remainingTime > 55)
    {
        unit = URT_MIN;
        remainingTime /= 60;
        if (remainingTime > 59)
        {
            unit = URT_HOUR;
            remainingTime /= 60;
            if (remainingTime > 23)
            {
                unit = URT_DAY;
                remainingTime /= 24;
            }
        }
    }

    int formattedTime = common::round(remainingTime);

    //reduce precision to 5 seconds
    if (unit == URT_SEC && formattedTime % 5 != 0)
        formattedTime += 5 - formattedTime % 5; //"ceiling"


    //avoid "jumping back and forth" when fluctuating around .5
    if (remainingTimeLast < formattedTime)
    {
        if (unit == URT_SEC)
        {
            formattedTime = common::round(remainingTime);
            formattedTime -= formattedTime % 5; //"floor"
        }
        else
            formattedTime = static_cast<int>(remainingTime); //"floor"
    }
    remainingTimeLast = formattedTime;

    //generate output message
    wxString output;
    switch (unit)
    {
        case URT_SEC:
            output = _P("1 sec", "%x sec", formattedTime);
            break;
        case URT_MIN:
            output = _P("1 min", "%x min", formattedTime);
            break;
        case URT_HOUR:
            output = _P("1 hour", "%x hours", formattedTime);
            break;
        case URT_DAY:
            output = _P("1 day", "%x days", formattedTime);
            break;
    }
    output.Replace(wxT("%x"), zen::toStringSep(formattedTime));
    return output;
}


Statistics::Statistics(int totalObjectCount,
                       double totalDataAmount,
                       unsigned windowSizeRemainingTime,
                       unsigned windowSizeBytesPerSecond) :
    objectsTotal(totalObjectCount),
    dataTotal(totalDataAmount),
    windowSizeRemTime(windowSizeRemainingTime),
    windowSizeBPS(windowSizeBytesPerSecond),
    windowMax(std::max(windowSizeRemainingTime, windowSizeBytesPerSecond)),
    remainingTimeLast(std::numeric_limits<int>::max()), //something "big"
    timer(new wxStopWatch) {}

Statistics::~Statistics()
{
    delete timer;
}

void Statistics::addMeasurement(int objectsCurrent, double dataCurrent)
{
    Record newRecord;
    newRecord.objects = objectsCurrent;
    newRecord.data    = dataCurrent;

    const long currentTime = timer->Time();

    const TimeRecordMap::value_type newEntry(currentTime, newRecord);

    //insert new record
    if (!measurements.empty())
    {
        //assert(dataCurrent >= (--measurements.end())->second.data);
        measurements.insert(--measurements.end(), newEntry); //use fact that time is monotonously ascending
    }
    else
        measurements.insert(newEntry);

    //remove all records earlier than "currentTime - windowSize"
    const long newBegin = currentTime - windowMax;
    TimeRecordMap::iterator windowBegin = measurements.upper_bound(newBegin);
    if (windowBegin != measurements.begin())
        measurements.erase(measurements.begin(), --windowBegin); //retain one point before newBegin in order to handle "measurement holes"
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
        const double timeDelta = backRecord.first       - frontRecord.first;
        const double dataDelta = backRecord.second.data - frontRecord.second.data;

        const double dataRemaining = dataTotal - backRecord.second.data;

        if (!isNull(dataDelta))
            return formatRemainingTime(dataRemaining * timeDelta / dataDelta);
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

        if (!isNull(timeDelta))
            if (dataDelta > 0) //may be negative if user cancels copying
                return zen::formatFilesizeToShortString(zen::UInt64(dataDelta * 1000 / timeDelta)) + _("/sec");
    }

    return wxT("-"); //fallback
}


void Statistics::pauseTimer()
{
    timer->Pause();
}


void Statistics::resumeTimer()
{
    timer->Resume();
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
