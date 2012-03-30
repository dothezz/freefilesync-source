// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef OPTIONAL_H_2857428578342203589
#define OPTIONAL_H_2857428578342203589

namespace zen
{
/*
Optional return value with static memory allocation!
 -> interface like a pointer, performance like a value

 Usage:
 ------
 Opt<MyEnum> someFunction();
{
   if (allIsWell)
       return enumVal;
   else
       return NoValue();
}

 Opt<MyEnum> optValue = someFunction();
 if (optValue)
       ... use *optValue ...
*/

struct NoValue {};

template <class T>
class Opt
{
public:
    Opt()             : value()   , valid(false) {}
    Opt(NoValue)      : value()   , valid(false) {}
    Opt(const T& val) : value(val), valid(true ) {}

    //rvalue optimization: only basic exception safety:
    Opt(Opt&& tmp) : value(std::move(tmp.value)), valid(tmp.valid) {}
    Opt& operator=(const Opt& tmp) { value = std::move(tmp.value); valid = tmp.valid; }

#ifdef _MSC_VER
private:
    struct ConversionToBool { int dummy; };
public:
    operator int ConversionToBool::* () const { return valid ? &ConversionToBool::dummy : nullptr; }
#else
    explicit operator bool() const { return valid; } //thank you C++11!!!
#endif

    const T& operator*() const { return value; }
    /**/  T& operator*()       { return value; }

    const T* operator->() const { return &value; }
    /**/  T* operator->()       { return &value; }

    void reset() { valid = false; }
private:
    T value;
    bool valid;
};

}

#endif //OPTIONAL_H_2857428578342203589
