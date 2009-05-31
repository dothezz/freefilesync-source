#ifndef STATISTICS_H_INCLUDED
#define STATISTICS_H_INCLUDED

#include <vector>
#include <wx/stopwatch.h>
#include <list>

class RetrieveStatistics
{
public:
    wxDEPRECATED(RetrieveStatistics() {}) //generate compiler warnings as a reminder to remove code after measurements
    ~RetrieveStatistics();

    void writeEntry(const double value, const int objects);

private:
    struct statEntry
    {
        long time;
        int objects;
        double value;
    };

    std::vector<statEntry> data;
    wxStopWatch timer;
};


class Statistics
{
public:
    Statistics(const int totalObjectCount,
               const double totalDataAmount,
               const unsigned windowSizeRemainingTime,   //time in ms
               const unsigned windowSizeBytesPerSecond); //time in ms

    void addMeasurement(const int objectsCurrent, const double dataCurrent);
    wxString getRemainingTime() const; //returns the remaining time in milliseconds
    wxString getBytesPerSecond() const;

    void pauseTimer();
    void resumeTimer();

private:
    wxString formatRemainingTime(const double timeInMs) const;

    const int      objectsTotal;
    const double   dataTotal;

    const unsigned windowSizeRemTime; //"window width" of statistics used for calculation of remaining time in ms
    const unsigned windowSizeBPS;     //
    const unsigned windowMax;

    mutable int remainingTimeLast; //used for "smoothening" remaining time

    struct record
    {
        int    objects;
        double data;
        long   time;
    };

    std::list<record> measurements;
    wxStopWatch timer;
};

#endif // STATISTICS_H_INCLUDED