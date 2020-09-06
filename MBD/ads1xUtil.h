// Common/MBD/ads1xUtil.h - minimal utility code for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug-Sept. 2020

#ifndef ADS1X_UTIL_H
#define ADS1X_UTIL_H

#include "ads1x.h"
#include "mbdUtil.h"


/***/

extern enum ADS1xMux ads1xGetMux (const U8 cfg[2]);
extern enum ADS1xGain ads1xGetGain(const U8 cfg[2]);
extern U8 ads1xGetRate (const U8 cfg[2]);

extern void ads1xSetMux (U8 cfg[2], const enum ADS1xMux mux);
//void ads1xSetGain
//void ads1xSetRate

extern void ads10GenCfg (U8 cfg[2], enum ADS1xMux mux, enum ADS1xGain gain, enum ADS10Rate rate, enum ADS1xCompare cmp);

// Convert hardware code to hi:lo nybble channel indices (0 1, 2, 3, Gnd)
extern U8 ads1xMuxToM4X4 (const enum ADS1xMux mux);

extern F32 ads1xGainToFSV (const enum ADS1xGain g);

extern U16 ads1xRateToU (const U8 r, const U8 x);

#endif // ADS1X_UTIL_H
