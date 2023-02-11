// Common/MBD/ads1xDev.c - development, test & debug code for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: AGPL3
// (c) Project Contributors Sept 2020

#include "ads1xDev.h"
//#include "ads1xAuto.h"


/***/


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
// Special extended values used to generate estimator parameters
#define EXT_RTS_GEP01  (5) // EXT_RTS_RDVAL_END -> EXT_RTS_RDVAL_BGN
#define EXT_RTS_GEP02  (6) // EXT_RTS_RDVAL_END -> EXT_RTS_WRCFG_END
#define EXT_RTS_GEP03  (7) // EXT_RTS_RDVAL_END -> EXT_RTS_WRCFG_BGN
#define EXT_RTS_GEP_BITS   (3)
#define EXT_RTS_GEP_MASK   ((1<<EXT_RTS_GEP_BITS)-1)
#define EXT_RTS_VAL_SHIFT  EXT_RTS_GEP_BITS

typedef struct
{
   U8 idx[2], pad[2];
   F32 r[2];
} EstimatorRTS;

EstimatorRTS *getTimeEstimator (U8 timeEst)
{
static EstimatorRTS e; // Haaack!!!
   EstimatorRTS *pE= NULL;
   if (timeEst >= EXT_RTS_COUNT)
   {
      pE= &e;
      pE->idx[1]= EXT_RTS_RDVAL_END;
      switch(timeEst & EXT_RTS_GEP_MASK)
      {
         case EXT_RTS_GEP01 : pE->idx[0]= EXT_RTS_RDVAL_BGN; break;
         case EXT_RTS_GEP02 : pE->idx[0]= EXT_RTS_WRCFG_END; break;
         case EXT_RTS_GEP03 : pE->idx[0]= EXT_RTS_WRCFG_BGN; break;
      }
      pE->r[1]=   0.02 + ((timeEst >> EXT_RTS_VAL_SHIFT) + 1) * 0.96 / (1<<(8-EXT_RTS_VAL_SHIFT));
      pE->r[0]=   1.0 - pE->r[1];
      LOG_CALL("(%d) -> %d, %d, %G, %G\n", timeEst, pE->idx[0], pE->idx[1], pE->r[0], pE->r[1]);
   }
   return(pE);
} // getTimeEstimator

/*
U8 packTE (U8 ta, U8 tb, F32 f)
{
   U8 v;
   if (tb > ta) { SWAP(U8, ta, tb); f= 1.0 - f; }

   v= (tb & 0x3) | (((ta-tb) & 0x3) << 2);
   v|= (U8)(f * 0xF0) & 0xF0;
   return(v);
} // packTE
*/


// DISPLACE : ads1xUtil ?


int convertRawAGR (F32 f[], const RawAGR r[], const int n, const ADSInstProp *pP)
{
   for (int i=0; i<n; i++)
   {
      if (r[i].flSt & AGR_FLAG_VROK) { f[i]= r[i].res * ads1xGainScaleV(r[i].cfgRB0, pP->hwID); }
      else { f[i]= 0; }
   }
   return(n);
} // convertRawAGR

int elapsedStrideRTS
(
   F32   f[],
   const int   nF,
   const RawTimeStamp   rawTS[],
   const int   stride,
   const RawTimeStamp   *pRef
)
{
   for (int i=0; i<nF; i++) { f[i]= timeDiff(pRef, rawTS + i * stride); }
   return(nF);
} // elapsedStrideRTS

int elapsedEstStrideRTS
(
   F32   f[],
   const int   nF,
   const RawTimeStamp   rawTS[],
   const int   stride,
   const RawTimeStamp   *pRef,
   const EstimatorRTS   *pEst
)
{
   for (int i=0; i<nF; i++)
   {
      f[i]= timeEstDiff(pRef, rawTS + pEst->idx[0] + i * stride, rawTS + pEst->idx[1] + i * stride, pEst->r);
   }
   return(nF);
} // elapsedStrideRTS

int elapsedTrnStrdRTS
(
   F32   f[],
   const int   nF,
   const RawTimeStamp   rawTS[],
   const int   nRow,
   const int   stride,
   const RawTimeStamp   *pRef
)
{ // strided access / 2D transposition
   int offs=0, nR= 0;
   if (nRow > 0)
   {
      do
      {
         for (int i=0; i<nRow; i++) { f[nR++]= timeDiff(pRef, rawTS + offs + i*stride); }
         offs++;
      } while (nR < nF);
   }
   return(nR);
} // elapsedTrnStrdRTS



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

void analyseInterval (const F32 t[], const int nM, const int nN)
{
   StatMomD1R2 sm={0,};
   StatResD1R2 sr;
   for (int i=0; i<nM; i++)
   {
      for (int j=1; j<nN; j++)
      {
         int k= i + j * nM;
         F32 dt= t[k] - t[k-nM];
         sm.m[1]+= dt;
         sm.m[2]+= dt * dt;
      }
   }
   sm.m[0]= (nN-1) * nM;
   statMom1Res1(&sr, &sm, sm.m[0]-1);
   F32 s= sqrt(sr.v), r= rcpF(sr.m);
   report(LOG0,"Mux channel inter-sample mean, stdev : %G, %G, D=%G, rate=%G\n", sr.m, s, s * r, r);
} // analyseInterval


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
   EstimatorRTS *pEstRTS=NULL;
   ExtRawTiming extT[ADS1X_MUX_MAX], *pET=NULL;
   F32 sumTransDT[3]={0};  // StatMomD1R2 md1r2;
   AutoExtCtx aec;
   int n=0, r=-1;

   if (nMax > 0)
   {
      r= setupAEC(&aec, pP, pM, pCfgPB, pC);
      if (r < 0) { return(r); }
      pEstRTS= getTimeEstimator(pM->timeEst);
      if (pDT || (pM->modeFlags & ADS1X_MODE_XTIMING))
      {
         pET= extT+0;
         if (NULL == pRefTS) { pRefTS= aec.targetTS+0; }
      }
      do
      {
         r= readAutoRawADS1x(aec.rawAGR, pET, aec.arc.nMux, aec.targetTS+1, &(aec.arc)); //LOG("readAutoRawADS1x() - r=%d\n", r);
         if (r > 0)
         {
            convertRawAGR(rV+n, aec.rawAGR, aec.arc.nMux, pP);
            if (pDT)
            {  // Convert post-reading time stamp to elapsed since reference
               if (NULL == pEstRTS) // pM->timeEst < EXT_RTS_COUNT)
               { elapsedStrideRTS(pDT+n, aec.arc.nMux, pET[0].ts+pM->timeEst, EXT_RTS_COUNT, pRefTS); }
               else
               { elapsedEstStrideRTS(pDT+n, aec.arc.nMux, pET[0].ts, EXT_RTS_COUNT, pRefTS, pEstRTS); }
            }

            if FLAGS_ARE_SET( ADS1X_MODE_XTIMING|ADS1X_MODE_VERBOSE, pM->modeFlags)
            { // extended raw data debug dump...
static const char *dtID[]= {"rdvE","rdvB","cfgE","cfgB","muxB"};
               F32 t[EXT_RTS_COUNT*ADS1X_MUX_MAX], timeScale= 1000;
               int k= 0;

               report(LOG0,"%d..%d\n",n,n+aec.arc.nMux-1);
               report(LOG0,"Raw\t");
               for (int i=0; i<aec.arc.nMux; i++)
               {
                  U8 g= ads1xGetGain(aec.rawAGR[i].cfgRB0);
                  U8 t= aec.rawAGR[i].flSt & RMG_MASK_TRNS;
                  U8 f= aec.rawAGR[i].flSt >> 4;	// rawAGR[i].res,
                  report(LOG0,"%d,%X,%d%c", g, f, t, gSepCh[i >= (aec.arc.nMux-1)]);
               }
               // Transpose timings into mux then category order
               elapsedTrnStrdRTS(t, aec.arc.nMux * EXT_RTS_COUNT, pET[0].ts+0, aec.arc.nMux, EXT_RTS_COUNT, pRefTS);
               for (int j=0; j<EXT_RTS_COUNT; j++)
               {
                  report(LOG0,"%s (ms)\t", dtID[j]);
                  for (int i=0; i<aec.arc.nMux; i++) { report(LOG0,"%G%c", t[k+i] * timeScale, gSepCh[i >= (aec.arc.nMux-1)]); }
                  k+= aec.arc.nMux;
               }
               for (int i=0; i<aec.arc.nMux; i++)
               {
                  const F32 dt1= timeDiff(pET[i].ts+EXT_RTS_WRCFG_BGN, pET[i].ts+EXT_RTS_WRCFG_END);
                  const F32 dt2= timeDiff(pET[i].ts+EXT_RTS_RDVAL_BGN, pET[i].ts+EXT_RTS_RDVAL_END);
                  //sumTransDT[0]+= 2;
                  sumTransDT[1]+= dt1 + dt2;
                  sumTransDT[2]+= dt1*dt1 + dt2*dt2;
               }
            }
            n+= aec.arc.nMux;
         }
         if (aec.outerIvlNanoSec > 0)
         {  RawTimeStamp wait[1];
            timeSetTarget(aec.targetTS+1, NULL, aec.outerIvlNanoSec, TIME_MODE_RELATIVE);
            timeSpinWaitUntil(wait+0, aec.targetTS+1);
         }
      } while ((n <= (nMax-aec.arc.nMux)) && (r > 0));
      //if (refTS.tv_nsec > 0) { ; } ???
      if FLAGS_ARE_SET( ADS1X_MODE_XTIMING|ADS1X_MODE_VERBOSE, pM->modeFlags)
      {
         sumTransDT[0]= 2 * n;
         reportStat(sumTransDT, 1000, sumTransDT[0]-1);
      }
      if (pCfgPB) { memcpy(pCfgPB, aec.arc.cfgPB, ADS1X_NRB); } // Copy back any changes
   }
   return(n);
} // readAutoADS1X

int testAutoGain
(
   const int maxSamples,
   const LXI2CBusCtx  *pC,
   const ADSInstProp  *pP,
   const ADSReadParam *pM,
   const ADSResDiv    *pRD
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
      analyseInterval(pDT, pM->nMux, maxSamples);
      {  // dump
         const U8 nMux= pM->nMux; // Hacky...
         const F32 timeScale= 1000;
         int n= 0;
         report(LOG0,"\t");
         for (int i=0; i<nMux; i++) { LOG("%s%c", ads1xMuxStr(pM->mux[i]), gSepCh[i >= (nMux-1)] ); }
         while (n < r)
         {
            report(LOG0,"[ %d..%d ] : \n", n, n+nMux-1);
            report(LOG0,"dt (ms)\t\t");
            for (int i=0; i<nMux; i++) { report(LOG0,"%G%c", pDT[n+i] * timeScale, gSepCh[i >= (nMux-1)]); }
            report(LOG0,"V (V)\t\t");
            for (int i=0; i<nMux; i++) { report(LOG0,"%G%c", pV[n+i], gSepCh[i >= (nMux-1)]); }

            if (pRD)
            {
               F32 rScale= 0.001, r[ADS_RES_DIV_MAX];
               adsGetResDiv(r, pV+n, nMux, pRD, pP);
               report(LOG0,"R (kOhm)\t",n);
               for (int i=0; i<nMux; i++)
               {
                  //F32 v0= pV[n+i]; // v1= pP->vdd - v0
                  //F32 res= pResDiv[i] * v0 * rcpF(pP->vdd - v0);
                  report(LOG0,"%G%c", r[i] * rScale, gSepCh[i >= (nMux-1)]);
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
      ads1xGenCfg(fpb.rc.cfg+1, pM->mux[0], ADS1X_GAIN_6V144, rateID, ADS1X_CMP_DISABLE);
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

#include "ads1xTxtIF.h"

int ads1xMuxMap (U8 m[], const char *s, const U8 hwID)
{
   return muxMapFromA(m, ADS1X_MUX_MAX, s, hwID);
} // ads1xMuxMap

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
   "/dev/i2c-1", ADS10, 0x48, 0
};

void usageMsg (const char name[])
{
static const char optCh[]="adimnrAvhMT";
static const char argCh[]="######    #";
static const char *desc[]=
{
   "I2C bus address: 2digit hex (no prefix)",
   "device index (-> path /dev/i2c-# )",
   "hardware ID: 0 -> ads10xx, 1 -> ads11xx",
   "multiplexer channel count",
   "max samples",
   "sample rate",
   "auto gain mode",
   "verbose diagnostic messages",
   "help (display this text)",
   "multiplexor selection (0/G,1/G,0/1,1/2 etc.)"
   "timestamp control"
};
   const int n= sizeof(desc)/sizeof(desc[0]);
   report(OUT,"Usage : %s [-%s]\n", name, optCh);
   for (int i= 0; i<n; i++)
   {
      report(OUT,"\t%c %c - %s\n", optCh[i], argCh[i], desc[i]);
   }
} // usageMsg

void paramDump (const ADSReadParam *pP)
{
   report(OUT,"\trate= %d, %d (Hz)", pP->rate[0], pP->rate[1]);
   report(OUT,"\tmux[%d]={", pP->nMux); reportBytes(OUT, pP->mux, pP->nMux);
   report(OUT,"%s", "}\n");
   report(OUT,"\tmaskAG= 0x%02X, timeEst= 0x%02X, modeFlags= 0x%02X\n", pP->maskAG, pP->timeEst, pP->modeFlags);
} // paramDump

void argDump (const ADS1XArgs *pA)
{
   report(OUT,"Device: devPath=%s, hwID:%d, busAddr=%02X\n", pA->devPath, pA->hwID, pA->busAddr);
   report(OUT,"\tflags=%02X maxS=%d\n", pA->testFlags,  pA->maxSamples);
   paramDump(&(pA->param));
} // argDump

#define ARG_AUTO    (1<<2)
#define ARG_HELP    (1<<1)
#define ARG_VERBOSE (1<<0)

void argTrans (ADS1XArgs *pA, int argc, char *argv[])
{
   int i, c, t;
   do
   {
      c= getopt(argc,argv,"a:d:i:m:n:r:M:T:Ahv");
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
         case 'M' :
            t= ads1xMuxMap(pA->param.mux, optarg, pA->hwID);
            //report(OUT,"* mux %d,%d *\n",pA->param.mux[0],pA->param.mux[1]);
            if (t > 0) { pA->param.nMux=  t; }
            break;
         case 'T' :
            sscanf(optarg, "%X", &t);
            //if ((t < 0) || (t > EXT_RTS_COUNT)) { pA->param.timeEst= EXT_RTS_COUNT; }
            pA->param.timeEst= t;
            //packTE(EXT_RTS_WRCFG_END, EXT_RTS_RDVAL_BGN, 0.5); // EXT_RTS_RDVAL_END;
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
      const ADSInstProp *pP= adsInitProp(NULL, 3.31, gArgs.hwID, gArgs.busAddr);
      gArgs.param.modeFlags|= ADS1X_MODE_XTIMING;
      if (gArgs.testFlags & ARG_AUTO)
      {
         ADSResDiv resDiv= {{2200, 330, 330, 10000}};
         r= testAutoGain(gArgs.maxSamples, &gBusCtx, pP, &(gArgs.param), &resDiv);
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
