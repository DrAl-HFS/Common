// Common/MBD/mbdDef.h - definitions for low-level/embedded stuff
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug 2020

#ifndef MBD_DEF_H
#define MBD_DEF_H

/* #include <sys/types.h> */

/***/


#ifdef __cplusplus
extern "C" {
#endif

#ifndef UTIL_H

// Cannot rely on <sys/types.h> etc. being available
// on all targets, so just home-brew as necessary.
typedef signed char       I8;
typedef signed short      I16;
typedef signed long       I32; // NB divergence from <int> for AVR 8b gcc compatibility

typedef unsigned char      U8;
typedef unsigned short     U16;
typedef unsigned long      U32; // not <int>

// Possibly unsupported by some build environments?
typedef signed long long I64;
typedef unsigned long long U64;

typedef float F32;
typedef double F64;

#endif // UTIL_H

// Types to support endian twiddling/debug
// typedef union { U64 u64; U8 u8[8]; } UU64;
typedef union { U32 u32; U8 u8[4]; } UU32;
typedef union { U16 u16; U8 u8[2]; } UU16;

// Compact descriptor for fragment within small buffer
// Size units determined by usage context (commonly bytes)
typedef struct { U16 offset, len; } FragBuff16;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // MBD_DEF_H
