#ifndef ASSERT_H
#define ASSERT_H 1
#include "config.h"
#include <cassert>

#ifdef DEBUG
//#define Assert(a) assert(a)

#ifdef _MSC_VER
#define BREAKPOINT __asm int 3
#else

#ifdef __i386__
#define BREAKPOINT asm("   int $3")
#else
#define BREAKPOINT   *(int *)0 = 0
#endif

#endif

#define Assert(expr) (expr) ? (void)0 :   \
                 _my_assert(__FILE__,__LINE__,#expr)

inline static void _my_assert(const char *file,unsigned line,const char *expr)
{
  Config::err() << file << "(" << line << ") Assertion failed: " << expr << std::endl;
  BREAKPOINT;
}
#define Paranoid(a) do { a; } while (0)

#else
#define BREAKPOINT
#define Assert(a)
#define Paranoid(a)

#endif
#endif
