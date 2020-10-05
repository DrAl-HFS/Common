// Common/MBD/lxI2C.c - I2C bus utils for Linux
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Feb 2018 - Sept 2020

// Linux Ref: https://www.kernel.org/doc/html/latest/driver-api/i2c.html

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "lxI2C.h"
#include "lxTiming.h"


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

int lxi2cReadRB (const LXI2CBusCtx *pBC, const U8 busAddr, U8 regBytes[], const U8 nRB)
{
   struct i2c_msg m[]= {
      { .addr= busAddr,  .flags= I2C_M_WR,  .len= 1,  .buf= regBytes },
      { .addr= busAddr,  .flags= I2C_M_RD,  .len= nRB-1,  .buf= regBytes+1 } };
   struct i2c_rdwr_ioctl_data d={ m, 2 };
   return ioctl(pBC->fd, I2C_RDWR, &d);
} // lxi2cReadRB

int lxi2cWriteRB (const LXI2CBusCtx *pBC, const U8 busAddr, const U8 regBytes[], const U8 nRB)
{
   struct i2c_msg m= { .addr= busAddr,  .flags= I2C_M_WR,  .len= nRB,  .buf= (void*)regBytes };
   struct i2c_rdwr_ioctl_data d={ &m, 1 };
   return ioctl(pBC->fd, I2C_RDWR, &d);
} // lxi2cWriteRB

#define MRBYTES (2 * sizeof(struct i2c_msg))
int lxi2cReadMultiRB (const LXI2CBusCtx *pBC, const MemBuff *pWS, const U8 busAddr, U8 regBytes[], const U8 nRB, const U8 nM)
{
   int offset= 0, r= 0;
   if (validMemBuff(pWS, nM * MRBYTES))
   {
      struct i2c_rdwr_ioctl_data d={ pWS->p, 2 * nM }; // NB: alignment assumed
      for (int i= 0; i<nM; i++)
      {
         int j= 2*i;
         d.msgs[j].addr=   busAddr;
         d.msgs[j].flags=  I2C_M_WR;
         d.msgs[j].len=    1;
         d.msgs[j].buf=    regBytes+offset;

         d.msgs[j+1].addr= busAddr;
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
         r+= lxi2cReadRB(pBC, busAddr, regBytes+offset, nRB);
         offset+= nRB;
      }
   }
   return(r);
} // lxi2cReadMultiRB

#define MWBYTES sizeof(struct i2c_msg)
int lxi2cWriteMultiRB (const LXI2CBusCtx *pBC, const MemBuff *pWS, const U8 busAddr, const U8 regBytes[], const U8 nRB, const U8 nM)
{
   int offset= 0, r=0;
   if (validMemBuff(pWS, nM * MWBYTES))
   {
      struct i2c_rdwr_ioctl_data d={ pWS->p, nM }; // NB: alignment assumed
      for (int i= 0; i<nM; i++)
      {
         d.msgs[i].addr=   busAddr;
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
         r+= lxi2cWriteRB(pBC, busAddr, regBytes+offset, nRB);
         offset+= nRB;
      }
   }
   return(r);
} // lxi2cWriteMultiRB

// DEPRECATE: inital "transaction" design flawed but retained for compatibility...
#define TRANS_WRITE_MAX 15
int lxi2cTrans (const LXI2CBusCtx *pBC, const U16 busAddr, const U16 f, U16 nB, U8 *pB, U8 reg)
{
   struct i2c_msg m[LX_I2C_TRANS_NM]=
   {  // Split buffer setup necessary for read (but single buffer required for write)
      { .addr= busAddr,  .flags= I2C_M_WR,  .len= 1,  .buf= &reg }, // command set device "address register"
      { .addr= busAddr,  .flags= f,  .len= nB,  .buf= pB } // transfer buffer to/from the address set on device
   };
   struct i2c_rdwr_ioctl_data d={ .msgs= m, .nmsgs= 2 };
   U8 wb[1+TRANS_WRITE_MAX];  // Caution! Using local stack buffer assumes synchronous execution of ioctl()
   if (gCtx.flags & LX_I2C_FLAG_TRACE)
   {
      TRACE_CALL("(Dev=0x%X, Flg=0x%04X, Buf={%u, %p}, Reg=x%02X)\n", busAddr, f, nB, pB, reg);
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

/***/

void lxi2cSleepm (U32 milliSec)
{
   int r=0;
   LX_TRC0("(%u)\n",milliSec);
#ifdef LX_TIMING_H
   if (milliSec < 100) { timeSpinSleep(milliSec * 1000000); } else
#endif // fall back to (deprecated) usleep - nanosleep() currently offers no advantage on embedded (ARM) platforms
   { r= usleep(milliSec*1000); }
   if (0 != r) { WARN_CALL("(%u) -> %d\n", milliSec, r); }
} // lxi2cSleepm

int lxi2cDumpDevAddr (const LXI2CBusCtx *pC, U8 busAddr, U8 bytes, U8 regAddr)
{
   int r;
   U8 buf[1<<8];

   memset(buf, 0xA5, 1<<8);
   // read into first 16byte row of buf
   r= lxi2cTrans(pC, busAddr, I2C_M_RD, bytes, buf+(regAddr&0xF), regAddr);
   if (r >= 0)
   {
      const int rl= regAddr & 0xF0;
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
   return(r);
} // lxi2cDumpDevAddr

/*** PING ***/

typedef struct
{
   //const LXI2CPing *pP;
   U32 maxP, nP, nE;
   int rT, rIO;
} PingState;

static PingState gPS;


#include <signal.h>
#include "sciFmt.h"

int pingReport (const PingState *pS, const LXI2CPing *pP)
{
   if (pS) // (pS->maxP > 0) || pS->pSig)
   {
      U32 maxP= pS->maxP;
      if (pP && (0 == maxP)) { maxP= pP->maxIter; }
      LOG("\tI2C-Ping: sent %u of %u frames: %d OK, %u Errors\n", pS->nP, maxP, pS->nP-pS->nE, pS->nE);
      if (pS->nE > 0)
      {
         ERROR("I2C-Ping: result return values: rT=%d, rIO=%d\n", pS->rT, pS->rIO);
         return(-1);
      }
   }
   return(0);
} // pingReport

void sigHandler (int sigV)
{
   switch (sigV)
   {
      case SIGINT :  gPS.maxP= 0; break;
      case SIGTSTP : pingReport(&gPS, NULL); break;
   }
} // sigHandler

void initPS (PingState *pS, const LXI2CPing *pP)
{
   memset(pS, 0, sizeof(*pS));
   pS->maxP= pP->maxIter;
   signal(SIGINT,sigHandler);
   signal(SIGTSTP,sigHandler);
   //pS->pP= pP;
} // initPS

void cleanPS (PingState *pS)
{
   signal(SIGINT,SIG_DFL);
   signal(SIGTSTP,SIG_DFL);
} // cleanPS

int lxi2cPing (const LXI2CBusCtx *pC, U8 busAddr, const LXI2CPing *pP, U8 modeFlags)
{
   struct i2c_msg m= { .addr= busAddr,  .flags= I2C_M_WR,  .len= pP->nB,  .buf= (void*)(pP->b) };
   struct i2c_rdwr_ioctl_data d={ &m, 1 };
   RawTimeStamp ts[3];
   PingState *pS= &gPS;
   int r= -1;

   initPS(pS, pP);
   if (modeFlags & I2C_PING_VERBOSE)
   {
      U32 nClk= I2C_BYTES_NCLK(m.len);
      U32 tns= (nClk * (float)NANO_TICKS) / pC->clk;
      TRACE_CALL("(0x%02X, 1+[%d]) - %dclk ", m.addr, m.len, nClk);
#ifdef SCI_FMT_H
      char mCh[2];
      LOG("@ %G%cHz -> %G%csec\n", sciFmtSetF(mCh+0,pC->clk), mCh[0], sciFmtSetF(mCh+1,1E-9*tns), mCh[1]);
#else
      LOG("@ %dHz -> %dnsec\n", pC->clk, tns);
#endif
   }
   pS->rT= timeSetTarget(ts+1, ts+0, 1000, TIME_MODE_NOW);
   do
   {
      pS->rT= timeSpinWaitUntil(ts+2, ts+1);
      pS->rIO= ioctl(pC->fd, I2C_RDWR, &d);
      pS->nE+= (1 != pS->rIO);
      pS->rT= timeSetTarget(ts+1, ts+1, pP->ivlNanoSec, TIME_MODE_RELATIVE);
   } while ((++(pS->nP) < pS->maxP) && (pS->nE <= pP->maxErr));
   r= pingReport(pS,pP);
   cleanPS(pS);
   return(r);
} // lxi2cPing


#ifdef LX_I2C_MAIN

/*** PING ARGS ***/

#define ARG_ACTION 0xF0  // Mask
#define ARG_PING   (1<<7)
#define ARG_DUMP   (1<<6)
#define ARG_XPT    (1<<5)
//#define ARG_READ   (1<<5)
//#define ARG_WRITE  (1<<4)

#define ARG_OPTION   0x0F  // Mask
#define ARG_HELP    (1<<1)
#define ARG_VERBOSE (1<<0)

typedef struct
{
   LXI2CPing ping;
   char devPath[14]; // host device path
   U8 busAddr; // bus device address
   U8 flags;
} LXI2CArgs;

static LXI2CArgs gArgs=
{
   {  1, {0x00,}, // FLAGS???,
      1000, 10, // n, e
      1000000  // interval (ns)
   },
   "/dev/i2c-1", 0x48, 0
};

void pingUsageMsg (const char name[])
{
static const char optCh[]="abcdetPDXvh";
static const char argCh[]="######     ";
static const char *desc[]=
{
   "I2C bus address: 2digit hex (no prefix)",
   "payload byte count",
   "message count (max pings to send)",
   "device index (-> path /dev/i2c-# )",
   "maximum errors to ignore (-1 -> all)",
   "interval (nanoseconds) between messages",
   "Ping",
   "Dump",
   "eXperimental"
   //"Read","Write",
   "verbose diagnostic messages",
   "help (display this text)"
};
   const int n= sizeof(desc)/sizeof(desc[0]);
   report(OUT,"Usage : %s [-%s]\n", name, optCh);
   for (int i= 0; i<n; i++)
   {
      report(OUT,"\t%c %c - %s\n", optCh[i], argCh[i], desc[i]);
   }
} // pingUsageMsg

void argDump (LXI2CArgs *pP)
{
   report(OUT,"Device: devPath=%s, busAddr=%02X, Flags=%02X\n", pP->devPath, pP->busAddr, pP->flags);
   report(OUT,"\tmaxIter=%d, maxErr=%d\n", pP->ping.maxIter, pP->ping.maxErr);
   report(OUT,"\tb[%d]={", pP->ping.nB); reportBytes(OUT, pP->ping.b, pP->ping.nB);
   report(OUT,"}\n\tinterval=");
#ifdef SCI_FMT_H
   char mCh[1]={'m'};
   report(OUT,"%G%csec\n", sciFmtSetF(mCh, 1E-9 * pP->ping.ivlNanoSec), mCh[0]);
#else
   report(OUT,"%dnsec\n", pP->ping.ivlNanoSec);
#endif
} // argDump

void i2cArgTrans (LXI2CArgs *pA, int argc, char *argv[])
{
   F64 f;
   int t, n[3]={0,0,0};
   signed char ch, nCh;
   do
   {
      ch= getopt(argc,argv,"a:b:c:d:e:t:PDXhv");
      if (ch > 0)
      {
         switch(ch)
         {
            case 'a' :
               sscanf(optarg, "%x", &t);
               if ((t & 0x7F) == t) { pA->busAddr= t; }
               break;
            case 'b' :
               sscanf(optarg, "%d", &t);
               if ((t & 0x07) == t) { pA->ping.nB= t; }
               break;
            case 'c' :
               sscanf(optarg, "%d", &t);
               pA->ping.maxIter= t;
               break;
            case 'd' :
               ch= optarg[0];
               if ((ch > '0') && (ch <= '9')) { pA->devPath[9]= ch; }
               break;
            case 'e' :
               sscanf(optarg,"%d", &t);
               pA->ping.maxErr= t;
               break;
            case 't' :
#ifdef SCI_FMT_H
               nCh= sciFmtScanF64(&f, optarg, strlen(optarg));
               if ((nCh > 0) && (nCh < 6) && (f > 1E-9) && (f < 2)) { t= 1.000001E9 * f; } else
#endif // SCI_FMT_H
               sscanf(optarg,"%d", &t);
               if (t > 0) { pA->ping.ivlNanoSec= t; }
               break;
            case 'P' : pA->flags|= ARG_PING; break;
            case 'D' : pA->flags|= ARG_DUMP; break;
            case 'X' : pA->flags|= ARG_XPT; break;
            //case 'R' : pA->flags|= ARG_READ; break;
            //case 'W' : pA->flags|= ARG_WRITE; break; // TODO: Require payload
            //
            case 'h' : n[1]++; pA->flags|= ARG_HELP; break;
            case 'v' : n[1]++; pA->flags|= ARG_VERBOSE; break;
            default : n[2]++; break; // unrecognised
         }
         n[0]++;
      }
   } while (ch > 0);
   //report(OUT,"nArg: P%d A%d ?%d\n", n[0]-(n[1]+n[2]), n[1], n[2]);
   if (pA->flags & ARG_VERBOSE) { argDump(pA); }
   if (pA->flags & ARG_HELP) { pingUsageMsg(argv[0]); }
   // disable processing if only help/verbose specified
   // otherwise add default action if necessary
   if (n[1] >= n[0]) { pA->flags&= ARG_OPTION; }
   else if (0 == (pA->flags & ARG_ACTION)) { pA->flags|= ARG_PING; }
} // i2cArgTrans

#include "ledMatrix.h"

/***/

LXI2CBusCtx gBusCtx={0,-1};

int main (int argc, char *argv[])
{
   int r= -1;

   i2cArgTrans(&gArgs, argc, argv);

   if ((gArgs.flags & ARG_ACTION) && lxi2cOpen(&gBusCtx, gArgs.devPath, 400))
   {
      if (gArgs.flags & ARG_XPT) { r= ledMatHack(&gBusCtx, 0x75); }
      if (gArgs.flags & ARG_PING) { r= lxi2cPing(&gBusCtx, gArgs.busAddr, &(gArgs.ping), gArgs.flags); }
      if (gArgs.flags & ARG_DUMP) { r= lxi2cDumpDevAddr(&gBusCtx, gArgs.busAddr, 0xFF,0x00); }

      lxi2cClose(&gBusCtx);
   }
   return(r);
} // main

#endif // LX_I2C_MAIN

