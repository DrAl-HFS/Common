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

Bool32 lxi2cOpen (LXI2CBusCtx *pBC, const char *path)
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
         pBC->clk= 100000; // default I2C clock rate - TODO: determine actual rate ???
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
int lxi2cTrans (const LXI2CBusCtx *pBC, const U16 dev, const U16 f, U16 nB, U8 *pB, U8 reg)
{
   struct i2c_msg m[LX_I2C_TRANS_NM]=
   { // define two "packets" necessary to complete a transaction
      { .addr= dev,  .flags= 0,  .len= 1,  .buf= &reg }, // command set device "address register"
      { .addr= dev,  .flags= f|I2C_M_STOP,  .len= nB,  .buf= pB } // transfer buffer to/from the address set on device
   };
   struct i2c_rdwr_ioctl_data d={m,LX_I2C_TRANS_NM};
   if (I2C_M_RD != (f & I2C_M_RD)) { m[1].flags|= I2C_M_NOSTART; }
   if (gCtx.flags & LX_I2C_FLAG_TRACE)
   {
      TRACE_CALL("(Dev=0x%X, Flg=0x%04X, Buf={%u, %p}, Reg=x%02X)\n", dev, f, nB, pB, reg);
      reportBytes(TRC1, pB, nB);
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
   if (lxi2cOpen(pBC,path))
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


LXI2CBusCtx gBusCtx={0,-1};

// Endian handling - displace to where ?
typedef union { U16 u16; struct { U8 u8[2]; }; } UU16; // for endian check/twiddle

// Read n bytes big-endian
int rdnbe (const U8 b[], const int n)
{
   int r= b[0];
   for (int i= 1; i<n; i++) { r= (r<<8) | b[i]; }
   return(r);
} // rdnbe

void dumpCfg (const UU16 c)
{
   printf("cfg: 0x%02x:%02x :\n", c.u8[0], c.u8[1]); // NB: Big Endian on-the-wire
   printf(" OS%d MUX%d PGA%d M%d", (c.u8[0]>>7) & 0x1, (c.u8[0]>>4) & 0x7, (c.u8[0]>>1) & 0x7, c.u8[0] & 0x1);
   printf(" DR%d CM%d CP%d CL%d CQ%d\n", (c.u8[1]>>5) & 0x7, (c.u8[1]>>4) & 0x1, (c.u8[1]>>3) & 0x1, (c.u8[1]>>2) & 0x1, c.u8[1] & 0x3);
} // dumpCfg

int testADS1015 (const LXI2CBusCtx *pC, const U16 dev)
{
   UU16 cfg={0}, cfg2={0}, dat={0xAAAA};
   int r= lxi2cTrans(pC, dev, I2C_M_RD, 2, cfg.u8, 0x1);
   if (0 == r)
   {  // default single shot, 1600sps => 625us
      dumpCfg(cfg);
      gCtx.flags &= ~LX_I2C_FLAG_TRACE; // enough verbosity
      cfg.u8[0]&= 0x0F; // IN-0/1 // cfg.u8[0]|= 4<<4; // IN-0/G single-shot // 3<<4; // IN-2/3, continuous
      // set data rate 1->250sps (4ms)
      //cfg.u8[1]&= 0x1F; cfg.u8[1]|= 3<<5; // 920sps (1.08ms)
      //dumpCfg(cfg);
      // Change settings wthout starting
      r= lxi2cTrans(pC, dev, I2C_M_WR, 2, cfg.u8, 0x1);
      usleep(1000);
      cfg.u8[0]|= ADS1X_FL0_OS; // Now enable conversion
      if (0 == r)
      {
         int i, n= 0;
         do
         {
            r= lxi2cTrans(pC, dev, I2C_M_WR, 2, cfg.u8, 0x1); // start conversion

            i= 0;
            do { // poll status
               usleep(500);
               r= lxi2cTrans(pC, dev, I2C_M_RD, 2, cfg2.u8, 0x1);
               //printf("%d: ", i); dumpCfg(cfg2);
            } while ((0 == r) && (++i < 5) && (0 == (cfg2.u8[0] & ADS1X_FL0_OS)));

            r= lxi2cTrans(pC, dev, I2C_M_RD, 2, dat.u8, 0x0); // read result
            printf("data: 0x%x\n", rdnbe(dat.u8, 2));
         } while (++n < 10);
      }
   }
   return(r);
} // testADS1015

int main (int argc, char *argv[])
{
   char *busDevPath="/dev/i2c-1";
   U8 devID= 0x48;

   if (lxi2cOpen(&gBusCtx, busDevPath))
   {
      //lxi2cDumpDevAddr(&gBusCtx, devID, 0xFF,0x00);
      testADS1015(&gBusCtx,devID);
      lxi2cClose(&gBusCtx);
   }

   return(0);
} // main

#endif // LX_I2C_MAIN

