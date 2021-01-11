// Common/MBD/lsmUtil.h - utility code for STMicro. lsm9ds1 IMU
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug 2020

#ifndef LSM_UTIL_H
#define LSM_UTIL_H

#include "lsm9ds1.h"
#include "mbdUtil.h"
#include "lxI2C.h" // temp ? for debug / flexibility

/***/


#ifdef __cplusplus
extern "C" {
#endif

typedef struct { U8 addr, id; } HWUAID; // Bus address and device ID


/***/

//#ifdef LSM_TEST
//#define LSM_TEST_MODE_

extern int testIMU (const LXI2CBusCtx *pC, const HWUAID aid[], const int nD, const int maxIter);

//#endif // LSM_TEST

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LSM_UTIL_H
