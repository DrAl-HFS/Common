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

#endif // INLINE

/* DEPRECATED Read n bytes big-endian
int rdnbe (const U8 b[], const int n)
{
   int r= b[0];
   for (int i= 1; i<n; i++) { r= (r<<8) | b[i]; }
   return(r);
} // rdnbe

void wrnbe (U8 b[], int x, const int n)
{
   for (int i= n-1; i>0; i--) { b[i]= x & 0xFF; x>>= 8; }
} // rdnbe
*/
