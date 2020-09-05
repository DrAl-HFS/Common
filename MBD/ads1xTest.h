// Common/MBD/ads1xTest.h - test & debug code for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Sept 2020

#ifndef ADS1X_TEST_H
#define ADS1X_TEST_H

#include "ads1xUtil.h"
#include "lxI2C.h"

typedef struct
{
   F32 gainFSV;   // Volts
   U16 rate;   // samples/sec (Hz)
   U8 m4x4; // two 4bit digits
   U8 cmp;
} ADS1xTrans; // Translation (rename needed)

#define ADS1X_NRB (3)
typedef struct // i2c "packet" bytes for ADS1x registers
{
   U8 res[ADS1X_NRB], cfg[ADS1X_NRB], cLo[ADS1X_NRB], cHi[ADS1X_NRB];
} ADS1xRB;

#define ADS1X_TEST_MODE_VERIFY  (1<<7)
#define ADS1X_TEST_MODE_SLEEP   (1<<6)
#define ADS1X_TEST_MODE_POLL    (1<<5)
#define ADS1X_TEST_MODE_ROTMUX  (1<<4)
#define ADS1X_TEST_MODE_TUNE    (1<<3)
#define ADS1X_TEST_MODE_VERBOSE (1<<0)


/***/
extern void ads1xTranslateCfg (ADS1xTrans *pT, const U8 cfg[2], const U8 x);

extern char muxCh (const U8 c);
extern void printMux4x4 (const U8 m4x4);
extern void ads1xDumpCfg (const U8 cfg[2], const U8 x);

extern int ads1xInitRB (ADS1xRB *pRB, const MemBuff *pWS, const LXI2CBusCtx *pC, const U8 dev);

extern int testADS1x15 (const LXI2CBusCtx *pC, const MemBuff *pWS, const U8 dev, const U8 x, const U8 mode, const U8 maxIter);

#endif // ADS1X_TEST_H
