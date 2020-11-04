// platform.h - fundamental type & macro declarations (to simplify multi-platform support)
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Jan-June 2019

#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef __GNUC__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE // Enable extras in <math.h> etc.
#endif
#endif // __GNUC__

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h> // date & time etc.
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>   // timeval etc.
#include <unistd.h>

#if 0 //def __PGI
#define __ABM__
#include <intrin.h>     // ABM
#include <immintrin.h>  // AVX
#include <mmintrin.h>   // MMX
#include <xmmintrin.h>  // SSE
#include <emmintrin.h>  // SSE2
#include <pmmintrin.h>  // SSE3
#include <tmmintrin.h>  // SSSE3
#include <ammintrin.h>  // SSE4a
#define PGI_INTRINSIC
#endif

#ifdef __cplusplus
extern "C" {
#endif

// General compiler tweaks
//pragma clang diagnostic ignored "-Wmissing-field-initializers"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif
#ifndef SWAP
#define SWAP(Type,a,b) { Type tmp= (a); (a)= (b); (b)= tmp; }
#endif
#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

#define LOG2I(x)     ((int)log2(x))

#define BIT_MASK(n)        ((1<<(n))-1)
#define BITS_TO_BYTES(b)   (((b)+7)>>3)   // Consider renaming to NBITS_...() for clarity
#define BITS_TO_WRDSH(b,s) (((b)+BIT_MASK(s))>>s)

// a/b round up with denominator check
// Returns numerator if denominator <= 1 i.e. neg. denom. unsupported!
#define DIV_RUP(a,b) ((b)>1 ? ((a)+(b)-1)/(b) : (a))

//define _PASTE(a,b) a##b
//define PASTE(a,b) _PASTE(a,b)

#ifndef MBD_DEF_H // Prevent grumbling about int32_t vs. long int definitions
// Terse type names
#if _STDINT_H
typedef int8_t   I8;
typedef int16_t  I16;
typedef int32_t  I32;
typedef int64_t  I64;

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
#else // _STDINT_H broken
// Use old style homebrew (may need tweaking per platform)
#include "mbdDef.h"
#endif // _STDINT_H broken
#endif // MBD_DEF_H

//typedef __fp16 F16; // gcc on ARM: -mfpu=neon-fp16 -mfp16-format=ieee (or -mfp16-format=alternative for no Inf/NaN)
//typedef _Float16 F16; // CLANG / CStd no ABI ?
typedef float F32;
typedef double F64;

typedef int B32, Bool32; // Boolean

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PLATFORM_H
