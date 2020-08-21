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
} ADS1xUnpack;

#define ADS1X_NRB (3)
typedef struct
{
   U8 res[ADS1X_NRB], cfg[ADS1X_NRB], cLo[ADS1X_NRB], cHi[ADS1X_NRB];
} ADS1xRB;

/***/

extern int ads1xInitRB (ADS1xRB *pRB, const MemBuff *pWS, const LXI2CBusCtx *pC, const U8 dev);

extern U8 ads1xGetMux (const U8 cfg[2]);
extern void ads1xSetMux (U8 cfg[2], const enum ADS1xMux mux);
extern U8 ads1xMuxToM4X4 (const enum ADS1xMux mux);
extern char muxCh (const U8 c);
extern void printMux4x4 (const U8 m4x4);

F32 ads1xGainToFSV (const enum ADS1xGain g);

U16 adsx1RateToU (const U8 r, const U8 x);

void ads1xUnpackCfg (ADS1xUnpack *pU, const U8 cfg[2], const U8 x);
int convInterval (ADS1xUnpack *pU, const U8 cfg[2], const U8 x);

extern void ads1xDumpCfg (const U8 cfg[2], const U8 x);

extern void ads10GenCfg (U8 cfg[2], enum ADS1xMux mux, enum ADS1xGain gain, enum ADS10Rate rate, enum ADS1xCompare cmp);

//#ifdef TEST
#define MODE_VERIFY  (1<<7)
#define MODE_SLEEP   (1<<6)
#define MODE_POLL    (1<<5)
#define MODE_ROTMUX  (1<<4)
#define MODE_VERBOSE (1<<0)

extern int testADS1015 (const LXI2CBusCtx *pC, const MemBuff *pWS, const U8 dev, const U8 mode);

#endif // ADS1X_UTIL_H
