// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef STATISTICS_H_INCLUDED
#define STATISTICS_H_INCLUDED

#include <map>
#include <string>
#include <zen/optional.h>

class PerfCheck
{
public:
    PerfCheck(unsigned int windowSizeRemainingTime, //unit: [ms]
              unsigned int windowSizeSpeed);        //
    ~PerfCheck();

    void addSample(int itemsCurrent, double dataCurrent, long timeMs); //timeMs must be ascending!

    zen::Opt<std::wstring> getRemainingTime(double dataRemaining) const;
    zen::Opt<std::wstring> getBytesPerSecond() const; //for window
    zen::Opt<std::wstring> getItemsPerSecond() const; //for window

private:
    struct Record
    {
        Record(int itemCount, double data) : itemCount_(itemCount), data_(data) {}
        int itemCount_;
        double data_; //unit: [bytes]
    };

    std::pair<const std::multimap<long, Record>::value_type*,
        const std::multimap<long, Record>::value_type*> getBlockFromEnd(long windowSize) const;

    const long windowSizeRemTime; //unit: [ms]
    const long windowSizeSpeed_;  //
    const long windowMax;

    std::map<long, Record> samples; //time, unit: [ms]
};

#endif // STATISTICS_H_INCLUDED
