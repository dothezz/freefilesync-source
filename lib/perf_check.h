// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef STATISTICS_H_INCLUDED
#define STATISTICS_H_INCLUDED

#include <map>
#include <string>

class PerfCheck
{
public:
    PerfCheck(unsigned int windowSizeRemainingTime,   //unit: [ms]
              unsigned int windowSizeBytesPerSecond); //
    ~PerfCheck();

    void addSample(int objectsCurrent, double dataCurrent, long timeMs); //timeMs must be ascending!

    std::wstring getRemainingTime(double dataRemaining) const;
    std::wstring getBytesPerSecond() const; //for window

private:
    const long windowSizeRemTime; //unit: [ms]
    const long windowSizeBPS;     //
    const long windowMax;

    struct Record
    {
        Record(int objCount, double data) : objCount_(objCount), data_(data) {}
        int objCount_;
        double data_; //unit: [bytes]
    };
    std::multimap<long, Record> samples; //time, unit: [ms]
};

#endif // STATISTICS_H_INCLUDED
