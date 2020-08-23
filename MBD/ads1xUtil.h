// Common/MBD/ads1xUtil.h - utility code for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug 2020

#ifndef ADS1X_UTIL_H
#define ADS1X_UTIL_H

#include "ads1x.h"
#include "mbdUtil.h"
#include "lxI2C.h" // temp ? for debug / flexibility

typedef struct
{
   F32 gainFSV;   // Volts
   U16 rate;   // samples/sec (Hz)
   U8 m4x4; // two 4bit digits
   U8 cmp;
} ADS1xUnpack; // Translation (rename needed)

#define ADS1X_NRB (3)
typedef struct
{
   U8 res[ADS1X_NRB], cfg[ADS1X_NRB], cLo[ADS1X_NRB], cHi[ADS1X_NRB];
} ADS1xRB;

/***/

extern int ads1xInitRB (ADS1xRB *pRB, const MemBuff *pWS, const LXI2CBusCtx *pC, const U8 dev);

extern enum ADS1xMux ads1xGetMux (const U8 cfg[2]);
extern enum ADS1xGain ads1xGetGain(const U8 cfg[2]);
extern U8 ads1xGetRate (const U8 cfg[2]);

extern void ads1xSetMux (U8 cfg[2], const enum ADS1xMux mux);
//void ads1xSetGain
//void ads1xSetRate

extern void ads10GenCfg (U8 cfg[2], enum ADS1xMux mux, enum ADS1xGain gain, enum ADS10Rate rate, enum ADS1xCompare cmp);

extern U8 ads1xMuxToM4X4 (const enum ADS1xMux mux);
extern char muxCh (const U8 c);
extern void printMux4x4 (const U8 m4x4);

extern F32 ads1xGainToFSV (const enum ADS1xGain g);

extern U16 adsx1RateToU (const U8 r, const U8 x);

extern void ads1xTranslateCfg (ADS1xUnpack *pU, const U8 cfg[2], const U8 x);

extern int ads1xConvIvl (const U8 cfg[2], const U8 x);
extern F32 ads1xGainScaleV (const U8 cfg[2], const U8 x);

extern void ads1xDumpCfg (const U8 cfg[2], const U8 x);

//#ifdef ADS1X_TEST
#define ADS1X_TEST_MODE_VERIFY  (1<<7)
#define ADS1X_TEST_MODE_SLEEP   (1<<6)
#define ADS1X_TEST_MODE_POLL    (1<<5)
#define ADS1X_TEST_MODE_ROTMUX  (1<<4)
#define ADS1X_TEST_MODE_TUNE    (1<<3)
#define ADS1X_TEST_MODE_VERBOSE (1<<0)

extern int testADS1015 (const LXI2CBusCtx *pC, const MemBuff *pWS, const U8 dev, const U8 mode, const U8 maxIter);

//#endif // ADS1X_TEST

#endif // ADS1X_UTIL_H
