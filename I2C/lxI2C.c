// Common/I2C/lxI2C.c - I2C bus utils for Linux
// https://github.com/DrAl-HFS/Common.git
// (c) Project Contributors Feb 2018 - July 2019

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

/*  github.com/torvalds/linux/blob/master/drivers/i2c/i2c-dev.c
 * After opening an instance of this character special file, a file
 * descriptor starts out associated only with an i2c_adapter (and bus).
 *
 * Using the I2C_RDWR ioctl(), you can then *immediately* issue i2c_msg
 * traffic to any devices on the bus used by that adapter.  That's because
 * the i2c_msg vectors embed all the addressing information they need, and
 * are submitted directly to an i2c_adapter.  However, SMBus-only adapters
 * don't support that interface.
 *
 * To use read()/write() system calls on that file descriptor, or to use
 * SMBus interfaces (and work with SMBus-only hosts!), you must first issue
 * an I2C_SLAVE (or I2C_SLAVE_FORCE) ioctl.  That configures an anonymous
 * (never registered) i2c_client so it holds the addressing information
 * needed by those system calls and by this SMBus interface.

struct i2c_msg {
	__u16 addr;	// slave address
	__u16 flags;// see below
	__u16 len;	// msg length
	__u8 *buf;	// pointer to msg data
};

I2C_M_RD		0x0001	// read data, from slave to master  I2C_M_RD is guaranteed to be 0x0001!
I2C_M_TEN		0x0010	// this is a ten bit chip address
I2C_M_RECV_LEN		0x0400	// length will be first received byte
I2C_M_NO_RD_ACK		0x0800	// if I2C_FUNC_PROTOCOL_MANGLING
I2C_M_IGNORE_NAK	0x1000	// if I2C_FUNC_PROTOCOL_MANGLING
I2C_M_REV_DIR_ADDR	0x2000	// if I2C_FUNC_PROTOCOL_MANGLING
I2C_M_NOSTART		0x4000	// if I2C_FUNC_NOSTART
I2C_M_STOP		0x8000	// if I2C_FUNC_PROTOCOL_MANGLING

I2C_FUNCS:

I2C_FUNC_SMBUS_READ_BYTE|I2C_FUNC_SMBUS_WRITE_BYTE
I2C_FUNC_SMBUS_READ_BYTE_DATA|I2C_FUNC_SMBUS_WRITE_BYTE_DATA
I2C_FUNC_SMBUS_READ_WORD_DATA|I2C_FUNC_SMBUS_WRITE_WORD_DATA
I2C_FUNC_SMBUS_READ_BLOCK_DATA|I2C_FUNC_SMBUS_WRITE_BLOCK_DATA
I2C_FUNC_SMBUS_READ_I2C_BLOCK|I2C_FUNC_SMBUS_WRITE_I2C_BLOCK

I2C_FUNC_10BIT_ADDR, I2C_FUNC_SLAVE, I2C_FUNC_SMBUS_QUICK
I2C_FUNC_PROTOCOL_MANGLING I2C_FUNC_SMBUS_PEC
I2C_FUNC_SMBUS_BLOCK_PROC_CALL, I2C_FUNC_SMBUS_PROC_CALL, I2C_FUNC_SMBUS_HOST_NOTIFY
*/
#include "lxI2C.h"
#include "report.h"

#define LX_I2C_TRANS_NM (2) // number of i2c_msg blocks per transaction
#define LX_I2C_FLAG_TRACE (1<<4)

// Note: adjacent string concatenation, compiler defined symbol, variadic args
#define LX_TRC0(fmt,...) TRACE_CALL(fmt,__VA_ARGS__)
#define LX_TRC1(fmt,...) report(TRC1,fmt,__VA_ARGS__)

typedef struct
{
   U8 flags;
   U8 pad[3];
} LXI2CCtx;

static LXI2CCtx gCtx={LX_I2C_FLAG_TRACE, }; // {{0,-1},{0,-1}},0,0,};


/***/

Bool32 lxOpenI2C (LXI2CBusCtx *pBC, const char *path)
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
      }
      else { report(ERR0,"lxOpenI2C(.. %s)\n", path); }
   }
   return(r >= 0);
} // lxOpenI2C

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
int lxTransactI2C (const LXI2CBusCtx *pBC, const U16 dev, const U16 f, U16 nB, U8 *pB, U8 reg)
{
   struct i2c_msg m[LX_I2C_TRANS_NM]=
   { // define two "packets" necessary to complete a transaction
      { .addr= dev,  .flags= 0,  .len= 1,  .buf= &reg }, // command set device "address register"
      { .addr= dev,  .flags= f | I2C_M_NOSTART,  .len= nB,  .buf= pB } // transfer buffer to/from the address set on device
   };
   struct i2c_rdwr_ioctl_data d={m,LX_I2C_TRANS_NM};
   if (gCtx.flags & LX_I2C_FLAG_TRACE)
   {
      LX_TRC0("(pBC->%d, Dev=0x%X, Flg=0x%04X, Buf={%u, %p}, Reg=x%02X)\n", pBC->fd, dev, f, nB, pB, reg);
      reportBytes(TRC1, pB, nB);
   }
   int r= ioctl(pBC->fd, I2C_RDWR, &d);
   if (gCtx.flags & LX_I2C_FLAG_TRACE)
   {
      const U8 gTEC[2]={TRC1,ERR0};
      REPORT(gTEC[(r<0)]," -> %d\n", r);
      reportBytes(TRC1, pB, nB);
   }
   if (r > 0) { r= 0 - (LX_I2C_TRANS_NM != r); }
   return(r);
} // lxTransactI2C

void lxSleepm (U32 ms)
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
} // lxSleepm

/*
int lxOpenSMBUS (const char *path, I8 devID)
{
   int r=-1;
   if (lxOpenI2C(path))
   {
      r= ioctl(activeBC()->fd, I2C_SLAVE, &devID); // SMBUS address locked
      //if (r >= 0) { gCtx.smDevID= devID; }
   }
   return(r >= 0);
} // lxOpenSMBUS
* */

void lxClose (LXI2CBusCtx *pC)
{
   if (pC->fd >= 0)
   {
      close(pC->fd);
      pC->fd= -1;
   }
} // lxClose

void lxDumpI2C (const LXI2CBusCtx *pC, U16 dev, U8 bytes, U8 addr)
{
   int r;
   U8 buf[1<<8];

   memset(buf, 0, 1<<8);
   // read into first 16byte row of buf
   r= lxTransactI2C(pC, dev, I2C_M_RD, bytes, buf+(addr&0xF), addr);
   if (r >= 0)
   {
      const int rl= addr & 0xF0;
      const int rh= bytes & 0xF0;
      char row[17]; row[16]= 0;

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
} // lxDumpI2C

#ifdef LX_I2C_MAIN

LXI2CBusCtx gBusCtx={0,-1};

int main (int argc, char *argv[])
{
   char *busDevPath="/dev/i2c-1";
   U8 devID= 0x76;

   if (lxOpenI2C(&gBusCtx, busDevPath))
   {
      lxDumpI2C(&gBusCtx, devID, 0xFF,0x00);
      lxClose(&gBusCtx);
   }

   return(0);
} // main

#endif // LX_I2C_MAIN
