// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License, Version 1.0. See accompanying file             *
// * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.       *
// * Copyright (C) 2011 ZenJu (zhnmju123 AT gmx.de)                         *
// **************************************************************************

#ifndef ZEN_SCOPEGUARD_8971632487321434
#define ZEN_SCOPEGUARD_8971632487321434

//best of Zen, Loki and C++11

namespace zen
{
//Scope Guard
/*
    zen::ScopeGuard lockAio = zen::makeGuard([&]() { ::CancelIo(hDir); });
		...
	lockAio.dismiss();
*/

//Scope Exit
/*
	ZEN_ON_BLOCK_EXIT(::CloseHandle(hDir));
*/

class ScopeGuardBase
{
public:
    void dismiss() const { dismissed_ = true; }

protected:
    ScopeGuardBase() : dismissed_(false) {}
    ScopeGuardBase(const ScopeGuardBase& other) : dismissed_(other.dismissed_) { other.dismissed_ = true; } //take over responsibility
    ~ScopeGuardBase() {}

    bool isDismissed() const { return dismissed_; }

private:
    ScopeGuardBase& operator=(const ScopeGuardBase&); // = delete;

    mutable bool dismissed_;
};


template <typename F>
class ScopeGuardImpl : public ScopeGuardBase
{
public:
    ScopeGuardImpl(F fun) : fun_(fun) {}

    ~ScopeGuardImpl()
    {
        if (!this->isDismissed())
            try
            {
                fun_();
            }
            catch (...) {}
    }

private:
    F fun_;
};

typedef const ScopeGuardBase& ScopeGuard;

template <class F> inline
ScopeGuardImpl<F> makeGuard(F fun) { return ScopeGuardImpl<F>(fun); }
}

#define ZEN_CONCAT_SUB(X, Y) X ## Y
#define ZEN_CONCAT(X, Y) ZEN_CONCAT_SUB(X, Y)

#define ZEN_ON_BLOCK_EXIT(X) zen::ScopeGuard ZEN_CONCAT(dummy, __LINE__) = zen::makeGuard([&](){X;}); (void)ZEN_CONCAT(dummy, __LINE__);

#endif //ZEN_SCOPEGUARD_8971632487321434
