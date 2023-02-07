// Common/MBD/ads1xUtil.h - low level utility code for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: AGPL3
// (c) Project Contributors Aug-Sept. 2020

#ifndef ADS1X_UTIL_H
#define ADS1X_UTIL_H

#include "ads1x.h"
#include "mbdUtil.h"


/***/

// Device installation reference data: deliberately transport-agnostic
// (I2C bus device/interface descriptors belong to higher level).
// Additional input conditioning (e.g. voltage divider)
// might be described/linked to this struct?
#define ADS_INST_FLAGS_DEFAULT 0
typedef struct
{
   F32 vdd; // Supply voltage, determines useful gain ID (measurement range)
   U8 hwID; // Hardware version
   U8 busAddr; // I2C bus address
   U8 minUGID, maxUGID; // Limits of useful gainID (derived from vdd)
} ADSInstProp;


/***/


//extern ADSInstProp gDefaultADSIP;

/***/

#ifndef INLINE

// Set/get field within local copy (no transport provided in this module)
extern enum ADS1xMux ads1xGetMux (const U8 cfg[1]);   // Mux & Gain in 1st byte
extern enum ADS1xGain ads1xGetGain(const U8 cfg[1]);
extern enum ADS1xRate ads1xGetRate (const U8 cfg[2]); // Rate & Cmp in 2nd byte

extern void ads1xSetMux (U8 cfg[1], const enum ADS1xMux muxID);
extern void ads1xSetGain (U8 cfg[1], const enum ADS1xGain gainID);
extern void ads1xSetRate (U8 cfg[2], const enum ADS1xRate rateID);

// Generate complete first byte of config register
extern void ads1xGenCfgRB0 (U8 cfg[1], enum ADS1xMux muxID, enum ADS1xGain gainID, enum ADS1xFlag0 f);

// Overwrite both control bytes leaving all flags unset
extern void ads1xGenCfg (U8 cfg[2], enum ADS1xMux muxID, enum ADS1xGain gainID, enum ADS1xRate rateID, enum ADS1xCompare cmpID);

#else // INLINE

INLINE enum ADS1xMux ads1xGetMux (const U8 cfg[1]) { return((cfg[0] >> ADS1X_SH0_MUX) & ADS1X_MUX_M); }
INLINE enum ADS1xGain ads1xGetGain (const U8 cfg[1]) { return((cfg[0] >> ADS1X_SH0_GAIN) & ADS1X_GAIN_M); }
INLINE enum ADS1xRate ads1xGetRate (const U8 cfg[2]) { return((cfg[1] >> ADS1X_SH1_DR) & ADS1X_DR_M); }

INLINE void ads1xSetMux (U8 cfg[1], enum ADS1xMux muxID) { cfg[0]= setMaskU8(cfg[0], ADS1X_MUX_M, muxID, ADS1X_SH0_MUX); }
INLINE void ads1xSetGain (U8 cfg[1], enum ADS1xGain gainID) { cfg[0]= setMaskU8(cfg[0], ADS1X_GAIN_M, gainID, ADS1X_SH0_GAIN); }
INLINE void ads1xSetRate (U8 cfg[2], enum ADS1xRate rateID) { cfg[1]= setMaskU8(cfg[1], ADS1X_DR_M, rateID, ADS1X_SH1_DR); }

INLINE void ads1xGenCfgRB0 (U8 cfgRB[1], enum ADS1xMux muxID, enum ADS1xGain gainID, enum ADS1xFlag0 f)
{ cfgRB[0]= (muxID << ADS1X_SH0_MUX) | (gainID << ADS1X_SH0_GAIN) | f; }

INLINE void ads1xGenCfg (U8 cfg[2], enum ADS1xMux muxID, enum ADS1xGain gainID, enum ADS1xRate rateID, enum ADS1xCompare cmpID)
{ cfg[0]= (muxID << ADS1X_SH0_MUX) | (gainID << ADS1X_SH0_GAIN); cfg[1]= (rateID << ADS1X_SH1_DR) | (cmpID << ADS1X_SH1_CQ); }

#endif // INLINE

// Lookup Full Scale Volts associated with gain setting
extern F32 ads1xGainToFSV (enum ADS1xGain gainID);

// Lookup sample rate for hardware & setting
extern U16 ads1xRateToU (enum ADS1xRate rate, ADS1xHWID hwID);

// Search for suitable rateID to deliver data at specified rate
extern enum ADS1xRate ads1xSelectRate (const int targetRate, ADS1xHWID hwID);

// Lookup Full Scale Raw reading value for particular hardware
extern I16 ads1xRawFSR (ADS1xHWID hwID);

// Initialise device installation properties (determined by device type and actual supply voltage)
extern const ADSInstProp *adsInitProp (ADSInstProp *pP, const F32 vdd, const U8 hwID, const U8 busAddr);

#endif // ADS1X_UTIL_H
