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
extern enum ADS1xRate ads1xGetRate (const U8 cfg[2]);

extern void ads1xSetMux (U8 cfg[2], const enum ADS1xMux mux);
extern void ads1xSetGain (U8 cfg[2], const enum ADS1xGain gain);
extern void ads1xSetRate (U8 cfg[2], const enum ADS1xRate rate);

// Overwrite everything (NB: all flags -> zero)
extern void ads1xGenCfg (U8 cfg[2], enum ADS1xMux mux, enum ADS1xGain gain, enum ADS1xRate rate, enum ADS1xCompare cmp);

extern F32 ads1xGainToFSV (enum ADS1xGain gain);

extern U16 ads1xRateToU (enum ADS1xRate rate, ADS1xHWID id);

#endif // ADS1X_UTIL_H
