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
   // U8 busAddr, pad[1];  // Complete info for device instance ? Considering...
} ADS1xFullPB;


/***/

extern void ads1xTransCfg (ADS1xTrans *pT, const U8 cfg[2], const ADS1xHWID id);

extern void ads1xDumpCfg (const U8 cfg[2], const ADS1xHWID id);

extern int ads1xInitRB (ADS1xFullPB *pFPB, const MemBuff *pWS, const LXI2CBusCtx *pC, const U8 dev);

#ifdef ADS1X_TEST

#define ADS1X_TEST_MODE_MASK    (0xF0) // Upper nybble mask (for original test mode only)
#define ADS1X_TEST_MODE_VERIFY  (1<<7) // Verify transactions (paranoid / faulty bus test)
#define ADS1X_TEST_MODE_SLEEP   (1<<6) // Sleep for expected conversion interval
#define ADS1X_TEST_MODE_POLL    (1<<5) // Poll the device for conversion ready flag
#define ADS1X_TEST_MODE_TUNE    (1<<4) // Primitive performance tuning attempt
#define ADS1X_MODE_MASK    (0x0F)   // Lower nybble mask applies to all
#define ADS1X_MODE_X3      (1<<3)   // Presently unused
#define ADS1X_MODE_X2      (1<<2)   //   "         "
#define ADS1X_MODE_XTIMING (1<<1)   // Extended timing information
#define ADS1X_MODE_VERBOSE (1<<0)   // Diagnostic info (to console)

#define ADS1X_MUX_MAX 8

typedef struct
{
   U16 rate[2]; // outer, inner
   U8 mux[ADS1X_MUX_MAX];
   U8 nMux;
   //U8 cfg[ADS1X_NRB]; ? consider ?
   U8 maskAG;
   U8 timeEst; // Sample timing measure/estimation method
   U8 modeFlags;
} ADSReadParam;

// Auto gain, & accurate timing test
int testAutoGain
(
   const int maxSamples,
   const LXI2CBusCtx *pC,
   const ADSInstProp *pP,
   const ADSReadParam *pM,
   const F32      *pResDiv
);

// Hacky mode tests
int testADS1x15
(
   const int maxSamples,
   const LXI2CBusCtx *pC,
   const MemBuff      *pWS,
   const ADSInstProp *pP,
   const ADSReadParam *pM
);

#endif // ADS1X_TEST

#endif // ADS1X_DEV_H
