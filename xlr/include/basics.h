#ifndef BASICS_H
// ****************************************************************************
//  basics.h                        (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//      Basic operations (arithmetic, etc)
//
//
//
//
//
//
//
//
// ****************************************************************************
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "tree.h"
#include "context.h"
#include "opcodes.h"
#include "main.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>

XL_BEGIN

// ============================================================================
//
//   Top level entry point
//
// ============================================================================

// Top-level entry point: enter all basic operations in the context
void EnterBasics();

// Top-level entry point: delete all globals related to basic operations
void DeleteBasics();



// ============================================================================
// 
//    Some utility functions used in basics.tbl
// 
// ============================================================================

inline longlong xl_text2int(text_r t)
// ----------------------------------------------------------------------------
//   Converts text to a numerical value
// ----------------------------------------------------------------------------
{
    std::istringstream stream(t.value);
    longlong result = 0.0;
    stream >> result;
    return result;
}


inline double xl_text2real(text_r t)
// ----------------------------------------------------------------------------
//   Converts text to a numerical value
// ----------------------------------------------------------------------------
{
    std::istringstream stream(t.value);
    double result = 0.0;
    stream >> result;
    return result;
}


inline text xl_int2text(longlong value)
// ----------------------------------------------------------------------------
//   Convert a numerical value to text
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << value;
    return out.str();
}


inline text xl_real2text(double value)
// ----------------------------------------------------------------------------
//   Convert a numerical value to text
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << value;
    return out.str();
}


inline integer_t xl_mod(integer_r xr, integer_r yr)
// ----------------------------------------------------------------------------
//   Compute a mathematical 'mod' from the C99 % operator
// ----------------------------------------------------------------------------
{
    integer_t x = XL_INT(xr);
    integer_t y = XL_INT(yr);
    integer_t tmp = x % y;
    if (tmp && (x^y) < 0)
        tmp += y;
    return tmp;
}


inline integer_t xl_pow(integer_r xr, integer_r yr)
// ----------------------------------------------------------------------------
//   Compute integer power
// ----------------------------------------------------------------------------
{
    integer_t x = XL_INT(xr);
    integer_t y = XL_INT(yr);
    integer_t tmp = 0;
    if (y >= 0)
    {
        tmp = 1;
        while (y)
        {
            if (y & 1)
                tmp *= x;
            x *= x;
            y >>= 1;
        }
    }
    return tmp;
}


inline real_t xl_modf(real_r xr, real_r yr)
// ----------------------------------------------------------------------------
//   Compute a mathematical 'mod' from fmod
// ----------------------------------------------------------------------------
{
    real_t x = XL_REAL(xr);
    real_t y = XL_REAL(yr);
    real_t tmp = fmod(x,y);
    if (tmp != 0.0 && x*y < 0.0)
        tmp += y;
    return tmp;
}


inline real_t xl_powf(real_r xr, integer_r yr)
// ----------------------------------------------------------------------------
//   Compute real power with an integer on the right
// ----------------------------------------------------------------------------
{
    real_t x = XL_REAL(xr);
    integer_t y = XL_INT(yr);
    boolean_t negative = y < 0;
    real_t tmp = 1.0;
    if (negative)
        y = -y;
    while (y)
    {
        if (y & 1)
            tmp *= x;
        x *= x;
        y >>= 1;
    }
    if (negative)
        tmp = 1.0/tmp;
    return tmp;
}


inline integer_t xl_time(double delay)
// ----------------------------------------------------------------------------
//   Return the current system time
// ----------------------------------------------------------------------------
{
    time_t t;
    time(&t);
    MAIN->Refresh(delay);
    return t;
}


#define XL_RTIME(tmfield)                       \
    struct tm tm;                               \
    time_t clock = t;                           \
    localtime_r(&clock, &tm);                   \
    XL_RINT(tmfield)


#define XL_RCTIME(tmfield, delay)               \
    struct tm tm;                               \
    time_t clock;                               \
    time(&clock);                               \
    localtime_r(&clock, &tm);                   \
    MAIN->Refresh(delay);                       \
    XL_RINT(tmfield)


template<typename number>
inline number xl_random(number low, number high)
// ----------------------------------------------------------------------------
//    Return a pseudo-random number in the low..high range
// ----------------------------------------------------------------------------
{
#ifndef CONFIG_MINGW
    double base = drand48();
#else
    double base = double(rand()) / RAND_MAX;
#endif // CONFIG_MINGW
    return number(base * (high-low) + low);
}

inline bool xl_random_seed(int seed)
// ----------------------------------------------------------------------------
//    Initialized random number generator using the argument passed as seed
// ----------------------------------------------------------------------------
{
#ifndef CONFIG_MINGW
    srand48(seed);
#else
    srand(seed);
#endif // CONFIG_MINGW

    return true;
}

#ifdef CONFIG_MINGW
#ifndef localtime_r
inline struct tm *localtime_r (const time_t * timep, struct tm * result)
// ----------------------------------------------------------------------------
//   MinGW doesn't have localtime_r, but its localtime is thread-local storage
// ----------------------------------------------------------------------------
{
    *result = *localtime (timep);
    return result;
}
#endif
#endif // CONFIG_MINGW


inline text xl_text_replace(text txt, text before, text after)
// ----------------------------------------------------------------------------
//   Return a copy of txt with all occurrences of before replaced with after
// ----------------------------------------------------------------------------
{
  size_t pos = 0;
  while ((pos = txt.find(before, pos)) != std::string::npos)
  {
     txt.replace(pos, before.length(), after);
     pos += after.length();
  }
  return txt;
}


XL_END

#endif // BASICS_H
