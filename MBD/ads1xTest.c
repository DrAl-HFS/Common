// Common/MBD/ads1xTest.c - test & debug code for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Sept 2020

#include "ads1xTest.h"
#include <stdio.h> // -> REPORT !


/***/

void ads1xTranslateCfg (ADS1xTrans *pT, const U8 cfg[2], const U8 x)
{
   pT->gainFSV= ads1xGainToFSV( ads1xGetGain(cfg) );
   pT->rate= ads1xRateToU( ads1xGetRate(cfg), x);
   pT->m4x4= ads1xMuxToM4X4( ads1xGetMux(cfg) );
   pT->cmp= (((cfg[1] >> ADS1X_SH1_CQ) & ADS1X_CMP_M) + 1 ) & ADS1X_CMP_M; // rotate by +1 so 0->off
} // ads1xTranslateCfg

// Conversion interval (us)
int ads1xConvIvl (const U8 cfg[2], const U8 x)
{
   return(1000000 / ads1xRateToU( ads1xGetRate(cfg), x));
} // ads1xConvIvl

F32 ads1xGainScaleV (const U8 cfg[2], const U8 x)
{
   static const F32 rawFS[]= { 1.0 / ADS10_FSR, 1.0 / ADS11_FSR };
   if ((x & 1) != x) { return(0); } //else
   return ads1xGainToFSV( ads1xGetGain(cfg) ) * rawFS[x];
} // ads1xGainScaleV

// DEBUG & TEST
char muxCh (const U8 c) { if (c<=3) return('0'+c); else return('G'); }

void printMux4x4 (const U8 m4x4) { printf("%c/%c", muxCh(m4x4 >> 4), muxCh(m4x4 & 0xF)); }

void ads1xDumpCfg (const U8 cfg[2], const U8 x)
{
   ADS1xTrans t;
   printf("[%02x, %02x] = ", cfg[0], cfg[1]); // NB: Big Endian on-the-wire
   printf(" OS%d MUX%d PGA%d M%d, ", (cfg[0]>>7) & 0x1, (cfg[0]>>4) & 0x7, (cfg[0]>>1) & 0x7, cfg[0] & 0x1);
   printf(" DR%d CM%d CP%d CL%d CQ%d : ", (cfg[1]>>5) & 0x7, (cfg[1]>>4) & 0x1, (cfg[1]>>3) & 0x1, (cfg[1]>>2) & 0x1, cfg[1] & 0x3);
   ads1xTranslateCfg(&t, cfg, x);
   printMux4x4(t.m4x4);
   printf(" %GV, %d/s C:%d\n", t.gainFSV, t.rate, t.cmp);
} // ads1xDumpCfg

int ads1xInitRB (ADS1xRB *pRB, const MemBuff *pWS, const LXI2CBusCtx *pC, const U8 dev)
{  // setup i2c-reg id's in frames
   pRB->res[0]= ADS1X_REG_RES;
   pRB->cfg[0]= ADS1X_REG_CFG;
   pRB->cLo[0]= ADS1X_REG_CLO;
   pRB->cHi[0]= ADS1X_REG_CHI;
   if (pC && dev)
   {  // Get actual device data if possible
      return lxi2cReadMultiRB(pC, pWS, dev, pRB->res, ADS1X_NRB, 4);
   }
   return(0);
} // ads1xInitRB

int testADS1x15
(
   const LXI2CBusCtx *pC,
   const MemBuff *pWS,
   const U8 dev,
   const U8 x,
   const U8 mode,
   const U8 maxIter
)
{
   ADS1xRB rb;
   const int i2cWait= ADS1X_TRANS_NCLK * 1E6 / pC->clk;
   int convWait=0, expectWait=0, minWaitStep=10;
   int r;
   float sv;
   U8 cfgStatus[ADS1X_NRB];

   if ((x & 1) != x) { return(0); }
   r= ads1xInitRB(&rb, pWS, pC, dev);
   if (r >= 0)
   {
      //const U8 xRate[]={ADS10_R920, ADS11_R860};
      const U8 xRate[]={ADS10_R128, ADS11_R8};

      ads1xDumpCfg(rb.cfg+1, 0);
      memcpy(cfgStatus, rb.cfg, ADS1X_NRB);
      ads10GenCfg(rb.cfg+1, ADS1X_M0G, ADS1X_GFS_6V144, xRate[x], ADS1X_CMP_DISABLE);
      rb.cfg[1]|= ADS1X_FL0_OS|ADS1X_FL0_MODE; // Now enable single-shot conversion
      convWait= ads1xConvIvl(rb.cfg+1, x);
      sv= ads1xGainScaleV(rb.cfg+1, x);
      printf("cfg: "); ads1xDumpCfg(rb.cfg+1, x);
      printf("Ivl: conv=%dus comm=%dus\n", convWait, i2cWait);
      if (mode & ADS1X_TEST_MODE_SLEEP)
      {
         int nDelay= 1+bitCountZ(mode & (ADS1X_TEST_MODE_VERIFY|ADS1X_TEST_MODE_POLL));
         int i2cDelay= nDelay * i2cWait;
         if (convWait > i2cDelay) { expectWait= convWait-i2cDelay; } // Hacky : conversion time seems less than sample interval...
         if (i2cWait < minWaitStep) { minWaitStep= i2cWait; }
      }
      {
         I16 v[3];
         v[0]= rdI16BE(rb.res+1);
         v[1]= rdI16BE(rb.cLo+1);
         v[2]= rdI16BE(rb.cHi+1);
         printf("res: %04x (%d) cmp: %04x %04x (%d %d)\n", v[0], v[0], v[1], v[2], v[1], v[2]);
      }
      if (r >= 0)
      {
         U8 n= 0;
         printf("***\n");
         do
         {
            int i0= 0, i1= 0;
            do
            {
               r= lxi2cWriteRB(pC, dev, rb.cfg, ADS1X_NRB);
               if ((mode & ADS1X_TEST_MODE_VERIFY) && (r >= 0))
               {
                  r= lxi2cReadRB(pC, dev, cfgStatus, ADS1X_NRB);
                  if (r >= 0)
                  {
                     if (mode & ADS1X_TEST_MODE_VERBOSE) { printf("ver%d: ", i0); ads1xDumpCfg(cfgStatus+1, x); }
                     // Device goes busy (OS1->0) immediately on write, so merge back in for check
                     cfgStatus[1] |= (rb.cfg[1] & ADS1X_FL0_OS);
                  }
               }
            } while ((mode & ADS1X_TEST_MODE_VERIFY) && ((r < 0) || (0 != memcmp(cfgStatus, rb.cfg, ADS1X_NRB))) && (++i0 < 10));

            if (expectWait > 0) { usleep(expectWait); }

            if (mode & ADS1X_TEST_MODE_POLL)
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
               printf(" (i=%d,%d) W%d [%d] : 0x%x -> %GV\n", i0, i1, expectWait, n, raw, v);
               if (mode & ADS1X_TEST_MODE_ROTMUX)
               {
                  static const U8 muxRot[]= { ADS1X_M0G, ADS1X_M1G, ADS1X_M2G, ADS1X_M3G };
                  const int iNM= (1+n) & 0x3;
                  ads1xSetMux(rb.cfg+1, muxRot[iNM]);
                  if (mode & ADS1X_TEST_MODE_VERBOSE)
                  {
                     m4x4= ads1xMuxToM4X4( muxRot[iNM] );
                     printf("%d -> 0x%02x -> ", iNM, muxRot[iNM]); printMux4x4(m4x4); printf(" verify: ");
                     m4x4= ads1xMuxToM4X4(ads1xGetMux(rb.cfg+1));
                     printMux4x4(m4x4); printf("\n");
                  }
               }
            }
            if ((mode & ADS1X_TEST_MODE_TUNE) && (0 == i0))
            {
               if (expectWait > minWaitStep)
               {
                  if (0 == i1)
                  {
                     if (expectWait > (convWait>>1)) { expectWait-= i2cWait; } else { expectWait-= minWaitStep; }
                  }
                  else { expectWait+= i2cWait; }
               }
            }
         } while (++n < maxIter);
         printf("---\n");
      }
   }
   return(r);
} // testADS1x15

#ifdef ADS1X_MAIN

LXI2CBusCtx gBusCtx={0,-1};

int main (int argc, char *argv[])
{
   if (lxi2cOpen(&gBusCtx, "/dev/i2c-1", 400))
   {
      const U8 adcHWV=0, adcMF= ADS1X_TEST_MODE_VERIFY|ADS1X_TEST_MODE_SLEEP|ADS1X_TEST_MODE_POLL|ADS1X_TEST_MODE_ROTMUX;

      //MemBuff ws={0,};
      //allocMemBuff(&ws, 4<<10);//
      testADS1x15(&gBusCtx, NULL, 0x48, adcHWV, adcMF, 100);
      //releaseMemBuff(&ws);
      lxi2cClose(&gBusCtx);
   }

   return(0);
} // main

#endif // ADS1X_MAIN
