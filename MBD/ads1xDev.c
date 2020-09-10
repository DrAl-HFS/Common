// Common/MBD/ads1xDev.c - development, test & debug code for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Sept 2020

#include "ads1xDev.h"
#include <stdio.h> // -> REPORT !


/***/

void ads1xTranslateCfg (ADS1xTrans *pT, const U8 cfg[2], const ADS1xHWID id)
{
   pT->gainFSV= ads1xGainToFSV( ads1xGetGain(cfg) );
   pT->rate= ads1xRateToU( ads1xGetRate(cfg), id);
   pT->cmp= (((cfg[1] >> ADS1X_SH1_CQ) & ADS1X_CMP_M) + 1 ) & ADS1X_CMP_M; // rotate by +1 so 0->off
} // ads1xTranslateCfg

// Conversion interval (us)
int ads1xConvIvl (const U8 cfg[2], const ADS1xHWID id)
{
   return(1000000 / ads1xRateToU( ads1xGetRate(cfg), id));
} // ads1xConvIvl

F32 ads1xGainScaleV (const U8 cfg[2], const ADS1xHWID x)
{
   static const F32 rawFS[]= { 1.0 / ADS10_FSR, 1.0 / ADS11_FSR };
   if ((x & 1) != x) { return(0); } //else
   return ads1xGainToFSV( ads1xGetGain(cfg) ) * rawFS[x];
} // ads1xGainScaleV

// DEBUG & TEST
//char muxCh (const U8 c) { if (c<=3) return('0'+c); else return('G'); }
//void printMux4x4 (const U8 m4x4) { printf("%c/%c", muxCh(m4x4 >> 4), muxCh(m4x4 & 0xF)); }

const char * ads1xMuxStr (const enum ADS1xMux m)
{
   static const char* muxStr[]= {"0/1", "0/3", "1/3", "2/3", "0/G", "1/G", "2/G", "3/G"};
   return muxStr[ m ];
} // ads1xMuxStr

void ads1xDumpCfg (const U8 cfg[2], const ADS1xHWID id)
{
   ADS1xTrans t;
   LOG("cfg= [%02x, %02x] = ", cfg[0], cfg[1]); // NB: Big Endian on-the-wire
   // Evil magic numbers left for insanity check...
   LOG(" OS%d MUX%d PGA%d M%d, ", (cfg[0]>>7) & 0x1, (cfg[0]>>4) & 0x7, (cfg[0]>>1) & 0x7, cfg[0] & 0x1);
   LOG(" DR%d CM%d CP%d CL%d CQ%d : ", (cfg[1]>>5) & 0x7, (cfg[1]>>4) & 0x1, (cfg[1]>>3) & 0x1, (cfg[1]>>2) & 0x1, cfg[1] & 0x3);
   //
   ads1xTranslateCfg(&t, cfg, id);
   LOG("%s %GV, %d/s C:%d\n", ads1xMuxStr(ads1xGetMux(cfg)), t.gainFSV, t.rate, t.cmp);
} // ads1xDumpCfg

int ads1xInitRB (ADS1xFullPB *pFPB, const MemBuff *pWS, const LXI2CBusCtx *pC, const U8 dev)
{  // setup i2c-reg id's in "packet" frames
   pFPB->rc.res[0]= ADS1X_REG_RES;
   pFPB->rc.cfg[0]= ADS1X_REG_CFG;
   pFPB->cLo[0]=  ADS1X_REG_CLO;
   pFPB->cHi[0]=  ADS1X_REG_CHI;
   if (pC && dev)
   {  // Get actual device data if possible
      return lxi2cReadMultiRB(pC, pWS, dev, pFPB->rc.res, ADS1X_NRB, 4);
   }
   return(0);
} // ads1xInitRB

int readAutoADS1x (F32 resV, int maxR, ADS1xRCPB *pP, const LXI2CBusCtx *pC, const U8 dev)
{
   int r=-1, n= 0;
   U8 cfg[ADS1X_NRB];
   U8 mgr[3];

   if (pP) { memcpy(cfg, pP->cfg, ADS1X_NRB); }
   else
   {
      cfg[0]= ADS1X_REG_CFG;
      r= lxi2cReadRB(pC, dev, cfg, ADS1X_NRB);
      if (r>=0)
      {

      }
   }
   mgr[0]= ads1xGetMux(cfg+1);
   mgr[1]= ads1xGetGain(cfg+1);
   mgr[2]= ads1xGetRate(cfg+1);
   if (mgr[0] >= ADS1X_MUX0G)
   { // single ended: no rev. pol.

   }
   return(n);
} // readAutoADS1x

#ifdef ADS1X_TEST

int testADS1x15
(
   const LXI2CBusCtx *pC,
   const MemBuff *pWS,
   const U8 dev,
   const ADS1xHWID id,
   const U8 mode,
   const U8 maxIter
)
{
   ADS1xFullPB fpb;
   const int i2cWait= ADS1X_TRANS_NCLK * 1E6 / pC->clk;
   int convWait=0, expectWait=0, minWaitStep=10;
   int r;
   float sv;
   U8 cfgStatus[ADS1X_NRB];

   r= ads1xInitRB(&fpb, pWS, pC, dev);
   if (r >= 0)
   {
      const enum ADS1xRate idRate[]={ADS10_DR920, ADS11_DR860};
      //const enum ADS1xRate idRate[]={ADS10_DR128, ADS11_DR8};

      ads1xDumpCfg(fpb.rc.cfg+1, 0);
      memcpy(cfgStatus, fpb.rc.cfg, ADS1X_NRB);
      ads1xGenCfg(fpb.rc.cfg+1, ADS1X_MUX0G, ADS1X_GAIN_6V144, idRate[id], ADS1X_CMP_DISABLE);
      fpb.rc.cfg[1]|= ADS1X_FL0_OS|ADS1X_FL0_MODE; // Now enable single-shot conversion
      // NB: Config packet written to device in loop that follows
      convWait= ads1xConvIvl(fpb.rc.cfg+1, id);
      sv= ads1xGainScaleV(fpb.rc.cfg+1, id);
      ads1xDumpCfg(fpb.rc.cfg+1, id);
      LOG("Ivl: conv=%dus comm=%dus\n", convWait, i2cWait);
      if (mode & ADS1X_TEST_MODE_SLEEP)
      {
         int nDelay= 1+bitCountZ(mode & (ADS1X_TEST_MODE_VERIFY|ADS1X_TEST_MODE_POLL));
         int i2cDelay= nDelay * i2cWait;
         if (convWait > i2cDelay) { expectWait= convWait-i2cDelay; } // Hacky : conversion time seems less than sample interval...
         if (i2cWait < minWaitStep) { minWaitStep= i2cWait; }
      }
      {
         I16 v[3];
         v[0]= rdI16BE(fpb.rc.res+1);
         v[1]= rdI16BE(fpb.cLo+1);
         v[2]= rdI16BE(fpb.cHi+1);
         LOG("res: %04x (%d) cmp: %04x %04x (%d %d)\n", v[0], v[0], v[1], v[2], v[1], v[2]);
      }
      if (r >= 0)
      {
         U8 n= 0;
         LOG("%s\n","***");
         do
         {
            int iR0= 0, iR1= 0; // retry counters
            U8 cfgVer=FALSE;
            do
            {
               r= lxi2cWriteRB(pC, dev, fpb.rc.cfg, ADS1X_NRB);
               if ((mode & ADS1X_TEST_MODE_VERIFY) && (r >= 0))
               {
                  r= lxi2cReadRB(pC, dev, cfgStatus, ADS1X_NRB);
                  if (r >= 0)
                  {  // Device goes busy (OS1->0) immediately on write, so merge back in for check
                     cfgStatus[1] |= (fpb.rc.cfg[1] & ADS1X_FL0_OS);
                     cfgVer= (0 == memcmp(cfgStatus, fpb.rc.cfg, ADS1X_NRB));
                     if (mode & ADS1X_TEST_MODE_VERBOSE) { LOG("ver%d=%d: ", iR0, cfgVer); ads1xDumpCfg(cfgStatus+1, id); }
                  }
               }
            } while ((mode & ADS1X_TEST_MODE_VERIFY) && ((r < 0) || !cfgVer) && (++iR0 < 10));

            if (expectWait > 0) { usleep(expectWait); }

            if (mode & ADS1X_TEST_MODE_POLL)
            {
               do { // poll status
                  r= lxi2cReadRB(pC, dev, cfgStatus, ADS1X_NRB);
               } while (((r < 0) || (0 == (cfgStatus[1] & ADS1X_FL0_OS))) && (++iR1 < 10));
            }

            r= lxi2cReadRB(pC, dev, fpb.rc.res, ADS1X_NRB); // read result
            if (r >= 0)
            {
               int raw=  rdI16BE(fpb.rc.res+1);
               float v= raw * sv;
               const char muxVerCh[]={'?','V'};
               enum ADS1xMux m;
               if (cfgVer) { m= ads1xGetMux(cfgStatus+1); }
               else { m= ads1xGetMux(fpb.rc.cfg+1); }
               LOG("%s (%c) (i=%d,%d) W%d [%d] : 0x%x -> %GV\n", ads1xMuxStr(m), muxVerCh[cfgVer], iR0, iR1, expectWait, n, raw, v);
               if (mode & ADS1X_TEST_MODE_ROTMUX)
               {
                  static const U8 muxRot[]= { ADS1X_MUX0G, ADS1X_MUX1G, ADS1X_MUX2G, ADS1X_MUX3G };
                  const int iNM= (1+n) & 0x3;
                  ads1xSetMux(fpb.rc.cfg+1, muxRot[iNM]); // 99999999);
                  if (mode & ADS1X_TEST_MODE_VERBOSE)
                  {
                     LOG("%d -> 0x%02x -> %s chk: %s\n", iNM, muxRot[iNM], ads1xMuxStr(muxRot[iNM]), ads1xMuxStr(ads1xGetMux(fpb.rc.cfg+1)));
                  }
               }
            }
            if ((mode & ADS1X_TEST_MODE_TUNE) && (0 == iR0))
            {
               if (expectWait > minWaitStep)
               {
                  if (0 == iR1)
                  {
                     if (expectWait > (convWait>>1)) { expectWait-= i2cWait; } else { expectWait-= minWaitStep; }
                  }
                  else { expectWait+= i2cWait; }
               }
            }
         } while (++n < maxIter);
         LOG("%s\n","---");
      }
   }
   return(r);
} // testADS1x15

#endif // ADS1X_TEST

#ifdef ADS1X_MAIN

LXI2CBusCtx gBusCtx={0,-1};

int main (int argc, char *argv[])
{
   if (lxi2cOpen(&gBusCtx, "/dev/i2c-1", 400))
   {
      U8 adcMF= ADS1X_TEST_MODE_VERIFY|ADS1X_TEST_MODE_SLEEP|ADS1X_TEST_MODE_POLL|ADS1X_TEST_MODE_ROTMUX;
      //adcMF|= ADS1X_TEST_MODE_VERBOSE;

      // Paranoid enum check: for (int i=ADS11_DR8; i<=ADS11_DR860; i++) { printf("%d -> %d\n", i, ads1xRateToU(i,1) ); }
      //MemBuff ws={0,};
      //allocMemBuff(&ws, 4<<10);//
      testADS1x15(&gBusCtx, NULL, 0x48, ADS11, adcMF, 100);
      //releaseMemBuff(&ws);
      lxi2cClose(&gBusCtx);
   }

   return(0);
} // main

#endif // ADS1X_MAIN
