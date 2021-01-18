// Common/MBD/lsmUtil.h - utility code for STMicro. lsm9ds1 IMU
// * DEPRECATED * due to hardware problems:-
// This is a consumer grade device with sizeable permitted error of
// +-10deg/sec in angular velocity (i.e. using the most accurate
// available sampling mode). This was judged tolerable as a learning or
// experimentation platform, especially given the low price point of
// (Shenzen sourced) breakout boards. However, it transpires that the
// unit requires very careful board design and assembly to prevent
// mechanical stress that is disasterous for measurement accuracy.
// Cheaply manufactured units may greatly exceed the manufacturers
// quoted values: a bias of +40deg/sec with variation of +-20deg/sec has
// been directly observed. This makes such a device useless for anything
// except the crudest form of gesture recognition. Additionally, SPI
// communication (necessary for high-rate applications) seems defective
// which suggests further deficiencies in board assembly.
// Although boards are available that have presumably been assembled
// under a strict quality control system, other newer devices from
// Bosch, TDK/Invensense etc. are easier to use while offering a better
// general price/performance ratio.
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
