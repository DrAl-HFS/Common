// Common/MBD/ads1xUtil.c - minimal utility code for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug-Sept. 2020

#include "ads1xUtil.h"


/***/

/*- Suitable for inlining  -*/
enum ADS1xMux ads1xGetMux (const U8 cfg[2]) { return((cfg[0] >> ADS1X_SH0_MUX) & ADS1X_MUX_M); }
enum ADS1xGain ads1xGetGain (const U8 cfg[2]) { return((cfg[0] >> ADS1X_SH0_GAIN) & ADS1X_GAIN_M); }
enum ADS1xRate ads1xGetRate (const U8 cfg[2]) { return((cfg[1] >> ADS1X_SH1_DR) & ADS1X_DR_M); }

// CONSIDER?? : cfg[0]= setMaskU8(cfg[0], ADS1X_M_M, mux, ADS1X_SH0_MUX);
void ads1xSetMux (U8 cfg[2], const enum ADS1xMux mux) { cfg[0]= (cfg[0] & ~(ADS1X_MUX_M << ADS1X_SH0_MUX)) | (mux << ADS1X_SH0_MUX); }
void ads1xSetGain (U8 cfg[2], const enum ADS1xGain gain) { cfg[0]= (cfg[0] & ~(ADS1X_GAIN_M << ADS1X_SH0_GAIN)) | (gain << ADS1X_SH0_GAIN); }
void ads1xSetRate (U8 cfg[2], const enum ADS1xRate rate) { cfg[1]= (cfg[1] & ~(ADS1X_DR_M << ADS1X_SH1_DR)) | (rate << ADS1X_SH1_DR); }

void ads10GenCfg (U8 cfg[2], enum ADS1xMux mux, enum ADS1xGain gain, enum ADS1xRate rate, enum ADS1xCompare cmp)//=ADS1X_CD
{
   cfg[0]= (mux << ADS1X_SH0_MUX) | (gain << ADS1X_SH0_GAIN);
   cfg[1]= (rate << ADS1X_SH1_DR) | (cmp << ADS1X_SH1_CQ);
} // ads10SetCfg

/*- - -*/

// DEPRECATE
U8 ads1xMuxToM4X4 (const enum ADS1xMux mux)
{
   static const U8 m4x4[]= {0x01, 0x03, 0x13, 0x23, 0x0F, 0x1F, 0x2F, 0x3F};
   return m4x4[ mux & 0x7 ];
} // ads1xGetMux4X4

F32 ads1xGainToFSV (const enum ADS1xGain gain)
{
   static const F32 gainFSV[]= { 6.144, 4.096, 2.048, 1.024, 0.512}; //, 0.256 };
   if (gain < ADS1X_GAIN_0V256) return(gainFSV[gain]); else return(0.256);
} // ads1xGainToFSV

U16 ads1xRateToU (const enum ADS1xRate rate, const U8 x)
{
static const U16 rateU[2][8]=
   {  {128,250,490,920,1600,2400,3300,3300},
      {8,16,32,64,128,250,475,860} };
   if ((x & 1) != x) { return(0); } //else
   return( rateU[ x ][ rate ] );
} // ads1xRateToU
