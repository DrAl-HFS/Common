// Common/MBD/ads1xDev.h - development, test & debug code for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Sept 2020

#ifndef ADS1X_DEV_H
#define ADS1X_DEV_H

#include "ads1xUtil.h"
#include "lxI2C.h"
#include "lxTiming.h"


/***/

// Translation from hardware encoding
// to more general (friendly) representation
typedef struct
{
   F32 gainFSV;   // Volts
   U16 rate;   // samples/sec (Hz)
   U8 cmp;
} ADS1xTrans;

// i2c "packet" bytes for ADS1x registers
#define ADS1X_NRB (3)
typedef struct
{  // Result & config registers (minimum required)
   U8 res[ADS1X_NRB], cfg[ADS1X_NRB];
} ADS1xRCPB;

typedef struct
{  // Full register set with compare values
   ADS1xRCPB rc;
   U8 cLo[ADS1X_NRB], cHi[ADS1X_NRB];
} ADS1xFullPB;

#define ADS1X_TEST_MODE_VERIFY  (1<<7)
#define ADS1X_TEST_MODE_SLEEP   (1<<6)
#define ADS1X_TEST_MODE_POLL    (1<<5)
#define ADS1X_TEST_MODE_TIMER   (1<<4)
#define ADS1X_TEST_MODE_TUNE    (1<<3)
//define ADS1X_TEST_MODE_        (1<<1)
#define ADS1X_TEST_MODE_VERBOSE (1<<0)


/***/

extern void ads1xTransCfg (ADS1xTrans *pT, const U8 cfg[2], const ADS1xHWID id);

extern void ads1xDumpCfg (const U8 cfg[2], const ADS1xHWID id);

extern int ads1xInitRB (ADS1xFullPB *pFPB, const MemBuff *pWS, const LXI2CBusCtx *pC, const U8 dev);

#ifdef ADS1X_TEST

typedef struct
{
   U16 rate, maxSamples;
   U8 mux[4];
   U8 nMux;
   U8 busAddr; // device bus address
   U8 hwID;
   U8 modeFlags;
} ADSReadParam;

extern int testADS1x15 (const LXI2CBusCtx *pC, const MemBuff *pWS, const ADSInstProp *pP, const ADSReadParam *pM);
#endif // ADS1X_TEST

#endif // ADS1X_DEV_H
