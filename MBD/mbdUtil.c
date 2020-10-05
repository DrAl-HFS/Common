// Common/MBD/mbdUtil.c - utilities for low-level/embedded stuff
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug 2020

#include "mbdUtil.h"

/***/


#ifndef INLINE

I16 rdI16BE (const U8 b[2]) { return((b[0] << 8) | b[1]); }
I16 rdI16LE (const U8 b[2]) { return((b[1] << 8) | b[0]); }

U8 setMaskU8 (U8 b, U8 m, U8 v, U8 s) { assert((v&m)==v); return((b & ~(m << s)) | (v << s)); }

float rcpI (int x) { if (0 != x) { return(1.0 / (float)x); } else return(0); }
float rcpF (float x) { if (0 != x) { return(1.0 / x); } else return(0); }

#endif // indef INLINE

int bitCountU8 (U8 u)
{
#if 0 // Good for 8bit MCU (no bit shifting)
   int c=0;
   for (; u; c++) { u&= u - 1; } // strip high bit
   return(c);
#else // Good for pipelined CPU (no branching)
   u= u - ((u >> 1) & 0x55);          // odd & even bits -> 4 packed 2bit numbers
   u= (u & 0x33) + ((u >> 2) & 0x33); //    -> 2 packed 4bit "nybbles"
   return((u & 0x0F) + (u >> 4));    //    -> 4 bit result
#endif
} // bitCountU8

int bitCountNU8 (const U8 u[], int n)
{
   int c=0;
   for (int i=0; i<n; i++) { c+= bitCountU8(u[i]); }
// __GNUC__ -> __builtin_popcount(u[i])
   return(c);
} // bitCountNU8
