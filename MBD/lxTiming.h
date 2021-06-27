// Common/MBD/lxTiming.h - timing utils for Linux
// https://github.com/DrAl-HFS/Common.git
// Licence: AGPL3
// (c) Project Contributors Feb 2018 - Sept 2020

#ifndef LX_TIMING_H
#define LX_TIMING_H

#include "util.h" // necessary ?
// #include <unistd.h>


/***/

#ifdef __cplusplus
extern "C" {
#endif

#define MILLI_TICKS  (1000)
#define MICRO_TICKS  (MILLI_TICKS * 1000)
#define NANO_TICKS   (MICRO_TICKS * 1000)

#define TIME_MODE_NOW      (1<<7)
#define TIME_MODE_RELATIVE (0)

// Posix structure permitting nanosecond precision on capable architectures.
// (Permits userland timing accuracy of ~0.5us in practice.)
typedef struct timespec RawTimeStamp;


/***/

#ifndef INLINE
extern int timeStamp (RawTimeStamp *pNow);
#else
INLINE int timeStamp (RawTimeStamp *pNow) { return clock_gettime(CLOCK_REALTIME, pNow); }
#endif

// Capture timestamp and return typical "seconds since start of epoch" measure (00:00 Jan 1st 1970)
extern F32 timeNow (RawTimeStamp *pT);

// Update timestamp and return elapsed time (since last call) in seconds
extern F32 timeElapsed (RawTimeStamp *pLast);

// Simple time difference from reference: T - R seconds
extern F32 timeDiff (const RawTimeStamp *pR, const RawTimeStamp *pT);

// Estimate time difference (in seconds) as a linear combination of two timestamps measure from some reference
extern F32 timeEstDiff (const RawTimeStamp *pR, const RawTimeStamp *pT1, const RawTimeStamp *pT2, const F32 r[2]);

// Set a target as base+offset. If pBase is NULL pTarget is used as base.
// If flag MODE_TIMENOW is given then *pBase is set to current timestamp.
// Returns >=0 if successful
extern int timeSetTarget (RawTimeStamp *pTarget, RawTimeStamp *pBase, const long offsetNanoSec, U8 modeFlags);

// Busy wait, updating timestamp "Now" until target is reached
// Typical overrun 0~100ns, occasionally up to 500ns (system under high load?)
// Returns >=0 if successful
extern int timeSpinWaitUntil (RawTimeStamp *pNow, const RawTimeStamp *pTarget);

// Hybrid delay of up to 1 second implemented as sleep (for 2+ millisecond
// delays) and spin-wait (using clock) to achieve improved time resolution
// with a (limited) degree of efficiency.
extern long timeSpinSleep (long nanoSec);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LX_TIMING_H
