// Common/MBD/ads1xUtil.c - low level utility code for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug-Sept. 2020

#include "ads1xUtil.h"
#include <stddef.h> // ???


/***/

// Hardware capability (singleton) reference data
#define ADS1X_GAIN_NID 6
#define ADS1X_RATE_NID 8
#define ADS1X_HW_NID 2
typedef struct
{
   F32 gainFSV[ADS1X_GAIN_NID];   // [gainID]
   U16 rateU[ADS1X_HW_NID][ADS1X_RATE_NID];  // [hwID][rateID]
   I16 rawFS[ADS1X_HW_NID];     // [hwID]
} ADS1XHardCap;


/***/

static const ADS1XHardCap gHC=
{
   { 6.144, 4.096, 2.048, 1.024, 0.512, 0.256 },   // gainFSV[gainID]
   {
      { 128, 250, 490,  920,  1600, 2400, 3300,   3300 }, // rateU[hwID][rateID]
      { 8,  16,   32,   64,   128,  250,  475,  860 }
   },
   { ADS10_FSR, ADS11_FSR } // rawFS[hwID]
};

ADSInstProp gDefaultADSIP= { 3.3, ADS11, ADS_INST_FLAGS_DEFAULT, ADS1X_GAIN_4V096, ADS1X_GAIN_0V256 };


/***/

#ifndef INLINE

enum ADS1xMux ads1xGetMux (const U8 cfg[1]) { return((cfg[0] >> ADS1X_SH0_MUX) & ADS1X_MUX_M); }
enum ADS1xGain ads1xGetGain (const U8 cfg[1]) { return((cfg[0] >> ADS1X_SH0_GAIN) & ADS1X_GAIN_M); }
enum ADS1xRate ads1xGetRate (const U8 cfg[2]) { return((cfg[1] >> ADS1X_SH1_DR) & ADS1X_DR_M); }

void ads1xSetMux (U8 cfg[1], enum ADS1xMux muxID)
{
   cfg[0]= setMaskU8(cfg[0], ADS1X_MUX_M, muIDx, ADS1X_SH0_MUX);
   //cfg[0]= (cfg[0] & ~(ADS1X_MUX_M << ADS1X_SH0_MUX)) | (mux << ADS1X_SH0_MUX);
}
void ads1xSetGain (U8 cfg[1], enum ADS1xGain gainID)
{
   cfg[0]= setMaskU8(cfg[0], ADS1X_GAIN_M, gainID, ADS1X_SH0_GAIN);
   //cfg[0]= (cfg[0] & ~(ADS1X_GAIN_M << ADS1X_SH0_GAIN)) | (gain << ADS1X_SH0_GAIN);
}
void ads1xSetRate (U8 cfg[2], enum ADS1xRate rateID)
{
   cfg[1]= setMaskU8(cfg[1], ADS1X_DR_M, rateID, ADS1X_SH1_DR);
   //cfg[1]= (cfg[1] & ~(ADS1X_DR_M << ADS1X_SH1_DR)) | (rate << ADS1X_SH1_DR);
}

void ads1xGenCfgRB0 (U8 cfgRB[1], enum ADS1xMux muxID, enum ADS1xGain gainID, enum ADS1xFlag0 f)
{
   cfgRB[0]= (muxID << ADS1X_SH0_MUX) | (gainID << ADS1X_SH0_GAIN) | f;
} // ads1xGenCfgRB0

void ads1xGenCfg (U8 cfg[2], enum ADS1xMux muxID, enum ADS1xGain gainID, enum ADS1xRate rateID, enum ADS1xCompare cmpID)
{
   cfg[0]= (muxID << ADS1X_SH0_MUX) | (gainID << ADS1X_SH0_GAIN);
   cfg[1]= (rateID << ADS1X_SH1_DR) | (cmpID << ADS1X_SH1_CQ);
} // ads1xGenCfg

#endif // ifndef INLINE

F32 ads1xGainToFSV (enum ADS1xGain gainID)
{
   if (gainID < ADS1X_GAIN_0V256) return(gHC.gainFSV[gainID]); else return(0.256);
} // ads1xGainToFSV

U16 ads1xRateToU (enum ADS1xRate rateID, ADS1xHWID hwID)
{
   return( gHC.rateU[ hwID ][ rateID ] );
} // ads1xRateToU

enum ADS1xRate ads1xSelectRate (const int targetRate, ADS1xHWID hwID)
{
   U8 i=0;
   if (targetRate > 0)
   {
      const U16 *pR= gHC.rateU[ hwID ];
      while ((pR[i] < targetRate) && (i < (ADS1X_RATE_NID-1))) { ++i; }
   }
   return(i);
} // ads1xSelectRateID

I16 ads1xRawFSR (ADS1xHWID hwID)
{
   //static const I16 fsr[]={ ADS10_FSR, ADS11_FSR };
   return(gHC.rawFS[hwID]);
} // ads1xRawFSR

const ADSInstProp * adsInitProp (ADSInstProp *pP, const F32 vdd, const U8 hwID)
{
   U8 i=0;
   while ((gHC.gainFSV[i+1] > vdd) && (i < (ADS1X_GAIN_NID-2))) { ++i; }
   if (NULL == pP) { pP= &gDefaultADSIP; }
   pP->minUGID= i;
   pP->maxUGID= ADS1X_GAIN_0V256;
   pP->hwID= hwID;
   pP->vdd= vdd;
   return(pP);
} // adsInitProp
