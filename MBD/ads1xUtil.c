// Common/MBD/ads1xUtil.c - utility code for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug 2020

#include "ads1xUtil.h"
#include <stdio.h> // -> report !
/*
typedef struct
{
   F32 gainFSV;
   U16 rate;
   U8 m4x4;
   U8 cmp;
} ADS1xUnpack;

#define ADS1X_NRB (3)
typedef struct
{
   U8 res[ADS1X_NRB], cfg[ADS1X_NRB], cLo[ADS1X_NRB], cHi[ADS1X_NRB];
} ADS1xRB;
*/

/***/

int ads1xInitRB (ADS1xRB *pRB, const MemBuff *pWS, const LXI2CBusCtx *pC, const U8 dev)
{
   pRB->res[0]= ADS1X_REG_RES;
   pRB->cfg[0]= ADS1X_REG_CFG;
   pRB->cLo[0]= ADS1X_REG_CLO;
   pRB->cHi[0]= ADS1X_REG_CHI;
   if (pC && dev)
   {
#if 1
      lxi2cReadMultiRB(pC, pWS, dev, pRB->res, ADS1X_NRB, 4);
#else
      lxi2cReadRB(pC, dev, pRB->res, ADS1X_NRB);
      lxi2cReadRB(pC, dev, pRB->cfg, ADS1X_NRB);
      lxi2cReadRB(pC, dev, pRB->cLo, ADS1X_NRB);
      lxi2cReadRB(pC, dev, pRB->cHi, ADS1X_NRB);
#endif
   }
   return(0);
} // ads1xInitRB

U8 ads1xGetMux (const U8 cfg[2]) { return((cfg[0] >> ADS1X_SH0_MUX) & ADS1X_M_M); }

void ads1xSetMux (U8 cfg[2], const enum ADS1xMux mux) { cfg[0]= (cfg[0] & ~(ADS1X_M_M << ADS1X_SH0_MUX)) | (mux << ADS1X_SH0_MUX); }

U8 ads1xMuxToM4X4 (const enum ADS1xMux mux)
{
   static const U8 m4x4[]= {0x01, 0x03, 0x13, 0x23, 0x0F, 0x1F, 0x2F, 0x3F};
   return m4x4[ mux & 0x7 ];
} // ads1xGetMux4X4
char muxCh (const U8 c) { if (c<=3) return('0'+c); else return('G'); }
void printMux4x4 (const U8 m4x4) { printf("%c/%c", muxCh(m4x4 >> 4), muxCh(m4x4 & 0xF)); }

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
   return( rate[ x & 1 ][ r & 0x7 ] );
} // ads1xRateToU

void ads1xUnpackCfg (ADS1xUnpack *pU, const U8 cfg[2], const U8 x)
{
   pU->gainFSV= ads1xGainToFSV( (cfg[0] >> ADS1X_SH0_PGA) & ADS1X_GFS_M );
   pU->rate= ads1xRateToU( (cfg[1] >> ADS1X_SH1_DR) & ADS10_R_M, x);
   pU->m4x4= ads1xMuxToM4X4(ads1xGetMux(cfg));
   pU->cmp= (((cfg[1] >> ADS1X_SH1_CQ) & ADS1X_CMP_M) + 1 ) & ADS1X_CMP_M;
} // adsExtCfg

int convInterval (ADS1xUnpack *pU, const U8 cfg[2], const U8 x)
{
   if (pU)
   {
      ads1xUnpackCfg(pU, cfg, x);
      if (pU->rate > 0) return(1000000 / pU->rate);
   }
   return(0);
} // convInterval

void ads1xDumpCfg (const U8 cfg[2], const U8 x)
{
   ADS1xUnpack u;
   printf("[%02x, %02x] = ", cfg[0], cfg[1]); // NB: Big Endian on-the-wire
   printf(" OS%d MUX%d PGA%d M%d, ", (cfg[0]>>7) & 0x1, (cfg[0]>>4) & 0x7, (cfg[0]>>1) & 0x7, cfg[0] & 0x1);
   printf(" DR%d CM%d CP%d CL%d CQ%d : ", (cfg[1]>>5) & 0x7, (cfg[1]>>4) & 0x1, (cfg[1]>>3) & 0x1, (cfg[1]>>2) & 0x1, cfg[1] & 0x3);
   ads1xUnpackCfg(&u, cfg, x);
   printMux4x4(u.m4x4);
   printf(" %GV, %d/s C:%d\n",u.gainFSV, u.rate, u.cmp);
} // ads1xDumpCfg

void ads10GenCfg (U8 cfg[2], enum ADS1xMux mux, enum ADS1xGain gain, enum ADS10Rate rate, enum ADS1xCompare cmp)//=ADS1X_CD
{
   cfg[0]= (mux << ADS1X_SH0_MUX) | (gain << ADS1X_SH0_PGA);
   cfg[1]= (rate << ADS1X_SH1_DR) | (cmp << ADS1X_SH1_CQ);
} // ads10SetCfg

//#ifdef ADS1X_TEST

//#include "lxI2C.h"

#define MODE_VERIFY  (1<<7)
#define MODE_SLEEP   (1<<6)
#define MODE_POLL    (1<<5)
#define MODE_ROTMUX  (1<<4)
#define MODE_VERBOSE (1<<0)

int testADS1015 (const LXI2CBusCtx *pC, const MemBuff *pWS, const U8 dev, const U8 mode)
{
   ADS1xRB rb;
   ADS1xUnpack u;
   const int i2cWait= ADS1X_TRANS_NCLK * 1E6 / pC->clk;
   int convWait=0, expectWait=0;
   int r;
   float sv;
   U8 cfgStatus[ADS1X_NRB];

   r= ads1xInitRB(&rb, pWS, pC, dev);
   ads1xDumpCfg(rb.cfg+1, 0);
   if (r >= 0)
   {
      memcpy(cfgStatus, rb.cfg, ADS1X_NRB);
      ads10GenCfg(rb.cfg+1, ADS1X_M0G, ADS1X_GFS_6V144, ADS10_R2400, ADS1X_CMP_DISABLE);
      rb.cfg[1]|= ADS1X_FL0_OS|ADS1X_FL0_MODE; // Now enable single-shot conversion
      convWait= convInterval(&u, rb.cfg+1, 0);
      sv= u.gainFSV / ADS10_FSR;
      printf("cfg: "); ads1xDumpCfg(rb.cfg+1, 0);
      printf("Ivl: conv=%dus comm=%dus\n", convWait, i2cWait);
      {
         I16 v[3];
         v[0]= rdI16BE(rb.res+1);
         v[1]= rdI16BE(rb.cLo+1);
         v[2]= rdI16BE(rb.cHi+1);
         printf("res: %04x (%d) cmp: %04x %04x (%d %d)\n", v[0], v[0], v[1], v[2], v[1], v[2]);
      }
      if (r >= 0)
      {
         int n= 0;
         printf("***\n");
         do
         {
            int i0= 0, i1= 0;
            do
            {
               r= lxi2cWriteRB(pC, dev, rb.cfg, ADS1X_NRB);
               if ((mode & MODE_VERIFY) && (r >= 0))
               {
                  r= lxi2cReadRB(pC, dev, cfgStatus, ADS1X_NRB);
                  if (r >= 0)
                  {
                     if (mode & MODE_VERBOSE) { printf("ver%d: ", i0); ads1xDumpCfg(cfgStatus+1, 0); }
                     // Device goes busy (OS1->0) immediately on write, so merge back in for check
                     cfgStatus[1] |= (rb.cfg[1] & ADS1X_FL0_OS);
                  }
               }
            } while ((mode & MODE_VERIFY) && ((r < 0) || (0 != memcmp(cfgStatus, rb.cfg, ADS1X_NRB))) && (++i0 < 10));

            if (mode & MODE_SLEEP)
            {
               usleep(convWait-i2cWait);
               expectWait= (1+i0) * i2cWait;
               if (convWait > expectWait); // { usleep(convWait-expectWait); }
            }

            if (mode & MODE_POLL)
            {
               do { // poll status
                  r= lxi2cReadRB(pC, dev, cfgStatus, ADS1X_NRB);
               } while (((r < 0) || (0 == (cfgStatus[1] & ADS1X_FL0_OS))) && (++i1 < 10));
            }

            r= lxi2cReadRB(pC, dev, rb.res, ADS1X_NRB); // read result
            if (r >= 0)
            {
               int raw=  rdI16BE(rb.res+1);
               float v= raw * sv;
               U8 m4x4= ads1xMuxToM4X4(ads1xGetMux(cfgStatus+1));
               printMux4x4(m4x4);
               printf(" (i=%d,%d) [%d] : 0x%x -> %GV\n", i0, i1, n, raw, v);
               if (mode & MODE_ROTMUX)
               {
                  static const U8 muxRot[]= { ADS1X_M0G, ADS1X_M1G, ADS1X_M2G, ADS1X_M3G };
                  const int iNM= (1+n) & 0x3;
                  ads1xSetMux(rb.cfg+1, muxRot[iNM]);
                  if (mode & MODE_VERBOSE)
                  {
                     m4x4= ads1xMuxToM4X4( muxRot[iNM] );
                     printf("%d -> 0x%02x -> ", iNM, muxRot[iNM]); printMux4x4(m4x4); printf(" verify: ");
                     m4x4= ads1xMuxToM4X4(ads1xGetMux(rb.cfg+1));
                     printMux4x4(m4x4); printf("\n");
                  }
               }
            }
         } while (++n < 9);
         printf("---\n");
      }
   }
   return(r);
} // testADS1015

//#endif // ADS1X_TEST
