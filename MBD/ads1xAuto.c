// Common/MBD/ads1xAuto.c - automated reading of TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: AGPL3
// (c) Project Contributors June 2021

#include "ads1xAuto.h"


/***/

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


/***/

// Read a set of mux channels, updating the gain setting for each to maximise precison.
// Performs multiple reads per channel as appropriate, if permitted.
// Optionally provides transaction timing information
int readAutoRawADS1x (RawAGR agr[], ExtRawTiming *pT, int nR, RawTimeStamp *pTarget, AutoRawCtx *pARC)
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
         pARC->cfgPB[1]= agr[i].cfgRB0[0]; // Sets Gain, Mux, flags (Single Shot & Start)
         if (pT) { timeStamp(pT[i].ts+EXT_RTS_WRCFG_BGN); }  // Config write begin
         r= lxi2cWriteRB(pARC->pC, pARC->busAddr, pARC->cfgPB, ADS1X_NRB);
         if (r > 0)
         {  // Config write complete, conversion started
            timeSetTarget(wait+1, wait+0, pARC->ivlNanoSec[0], TIME_MODE_NOW);
            iTrans++;
            timeSpinWaitUntil(wait+2, wait+1); // Wait for conversion
            r= lxi2cReadRB(pARC->pC, pARC->busAddr, resPB, ADS1X_NRB);
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

int setupAEC (AutoExtCtx *pAEC, const ADSInstProp *pP, const ADSReadParam *pM, const U8 * pCfgPB, const LXI2CBusCtx *pC)
{
   int r= -1;

   pAEC->arc.nMux= setupRawAGR(pAEC->rawAGR, pM->mux, pM->nMux, pM->maskAG, ADS1X_GAIN_4V096);
   if (pAEC->arc.nMux > 0)
   {
      r= 0;
      pAEC->arc.pC= pC;
      if (pCfgPB) { memcpy(pAEC->arc.cfgPB, pCfgPB, ADS1X_NRB); } // Paranoid VALIDATE ?
      else
      {
         pAEC->arc.cfgPB[0]= ADS1X_REG_CFG;
         r= lxi2cReadRB(pAEC->arc.pC, pP->busAddr, pAEC->arc.cfgPB, ADS1X_NRB);
         if (r <= 0) { return(r); }
      }

      // Set hard & soft rates
      U16 hardRate, softRate=0;
      pAEC->arc.ivlNanoSec[0]= ads1xConvIvl(&hardRate, pAEC->arc.cfgPB+1, pP->hwID);
      if (pM->rate[1] >= hardRate) { pAEC->arc.ivlNanoSec[1]= 0; } // no extra delays
      else { pAEC->arc.ivlNanoSec[1]= NANO_TICKS / (long)(pM->rate[1]); }
      if (pM->rate[0] > 0)
      {
         U16 consRate= MIN(hardRate, pM->rate[1]) / pAEC->arc.nMux; // *nAG
         if (pM->rate[0] > consRate) { WARN_CALL("() - requested rate %d exceeds constraint %d\n", pM->rate[0], consRate); }
         if (pM->rate[0] < consRate)
         {  // delay required
            softRate= pM->rate[0];
            pAEC->outerIvlNanoSec= NANO_TICKS / (long)softRate;
         }
      }
      //LOG_CALL("() - rate=%d -> ivl=%d\n", r, pAEC->arc.ivl);
      pAEC->arc.fsr= ads1xRawFSR(pP->hwID);
      pAEC->arc.busAddr= pP->busAddr;
      if (softRate > 0)
      {
         const long i2cTransWait= ADS1X_TRANS_NCLK * (float)NANO_TICKS / pC->clk;
         const long hardTime= (pAEC->arc.ivlNanoSec[0] + 2 * i2cTransWait);
         const long window= pAEC->outerIvlNanoSec / pAEC->arc.nMux;
         pAEC->arc.maxTrans= 2*(window / hardTime);
         LOG("transWait=%d, hardTime=%d, window=%d -> maxTrans=%d\n", i2cTransWait, hardTime, window, pAEC->arc.maxTrans);
      } else { pAEC->arc.maxTrans= 2; }

      timeSetTarget(pAEC->targetTS+1, pAEC->targetTS+0, 100, TIME_MODE_NOW); // start at +100ns
   }
   return(r);
} // setupAEC

