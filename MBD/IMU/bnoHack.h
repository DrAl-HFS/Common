// Common/MBD/IMU/bnoHack.h - Hacks for BNO085 IMU
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Feb. 2021

#ifndef BNO_HACK_H
#define LED_MATRIX_H

#include "util.h"
#include "mbdUtil.h"
#include "lxI2C.h"


/***/



/***/

extern int bnoHack (const LXI2CBusCtx *pC, const U8 busAddr, const U8 modeFlags);

#endif // BNO_HACK_H

