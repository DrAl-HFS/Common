// Common/MBD/ledMatrix.h - Hacks for IS31FL3731 LEDshim
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct. 2020

#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include "util.h"
#include "mbdUtil.h"
#include "lxI2C.h"


/***/

// LM_ ???
#define MODE_SHUTDOWN (0x80)


/***/

extern int ledMatHack (const LXI2CBusCtx *pC, const U8 busAddr, const U8 modeFlags);

#endif // LED_MATRIX_H
