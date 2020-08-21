// Common/MBD/mbdUtil.c - utilities for low-level/embedded stuff
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug 2020

#include "mbdUtil.h"

/* Endian handling - displace to where ?
typedef union { U64 u64; struct { U8 u8[8]; }; } UU64;
typedef union { U32 u32; struct { U8 u8[4]; }; } UU32;
typedef union { U16 u16; struct { U8 u8[2]; }; } UU16;
*/
I16 rdI16BE (const U8 b[2]) { return((b[0] << 8) | b[1]); }
I16 rdI16LE (const U8 b[2]) { return((b[1] << 8) | b[0]); }

// Read n bytes big-endian
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

