// Common/MBD/lxI2C.c - I2C bus utils for Linux
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Feb 2018 - Aug 2020

// Linux Ref: https://www.kernel.org/doc/html/latest/driver-api/i2c.html

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "lxI2C.h"


/***/

#define I2C_BYTES_NCLK(b) ((1+(b)) * 9 + 1)

#define LX_I2C_TRANS_NM (2) // number of i2c_msg blocks per transaction
#define LX_I2C_FLAG_TRACE (1<<4)

// Note: adjacent string concatenation, compiler defined symbol, variadic args
#define LX_TRC0(fmt,...) TRACE_CALL(fmt,__VA_ARGS__)
#define LX_TRC1(fmt,...) report(TRC1,fmt,__VA_ARGS__)

typedef struct // Global Module Context: debug flags etc...
{
   U8 flags;
   U8 pad[3];
} LXI2CCtx;

static LXI2CCtx gCtx={LX_I2C_FLAG_TRACE, }; // {{0,-1},{0,-1}},0,0,};


/***/

Bool32 lxi2cOpen (LXI2CBusCtx *pBC, const char *path, const int clk)
{
   struct stat st;
   int r= -1;
   if ((0 == stat(path, &st)) && S_ISCHR(st.st_mode)) // ensure device exists
   {
      pBC->fd= open(path, O_RDWR); // |O_DYSNC ?? |O_NONBLOCK);
      if (pBC->fd >= 0)
      {
         r= ioctl(pBC->fd, I2C_FUNCS, &(pBC->flags));
         if ((r < 0) || (0 == (pBC->flags & I2C_FUNC_I2C)))
         {
            ERROR_CALL("(.. %s) - ioctl(.. I2C_FUNCS ..) -> %d\n", path, r);
            //report(ERR1,"(non I2C device ?) ERR#%d -> %s\n", errno, strerror(errno));
            if (r > 0) { r= -1; }
         }
         if (gCtx.flags & LX_I2C_FLAG_TRACE)
         {
            LX_TRC0("(%s) -> %d\n", path, pBC->fd);
            LX_TRC1("ioctl(.. I2C_FUNCS ..) -> %d\n", r);
            LX_TRC1("flags=0x%0X (%dbytes)\n", pBC->flags, sizeof(pBC->flags));
         }
         if (clk < 1) { pBC->clk= 100000; } // standard/default I2C clock rate
         else if (clk < 10000) { pBC->clk= clk*1000; } // assume kHz
         else { pBC->clk= clk; }  // assume Hz
      }
      else { report(ERR0,"lxOpenI2C(.. %s)\n", path); }
   }
   return(r >= 0);
} // lxi2cOpen

#if 0
   m[0].addr=   dev;
   m[0].flags=  0;
   m[0].len=    1;
   m[0].buf=    &reg;

   m[1].addr=   dev;
   m[1].flags=  f | I2C_M_NOSTART; // |I2C_M_RECV_LEN
   m[1].len=    nB;
   m[1].buf=    pB;
#endif

int lxi2cReadRB (const LXI2CBusCtx *pBC, const U8 dev, U8 regBytes[], const U8 nRB)
{
   struct i2c_msg m[]= {
      { .addr= dev,  .flags= I2C_M_WR,  .len= 1,  .buf= regBytes },
      { .addr= dev,  .flags= I2C_M_RD,  .len= nRB-1,  .buf= regBytes+1 } };
   struct i2c_rdwr_ioctl_data d={ m, 2 };
   return ioctl(pBC->fd, I2C_RDWR, &d);
} // lxi2cReadRB

int lxi2cWriteRB (const LXI2CBusCtx *pBC, const U8 dev, const U8 regBytes[], const U8 nRB)
{
   struct i2c_msg m= { .addr= dev,  .flags= I2C_M_WR,  .len= nRB,  .buf= (void*)regBytes };
   struct i2c_rdwr_ioctl_data d={ &m, 1 };
   return ioctl(pBC->fd, I2C_RDWR, &d);
} // lxi2cWriteRB

#define MRBYTES (2 * sizeof(struct i2c_msg))
int lxi2cReadMultiRB (const LXI2CBusCtx *pBC, const MemBuff *pWS, const U8 dev, U8 regBytes[], const U8 nRB, const U8 nM)
{
   int offset= 0, r= 0;
   if (validMemBuff(pWS, nM * MRBYTES))
   {
      struct i2c_rdwr_ioctl_data d={ pWS->p, 2 * nM }; // NB: alignment assumed
      for (int i= 0; i<nM; i++)
      {
         int j= 2*i;
         d.msgs[j].addr=   dev;
         d.msgs[j].flags=  I2C_M_WR;
         d.msgs[j].len=    1;
         d.msgs[j].buf=    regBytes+offset;

         d.msgs[j+1].addr= dev;
         d.msgs[j+1].flags= I2C_M_RD;
         d.msgs[j+1].len=  nRB-1;
         d.msgs[j+1].buf=  regBytes+offset+1;

         offset+= nRB;
      }
      return ioctl(pBC->fd, I2C_RDWR, &d);
   }
   else
   {
      for (int i= 0; i<nM; i++)
      {
         r+= lxi2cReadRB(pBC, dev, regBytes+offset, nRB);
         offset+= nRB;
      }
   }
   return(r);
} // lxi2cReadMultiRB

#define MWBYTES sizeof(struct i2c_msg)
int lxi2cWriteMultiRB (const LXI2CBusCtx *pBC, const MemBuff *pWS, const U8 dev, const U8 regBytes[], const U8 nRB, const U8 nM)
{
   int offset= 0, r=0;
   if (validMemBuff(pWS, nM * MWBYTES))
   {
      struct i2c_rdwr_ioctl_data d={ pWS->p, nM }; // NB: alignment assumed
      for (int i= 0; i<nM; i++)
      {
         d.msgs[i].addr=   dev;
         d.msgs[i].flags=  I2C_M_WR;
         d.msgs[i].len=    nRB;
         d.msgs[i].buf=    (void*)(regBytes+offset);

         offset+= nRB;
      }
      return ioctl(pBC->fd, I2C_RDWR, &d);
   }
   else
   {
      for (int i= 0; i<nM; i++)
      {
         r+= lxi2cWriteRB(pBC, dev, regBytes+offset, nRB);
         offset+= nRB;
      }
   }
   return(r);
} // lxi2cWriteMultiRB

// DEPRECATE: inital "transaction" design flawed but retained for compatibility...
#define TRANS_WRITE_MAX 15
int lxi2cTrans (const LXI2CBusCtx *pBC, const U16 dev, const U16 f, U16 nB, U8 *pB, U8 reg)
{
   struct i2c_msg m[LX_I2C_TRANS_NM]=
   {  // Split buffer setup necessary for read (but single buffer required for write)
      { .addr= dev,  .flags= I2C_M_WR,  .len= 1,  .buf= &reg }, // command set device "address register"
      { .addr= dev,  .flags= f,  .len= nB,  .buf= pB } // transfer buffer to/from the address set on device
   };
   struct i2c_rdwr_ioctl_data d={ .msgs= m, .nmsgs= 2 };
   U8 wb[1+TRANS_WRITE_MAX];  // Caution! Using local stack buffer assumes synchronous execution of ioctl()
   if (gCtx.flags & LX_I2C_FLAG_TRACE)
   {
      TRACE_CALL("(Dev=0x%X, Flg=0x%04X, Buf={%u, %p}, Reg=x%02X)\n", dev, f, nB, pB, reg);
      reportBytes(TRC1, pB, nB);
   }
   if (I2C_M_RD != (f & I2C_M_RD))
   {
      if (nB > TRANS_WRITE_MAX) return(-1);
      //else // need to build register+data into single buffer
      wb[0]= reg;
      memcpy(wb+1, pB, nB);
      m[0].buf= wb;
      m[0].len= 1+nB;
      d.nmsgs= 1;
   }
   int r= ioctl(pBC->fd, I2C_RDWR, &d);
   if ((I2C_M_RD & f) && (gCtx.flags & LX_I2C_FLAG_TRACE))
   {
      const U8 gTEC[2]={TRC1,ERR0};
      REPORT(gTEC[(r<0)]," -> %d\n", r);
      reportBytes(TRC1, pB, nB);
   }
   if (r > 0) { r= 0 - (LX_I2C_TRANS_NM != r); }
   return(r);
} // lxi2cTrans

void lxi2cSleepm (U32 ms)
{
   int r;
#if 0
   struct timespec rq, rem={0};
   rq.tv_sec= ms / 1000;
   rq.tv_nsec= (ms % 1000) * 1000000;
   r= nanosleep(&rq,&rem);
   //if ((0 != r) || (0 != rem.tv_sec)) "WARNING"
#else
   LX_TRC0("(%u)\n",ms);
   if (ms < 5000)
   {
      r= usleep(ms*1000); // deprecated ?
   }
   else { r= sleep(ms / 1000); }
#endif
   if (0 != r) { WARN_CALL("(%u) -> %d\n", ms, r); }
} // lxi2cSleepm

Bool32 lxi2cOpenSMBUS (LXI2CBusCtx *pBC, const char *path, I8 devID)
{
   int r=-1;
   if (lxi2cOpen(pBC,path,0))
   {
      r= ioctl(pBC->fd, I2C_SLAVE, devID); // SMBUS address locked
      TRACE_CALL("(0x%X) I2C:0x%X - ioctl(..I2C_SLAVE..)->%d\n", devID, pBC->flags & I2C_FUNC_SMBUS_EMUL, r);
      //if (r >= 0) { gCtx.smDevID= devID; }
   }
   return(r >= 0);
} // lxi2cOpenSMBUS

static U32 getSizeSMBUS (U16 bytes)
{
   if (bytes <= I2C_SMBUS_BLOCK_MAX)
   {
      switch(bytes)
      {
         case 1 : return(I2C_SMBUS_BYTE_DATA);
         case 2 : return(I2C_SMBUS_WORD_DATA);
         default : return(I2C_SMBUS_BLOCK_DATA);
      }
   }
   //else
   return(I2C_SMBUS_I2C_BLOCK_DATA);
} // getSizeSMBUS

int lxi2cTransSMBUS (const LXI2CBusCtx *pBC, const U16 f, U16 nB, U8 *pB, U8 reg)
{
   int r= -1;
   struct i2c_smbus_ioctl_data s={ .read_write=f, .command= reg, .size= getSizeSMBUS(nB), .data=(void*)pB };
//   union i2c_smbus_data d={???};
   //
   if (gCtx.flags & LX_I2C_FLAG_TRACE)
   {
      TRACE_CALL("(Flg=0x%X, Buf={%u, %p}, Reg=x%02X)\n", f, nB, pB, reg);
      reportBytes(TRC1, pB, nB);
   }
   r= ioctl(pBC->fd, I2C_SMBUS, &s);
   if ((I2C_SMBUS_READ & f) && (gCtx.flags & LX_I2C_FLAG_TRACE))
   {
      const U8 gTEC[2]={TRC1,ERR0};
      REPORT(gTEC[(r<0)]," -> %d\n", r);
      reportBytes(TRC1, pB, nB);
   }
   return(r);
} // lxi2cTransSMBUS

void lxi2cClose (LXI2CBusCtx *pC)
{
   if (pC->fd >= 0)
   {
      close(pC->fd);
      pC->fd= -1;
   }
} // lxi2cClose

void lxi2cDumpDevAddr (const LXI2CBusCtx *pC, U16 dev, U8 bytes, U8 addr)
{
   int r;
   U8 buf[1<<8];

   memset(buf, 0xA5, 1<<8);
   // read into first 16byte row of buf
   r= lxi2cTrans(pC, dev, I2C_M_RD, bytes, buf+(addr&0xF), addr);
   if (r >= 0)
   {
      const int rl= addr & 0xF0;
      const int rh= bytes & 0xF0;
      char row[17]; row[16]= 0;

      printf("lxi2cTrans() -> %d\n",r);
      printf("\n   "); // headers
      for (int j= 0; j<16; j++) { printf("  %x", j); }
      printf("\t");
      for (int j= 0; j<16; j++) { printf("%x", j); }

      for (int i= 0; i<=rh; i+=0x10)
      { // rows
         printf("\n%02x: ", i+rl);
         for (int j= 0; j<16; j++)
         { // columns
            U8 b= buf[i|j];
            printf("%02x ", b);
            // map as necessary, forming printable string
            if (b >= 0x7F) { b= '?'; } else if (b <= ' ') { b= '.'; }
            row[j]= b;
         }
         printf("\t%s", row);
      }
      printf("\n");
   }
   //else { printf(WARN/ERROR); }
} // lxi2cDumpDevAddr

#ifdef LX_I2C_MAIN

// Was a standalone test, now being used as a device test harness...

#ifdef RPI_VC4 // Broadcom VideoCore IV timestamp register(s) mapped into (root-only) process address space

#include "rpiUtil.h"

void setup (void)
{
   uint64_t ts0, ts1;
   if (vcAcquire(-1))
   {
      ts0= vcTimestamp();
      ts1= vcTimestamp();
      printf("dt=%d\n",(int)(ts1-ts0));
   }
} // setup

#else // RPI_VC4
// Evaluate other timing mechanisms that should provide better-than-millisecond accuracy
//#include <sys/time.h>
#include <time.h>
#include <signal.h>
#define USEC(t1,t2) (((t2).tv_sec-(t1).tv_sec)*1000000+((t2).tv_usec-(t1).tv_usec))
#define NSEC(t1,t2) (((t2).tv_sec-(t1).tv_sec)*1000000000+((t2).tv_nsec-(t1).tv_nsec))

void setup (void)
{
   int r, u, n, sdd, i;

   //gettimeofday(&(t0.it_value),NULL);
   //thrd_sleep();
   //nanosleep();
   {
      struct timespec ts0, ts1;
      struct timespec sr0={0}, sr1={0};
      r= clock_getres(CLOCK_REALTIME, &ts0);
      if (0 == r)
      {
         printf("r=%d, res: %ld %ld\n", r, ts0.tv_sec, ts0.tv_nsec);
         r= clock_gettime(CLOCK_REALTIME, &ts0);
         u= 2000;
         i= 0;
         sdd= 0;
         printf("slept\tresid\tmeasure\tdiff (nsec)\n");
         do
         {
            n= u * 1000;
            //usleep(u);
            sr0.tv_nsec= n;
            nanosleep(&sr0, &sr1);
            r= clock_gettime(CLOCK_REALTIME, &ts1);
            if (0==r)
            {
               int dtn= NSEC(ts0, ts1);
               int ddn= dtn - n;
               sdd+= ddn; ++i;
               printf("%d\t%ld:%ld\t%d\t%d\n", n, sr1.tv_sec,sr1.tv_nsec, dtn, ddn);
            }
            ts0= ts1; // delta, not cumulative
            u-= 100;
         } while ((0 == r) && (u > 0)); //<= 2000));
         if (i > 0) { printf("%d : mean diff= %d\n", i, sdd / i); }
      }
   }
   /***/
   {  // Interval timer
      struct itimerval t0={0}, t1={0};

      t0.it_value.tv_sec= 10;
      t0.it_interval=   t0.it_value;

      r= setitimer(ITIMER_REAL, &t0, NULL);
      if (0==r)
      {
         printf("%ld : %ld (%ld : %ld)\n", t0.it_value.tv_sec, t0.it_value.tv_usec, t0.it_interval.tv_sec, t0.it_interval.tv_usec);
         u= 2000;
         i= 0;
         sdd= 0;
         printf("slept\tmeasure\tdiff (usec)\n");
         do
         {
            usleep(u);
            r= getitimer(ITIMER_REAL, &t1);
            if (0==r)
            {
               int dtu= USEC(t1.it_value, t0.it_value);
               int ddu= dtu - u;
               sdd+= ddu; ++i;
               printf("%d\t%d\t%d\n", u, dtu, ddu);
            }
            t0= t1; // delta, not cumulative
            u-= 100;
         } while ((0 ==r) && (u > 0)); //<= 2000));
         if (i > 0) { printf("%d : mean diff= %d\n", i, sdd / i); }
      }
   }
#if 0 // not functional
   {  // Process clock
      // CLOCK_REALTIME CLOCK_MONOTONIC
      timer_t hT=NULL;
      struct sigevent ev={SIGEV_NONE, };
      struct itimerspec ts0={0}, ts1={0};

      r= timer_create(CLOCK_REALTIME, &ev, &hT);
      printf("r=%d %p\n", r, hT);
      if (0 == r)
      {
         timer_gettime(hT, &ts0);
         printf("%ld : %ld (%ld : %ld)\n", ts0.it_value.tv_sec, ts0.it_value.tv_nsec, ts0.it_interval.tv_sec, ts0.it_interval.tv_nsec);
         //timer_settime(t, int flags, const struct itimerspec *new_value, struct itimerspec * old_value);
         timer_gettime(hT, &ts1);
         printf("%ld : %ld (%ld : %ld)\n", ts1.it_value.tv_sec, ts1.it_value.tv_nsec, ts1.it_interval.tv_sec, ts1.it_interval.tv_nsec);
      }
   }
#endif
} // setup

#endif // RPI_VC4

LXI2CBusCtx gBusCtx={0,-1};

#include "ads1xUtil.h"

/* IMU test hacks */

#include "lsm9ds1.h"

typedef struct
{
   LSMAccValI16RegFrames   avI16;
   LSMAccCtrlRegFrames     actrl;
   LSMMagValI16RegFrames   mvI16;
   LSMMagCtrlRegFrames     mctrl;
} IMURegFrames;

void initFrames (IMURegFrames *pR)
{
   memset(pR, 0, sizeof(*pR));
   // accel.
   pR->avI16.temp[0]= LSM_REG_TEMP;
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

/*typedef struct
{
} LSMAccUnpack;*/

typedef struct
{
   F32 magSR, magFS;
} LSMMagCfgTrans;

void lsmTranslateMagCfg (LSMMagCfgTrans *pMCT, const U8 cfg[5])
{
   //static const F32 rate[]={ 0.625, 1.25, 2.5, 5, 10, 20, 40, 80 }; // 5 * 2^(n-3)
   //static const U8 scale[]={ 4, 8, 12, 16 }; // 4 * (n+1)
   const U8 rn= (cfg[0] >> 2) & 0x7;
   const U8 sn= (cfg[1] >> 5) & 0x3;
   pMCT->magSR=   5 * pow(2,rn-3);  // rate[rn];
   pMCT->magFS=  4 * (sn+1);        // scale[sn];
} // lsmTranslateMagCfg

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
} // lsmMagSetMode

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

void dumpV (const char hdr[], const U8 raw[6], const F32 s, const char ftr[])
{
   F32 v3f[3];
   vscaleNI16LEtoF32(v3f, raw, 6, s);
   LOG("%s(%.3f, %.3f, %.3f)%s", hdr, v3f[0], v3f[1], v3f[2], ftr);
} // dumpMagV

void dumpOffsetV (const char hdr[], const U8 raw[6], I32 o[3], const F32 s, const char ftr[])
{
   I32 v3i[3]={o[0], o[1], o[2]};
   vaddNI16LEtoI32(v3i, raw, 6);
   LOG("%s(%.3f, %.3f, %.3f)%s", hdr, v3i[0]*s, v3i[1]*s, v3i[2]*s, ftr);
} // dumpMagV

void dumpMagTiltV (const char hdr[], const U8 raw[6], const F32 s, const char ftr[])
{
   F32 v3f[3];
   vscaleNI16LEtoF32(v3f, raw, 6, s);
   LOG("%s(%.3f, %.3f, %.3f)", hdr, v3f[0], v3f[1], v3f[2]);
   LOG("\t|XYZ|= %.3f\ttilt:XZ,YZ= %.3f, %.3f (deg.)%s", magV3F32(v3f), tiltDeg2F32(v3f[0],v3f[2]), tiltDeg2F32(v3f[1],v3f[2]), ftr);
} // dumpMagTiltV

void dumpMagBearV (const char hdr[], const U8 raw[6], const F32 s, const char ftr[])
{
   F32 v3f[3];
   vscaleNI16LEtoF32(v3f, raw, 6, s);
   LOG("%s(%.3f, %.3f, %.3f)", hdr, v3f[0], v3f[1], v3f[2]);
   LOG("\t|XYZ|= %.3f\tbearXY= %.1f (deg.)%s", magV3F32(v3f), bearingDegV2F32(v3f), ftr);
} // dumpMagBearV

void magTest (const LXI2CBusCtx *pC, const U8 dev, IMURegFrames *pF, const U8 maxIter)
{
   LSMMagCfgTrans mct={0,};
   int r;

   r= lxi2cReadMultiRB(pC, NULL, dev, pF->mvI16.offs, sizeof(pF->mvI16.offs), 2);
   if (r >= 0)
   {
      dumpV("RAW: offset", pF->mvI16.offs+1, 1.0, "\n");
      dumpMagBearV("magnetic= ", pF->mvI16.mag+1, 1.0, "\n");
   }

   lsmMagSetRateMode(pF->mctrl.r1_5+1, 7, CONTINUOUS);
   r= lxi2cWriteRB(pC, dev, pF->mctrl.r1_5, sizeof(pF->mctrl.r1_5));

   lsmTranslateMagCfg(&mct, pF->mctrl.r1_5+1);
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
         if (r >= 0) { dumpMagBearV("mag= ", pF->mvI16.mag+1, mS, "\n"); }
         //r= lxi2cReadRB(pC, dev, pF->mctrl.stat, sizeof(pF->mctrl.stat));
         //if (r > 0) { LOG("S%02x\n", pF->mctrl.stat[1]); }
      }
      lsmMagSetRateMode(pF->mctrl.r1_5+1, 7, OFF);
      r= lxi2cWriteRB(pC, dev, pF->mctrl.r1_5, sizeof(pF->mctrl.r1_5));
   }
} // magTest

typedef struct
{
   F32 angRate, angFSD;
   F32 linRate, linFSG;
} LSMAccCfgTrans;

void lsmAccSetMode (U8 ang[4], U8 lin[4], const U8 m)
{
   ang[0]= (ang[0] & 0x1F) | (m<<5); // ODR: 0, 14.9, 59.5...
   lin[1]= (lin[1] & 0x1F) | (m<<5); // ODR: 0, 10, 50..
} // lsmAccSetMode

void lsmTranslateAccCfg (LSMAccCfgTrans *pACT, const U8 ang[4], const U8 lin[4], const U8 angUnit)
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
   if (0 == angUnit) { pACT->angFSD*= M_PI / 180; }
   pACT->angRate= angr[nar];
   pACT->linFSG= lfsg[nlg];
   pACT->linRate= linr[nlr];
} // lsmTranslateAccCfg

void accTest (const LXI2CBusCtx *pC, const U8 dev, IMURegFrames *pF, const U8 maxIter, const U8 maxCal)
{
   LSMAccCfgTrans act={0,};
   int r, ivl=0;
   r= lxi2cReadRB(pC, dev, pF->avI16.temp, sizeof(pF->avI16.temp));
   if (r >= 0)
   {
      I16 rawT= rdI16LE(pF->avI16.temp+1);
      LOG("T= %d -> %GC\n", rawT, 25.0 + rawT * 1.0 / 16);
   }

   lsmAccSetMode(pF->actrl.ang+1, pF->actrl.lin+1, 1); // ON: 14.9 / 10 Hz
   r= lxi2cWriteMultiRB(pC, NULL, dev, pF->actrl.ang, sizeof(pF->actrl.ang), 2);

   lsmTranslateAccCfg(&act, pF->actrl.ang+1, pF->actrl.lin+1, 1);
   LOG("rate: A=%G, L=%G\n", act.angRate, act.linRate);
   LOG("FS  : A=%G deg/s, L=%G g\n", act.angFSD, act.linFSG);
   ivl= 1000000 / MIN(act.angRate, act.linRate); // sync to slowest

   if (ivl > 0)
   {
      I32 nSM= 0, aSM[3]={0,}, lSM[3]={0,};

      LOG("Acc ON, interval %dus\n", ivl);

      for (U8 i=0; i<maxCal; i++)
      {
         usleep(ivl);

         r= lxi2cReadMultiRB(pC, NULL, dev, pF->avI16.ang, sizeof(pF->avI16.ang), 2);
         if (r >= 0)
         {
            vaddNI16LEtoI32(aSM, pF->avI16.ang+1, 6);
            vaddNI16LEtoI32(lSM, pF->avI16.lin+1, 6);
            nSM++;
         }
      }
      if (nSM > 0)
      {  // Sums to means
         for (int i= 0; i<3; i++)
         {
            aSM[i]= -aSM[i] / nSM;
            lSM[i]/= nSM;
         }
         LOG("Calibration:\nRAW means:\tang=(%d, %d, %d)\tlin=(%d, %d, %d)\n\n", aSM[0], aSM[1], aSM[2], lSM[0], lSM[1], lSM[2]);
         //dumpV("RAW: angVel= ", pF->avI16.ang+1, 1.0, "\t");
         //dumpMagTiltV("linAcc= ", pF->avI16.lin+1, 1.0, "\n");
      }
      if (r >= 0)
      {
         const F32 sA= act.angFSD / 0x7FFF;
         const F32 sL= act.linFSG / 0x7FFF;

         for (U8 i=0; i<maxIter; i++)
         {
            usleep(ivl);
            r= lxi2cReadMultiRB(pC, NULL, dev, pF->avI16.ang, sizeof(pF->avI16.ang), 2);
            if (r >= 0)
            {
               dumpOffsetV("angVel= ",pF->avI16.ang+1, aSM, sA, "\t");
               dumpMagTiltV("linAcc= ", pF->avI16.lin+1, sL, "\n");
            }
         }
      }
   }
   lsmAccSetMode(pF->actrl.ang+1, pF->actrl.lin+1, 0); // off
   r= lxi2cWriteMultiRB(pC, NULL, dev, pF->actrl.ang, sizeof(pF->actrl.ang), 2);
} // accTest

int testIMU (const LXI2CBusCtx *pC, const U8 dev[2], const U8 maxIter)
{
   int r=-1;

   //r= I2C_BYTES_NCLK(sizeof(frm.a16.ang)) * 3;
   //printf("testIMU() - peak rate= %G full data frames per sec\n", gBusCtx.clk * 1.0 / n);
   if (lsmIdentify(pC,dev) > 0)
   {
      IMURegFrames frm;
      initFrames(&frm);

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
      accTest(pC, dev[0], &frm, maxIter, 20);
      LOG("\n%s\n", "---");

      // Magnetometer: control & measure
      r= lxi2cReadRB(pC, dev[1], frm.mctrl.r1_5, sizeof(frm.mctrl.r1_5));
      if (r >= 0)
      {
         LOG("%s.%s= ", "mctrl", "r1_5");
         reportBytes(LOG0, frm.mctrl.r1_5+1, sizeof(frm.mctrl.r1_5)-1);
      }
      magTest(pC, dev[1], &frm, maxIter);
      LOG("\n%s\n", "---");
   }
   return(r);
} // testIMU

/*---*/

int main (int argc, char *argv[])
{
   if (lxi2cOpen(&gBusCtx, "/dev/i2c-1", 400))
   {
      //MemBuff ws={0,};
      //allocMemBuff(&ws, 4<<10);//
      testADS1x15(&gBusCtx, NULL, 0x48, 1, ADS1X_TEST_MODE_VERIFY|ADS1X_TEST_MODE_SLEEP|ADS1X_TEST_MODE_POLL|ADS1X_TEST_MODE_ROTMUX, 4);
      //releaseMemBuff(&ws);
      //lxi2cDumpDevAddr(&gBusCtx, 0x00, 0xFF,0x00);
      const U8 ag_m[]={0x6b,0x1e};
      testIMU(&gBusCtx, ag_m, 30);
      lxi2cClose(&gBusCtx);
   }

   return(0);
} // main

#endif // LX_I2C_MAIN

