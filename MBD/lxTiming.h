// Common/MBD/lxTiming.h - timing utils for Linux
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Feb 2018 - Sept 2020

#ifndef LX_TIMING_H
#define LX_TIMING_H

#include "util.h" // necessary ?


/***/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct timespec RawTimeStamp;


/***/

extern F32 timeNow (RawTimeStamp *pT);

extern F32 timeElapsed (RawTimeStamp *pLast);

// Hybrid delay of up to 1 second implemented as sleep (for 2+ millisecond
// delays) and spin-wait (using clock) to achieve improved time resolution
// with a (limited) degree of efficiency.
extern long timeSpinSleep (long nanoSec);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LX_TIMING_H
