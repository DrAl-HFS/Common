// Common/MBD/ads1xAuto.h - automated reading of TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: AGPL3
// (c) Project Contributors June 2021

#ifndef ADS1X_AUTO_H
#define ADS1X_AUTO_H

#include "ads1xUtil.h"
#include "lxI2C.h"
#include "lxTiming.h"


/***/

#define ADS1X_MUX_MAX 8

// Auto-gain raw reading group parameters
typedef struct
{
   const LXI2CBusCtx *pC;
   long  ivlNanoSec[2]; // Conversion interval (hardware rate dependant 303usec~125msec. for 400kHz bus clock)
   I16   fsr;    // Full scale raw reading (value is hardware version dependant)
   U8    busAddr; // I2C device address
   U8    maxTrans;    // Max transactions per mux channel
   U8    nMux;
   U8    cfgPB[ADS1X_NRB]; // Config packet bytes
} AutoRawCtx;

#define AGR_FLAG_AUTO 1<<7 // Enable auto gain
#define AGR_FLAG_SGND 1<<6 // Mux selects single ended operation
#define AGR_FLAG_VROK 1<<5 // Valid result
#define AGR_FLAG_ORNG 1<<4 // out of range (uncorrectable over/under-flow)
#define RMG_MASK_TRNS 0xF  // Transaction count mask
typedef struct
{
   I16 res;       // raw result (target hardware format)
   U8  cfgRB0[1]; // First byte of config register: describes mux gain and single/multi conversion control
   U8  flSt;      // flags (see above) in high nybble & 4bit transaction count in low
} RawAGR;

typedef struct
{
   AutoRawCtx arc;
   RawAGR      rawAGR[ADS1X_MUX_MAX];
   long        outerIvlNanoSec;
   RawTimeStamp targetTS[2];
} AutoExtCtx;

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

#define EXT_RTS_COUNT (5)
#define EXT_RTS_MUXCH_BGN (4)
#define EXT_RTS_WRCFG_BGN (3)
#define EXT_RTS_WRCFG_END (2)
#define EXT_RTS_RDVAL_BGN (1)
#define EXT_RTS_RDVAL_END (0)

typedef struct {  // TODO: reverse order
   RawTimeStamp ts[EXT_RTS_COUNT]; // channel start plus begin-end for two transactions (setup & read)
} ExtRawTiming;


/***/

extern long ads1xConvIvl (U16 *pRate, const U8 cfg[2], const ADS1xHWID id);
extern F32 ads1xGainScaleV (const U8 cfg[1], const ADS1xHWID x);

extern int readAutoRawADS1x (RawAGR agr[], ExtRawTiming *pT, int nR, RawTimeStamp *pTarget, AutoRawCtx *pARC);

extern U8 setupRawAGR (RawAGR r[], const U8 mux[], U8 n, const U8 maskAG, const enum ADS1xGain initGain);

extern int setupAEC (AutoExtCtx *pAEC, const ADSInstProp *pP, const ADSReadParam *pM, const U8 * pCfgPB, const LXI2CBusCtx *pC);


#endif // ADS1X_AUTO_H
