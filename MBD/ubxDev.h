// Common/MBD/ubxDev.h - dev, test & debug code for u-blox GPS module
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#ifndef UBX_DEV_H
#define UBX_DEV_H

#include "ubxUtil.h"
#include "lxI2C.h"

/***/


#ifdef __cplusplus
extern "C" {
#endif

/***/

// Read DDS (I2C) stream access register: up to <max> bytes
// read in <chunk> size blocks. Terminates if a block starts
// with invalid (0xFF) data byte.
// Returns total bytes transferred or zero
extern int ubxReadStream
(
   FragBuff16 *pFB,
   U8 b[],
   const int max,
   int chunk,
   const LXI2CBusCtx *pC,
   const U8 busAddr
);

//extern int test (const LXI2CBusCtx *pC, const U8 dev[2], const U8 maxIter);
extern int ubloxHack (const LXI2CBusCtx *pC, const U8 busAddr);

//#endif // UBX_UTIL_H

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UBX_DEV_H
