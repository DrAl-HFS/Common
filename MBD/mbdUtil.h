// Common/MBD/mbdUtil.c - utilities for low-level/embedded stuff
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug 2020

#ifndef MBD_UTIL_H
#define MBD_UTIL_H

#include "mbdDef.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

// GCC happiest, others?
#define INLINE static inline

#ifdef INLINE

INLINE I16 rdI16BE (const U8 b[2]) { return((b[0] << 8) | b[1]); }
INLINE I16 rdI16LE (const U8 b[2]) { return((b[1] << 8) | b[0]); }

// Insert into byte b, the value v using mask m and shift s
INLINE U8 setMaskU8 (U8 b, U8 m, U8 v, U8 s) { assert((v&m)==v); return((b & ~(m << s)) | (v << s)); }

#else

extern I16 rdI16BE (const U8 b[2]);
extern I16 rdI16LE (const U8 b[2]);
extern U8 setMaskU8 (U8 b, U8 m, U8 v, U8 s);

#endif

// DEPRECATED: use <util.h> versions
// Read/write n bytes big-endian
//extern int rdnbe (const U8 b[], const int n);
//extern void wrnbe (U8 b[], int x, const int n);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // MBD_UTIL_H
