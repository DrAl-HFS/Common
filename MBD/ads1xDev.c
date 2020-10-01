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
   U8 maxTrans;    // Max transactions per mux channel
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
long ads1xConvIvl (U16 *pRate, const U8 cfg[2], const ADS1xHWID id)
{
   U16 rate= ads1xRateToU( ads1xGetRate(cfg), id);
   if (pRate) { *pRate= rate; }
   if (0 != rate) { return(NANO_TICKS / (long)rate); } // else
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

// DISPLACE ... ???
int idxMaxNU16 (const U16 u[], int n)
{
   int iM= 0;
   for (int i=1; i<n; i++) { if (u[i] > u[iM]) { iM= i; } }
   return(iM);
} // idxMaxNU16

/***/

#define EXT_RTS_COUNT (5)
#define EXT_RTS_MUXCH_BGN (4)
#define EXT_RTS_WRCFG_BGN (3)
#define EXT_RTS_WRCFG_END (2)
#define EXT_RTS_RDVAL_BGN (1)
#define EXT_RTS_RDVAL_END (0)

typedef struct {  // TODO: reverse order
   RawTimeStamp ts[EXT_RTS_COUNT]; // channel start plus begin-end for two transactions (setup & read)
} ExtTiming;

// DISPLACE : ads1xUtil ?
// Read a set of mux channels, updating the gain setting for each to maximise precison.
// Performs multiple reads per channel as appropriate, if permitted.
// Optionally provides transaction timing information
int readAutoRawADS1x (RawAGR agr[], ExtTiming *pT, int nR, RawTimeStamp *pTarget, AutoRawCtx *pARC, const LXI2CBusCtx *pC)
{
   RawTimeStamp wait[3];
   int n=0, r=-1, vr, tFSR; // intermediate values at machine word-length: intended to reduce operations
   U8 resPB[ADS1X_NRB], gainID, iTrans;

   resPB[0]= ADS1X_REG_RES;
   for (int i=0; i<nR; i++)
   {  // per-mux iteration
      vr= 0; iTrans= 0; // clean start
      agr[i].flSt&= AGR_FLAG_AUTO|AGR_FLAG_SGND; // clear status, preserve setting
      gainID= ads1xGetGain(agr[i].cfgRB0);
      if (pARC->ivlNanoSec[1] > 0)
      {
         timeSpinWaitUntil(wait+0, pTarget);
         if (pT) { pT[i].ts[EXT_RTS_MUXCH_BGN]= wait[0]; }
         timeSetTarget(pTarget, NULL, pARC->ivlNanoSec[1], TIME_MODE_RELATIVE);
      } else if (pT) { timeStamp(pT[i].ts+EXT_RTS_MUXCH_BGN); } // MUX channel begin

      do
      {  // Get best available reading
         pARC->pCfgPB[1]= agr[i].cfgRB0[0]; // Sets Gain, Mux, flags (Single Shot & Start)
         if (pT) { timeStamp(pT[i].ts+EXT_RTS_WRCFG_BGN); }  // Config write begin
         r= lxi2cWriteRB(pC, pARC->busAddr, pARC->pCfgPB, ADS1X_NRB);
         if (r > 0)
         {  // Config write complete, conversion started
            timeSetTarget(wait+1, wait+0, pARC->ivlNanoSec[0], TIME_MODE_NOW);
            iTrans++;
            timeSpinWaitUntil(wait+2, wait+1); // Wait for conversion
            r= lxi2cReadRB(pC, pARC->busAddr, resPB, ADS1X_NRB);
            if (r > 0)
            {  // got result
               if (pT)
               {
                  timeStamp(pT[i].ts+EXT_RTS_RDVAL_END);  // Read complete
                  pT[i].ts[EXT_RTS_RDVAL_BGN]= wait[2];   // Wait done / begin read
                  pT[i].ts[EXT_RTS_WRCFG_END]= wait[0];   // Write complete
               }
               iTrans++;
               vr= rdI16BE(resPB+1);
               if ((vr < 0) && (agr[i].flSt & AGR_FLAG_SGND))
               {  // single ended: reverse polarity -> abort
                  agr[i].flSt|= AGR_FLAG_ORNG;
                  r= 0;
               }
               else if (agr[i].flSt & AGR_FLAG_AUTO)
               {  // Check for better gain setting
                  tFSR= (int)(pARC->fsr) / 2;
                  if ((vr < tFSR) && (gainID < ADS1X_GAIN_0V256))
                  {
                     do // Search for appropriate gain to improve
                     {  // precision based on last reading
                        gainID++;
                        tFSR/= 2;
                     } while ((vr < tFSR) && (gainID < ADS1X_GAIN_0V256));
                     ads1xSetGain(agr[i].cfgRB0, gainID);
                  }
                  else if ((vr >= pARC->fsr) && (gainID > ADS1X_GAIN_6V144))
                  {  // Reduce gain by half
                     gainID/= 2;
                     ads1xSetGain(agr[i].cfgRB0, gainID);
                  }
                  else { r= 0; } // done
               }
            } // Result
         } // Start
      } while ((r > 0) && (iTrans < pARC->maxTrans));
      if (r >= 0)
      { // Set final status info
         const int fsr= pARC->fsr;
         if (0 == (agr[i].flSt & AGR_FLAG_ORNG))
         {
            if ((vr >= fsr) || (vr <= -(fsr+1))) { agr[i].flSt|= AGR_FLAG_ORNG; }
            else
            {
               agr[i].flSt|= AGR_FLAG_VROK;
               ++n;
            }
         }
         agr[i].res= vr;
         agr[i].flSt|= RMG_MASK_TRNS & iTrans;
      }
   }
   return(n);
} // readAutoRawADS1x

U8 setupRawAGR (RawAGR r[], const U8 mux[], U8 n, const U8 maskAG, const enum ADS1xGain initGain)
{
   if (n > ADS1X_MUX_MAX) { WARN_CALL("(..nMux=%u..) - clamped to ADS_MUX_MAX=%d\n", n, ADS1X_MUX_MAX); n= ADS1X_MUX_MAX; }
   for (int i=0; i<n; i++)
   {  // set config byte to start single conversion (plus gain & mux)
      ads1xGenCfgRB0(r[i].cfgRB0, mux[i], initGain, ADS1X_FL0_OS|ADS1X_FL0_MODE);
      if (maskAG & (1<<n)) { r[i].flSt= AGR_FLAG_AUTO; }
      if (mux[i] >= ADS1X_MUX0G) { r[i].flSt|= AGR_FLAG_SGND; }
   }
   return(n);
} // setupRawAGR

int convertRawAGR (F32 f[], const RawAGR r[], const int n, const ADSInstProp *pP)
{
   for (int i=0; i<n; i++)
   {
      if (r[i].flSt & AGR_FLAG_VROK) { f[i]= r[i].res * ads1xGainScaleV(r[i].cfgRB0, pP->hwID); }
      else { f[i]= 0; }
   }
   return(n);
} // convertRawAGR

int elapsedRawTimeStamp
(
   F32 f[],
   const RawTimeStamp rawTS[],
   const int nTS,
   const int stride,
   const RawTimeStamp *pRef,
   const F32 timeScale  // scale from sec. to eg. milliSec
)
{
   int iR=0;
   if (stride > 1)
   {
      const int nI= nTS / stride;
      for (int j=0; j<stride; j++)  // 2D transposition
      {
         for (int i=0; i<nI; i++) { f[iR++]= timeScale * timeDiff(pRef, rawTS + j + i*stride); }
      }
   }
   else { for (int i=0; i<nTS; i++) { f[i]= timeScale * timeDiff(pRef, rawTS + i); } }
   return(iR);
} // elapsedRawTimeStamp

/***/

#include "util.h"
// Hackity hack..
void reportStat (F32 x[3], F32 timeScale, F32 dof)
{
   StatMomD1R2 sm={ { x[0], x[1] * timeScale, x[2] * timeScale * timeScale } };
   StatResD1R2 sr;
   statMom1Res1(&sr, &sm, dof);
   F32 s= sqrt(sr.v);
   report(LOG0,"Transaction mean, stdev : %G, %G, D=%G\n", sr.m, s, s * rcpF(sr.m));
}

#define FLAGS_ARE_SET(f,x) (((f) & (x)) == (f))

static const char gSepCh[2]={'\t','\n'};

int readAutoADS1X
(
   F32        rV[],  // result Voltage
   F32        *pDT,  // result time difference from reference (optional)
   const int nMax,  // Max readings (over all MUX channels)
   const RawTimeStamp *pRefTS,
   U8 * pCfgPB,   // Optional device config register bytes (initial value modified)
   const LXI2CBusCtx *pC,
   const ADSInstProp *pP,
   const ADSReadParam *pM
)
{
   ExtTiming extT[ADS1X_MUX_MAX], *pET=NULL;
   F32 sumTransDT[3]={0};  // StatMomD1R2 md1r2;
   RawTimeStamp targetTS[2];
   long outerIvlNanoSec=0;
   AutoRawCtx arc;
   RawAGR rawAGR[ADS1X_MUX_MAX];
   int n=0, r=-1;
   U8 cfgPB[ADS1X_NRB];

   const U8 nMux= setupRawAGR(rawAGR, pM->mux, pM->nMux, pM->maskAG, ADS1X_GAIN_4V096);
   if ((nMax > 0) && (nMux > 0))
   {
      if (pCfgPB) { arc.pCfgPB= pCfgPB; } // Paranoid VALIDATE ?
      else
      {
         cfgPB[0]= ADS1X_REG_CFG;
         r= lxi2cReadRB(pC, pP->busAddr, cfgPB, ADS1X_NRB);
         //LOG_CALL(" : lxi2cReadRB() - r=%d\n", r);
         if (r <= 0) { return(r); }
         //else
         arc.pCfgPB= cfgPB;
      }

      // Set hard & soft rates
      U16 hardRate, softRate=0;
      arc.ivlNanoSec[0]= ads1xConvIvl(&hardRate, arc.pCfgPB+1, pP->hwID);
      if (pM->rate[1] >= hardRate) { arc.ivlNanoSec[1]= 0; } // no extra delays
      else { arc.ivlNanoSec[1]= NANO_TICKS / (long)(pM->rate[1]); }
      if (pM->rate[0] > 0)
      {
         U16 consRate= MIN(hardRate, pM->rate[1]) / nMux; // *nAG
         if (pM->rate[0] > consRate) { WARN_CALL("() - requested rate %d exceeds constraint %d\n", pM->rate[0], consRate); }
         if (pM->rate[0] < consRate)
         {  // delay required
            softRate= pM->rate[0];
            outerIvlNanoSec= NANO_TICKS / (long)softRate;
         }
      }
      //LOG_CALL("() - rate=%d -> ivl=%d\n", r, arc.ivl);
      arc.fsr= ads1xRawFSR(pP->hwID);
      arc.busAddr= pP->busAddr;
      if (softRate > 0)
      {
         const long i2cTransWait= ADS1X_TRANS_NCLK * (float)NANO_TICKS / pC->clk;
         const long hardTime= (arc.ivlNanoSec[0] + 2 * i2cTransWait);
         const long window= outerIvlNanoSec / nMux;
         arc.maxTrans= 2*(window / hardTime);
         LOG("transWait=%d, hardTime=%d, window=%d -> maxTrans=%d\n", i2cTransWait, hardTime, window, arc.maxTrans);
      } else { arc.maxTrans= 2; }

      timeSetTarget(targetTS+1, targetTS+0, 100, TIME_MODE_NOW);
      if (pDT || (pM->modeFlags & ADS1X_MODE_XTIMING))
      {
         pET= extT+0;
         if (NULL == pRefTS) { pRefTS= targetTS+0; }
      }
      do
      {
         r= readAutoRawADS1x(rawAGR, pET, nMux, targetTS+1, &arc, pC); //LOG("readAutoRawADS1x() - r=%d\n", r);
         if (r > 0)
         {
            convertRawAGR(rV+n, rawAGR, nMux, pP);
            if (pDT)
            {  // Convert post-reading time stamp to elapsed since reference
               if (pM->timeEst < EXT_RTS_COUNT)
               { elapsedRawTimeStamp(pDT+n, pET[0].ts+pM->timeEst, nMux, 1, pRefTS, 1000); }
               else
               {
               }
            }

            if FLAGS_ARE_SET( ADS1X_MODE_XTIMING|ADS1X_MODE_VERBOSE, pM->modeFlags)
            { // extended raw data debug dump...
               F32 t[EXT_RTS_COUNT*ADS1X_MUX_MAX];
               int k= 0;

               report(LOG0,"%d..%d\n",n,n+nMux-1);
               report(LOG0,"Raw\t");
               for (int i=0; i<nMux; i++)
               {
                  U8 g= ads1xGetGain(rawAGR[i].cfgRB0);
                  U8 t= rawAGR[i].flSt & RMG_MASK_TRNS;
                  U8 f= rawAGR[i].flSt >> 4;	// rawAGR[i].res,
                  report(LOG0,"%d,%X,%d%c", g, f, t, gSepCh[i >= (nMux-1)]);
               }

               elapsedRawTimeStamp(t, pET[0].ts+0, nMux * EXT_RTS_COUNT, EXT_RTS_COUNT, pRefTS, 1000);
               for (int j=0; j<EXT_RTS_COUNT; j++)
               {
                  report(LOG0,"dt (ms)\t");
                  for (int i=0; i<nMux; i++) { report(LOG0,"%G%c", t[k+i], gSepCh[i >= (nMux-1)]); }
                  k+= nMux;
               }
               for (int i=0; i<nMux; i++)
               {
                  F32 dt1= timeDiff(pET[i].ts+1, pET[i].ts+2), dt2= timeDiff(pET[i].ts+3, pET[i].ts+4);
                  //sumTransDT[0]+= 2;
                  sumTransDT[1]+= dt1 + dt2;
                  sumTransDT[2]+= dt1*dt1 + dt2*dt2;
               }
            }
            n+= nMux;
         }
         if (outerIvlNanoSec > 0)
         {  RawTimeStamp wait[1];
            timeSetTarget(targetTS+1, NULL, outerIvlNanoSec, TIME_MODE_RELATIVE);
            timeSpinWaitUntil(wait+0, targetTS+1);
         }
      } while ((n <= (nMax-nMux)) && (r > 0));
      //if (refTS.tv_nsec > 0) { ; } ???
      if FLAGS_ARE_SET( ADS1X_MODE_XTIMING|ADS1X_MODE_VERBOSE, pM->modeFlags)
      {
         sumTransDT[0]= 2 * n;
         reportStat(sumTransDT, 1000, sumTransDT[0]-1);
      }
   }
   return(n);
} // readAutoADS1X

int testAutoGain
(
   const int maxSamples,
   const LXI2CBusCtx *pC,
   const ADSInstProp *pP,
   const ADSReadParam *pM,
   const F32      *pResDiv
)
{
   RawTimeStamp   ts;
   const U8 rateID= ads1xSelectRate(pM->rate[1], pP->hwID);
   U8 cfgPB[ADS1X_NRB];
   F32 dt=0, *pV, *pDT;
   int r;

   if (maxSamples <= 0) { return(0); }
   const U32 maxSM= pM->nMux * maxSamples;
   pV= malloc(maxSM * (sizeof(*pV) + sizeof(*pDT)));
   if (NULL == pV) return(-1);
   pDT= pV + maxSM;

   r= ads1xRateToU(rateID, pP->hwID);
   LOG_CALL("(..rate=[%d,%d]) - selected id=%d -> %d\n", pM->rate[0], pM->rate[1], rateID, r);
   cfgPB[0]= ADS1X_REG_CFG;
   r= lxi2cReadRB(pC, pP->busAddr, cfgPB, ADS1X_NRB);
   //LOG_CALL("(...rateID=%d...) : ReadRB() r=%d \n", rateID, r);
   if (r > 0)
   {
      r= ads1xGetRate(cfgPB+1);
      //LOG("\tcfg=%02X%02X rateID=%d \n",cfgPB[1], cfgPB[2], r);
      if (rateID == r) { r= 1; }
      else
      {
         ads1xSetRate(cfgPB+1, rateID);
         r= lxi2cWriteRB(pC, pP->busAddr, cfgPB, ADS1X_NRB);
         //LOG_CALL(" : WriteRB() cfg=%02X%02X r=%d\n", cfgPB[1], cfgPB[2], r);
      }
   }
   if (r > 0)
   {
      timeNow(&ts);
      r= readAutoADS1X(pV, pDT, maxSM, &ts, cfgPB, pC, pP, pM);
      dt= timeElapsed(&ts);
      report(LOG0,"%d samples, dt= %G sec : mean rate= %G Hz\n\n", r, dt, r * rcpF(dt));
      {
         const U8 nMux= pM->nMux; // Hacky...
         int n= 0;
         report(LOG0,"\t");
         for (int i=0; i<nMux; i++) { LOG("%s%c", ads1xMuxStr(pM->mux[i]), gSepCh[i >= (nMux-1)] ); }
         while (n < r)
         {
            report(LOG0,"[ %d..%d ] : \n", n, n+nMux-1);
            report(LOG0,"DT\t");
            for (int i=0; i<nMux; i++) { report(LOG0,"%G%c", pDT[n+i], gSepCh[i >= (nMux-1)]); }
            report(LOG0,"V\t");
            for (int i=0; i<nMux; i++) { report(LOG0,"%G%c", pV[n+i], gSepCh[i >= (nMux-1)]); }

            if (pResDiv)
            {
               report(LOG0,"R\t",n);
               for (int i=0; i<nMux; i++)
               {
                  F32 v0= pV[n+i]; // v1= pP->vdd - v0
                  F32 res= pResDiv[i] * v0 * rcpF(pP->vdd - v0);
                  report(LOG0,"%G%c", res, gSepCh[i >= (nMux-1)]);
               }
            }
            n+= nMux;
         }
      }
   }
   if (pV) { free(pV); }
   return(r);
} // testAutoGain

/***/

#ifdef ADS1X_TEST

// First gen. test routine with hacky microcontroller style sleep between readings
// Configurable behaviour (verification etc.) means this still has some uses.
int testADS1x15
(
   const int maxSamples,
   const LXI2CBusCtx *pC,
   const MemBuff *pWS,
   const ADSInstProp *pP,
   const ADSReadParam *pM
)
{
   ADS1xFullPB fpb;
   const long i2cWait= ADS1X_TRANS_NCLK * (float)NANO_TICKS / pC->clk;
   RawTimeStamp timer;
   int i2cDelay=0, convWait=0, expectWait=0, minWaitStep=10;
   int r, n;
   float sv;
   U8 cfgStatus[ADS1X_NRB];

   r= ads1xInitFPB(&fpb, pWS, pC, pP->busAddr);
   if (r >= 0)
   {
      const U8 idxMaxRate= idxMaxNU16(pM->rate, 2);
      const U8 rateID= ads1xSelectRate(pM->rate[idxMaxRate], pP->hwID);
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
      if (pM->modeFlags & ADS1X_MODE_XTIMING) { timeNow(&timer); }
      LOG("%s\n","***");
      n= 0;
      do
      {
         int iR0= 0, iR1= 0; // retry counters
         U8 cfgVer=FALSE;
         do
         {
            r= lxi2cWriteRB(pC, pP->busAddr, fpb.rc.cfg, ADS1X_NRB);
            if ((pM->modeFlags & ADS1X_TEST_MODE_VERIFY) && (r >= 0))
            {
               r= lxi2cReadRB(pC, pP->busAddr, cfgStatus, ADS1X_NRB);
               if (r >= 0)
               {  // Device goes busy (OS1->0) immediately on write, so merge back in for check
                  cfgStatus[1] |= (fpb.rc.cfg[1] & ADS1X_FL0_OS);
                  cfgVer= (0 == memcmp(cfgStatus, fpb.rc.cfg, ADS1X_NRB));
                  if (pM->modeFlags & ADS1X_MODE_VERBOSE) { LOG("ver%d=%d: ", iR0, cfgVer); ads1xDumpCfg(cfgStatus+1, pP->hwID); }
               }
            }
         } while ((pM->modeFlags & ADS1X_TEST_MODE_VERIFY) && ((r < 0) || !cfgVer) && (++iR0 < 10));

         if (expectWait > 0) { timeSpinSleep(expectWait); }

         if (pM->modeFlags & ADS1X_TEST_MODE_POLL)
         {
            do { // poll status
               r= lxi2cReadRB(pC, pP->busAddr, cfgStatus, ADS1X_NRB);
            } while (((r < 0) || (0 == (cfgStatus[1] & ADS1X_FL0_OS))) && (++iR1 < 10));
         }

         r= lxi2cReadRB(pC, pP->busAddr, fpb.rc.res, ADS1X_NRB); // read result
         if (r >= 0)
         {
            F32 dt=-1;
            if (pM->modeFlags & ADS1X_MODE_XTIMING) { dt= timeElapsed(&timer); }
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
               if (pM->modeFlags & ADS1X_MODE_VERBOSE)
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
      } while (++n < maxSamples);
      LOG("%s\n","---");
   }
   return(r);
} // testADS1x15

#endif // ADS1X_TEST


#ifdef ADS1X_MAIN

typedef struct
{
   ADSReadParam param;
   int maxSamples;
   char devPath[14]; // host device path
   U8 hwID;
   U8 busAddr;
   U8 testFlags;
} ADS1XArgs;

static ADS1XArgs gArgs=
{
   {  { 20, 250 },   // inner & outer rates
      { ADS1X_MUX0G, ADS1X_MUX1G, ADS1X_MUX2G, ADS1X_MUX3G }, 4,
      0x0F, EXT_RTS_RDVAL_END, 0    // maskAG, timeEst, modeFlags

   },
   32,   // samples
   "/dev/i2c-1", ADS11, 0x48, 0
};


void usageMsg (const char name[])
{
static const char optCh[]="adimnrAvh";
static const char argCh[]="######   ";
static const char *desc[]=
{
   "I2C bus address: 2digit hex (no prefix)",
   "device index (-> path /dev/i2c-# )",
   "hardware ID: 0 -> ads10xx, 1 -> ads11xx",
   "multiplexer channel count",
   "max samples",
   "sample rate",
   "auto gain test",
   "verbose diagnostic messages",
   "help (display this text)"
};
   const int n= sizeof(desc)/sizeof(desc[0]);
   report(OUT,"Usage : %s [-%s]\n", name, optCh);
   for (int i= 0; i<n; i++)
   {
      report(OUT,"\t%c %c - %s\n", optCh[i], argCh[i], desc[i]);
   }
} // usageMsg

void argDump (ADS1XArgs *pA)
{
   report(OUT,"Device: devPath=%s, busAddr=%02X, Flags=%02X\n", pA->devPath, pA->busAddr, pA->testFlags);
   report(OUT,"\tHWID:%d Rate= %d, %d (Hz) maxS=%d\n", pA->hwID, pA->param.rate[0], pA->param.rate[1], pA->maxSamples);
   report(OUT,"\tmux[%d]={", pA->param.nMux); reportBytes(OUT, pA->param.mux, pA->param.nMux);
   report(OUT,"\t%s", "}\n");
} // argDump

#define ARG_AUTO    (1<<2)
#define ARG_HELP    (1<<1)
#define ARG_VERBOSE (1<<0)

void argTrans (ADS1XArgs *pA, int argc, char *argv[])
{
   int i, c, t;
   do
   {
      c= getopt(argc,argv,"a:d:i:n:r:Ahv");
      switch(c)
      {
         case 'a' :
            sscanf(optarg, "%x", &t);
            if (t <= 0x7F) { pA->busAddr= t; }
            break;
         case 'd' :
         {
            char ch= optarg[0];
            if ((ch > '0') && (ch <= '9')) { pA->devPath[9]= ch; }
            break;
         }
         case 'i' :
            sscanf(optarg, "%d", &t);
            if ((0x1 & t) == t) { pA->hwID= t; }
            break;
         case 'm' :
            sscanf(optarg, "%d", &t);
            i= t-1;
            if ((i & 0x3) == i) { pA->param.nMux= t; }
            break;
         case 'n' :
            sscanf(optarg, "%d", &t);
            if (t > 0) { pA->maxSamples= t; }
            break;
         case 'r' :
            sscanf(optarg, "%d", &t);
            if (t > 0) { pA->param.rate[0]= t; }
            break;
         case 'A' :
            pA->testFlags|= ARG_AUTO;
            break;
         case 'h' :
            pA->testFlags|= ARG_HELP;
            break;
         case 'v' :
            pA->testFlags|= ARG_VERBOSE;
            pA->param.modeFlags|= ADS1X_MODE_VERBOSE;
            break;
     }
   } while (c > 0);
   if (1 != idxMaxNU16(pA->param.rate, 2))
   {
      WARN_CALL("() - swapping rate order [%d,%d]\n", pA->param.rate[0], pA->param.rate[1]);
      SWAP(U16, pA->param.rate[0], pA->param.rate[1]);
   }
   if (pA->testFlags & ARG_HELP) { usageMsg(argv[0]); }
   if (pA->testFlags & ARG_VERBOSE) { argDump(pA); }
} // argTrans

LXI2CBusCtx gBusCtx={0,-1};

int main (int argc, char *argv[])
{
   int r= -1;

   argTrans(&gArgs, argc, argv);

   if (lxi2cOpen(&gBusCtx, gArgs.devPath, 400))
   {
      const ADSInstProp *pP= adsInitProp(NULL, 3.3065, gArgs.hwID, gArgs.busAddr);
      gArgs.param.modeFlags|= ADS1X_MODE_XTIMING;
      if (gArgs.testFlags & ARG_AUTO)
      {
         F32 rDiv[]= {2200, 330, 330, 0};
         r= testAutoGain(gArgs.maxSamples, &gBusCtx, pP, &(gArgs.param), rDiv);
      }
      else
      {
         gArgs.param.modeFlags|= ADS1X_TEST_MODE_VERIFY|ADS1X_TEST_MODE_SLEEP|ADS1X_TEST_MODE_POLL;
         // Paranoid enum check: for (int i=ADS11_DR8; i<=ADS11_DR860; i++) { printf("%d -> %d\n", i, ads1xRateToU(i,1) ); }
         //MemBuff ws={0,};
         //allocMemBuff(&ws, 4<<10);//
         r= testADS1x15(gArgs.maxSamples, &gBusCtx, NULL, pP, &(gArgs.param));
         //releaseMemBuff(&ws);
      }
      lxi2cClose(&gBusCtx);
   }

   return(r);
} // main

#endif // ADS1X_MAIN
