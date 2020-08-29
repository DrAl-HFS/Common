// Common/MBD/lsmUtil.h - utility code for STMicro. lsm9ds1 IMU
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug 2020

#ifndef LSM_UTIL_H
#define LSM_UTIL_H

#include "lsm9ds1.h"
#include "mbdUtil.h"
#include "lxI2C.h" // temp ? for debug / flexibility

#ifdef __cplusplus
extern "C" {
#endif

/***/


//#ifdef LSM_TEST
#define LSM_TEST_MODE_VERIFY  (1<<7)
#define LSM_TEST_MODE_SLEEP   (1<<6)
#define LSM_TEST_MODE_POLL    (1<<5)
#define LSM_TEST_MODE_ROTMUX  (1<<4)
#define LSM_TEST_MODE_TUNE    (1<<3)
#define LSM_TEST_MODE_VERBOSE (1<<0)

extern int testIMU (const LXI2CBusCtx *pC, const U8 dev[2], const U8 maxIter);

//#endif // LSM_TEST

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LSM_UTIL_H
