#ifndef STATICASSERT_H_INCLUDED
#define STATICASSERT_H_INCLUDED

//Reference: Compile-Time Assertions, C/C++ Users Journal November, 2004 (http://pera-software.com/articles/compile-time-assertions.pdf)

#ifdef NDEBUG
//If not debugging, assert does nothing.
#define assert_static(x)	((void)0)

#else /* debugging enabled */

#define assert_static(e) \
do { \
enum { assert_static__ = 1/(e) }; \
} while (0)
#endif

#endif // STATICASSERT_H_INCLUDED
