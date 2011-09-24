// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef GLOBALFUNCTIONS_H_INCLUDED
#define GLOBALFUNCTIONS_H_INCLUDED

#include <cmath>


namespace common
{
//little rounding function
inline
int round(double d) { return static_cast<int>(d < 0 ? d - 0.5 : d + 0.5); }

//absolute value
template <class T> inline
T abs(const T& d) { return d < 0 ? -d : d; }

inline
size_t getDigitCount(size_t number) { return number == 0 ? 1 : static_cast<size_t>(std::log10(static_cast<double>(number))) + 1; } //count number of digits
}

#endif //GLOBALFUNCTIONS_H_INCLUDED
