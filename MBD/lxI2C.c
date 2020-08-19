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

// Standalone test
#include "ads1x.h"

#ifdef RPI_VC4

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

#else

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

#endif

LXI2CBusCtx gBusCtx={0,-1};

/* Endian handling - displace to where ?
typedef union { U64 u64; struct { U8 u8[8]; }; } UU64;
typedef union { U32 u32; struct { U8 u8[4]; }; } UU32;
typedef union { U16 u16; struct { U8 u8[2]; }; } UU16;
*/
I16 rdI16BE (const U8 b[2]) { return((b[0] << 8) | b[1]); }

// Read n bytes big-endian
int rdnbe (const U8 b[], const int n)
{
   int r= b[0];
   for (int i= 1; i<n; i++) { r= (r<<8) | b[i]; }
   return(r);
} // rdnbe

void wrnbe (U8 b[], int x, const int n)
{
   for (int i= n-1; i>0; i--) { b[i]= x & 0xFF; x>>= 8; }
} // rdnbe

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

int ads1xInitRB (ADS1xRB *pRB, const LXI2CBusCtx *pC, const U8 dev)
{
   pRB->res[0]= ADS1X_REG_RES;
   pRB->cfg[0]= ADS1X_REG_CFG;
   pRB->cLo[0]= ADS1X_REG_CLO;
   pRB->cHi[0]= ADS1X_REG_CHI;
   if (pC && dev)
   {
      lxi2cReadRB(pC, dev, pRB->res, ADS1X_NRB);
      lxi2cReadRB(pC, dev, pRB->cfg, ADS1X_NRB);
      lxi2cReadRB(pC, dev, pRB->cLo, ADS1X_NRB);
      lxi2cReadRB(pC, dev, pRB->cHi, ADS1X_NRB);
   }
   return(0);
} // ads1xInitRB

U8 ads1xGetMux (const U8 cfg[2]) { return((cfg[0] >> ADS1X_SH0_MUX) & ADS1X_M_M); }
U8 ads1xMuxToM4X4 (const U8 m)
{
   static const U8 m4x4[]= {0x01, 0x03, 0x13, 0x23, 0x0F, 0x1F, 0x2F, 0x3F};
   return m4x4[ m & 0x7 ];
} // ads1xGetMux4X4

void ads1xSetMux (U8 cfg[2], enum ADS1xMux mux) { cfg[0]= (cfg[0] & ~(ADS1X_M_M << ADS1X_SH0_MUX)) | (mux << ADS1X_SH0_MUX); }

char muxCh (const U8 c) { if (c<=3) return('0'+c); else return('G'); }
void printMux4x4 (const U8 m4x4) { printf("%c/%c", muxCh(m4x4 >> 4), muxCh(m4x4 & 0xF)); }

void ads1xUnpackCfg (ADS1xUnpack *pU, const U8 cfg[2], const U8 x)
{
static const U16 r[2][8]=
{  {128,250,490,920,1600,2400,3300,3300},
   {8,16,32,64,128,250,475,860} };
static const F32 g[]= { 6.144, 4.096, 2.048, 1.024, 0.512, 0.256 };
   pU->m4x4= ads1xMuxToM4X4(ads1xGetMux(cfg));
   pU->gainFSV= g[ (cfg[0] >> ADS1X_SH0_PGA) & ADS1X_GFS_M ];
   pU->cmp= (((cfg[1] >> ADS1X_SH1_CQ) & ADS1X_CMP_M) + 1 ) & ADS1X_CMP_M;
   pU->rate= r[x&1][ (cfg[1] >> ADS1X_SH1_DR) & ADS10_R_M ];
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
   printf("cfg= [%02x, %02x] = ", cfg[0], cfg[1]); // NB: Big Endian on-the-wire
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


#define MODE_VERIFY  (1<<7)
#define MODE_SLEEP   (1<<6)
#define MODE_POLL    (1<<5)
#define MODE_ROTMUX  (1<<4)
#define MODE_VERBOSE (1<<0)

int testADS1015 (const LXI2CBusCtx *pC, const U8 dev, const U8 mode)
{
   ADS1xRB rb;
   ADS1xUnpack u;
   const int i2cWait= ADS1X_TRANS_NCLK * 1E6 / pC->clk;
   int convWait=0, expectWait=0;
   int r;
   float sv;
   U8 cfgStatus[ADS1X_NRB];

   r= ads1xInitRB(&rb, pC, dev);
   ads1xDumpCfg(rb.cfg+1, 0);
   if (r >= 0)
   {
      memcpy(cfgStatus, rb.cfg, ADS1X_NRB);
      ads10GenCfg(rb.cfg+1, ADS1X_M0G, ADS1X_GFS_6V144, ADS10_R250, ADS1X_CMP_DISABLE);
      rb.cfg[1]|= ADS1X_FL0_OS|ADS1X_FL0_MODE; // Now enable single-shot conversion
      printf("cfg: "); ads1xDumpCfg(rb.cfg+1, 0);
      convWait= convInterval(&u, rb.cfg+1, 0);
      sv= u.gainFSV / ADS10_FSR;
      if (r >= 0)
      {
         int n= 0;
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
         } while (++n < 16);
      }
   }
   return(r);
} // testADS1015

int main (int argc, char *argv[])
{
   if (lxi2cOpen(&gBusCtx, "/dev/i2c-1", 400))
   {
      //lxi2cDumpDevAddr(&gBusCtx, 0x00, 0xFF,0x00);
      testADS1015(&gBusCtx, 0x48, MODE_VERIFY|MODE_SLEEP|MODE_POLL|MODE_ROTMUX);
      lxi2cClose(&gBusCtx);
   }

   return(0);
} // main

#endif // LX_I2C_MAIN

