// Common/MBD/mbdUtil.c - utilities for low-level/embedded stuff
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug 2020

#ifndef MBD_UTIL_H
#define MBD_UTIL_H

#include "mbdDef.h"
#include <assert.h>

/***/


#ifdef __cplusplus
extern "C" {
#endif

// GCC happiest, others?
#define INLINE static inline

#ifndef INLINE

extern I16 rdI16BE (const U8 b[2]);
extern I16 rdI16LE (const U8 b[2]);
extern U8 setMaskU8 (U8 b, U8 m, U8 v, U8 s);

// Return reciprocal of x as float, or zero if x is zero
extern float rcpI (int x);
extern float rcpF (float x);

#else // INLINE

INLINE I16 rdI16BE (const U8 b[2]) { return((b[0] << 8) | b[1]); }
INLINE I16 rdI16LE (const U8 b[2]) { return(b[0] | (b[1] << 8)); }

INLINE int wrI16BE (U8 b[2], I16 v) { b[0]= v >> 8; b[1]= v; return(2); }
INLINE int wrI16LE (U8 b[2], I16 v) { b[0]= v; b[1]= v >> 8; return(2); }

// Insert into byte b, the value v using mask m and shift s
INLINE U8 setMaskU8 (U8 b, U8 m, U8 v, U8 s) { assert((v&m)==v); return((b & ~(m << s)) | (v << s)); }

INLINE float rcpI (int x) { if (0 != x) { return(1.0 / (float)x); } else return(0); }
INLINE float rcpF (float x) { if (0 != x) { return(1.0 / x); } else return(0); }

#endif // INLINE

// Kernighans method for MCU vs. in-place-recursive bit-parallel for pipelined CPU
extern int bitCountU8 (U8 u);

// NB : choice of int counter is deliberate here: size_t creates dependancy
// on <stdef> and vulnerability to negative results (arithmetic expression for n)
extern int bitCountNU8 (const U8 u[], int n);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // MBD_UTIL_H
