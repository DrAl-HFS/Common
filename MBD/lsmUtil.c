// Common/MBD/lsmUtil.c - utility code for STMicro. lsm9ds1 IMU
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug 2020 - Jan 2021

// HW Ref: https://www.st.com/resource/datasheet/lsm9ds1.pdf


/* IMU test hacks */

#include "lsmUtil.h"

// LSM9DS1 iNEMO is a dual device package having two I2C/SPI interfaces: one
// each for the accelerometer and magnetometer. There are a signifiant number
// of control and data registers within each interface - still considering
// how to make this more manageable without introducing gross inefficiencies...


/***/

// Register frames for I2C / SPI interfacing - each includes leading register
// address byte for simple message handling
typedef struct // I2C packet/frames for 16b acc. & temperature measurement
{                // X,Y,Z register tuple can be read as single block
   U8 temp[3]; // 0x15:16 - temperature in +-1/16ths Celcius offset from 25C
   //U8 statAng[8]; // 0x17 combined status byte & angular output
   U8 ang[7]; // X,Y,Z -> 0x18:19, 1A:1B, 1C:1D
   U8 lin[7]; // X,Y,Z -> 0x28:29, 2A:2B, 2C:2D
} LSMAccValI16RegFrames;

typedef struct // I2C packet/frames for acc. control registers
{
   U8 actInt[10]; // 0x04..0D activity/interrupt control
   U8 ang[4]; // 0x10..12
   U8 lin[4];  // 0x1F..21
   U8 r8_10[4]; // 0x22..24
   U8 r4[2];   // 0x1E
   U8 fifo[3]; // 0x2E,2F
   U8 intr[9]; // 0x30..37
} LSMAccCtrlRegFrames;

typedef struct // I2C packet/frames for 16b mag. calibration/measurement
{
   U8 offs[7]; // 0x05..
   //U8 statMag[8]; // 0x27 combined status byte & mag output?
   U8 mag[7]; // 0x28..
} LSMMagValI16RegFrames;

typedef struct // I2C packet/frames for mag. control registers
{
   U8 r1_5[6]; // 0x20...24
   U8 stat[2]; // 0x27
   U8 intr[4]; // 0x30
} LSMMagCtrlRegFrames;

typedef struct
{
   LSMAccValI16RegFrames   avI16;
   LSMAccCtrlRegFrames     actrl;
   LSMMagValI16RegFrames   mvI16;
   LSMMagCtrlRegFrames     mctrl;
} IMURegFrames;

/***/


void initFrames (IMURegFrames *pR)
{
   memset(pR, 0, sizeof(*pR));
   // accel.
   pR->avI16.temp[0]= LSM_REG_TEMP;
   //pR->avI16.statAng[0]= LSM_REG_ASTAT;
   pR->avI16.ang[0]= LSM_REG_ANG_X;
   pR->avI16.lin[0]= LSM_REG_LIN_X;

   pR->actrl.actInt[0]= LSM_REG_ACTRL_ACT1;

   pR->actrl.ang[0]= LSM_REG_ACTRL_ANG1;
   pR->actrl.lin[0]= LSM_REG_ACTRL_LIN5;

   pR->actrl.r8_10[0]= LSM_REG_ACTRL08;

   pR->actrl.r4[0]= LSM_REG_ACTRL04;
   pR->actrl.fifo[0]= LSM_REG_ACTRL_FIFO1;
   pR->actrl.intr[0]= LSM_REG_ACTRL_INTR1;

   // mag.
   pR->mvI16.offs[0]= LSM_REG_MAG_OFFSX;
   //pR->mvI16.statMag[0]= LSM_REG_MSTAT;
   pR->mvI16.mag[0]= LSM_REG_MAG_X;

   pR->mctrl.r1_5[0]= LSM_REG_MCTRL01; // 0x20...
   pR->mctrl.stat[0]= LSM_REG_MSTAT; // 0x27
   pR->mctrl.intr[0]= LSM_REG_MCTRL_INTR1; // 0x1E
} // initFrames

int lsmIdentify (const LXI2CBusCtx *pC, const U8 dev[2])
{
   //static const char* ms[]={"FAIL","PARTIAL","OK"};
   int r;
   U8 regID[2][2]={{0x0F,0},{0x0F,0}};
   r= lxi2cReadRB(pC, dev[0], regID[0], 2); if (r < 0) { regID[0][0]= 0; }
   r= lxi2cReadRB(pC, dev[1], regID[1], 2); if (r < 0) { regID[1][0]= 0; }
   r= (regID[0][1] == 0x68) + (regID[1][1] == 0x3D);
   //TRACE_CALL("(..[%x,%x]) - ID:", dev[0], dev[1]);
   LOG("lsmIdentify(..[%x,%x]) - regID:", dev[0], dev[1]); reportBytes(LOG0, regID[0], sizeof(regID));// LOG("- %s\n",ms[r]);
   return(r);
} // lsmIdentify

int vscaleNI16LEtoF32 (F32 r[], const U8 b[], const int nb, const F32 s)
{
   int i;
   for (i=0; i<nb; i+= 2) { r[i>>1]= rdI16LE(b+i) * s; }
   return(i>>1);
} // vscaleNI16LEtoF32

int vaddNI16LEtoI32 (I32 r[], const U8 b[], const int nb)
{
   int i;
   for (i=0; i<nb; i+= 2) { r[i>>1]+= rdI16LE(b+i); }
   return(i>>1);
} // vaddNI16LEtoI32

typedef struct
{
   F32 magSR, magFS;
} LSMMagCfgTrans;

void lsmTransMagCfg (LSMMagCfgTrans *pMCT, const U8 cfg[5])
{
   //static const F32 rate[]={ 0.625, 1.25, 2.5, 5, 10, 20, 40, 80 }; // 5 * 2^(n-3)
   //static const U8 scale[]={ 4, 8, 12, 16 }; // 4 * (n+1)
   const U8 rn= (cfg[0] >> 2) & 0x7;
   const U8 sn= (cfg[1] >> 5) & 0x3;
   pMCT->magSR=   5 * pow(2,rn-3);  // rate[rn];
   pMCT->magFS=  4 * (sn+1);        // scale[sn];
} // lsmTransMagCfg

enum MagMode
{
   CONTINUOUS=0,
   SINGLE=1,
   //OFF=2,
   OFF=3
};
//LSMMagCtrlRegFrames *pF, U8 *pC= pF->r1_5+1;
int lsmMagSetRateMode (U8 cfg[], U8 rate, enum MagMode m)
{

   if ((rate & 0x7) != rate) return(-1);
   cfg[0]= (cfg[0] & ~(0x7<<2)) | rate<<2;
   cfg[2]= (cfg[2] & ~0x3) | m;
   return(3);
} // lsmMagSetRateMode

F32 mag2NF32 (const F32 v[], const int n)
{
   F32 m2= 0;
   for (int i=0; i<n; i++) { m2+= v[i] * v[i]; }
   return(m2);
} // mag2NF32

F32 magV3F32 (const F32 v[3]) { return sqrtf( mag2NF32(v,3) ); }

F32 tiltDeg2F32 (const F32 opp, const F32 adj) { return atan2(opp, adj) * 180.0/M_PI; } // tiltDeg2F32

F32 bearingDegV2F32 (const F32 v[2])
{  // NB: transpose x,y to move reference from East (+X) to North (+Y)
   F32 bd= tiltDeg2F32(v[1], v[0]); // atan2(v[1], v[0]) * 180.0/M_PI;
   if (bd<0) bd+= 360.0;
   return(bd);
} // bearingDegV2F32

void dumpNIVD (const I32 v[], const int n, const int d, const char ftr[])
{
   char buf[64];
   int nCh, j= 0;
   buf[0]='(';
   while (j < n)
   {
      nCh= 1+snprintf(buf+1, sizeof(buf)-1, "%ld", v[j]);
      for (int i= 1; i<d; i++)
      {
         nCh+= snprintf(buf+nCh, sizeof(buf)-nCh, ",%ld", v[j+i]);
      }
      j+= d;
      if (n == j) { LOG("%s)%s",buf,ftr); }
      else { LOG("%s) ",buf); }
   }
} // dumpNIVD

void setScaleV3I (I32 v[3], const F32 x, const F32 y, const F32 z, const F32 s) { v[0]= x * s; v[1]= y * s; v[2]= z * s; }

void dumpRawV (const char hdr[], const U8 raw[6], const F32 s, const char ftr[])
{
   F32 v3f[3];
   vscaleNI16LEtoF32(v3f, raw, 6, s);
   LOG("%s(%.3f, %.3f, %.3f)%s", hdr, v3f[0], v3f[1], v3f[2], ftr);
} // dumpRawV

void dumpOffsetRawV (const char hdr[], const U8 raw[6], I32 o[3], const F32 s, const char ftr[])
{
   I32 v3i[3]={o[0], o[1], o[2]};
   vaddNI16LEtoI32(v3i, raw, 6);
   LOG("%s(%.3f, %.3f, %.3f)%s", hdr, v3i[0]*s, v3i[1]*s, v3i[2]*s, ftr);
} // dumpOffsetRawV

void dumpMagTiltRawV (const char hdr[], const U8 raw[6], const F32 s, const char ftr[])
{
   F32 v3f[3];
   vscaleNI16LEtoF32(v3f, raw, 6, s);
   LOG("%s(%.3f, %.3f, %.3f)", hdr, v3f[0], v3f[1], v3f[2]);
   LOG("\t|XYZ|= %.3f\ttilt:XZ,YZ= %.3f, %.3f (deg.)%s", magV3F32(v3f), tiltDeg2F32(v3f[0],v3f[2]), tiltDeg2F32(v3f[1],v3f[2]), ftr);
} // dumpMagTiltRawV

void dumpMagBearRawV (const char hdr[], const U8 raw[6], const F32 s, const char ftr[])
{
   F32 v3f[3];
   vscaleNI16LEtoF32(v3f, raw, 6, s);
   LOG("%s(%.3f, %.3f, %.3f)", hdr, v3f[0], v3f[1], v3f[2]);
   LOG("\t|XYZ|= %.3f\tbearXY= %.1f (deg.)%s", magV3F32(v3f), bearingDegV2F32(v3f), ftr);
} // dumpMagBearRawV

void magTest (const LXI2CBusCtx *pC, const U8 dev, IMURegFrames *pF, const U8 maxIter)
{
   LSMMagCfgTrans mct={0,};
   int r;

   r= lxi2cReadMultiRB(pC, NULL, dev, pF->mvI16.offs, sizeof(pF->mvI16.offs), 2);
   if (r >= 0)
   {
      dumpRawV("RAW: offset", pF->mvI16.offs+1, 1.0, "\n");
      dumpMagBearRawV("magnetic= ", pF->mvI16.mag+1, 1.0, "\n");
   }

   lsmMagSetRateMode(pF->mctrl.r1_5+1, 7, CONTINUOUS);
   r= lxi2cWriteRB(pC, dev, pF->mctrl.r1_5, sizeof(pF->mctrl.r1_5));

   lsmTransMagCfg(&mct, pF->mctrl.r1_5+1);
   LOG("rate=%G, scale=%G\n", mct.magSR, mct.magFS);

   if (mct.magSR > 0)
   {
      int ivl= 1000000 / mct.magSR;
      const F32 mS= mct.magFS / 0x7FFF;

      LOG("Mag ON, interval %dus\n", ivl);

      for (U8 i=0; i<maxIter; i++)
      {
         usleep(ivl); //ivl);
         r= lxi2cReadRB(pC, dev, pF->mctrl.stat, sizeof(pF->mctrl.stat));
         if (r > 0) { LOG("S%02x\t", pF->mctrl.stat[1]); }
         r= lxi2cReadRB(pC, dev, pF->mvI16.mag, sizeof(pF->mvI16.mag));
         if (r >= 0) { dumpMagBearRawV("mag= ", pF->mvI16.mag+1, mS, "\n"); }
         //r= lxi2cReadRB(pC, dev, pF->mctrl.stat, sizeof(pF->mctrl.stat));
         //if (r > 0) { LOG("S%02x\n", pF->mctrl.stat[1]); }
      }
      lsmMagSetRateMode(pF->mctrl.r1_5+1, 7, OFF);
      r= lxi2cWriteRB(pC, dev, pF->mctrl.r1_5, sizeof(pF->mctrl.r1_5));
   }
} // magTest

enum AngUnit
{
   ANG_RAD=0,
   ANG_DEG=1
}; // AngUnit

enum LSMLinAngRate
{
   LAR_OFF= 0, // Sample rate (Hz)
   LAR_1X=  1, // L 10 / A 14.9
   LAR_5X=  2, // L 50 / A 59.5
   LAR_119= 3,
   LAR_238= 4,
   LAR_476= 5,
   LAR_952= 6
}; // LSMLinAngRate

typedef struct
{
   F32 angRate, angFSD;
   F32 linRate, linFSG;
} LSMAccCfgTrans;

typedef struct
{
   I32 aSM[3], lSM[3];
} CalDat;

void lsmAccSetMode (U8 ang[4], U8 lin[4], const enum LSMLinAngRate m)
{
   ang[0]= (ang[0] & 0x1F) | (m<<5); // ODR: 0, 14.9, 59.5...
   ang[2]= 1<<7; // HP_EN

   lin[1]= (lin[1] & 0x1F) | (m<<5); // ODR: 0, 10, 50..
} // lsmAccSetMode

void lsmTransAccCfg (LSMAccCfgTrans *pACT, const U8 ang[4], const U8 lin[4], const enum AngUnit u)
{
   static const F32 angr[]={ 0, 14.9, 59.5, 119, 238, 476, 952, 0 };
   static const U16 linr[]={ 0, 10, 50, 119, 238, 476, 952, 0 };
   static const U16 afsd[]={ 245, 500, 0, 2000 }; // degrees-per-second
   static const U8 lfsg[]={ 2, 16, 4, 8 }; // g=9.81 bizarre order
   const U8 nag= (lin[1] >> 3) & 0x3; // r20
   const U8 nar= (ang[0] >> 5) & 0x7;
   const U8 nlg= (lin[1] >> 3) & 0x3; // r20
   const U8 nlr= (lin[1] >> 5) & 0x7;
   pACT->angFSD= afsd[nag];
   if (ANG_RAD == u) { pACT->angFSD*= M_PI / 180; }
   pACT->angRate= angr[nar];
   pACT->linFSG= lfsg[nlg];
   pACT->linRate= linr[nlr];
} // lsmTransAccCfg

void accTest
(
   const LXI2CBusCtx *pC,
   const U8          dev,
   IMURegFrames      *pF,
   const enum LSMLinAngRate lar,
   const U8            maxIter,
   const U8            maxCal
)
{
   CalDat cal={{2775, 6053, -278},{-460, 246, 16203}};
   LSMAccCfgTrans act={0,};
   int r, ivl=0;

   LOG("%s","Predef. cal. - "); dumpNIVD((void*)&cal, 6, 3, "\n");
   r= lxi2cReadRB(pC, dev, pF->avI16.temp, sizeof(pF->avI16.temp));
   if (r >= 0)
   {
      I16 rawT= rdI16LE(pF->avI16.temp+1);
      LOG("T= %d -> %GC\n", rawT, 25.0 + rawT * 1.0 / 16);
   }

   lsmAccSetMode(pF->actrl.ang+1, pF->actrl.lin+1, lar);
   r= lxi2cWriteMultiRB(pC, NULL, dev, pF->actrl.ang, sizeof(pF->actrl.ang), 2);

   lsmTransAccCfg(&act, pF->actrl.ang+1, pF->actrl.lin+1, ANG_DEG);
   LOG("rate: A=%G, L=%G\n", act.angRate, act.linRate);
   LOG("FS  : A=%G deg/s, L=%G g\n", act.angFSD, act.linFSG);
   ivl= 1000000 / MIN(act.angRate, act.linRate); // sync to slowest
   const F32 sA= act.angFSD / 0x7FFF;
   const F32 sL= act.linFSG / 0x7FFF;

   if (ivl > 0)
   {
      I32 nSM= 0;

      LOG("Acc ON, interval %dus\n", ivl);
      if (maxCal > 0)
      {
         memset(&cal, 0, sizeof(cal));
         for (U8 i=0; i<maxCal; i++)
         {
            usleep(ivl);

            r= lxi2cReadMultiRB(pC, NULL, dev, pF->avI16.ang, sizeof(pF->avI16.ang), 2);
            if (r >= 0)
            {
               vaddNI16LEtoI32(cal.aSM, pF->avI16.ang+1, 6);
               vaddNI16LEtoI32(cal.lSM, pF->avI16.lin+1, 6);
               nSM++;
            }
         }
         if (nSM > 1)
         {  // Sums to means
            F32 s= 1.0 / nSM;
            for (int i= 0; i<3; i++)
            {
               cal.aSM[i]= -cal.aSM[i] * s;
               cal.lSM[i]= -cal.lSM[i] * s;
            }
            LOG("%s","New cal. - "); dumpNIVD(cal.aSM+0, 6, 3, "\n");
            //LOG("ang->(%f,%f,%f)\n", cal.aSM[0] * sA, cal.aSM[1] * sA, cal.aSM[2] * sA);
            //LOG("lin->(%f,%f,%f)\n", cal.lSM[0] * sL, cal.lSM[1] * sL, cal.lSM[2] * sL);
            {
               I32 eps[3];
               setScaleV3I(eps, -0.449, -1.024, 0.037, 1.0 / sA);
               LOG("%s: ", "eps"); dumpNIVD(eps, 3, 3, "\n");
            }
         }
      }
      for (U8 i=0; i<maxIter; i++)
      {
         usleep(ivl);
         r= lxi2cReadMultiRB(pC, NULL, dev, pF->avI16.ang, sizeof(pF->avI16.ang), 2);
         if (r >= 0)
         {
            dumpOffsetRawV("angVel= ",pF->avI16.ang+1, cal.aSM, sA, "\t");
            dumpMagTiltRawV("linAcc= ", pF->avI16.lin+1, sL, "\n");
         }
      }
   }
   lsmAccSetMode(pF->actrl.ang+1, pF->actrl.lin+1, 0); // off
   r= lxi2cWriteMultiRB(pC, NULL, dev, pF->actrl.ang, sizeof(pF->actrl.ang), 2);
} // accTest

int testIMU (const LXI2CBusCtx *pC, const U8 dev[2], const U8 maxIter)
{
   int r=-1;

   if (lsmIdentify(pC,dev) > 0)
   {
      IMURegFrames frm;
      initFrames(&frm);

      r= I2C_BYTES_NCLK(sizeof(frm.avI16.ang)) * 3;
      LOG("testIMU() - Clk%d, pkt.len=%d, max rate= %d pkt/sec\n", pC->clk, r, pC->clk / r);

      if (0 != dev[0])
      {
         // Accelerometer: control & measure
         r= lxi2cReadMultiRB(pC, NULL, dev[0], frm.actrl.ang, sizeof(frm.actrl.ang), 3);
         if (r >= 0)
         {
            LOG("%s.%s= ", "actrl", "ang");
            reportBytes(LOG0,frm.actrl.ang+1, sizeof(frm.actrl.ang)-1);
            LOG("%s.%s= ", "actrl", "lin");
            reportBytes(LOG0,frm.actrl.lin+1, sizeof(frm.actrl.lin)-1);
            LOG("%s.%s= ", "actrl", "r8_10");
            reportBytes(LOG0,frm.actrl.r8_10+1, sizeof(frm.actrl.r8_10)-1);
         }
         for (enum LSMLinAngRate lar= LAR_119; lar <= LAR_952; lar++) // LAR_119 LAR_238; LAR_952)
         {
            accTest(pC, dev[0], &frm, lar, maxIter, 0);
            LOG("\n%s\n", "-");
         }
         LOG("\n%s\n", "---");
      }

      if (0 != dev[1])
      { // Magnetometer: control & measure
         r= lxi2cReadRB(pC, dev[1], frm.mctrl.r1_5, sizeof(frm.mctrl.r1_5));
         if (r >= 0)
         {
            LOG("%s.%s= ", "mctrl", "r1_5");
            reportBytes(LOG0, frm.mctrl.r1_5+1, sizeof(frm.mctrl.r1_5)-1);
         }
         magTest(pC, dev[1], &frm, maxIter);
         LOG("\n%s\n", "---");
      }
   }
   return(r);
} // testIMU

#ifdef LSM_MAIN

#include "lxSPI.h"

LXI2CBusCtx gI2C={0,-1};
LXSPICtx gSPI={0,-1};

// Experiment with SPI + I2C dual connection: SCL(K) requires
// isolation (diodes?) to function. SDA/MOSI may also ?
int main (int argc, char *argv[])
{
   int r= 0;
#if 1
   SPIProfile prof;

   prof.kdmf= SPI_MODE_3; // | SPI_CS_HIGH; // SPI_MODE_3=SPI_CPOL|SPI_CPHA
   prof.clk= 8E6;
   prof.delay= 0;
   prof.bpw= 8;
   // mode 1,2 -> 0xF,0 (mode 0,3 -> 0,0)
   if (lxSPIOpen(&gSPI, "/dev/spidev0.1", &prof))
   {
      U8 regID[2]={0x0F,0}, res[2];
      for (int i=0; i<5; i++)
      {
         sleep(3);
         res[0]= res[1]= 0xa5;
         r= lxSPIReadWrite(&gSPI, res, regID, 2);
         LOG("[%d]: r=%d : 0x%X,%0X\n", i, r, res[0], res[1]);
      }
      lxSPIClose(&gSPI);
   }
#else
   if (lxi2cOpen(&gI2C, "/dev/i2c-1", 400))
   {  // I2C addr: AG= 6B/6A
      const U8 ag_m[]={0x6a,0x1e}; // NB: AG alternate addr (SDO-AG pulldown disconnected)
      testIMU(&gI2C, ag_m, 3);

      lxi2cClose(&gI2C);
   }
#endif
   return(r);
} // main

#endif // LSM_MAIN
