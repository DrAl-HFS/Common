// Common/MBD/ads1xDev.c - development, test & debug code for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Sept 2020

#include "ads1xDev.h"


/***/

// Auto-gain raw reading group parameters
typedef struct
{
   U8    *pCfgPB;       // Config packet bytes - (pointer to outer level declaration eliminates copying)
   long  ivlNanoSec[2]; // Conversion interval (hardware rate dependant 303usec~125msec. for 400kHz bus clock)
   I16   fsr;    // Full scale raw reading (value is hardware version dependant)
   U8 busAddr; // I2C device address
   U8 maxT;    // Max transactions per mux channel
} AutoRawCtx;

#define AGR_FLAG_AUTO 1<<7 // Enable auto gain
#define AGR_FLAG_SGND 1<<6 // Mux selects single ended operation
#define AGR_FLAG_VROK 1<<5 // Valid result
#define AGR_FLAG_ORNG 1<<4 // out of range (uncorrectable over/under-flow)
#define RMG_MASK_TRNS 0xF
typedef struct
{
   I16 res;
   U8  cfgRB0[1]; // First byte of config register: describes mux gain and single/multi conversion control
   U8  flSt; // flags (see above) in high nybble & 4bit transaction count in low
} RawAGR;


/***/

int ads1xInitFPB (ADS1xFullPB *pFPB, const MemBuff *pWS, const LXI2CBusCtx *pC, const U8 dev)
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
} // ads1xInitFPB

void ads1xTransCfg (ADS1xTrans *pT, const U8 cfg[2], const ADS1xHWID id)
{
   pT->gainFSV= ads1xGainToFSV( ads1xGetGain(cfg) );
   pT->rate= ads1xRateToU( ads1xGetRate(cfg), id);
   pT->cmp= (((cfg[1] >> ADS1X_SH1_CQ) & ADS1X_CMP_M) + 1 ) & ADS1X_CMP_M; // rotate by +1 so 0->off
} // ads1xTransCfg

// Conversion interval (ns), rate stored optionally (else NULL)
long ads1xConvIvl (int *pRate, const U8 cfg[2], const ADS1xHWID id)
{
   int rate= ads1xRateToU( ads1xGetRate(cfg), id);
   if (pRate) { *pRate= rate; }
   if (rate > 0) { return(NANO_TICKS / rate); }
   // else
   return(0);
} // ads1xConvIvl

F32 ads1xGainScaleV (const U8 cfg[1], const ADS1xHWID x)
{
   static const F32 rawFS[]= { 1.0 / ADS10_FSR, 1.0 / ADS11_FSR };
   if ((x & 1) != x) { return(0); } //else
   return ads1xGainToFSV( ads1xGetGain(cfg) ) * rawFS[x];
} // ads1xGainScaleV

// DEBUG & TEST
//char muxCh (const U8 c) { if (c<=3) return('0'+c); else return('G'); }
//void RawAGR (const U8 m4x4) { printf("%c/%c", muxCh(m4x4 >> 4), muxCh(m4x4 & 0xF)); }

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
   ads1xTransCfg(&t, cfg, id);
   LOG("%s %GV, %d/s C:%d\n", ads1xMuxStr(ads1xGetMux(cfg)), t.gainFSV, t.rate, t.cmp);
} // ads1xDumpCfg

void ads1xDumpAll (const ADS1xFullPB *pFPB, const ADS1xHWID id)
{
   I16 v[3];
   ads1xDumpCfg(pFPB->rc.cfg+1, id);
   v[0]= rdI16BE(pFPB->rc.res+1);
   v[1]= rdI16BE(pFPB->cLo+1);
   v[2]= rdI16BE(pFPB->cHi+1);
   LOG("res: %04x (%d) cmp: %04x %04x (%d %d)\n", v[0], v[0], v[1], v[2], v[1], v[2]);
} // ads1xDumpAll

/***/
// DISPLACE : ads1xUtil ?
// Read a set of mux channels, updating the gain setting for each to maximise precison.
// Performs multiple reads per channel as appropriate, if permitted.
int readAutoRawADS1x (RawAGR rmg[], int nR, RawTimeStamp *pTarget, AutoRawCtx *pARC, const LXI2CBusCtx *pC)
{
   RawTimeStamp wait[2];
   int n=0, r=-1, vr, tFSR; // intermediate values at machine word-length: intended to reduce operations
   U8 resPB[ADS1X_NRB], gainID, iT;

   resPB[0]= ADS1X_REG_RES;
   for (int i=0; i<nR; i++)
   {  // per-mux iteration
      vr= 0; iT= 0; // clean start
      rmg[i].flSt&= AGR_FLAG_AUTO|AGR_FLAG_SGND; // clear status, preserve setting
      gainID= ads1xGetGain(rmg[i].cfgRB0);
      do
      {  // Get best available reading
         pARC->pCfgPB[1]= rmg[i].cfgRB0[0]; // Sets Gain, Mux, flags (Single Shot & Start)
         r= lxi2cWriteRB(pC, pARC->busAddr, pARC->pCfgPB, ADS1X_NRB);
         if (r > 0)
         {  // working
            iT++;
            //timeStamp(wait+0);
            timeSetTarget(wait+1, NULL, pARC->ivlNanoSec[0]);
            timeSpinWaitUntil(wait+0, wait+1);
            r= lxi2cReadRB(pC, pARC->busAddr, resPB, ADS1X_NRB);
            if (r > 0)
            {  // got result
               //timeStamp(wait+1); // mean of 0,1 gives stable estimate of sample time
               iT++;
               vr= rdI16BE(resPB+1);
               if ((vr < 0) && (rmg[i].flSt & AGR_FLAG_SGND))
               {  // single ended: reverse polarity -> abort
                  rmg[i].flSt|= AGR_FLAG_ORNG;
                  r= 0;
               }
               else if (rmg[i].flSt & AGR_FLAG_AUTO)
               {  // Check for better gain setting
                  tFSR= (int)(pARC->fsr) / 2;
                  if ((vr < tFSR) && (gainID < ADS1X_GAIN_0V256))
                  {
                     do // Search for appropriate gain to improve
                     {  // precision based on last reading
                        gainID++;
                        tFSR/= 2;
                     } while ((vr < tFSR) && (gainID < ADS1X_GAIN_0V256));
                     ads1xSetGain(rmg[i].cfgRB0, gainID);
                  }
                  else if ((vr >= pARC->fsr) && (gainID > ADS1X_GAIN_6V144))
                  {  // Reduce gain by half
                     gainID/= 2;
                     ads1xSetGain(rmg[i].cfgRB0, gainID);
                  }
                  else { r= 0; } // done
               }
            } // Result
         } // Start
      } while ((r > 0) && (iT < pARC->maxT));
      if (r >= 0)
      { // Set final status info
         const int fsr= pARC->fsr;
         if (0 == (rmg[i].flSt & AGR_FLAG_ORNG))
         {
            if ((vr >= fsr) || (vr <= -(fsr+1))) { rmg[i].flSt|= AGR_FLAG_ORNG; }
            else
            {
               rmg[i].flSt|= AGR_FLAG_VROK;
               ++n;
            }
         }
         rmg[i].res= vr;
         rmg[i].flSt|= RMG_MASK_TRNS & iT;
       }
       if (pARC->ivlNanoSec[1] > 0)
       {
         timeSpinWaitUntil(wait+0, pTarget);
         timeSetTarget(pTarget, pTarget, pARC->ivlNanoSec[1]);
      }
   }
   return(n);
} // readAutoRawADS1x

void setupRawAGR (RawAGR r[], const U8 mux[], const int n, const enum ADS1xGain initGain)
{
   for (int i=0; i<n; i++)
   {
      ads1xGenCfgRB0(r[i].cfgRB0, mux[i], initGain, ADS1X_FL0_OS|ADS1X_FL0_MODE);
      r[i].flSt= AGR_FLAG_AUTO;
      if (mux[i] >= ADS1X_MUX0G) { r[i].flSt|= AGR_FLAG_SGND; }
   }
} // setupRawAGR

void convertRawAGR (F32 f[], const RawAGR r[], const int n, const ADSInstProp *pP)
{
   for (int i=0; i<n; i++)
   {
      if (r[i].flSt & AGR_FLAG_VROK) { f[i]= r[i].res * ads1xGainScaleV(r[i].cfgRB0, pP->hwID); }
      else { f[i]= 0; }
   }
} // convertRawAGR

/***/

static const char gSepCh[2]={'\t','\n'};

int readAutoADS1X
(
   F32       f[],
   const int nF,
   const U8 mux[],
   int       nMux,
   U8       * pCfgPB,
   const LXI2CBusCtx *pC,
   const ADSInstProp *pP,
   const ADSReadParam *pM
)
{
   RawTimeStamp targetTS;
   AutoRawCtx arc;
   RawAGR rmg[4];
   int n=0, r=-1;
   U8 cfgPB[ADS1X_NRB];
   F32 R1[]={2200, 330, 330, 0}; // resistance measure hack
   if ((nF > 0) && (nMux > 0))
   {
      if (pCfgPB) { arc.pCfgPB= pCfgPB; } // Paranoid VALIDATE ?
      else
      {
         cfgPB[0]= ADS1X_REG_CFG;
         r= lxi2cReadRB(pC, pM->busAddr, cfgPB, ADS1X_NRB);
         //LOG_CALL(" : lxi2cReadRB() - r=%d\n", r);
         if (r <= 0) { return(r); }
         //else
         arc.pCfgPB= cfgPB;
      }
      arc.ivlNanoSec[0]= ads1xConvIvl(&r, arc.pCfgPB+1, pP->hwID);
      arc.ivlNanoSec[1]= NANO_TICKS / pM->rate;
      //LOG_CALL("() - rate=%d -> ivl=%d\n", r, arc.ivl);
      arc.fsr= ads1xRawFSR(pP->hwID);
      arc.busAddr= pM->busAddr;
      arc.maxT= 10; // => 5 iterations * 2 transactions

      if (nMux > 4) { WARN_CALL("(..nMux=%u..) - clamped to 4\n", nMux); nMux= 4; }
      setupRawAGR(rmg, mux, nMux, ADS1X_GAIN_4V096);

      // Messy debug dump...
      timeSetTarget(&targetTS, NULL, arc.ivlNanoSec[0]);
      do
      {
         r= readAutoRawADS1x(rmg, nMux, &targetTS, &arc, pC); //LOG("readAutoRawADS1x() - r=%d\n", r);
         if (r > 0)
         {
            LOG("\nRaw[%d]\t\t",n);
            for (int j=0; j<nMux; j++)
            {
               U8 g= ads1xGetGain(rmg[j].cfgRB0);
               U8 t= rmg[j].flSt & RMG_MASK_TRNS;
               U8 f= rmg[j].flSt >> 4;	// rmg[j].res,
               LOG("%d,%X,%d%c", g, f, t, gSepCh[j >= (nMux-1)]);
            }
            convertRawAGR(f+n, rmg, nMux, pP);
            LOG("Volts[%d]\t",n);
            for (int j=0; j<nMux; j++) { LOG("%G%c", f[n+j], gSepCh[j >= (nMux-1)]); }
            LOG("Resistance[%d]\t",n);
            for (int j=0; j<nMux; j++)
            {
               F32 V0, V1, R0=0;
               V0= f[n+j];
               V1= pP->vdd - V0;
               if (V1 > 0) { R0= R1[j] * V0 / V1; }
               LOG("%G%c", R0, gSepCh[j >= (nMux-1)]);
            }

            n+= nMux;
         }
      } while ((n < nF) && (r > 0));
   }
   return(n);
} // readAutoADS1X

int testAuto
(
   const LXI2CBusCtx *pC,
   const ADSInstProp *pP,
   const ADSReadParam *pM
)
{
   RawTimeStamp   ts;
   const U8 rateID= ads1xSelectRate(pM->rate, pP->hwID);
   U8 cfgPB[ADS1X_NRB];
   F32 dt=0, *pF;
   int r;

   pF= malloc(pM->maxSamples * sizeof(*pF));
   if (NULL == pF) return(-1);

   r= ads1xRateToU(rateID, pP->hwID);
   LOG_CALL("(..rate=%d) - selected id=%d -> %d\n", pM->rate, rateID, r);
   //if (pM->rate > r) { pM->rate= r; }
   cfgPB[0]= ADS1X_REG_CFG;
   r= lxi2cReadRB(pC, pM->busAddr, cfgPB, ADS1X_NRB);
   //LOG_CALL("(...rateID=%d...) : ReadRB() r=%d \n", rateID, r);
   if (r > 0)
   {
      r= ads1xGetRate(cfgPB+1);
      //LOG("\tcfg=%02X%02X rateID=%d \n",cfgPB[1], cfgPB[2], r);
      if (rateID == r) { r= 1; }
      else
      {
         ads1xSetRate(cfgPB+1, rateID);
         r= lxi2cWriteRB(pC, pM->busAddr, cfgPB, ADS1X_NRB);
         //LOG_CALL(" : WriteRB() cfg=%02X%02X r=%d\n", cfgPB[1], cfgPB[2], r);
      }
   }
   if (r > 0)
   {
      LOG("%s","\t\t");
      for (int j=0; j<pM->nMux; j++) { LOG("%s%c", ads1xMuxStr(pM->mux[j]), gSepCh[j >= (pM->nMux-1)] ); }
      timeNow(&ts);
      r= readAutoADS1X(pF, pM->maxSamples, pM->mux, pM->nMux, cfgPB, pC, pP, pM);
      dt= timeElapsed(&ts);
      LOG("%d samples, dt= %G sec : mean rate= %G Hz\n\n", r, dt, r * rcpF(dt));
   }
   if (pF) { free(pF); }
   return(r);
} // testAuto

/***/

#ifdef ADS1X_TEST

int testADS1x15
(
   const LXI2CBusCtx *pC,
   const MemBuff *pWS,
   const ADSInstProp *pP,
   const ADSReadParam *pM
)
{
   ADS1xFullPB fpb;
   const int i2cWait= ADS1X_TRANS_NCLK * (float)NANO_TICKS / pC->clk;
   RawTimeStamp timer;
   int i2cDelay=0, convWait=0, expectWait=0, minWaitStep=10;
   int r;
   float sv;
   U16 n;
   U8 cfgStatus[ADS1X_NRB];

   r= ads1xInitFPB(&fpb, pWS, pC, pM->busAddr);
   if (r >= 0)
   {
      const U8 rateID= ads1xSelectRate(pM->rate, pP->hwID);
      {
         int nDelay= 1+bitCountZ(pM->modeFlags & (ADS1X_TEST_MODE_VERIFY|ADS1X_TEST_MODE_POLL));
         i2cDelay= nDelay * i2cWait;
      }
      ads1xDumpCfg(fpb.rc.cfg+1, 0);
      memcpy(cfgStatus, fpb.rc.cfg, ADS1X_NRB);
      ads1xGenCfg(fpb.rc.cfg+1, ADS1X_MUX0G, ADS1X_GAIN_6V144, rateID, ADS1X_CMP_DISABLE);
      fpb.rc.cfg[1]|= ADS1X_FL0_OS|ADS1X_FL0_MODE; // Now enable single-shot conversion
      // NB: Config packet written to device in loop that follows
      convWait= ads1xConvIvl(NULL,fpb.rc.cfg+1, pP->hwID);
      sv= ads1xGainScaleV(fpb.rc.cfg+1, pP->hwID);
      ads1xDumpAll(&fpb, pP->hwID);
      LOG("Ivl: conv=%d ns comm=%d ns\n", convWait, i2cWait);
      if (pM->modeFlags & ADS1X_TEST_MODE_SLEEP)
      {
         if (convWait > i2cDelay) { expectWait= convWait-i2cDelay; } // Hacky : conversion time seems less than sample interval...
         if (i2cWait < minWaitStep) { minWaitStep= i2cWait; }
      } else { expectWait= 0; }
      if (pM->modeFlags & ADS1X_TEST_MODE_TIMER) { timeNow(&timer); }
      LOG("%s\n","***");
      n= 0;
      do
      {
         int iR0= 0, iR1= 0; // retry counters
         U8 cfgVer=FALSE;
         do
         {
            r= lxi2cWriteRB(pC, pM->busAddr, fpb.rc.cfg, ADS1X_NRB);
            if ((pM->modeFlags & ADS1X_TEST_MODE_VERIFY) && (r >= 0))
            {
               r= lxi2cReadRB(pC, pM->busAddr, cfgStatus, ADS1X_NRB);
               if (r >= 0)
               {  // Device goes busy (OS1->0) immediately on write, so merge back in for check
                  cfgStatus[1] |= (fpb.rc.cfg[1] & ADS1X_FL0_OS);
                  cfgVer= (0 == memcmp(cfgStatus, fpb.rc.cfg, ADS1X_NRB));
                  if (pM->modeFlags & ADS1X_TEST_MODE_VERBOSE) { LOG("ver%d=%d: ", iR0, cfgVer); ads1xDumpCfg(cfgStatus+1, pP->hwID); }
               }
            }
         } while ((pM->modeFlags & ADS1X_TEST_MODE_VERIFY) && ((r < 0) || !cfgVer) && (++iR0 < 10));

         if (expectWait > 0) { timeSpinSleep(expectWait); }

         if (pM->modeFlags & ADS1X_TEST_MODE_POLL)
         {
            do { // poll status
               r= lxi2cReadRB(pC, pM->busAddr, cfgStatus, ADS1X_NRB);
            } while (((r < 0) || (0 == (cfgStatus[1] & ADS1X_FL0_OS))) && (++iR1 < 10));
         }

         r= lxi2cReadRB(pC, pM->busAddr, fpb.rc.res, ADS1X_NRB); // read result
         if (r >= 0)
         {
            F32 dt=-1;
            if (pM->modeFlags & ADS1X_TEST_MODE_TIMER) { dt= timeElapsed(&timer); }
            int raw=  rdI16BE(fpb.rc.res+1);
            F32 v= raw * sv;
            const char muxVerCh[]={'?','V'};
            enum ADS1xMux m;
            if (cfgVer) { m= ads1xGetMux(cfgStatus+1); }
            else { m= ads1xGetMux(fpb.rc.cfg+1); }
            LOG("%s (%c) (i=%d,%d) W%d dt%G [%d] : 0x%x -> %GV\n", ads1xMuxStr(m), muxVerCh[cfgVer], iR0, iR1, expectWait, dt, n, raw, v);
            if (pM->nMux > 1)
            {
               const U8 mux= pM->mux[ n % pM->nMux ];
               ads1xSetMux(fpb.rc.cfg+1, mux);
               if (pM->modeFlags & ADS1X_TEST_MODE_VERBOSE)
               {
                  LOG("0x%02x -> %s chk: %s\n", mux, ads1xMuxStr(mux), ads1xMuxStr(ads1xGetMux(fpb.rc.cfg+1)));
               }
            }
         }
         if ((pM->modeFlags & ADS1X_TEST_MODE_TUNE) && (0 == iR0))
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
      } while (++n < pM->maxSamples);
      LOG("%s\n","---");
   }
   return(r);
} // testADS1x15

#endif // ADS1X_TEST


#ifdef ADS1X_MAIN

typedef struct
{
   ADSReadParam param;
   char devPath[15]; // host device path
   U8 flags;
} ADS1XCLA;

static ADS1XCLA gArgs=
{
   {  100, 200,   // rate & samples
      {ADS1X_MUX0G, ADS1X_MUX1G, ADS1X_MUX2G, ADS1X_MUX3G}, 4,
      0x48, ADS11, 0
   },
   "/dev/i2c-1", 0
};


void usageMsg (const char name[])
{
static const char optCh[]="adinrvh";
static const char argCh[]="#####  ";
static const char *desc[]=
{
   "I2C bus address: 2digit hex (no prefix)",
   "device index (-> path /dev/i2c-# )",
   "hardware ID 0 -> ads10xx 1 -> .ads11xx",
   "max samples",
   "sample rate",
   "verbose diagnostic messages",
   "help - diplay this text"
};
   const int n= sizeof(desc)/sizeof(desc[0]);
   report(OUT,"Usage : %s [-%s]\n", name, optCh);
   for (int i= 0; i<n; i++)
   {
      report(OUT,"\t%c %c - %s\n", optCh[i], argCh[i], desc[i]);
   }
} // usageMsg

void argDump (ADS1XCLA *pA)
{
   LOG("arg: %s, Addr:%02X, HWID:%d R=%d maxS=%d FLAGS:%02X\n", pA->devPath, pA->param.busAddr, pA->param.hwID, pA->param.rate, pA->param.maxSamples, pA->flags);
} // argDump

#define ARG_AUTO    (1<<2)
#define ARG_HELP    (1<<1)
#define ARG_VERBOSE (1<<0)

void argTrans (ADS1XCLA *pA, int argc, char *argv[])
{
   int c, t;
   do
   {
      c= getopt(argc,argv,"a:d:i:n:r:hvA");
      switch(c)
      {
         case 'a' :
            sscanf(optarg, "%x", &t);
            if (t <= 0x7F) { pA->param.busAddr= t; }
            break;
         case 'd' :
         {
            char ch= optarg[0];
            if ((ch > '0') && (ch <= '9')) { pA->devPath[9]= ch; }
            break;
         }
         case 'i' :
            sscanf(optarg, "%d", &t);
            if ((0x1 & t) == t) { pA->param.hwID= t; }
            break;
         case 'n' :
            sscanf(optarg, "%d", &t);
            if (t > 0) { pA->param.maxSamples= t; }
            break;
          case 'r' :
            sscanf(optarg, "%d", &t);
            if (t > 0) { pA->param.rate= t; }
            break;
         case 'h' :
            pA->flags|= ARG_HELP;
            break;
         case 'v' :
            pA->flags|= ARG_VERBOSE;
            pA->param.modeFlags|= ADS1X_TEST_MODE_VERBOSE;
            break;
         case 'A' :
            pA->flags|= ARG_AUTO;
            break;
     }
   } while (c > 0);
   if (pA->flags & ARG_HELP) { usageMsg(argv[0]); }
   if (pA->flags & ARG_VERBOSE) { argDump(pA); }
} // argTrans

LXI2CBusCtx gBusCtx={0,-1};

int main (int argc, char *argv[])
{
   int r= -1;

   argTrans(&gArgs, argc, argv);

   if (lxi2cOpen(&gBusCtx, gArgs.devPath, 400))
   {
      const ADSInstProp *pP= adsInitProp(NULL, 3.3065, gArgs.param.hwID);

      if (gArgs.flags & ARG_AUTO) { r= testAuto(&gBusCtx, pP, &(gArgs.param)); }
      else
      {
         gArgs.param.modeFlags|= ADS1X_TEST_MODE_VERIFY|ADS1X_TEST_MODE_SLEEP|ADS1X_TEST_MODE_POLL|ADS1X_TEST_MODE_TIMER;
         // Paranoid enum check: for (int i=ADS11_DR8; i<=ADS11_DR860; i++) { printf("%d -> %d\n", i, ads1xRateToU(i,1) ); }
         //MemBuff ws={0,};
         //allocMemBuff(&ws, 4<<10);//
         r= testADS1x15(&gBusCtx, NULL, pP, &(gArgs.param));
         //releaseMemBuff(&ws);
      }
      lxi2cClose(&gBusCtx);
   }

   return(r);
} // main

#endif // ADS1X_MAIN
