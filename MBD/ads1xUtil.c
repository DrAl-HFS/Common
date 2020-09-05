// Common/MBD/ads1xUtil.c - minimal utility code for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug-Sept. 2020

#include "ads1xUtil.h"


/***/

/*- Suitable for inlining  -*/
enum ADS1xMux ads1xGetMux (const U8 cfg[2]) { return((cfg[0] >> ADS1X_SH0_MUX) & ADS1X_M_M); }
enum ADS1xGain ads1xGetGain (const U8 cfg[2]) { return((cfg[0] >> ADS1X_SH0_PGA) & ADS1X_GFS_M); }
U8 ads1xGetRate (const U8 cfg[2]) { return((cfg[1] >> ADS1X_SH1_DR) & ADS10_R_M); }

// CONSIDER?? : cfg[0]= setMaskU8(cfg[0], ADS1X_M_M, mux, ADS1X_SH0_MUX);
void ads1xSetMux (U8 cfg[2], const enum ADS1xMux mux) { cfg[0]= (cfg[0] & ~(ADS1X_M_M << ADS1X_SH0_MUX)) | (mux << ADS1X_SH0_MUX); }
void ads1xSetGain (U8 cfg[2], const enum ADS1xGain gain) { cfg[0]= (cfg[0] & ~(ADS1X_GFS_M << ADS1X_SH0_PGA)) | (gain << ADS1X_SH0_PGA); }
void ads10SetRate (U8 cfg[2], const enum ADS10Rate rate) { cfg[1]= (cfg[1] & ~(ADS10_R_M << ADS1X_SH1_DR)) | (rate << ADS1X_SH1_DR); }
void ads11SetRate (U8 cfg[2], const enum ADS11Rate rate) { cfg[1]= (cfg[1] & ~(ADS10_R_M << ADS1X_SH1_DR)) | (rate << ADS1X_SH1_DR); }

void ads10GenCfg (U8 cfg[2], enum ADS1xMux mux, enum ADS1xGain gain, enum ADS10Rate rate, enum ADS1xCompare cmp)//=ADS1X_CD
{
   cfg[0]= (mux << ADS1X_SH0_MUX) | (gain << ADS1X_SH0_PGA);
   cfg[1]= (rate << ADS1X_SH1_DR) | (cmp << ADS1X_SH1_CQ);
} // ads10SetCfg

/*- - -*/

U8 ads1xMuxToM4X4 (const enum ADS1xMux mux)
{
   static const U8 m4x4[]= {0x01, 0x03, 0x13, 0x23, 0x0F, 0x1F, 0x2F, 0x3F};
   return m4x4[ mux & 0x7 ];
} // ads1xGetMux4X4

F32 ads1xGainToFSV (const enum ADS1xGain g)
{
   static const F32 gfsv[]= { 6.144, 4.096, 2.048, 1.024, 0.512}; //, 0.256 };
   if (g < ADS1X_GFS_0V256) return(gfsv[g]); else return(0.256);
} // ads1xGainToFSV

U16 ads1xRateToU (const U8 r, const U8 x)
{
static const U16 rate[2][8]=
   {  {128,250,490,920,1600,2400,3300,3300},
      {8,16,32,64,128,250,475,860} };
   if (((x & 1) != x) || ((r & 0x7) != r)) { return(0); } //else
   return( rate[ x ][ r ] );
} // ads1xRateToU
