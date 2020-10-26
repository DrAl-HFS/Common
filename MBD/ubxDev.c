// Common/MBD/ubxDev.c - utility code for u-blox GPS module (MAX-M8Q-0-10)
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#include "ubxDev.h"
#include "ubxUtil.h"
#include "ubxDissect.h"
#include "ubxDebug.h"


/***/

typedef struct
{
   const LXI2CBusCtx *pI2C;
   U8    busAddr, retry;
   U16   chunk;   // i2c-bus stream transaction granularity
   U32   syncus;
} UBXInfoDDS;

typedef struct
{
   MemBuff     mb;
   UBXInfoDDS dds;
   // Working buffers / storage
   LXUARTCtx uart;
} UBXCtx;

// Definitions ?

/***/

INLINE void ubxSyncDDS (const UBXInfoDDS *pD) { if (pD->syncus > 0) { usleep(pD->syncus); } }

int ubxGetAvailDDS (const UBXInfoDDS *pD)
{
   int t= pD->retry;
   U8 io[3];

   io[0]= UBXM8_RG_NBYTE_HI;
   do
   {
      int r= lxi2cReadRB(pD->pI2C, pD->busAddr, io, 3);
      if (r >= 0) { return rdU16BE(io+1); }
      else if (t > 0) { ubxSyncDDS(pD); }
   } while (t-- > 0);
   return(-1);
} // ubxGetAvailDDS

// DEPRECATE
int ubxReadStream (FragBuff16 *pFB, U8 b[], const int max, const UBXCtx *pUC)
{
   int r, retry= 3, l= 0, t= 0, chunk= pUC->dds.chunk;
   if (max > 1)
   {
      if (chunk <= 0) { chunk= MIN(32,max); }
      b[0]= UBXM8_RG_DATASTREAM;
      r= lxi2cReadRB(pUC->dds.pI2C, pUC->dds.busAddr, b, chunk);
      if (r >= 0)
      {
         t+= chunk;
         if (UBXM8_DSB_INVALID == b[1]) { chunk= 0; } // invalid data, quit?
         else { l= t-1; r= max-t; if (r < chunk) { chunk= r; } } // update chunk

         while (chunk > 0)
         {  // Continue reading, no register update necessary
            r= lxi2cRead(pUC->dds.pI2C, pUC->dds.busAddr, b+t, chunk);
            if (r >= 0)
            {
               t+= chunk;
               if (UBXM8_DSB_INVALID == b[t]) { chunk= 0; } // invalid data, quit?
               else { l= t-1; r= max-t; if (r < chunk) { chunk= r; } } // update chunk
            } else if (--retry < 0) { chunk= 0; }
         }
         if (pFB) { pFB->offset= 1; pFB->len= l; } // store result
      }
   }
   return(t);
} // ubxReadStream

int ubxReadAvailStream (FragBuff16 *pFB, const UBXCtx *pUC)
{
   int r= ubxGetAvailDDS(&(pUC->dds));
   LOG_CALL("()  - %d\n", r);
   if (r >= 0)
   {
      if (0 == r) { r= pUC->dds.chunk; }
      r= MIN(1+r, pUC->mb.bytes);
      // DEPRECATE
      return ubxReadStream(pFB, pUC->mb.p, r, pUC);
   }
   return(r);
} // ubxReadAvailStream

void initCtx (UBXCtx *pUC, const LXI2CBusCtx *pI2C, const LXUARTCtx *pU, const U8 busAddr, const size_t bytes)
{
   //lxUARTOpen(&(pC->uart), "/dev/ttyS0");
   if (allocMemBuff(&(pUC->mb), bytes))
   {
      pUC->dds.pI2C= pI2C;
      pUC->dds.busAddr= busAddr;
      pUC->dds.retry=   3;
      pUC->dds.chunk=   16;
      pUC->dds.syncus=  2000;
   }
} // initCtx

void releaseCtx (UBXCtx *pUC)
{
   lxUARTClose(&(pUC->uart));
   // pI2C ??
   releaseMemBuff(&(pUC->mb));
} // releaseCtx

// DISPLACE / rename ?
int iclamp (int x, const int min, const int max)
{
   if (x > max) { return(max); } // order important!
   if (x < min) { return(min); }
   return(x);
} // iclamp

int ubxWriteDDS (const UBXInfoDDS *pD, const U8 msg[], const int msgLen)
{
   int r, t= pD->retry;
   do
   {
      r= lxi2cWriteRB(pD->pI2C, pD->busAddr, msg, msgLen);
      if (r >= 0) { return(r); }
      else if (t > 0) { ubxSyncDDS(pD); }
   } while (t-- > 0);
   return(-1);
} // ubxWriteDDS

int ubxReadDDS (const MemBuff *pMB, const UBXInfoDDS *pD, const int avail, const int expectPld)
{
   const int bT= iclamp(avail, UBX_PKT_MIN+expectPld, pMB->bytes); // Target
   int chunk=  MIN(pD->chunk, bT);
   U8 *pB=  pMB->p;
   int bR=  0;   // Result
   int r, t= pD->retry;
   do // Repeated chunk read (no register update necessary)
   {
      pB[bR]= UBXM8_DSB_INVALID;  // set guard byte
      r= lxi2cRead(pD->pI2C, pD->busAddr, pB+bR, chunk);
      if ((r < 0) || (UBXM8_DSB_INVALID == pB[bR]))
      {
         if (t-- > 0) { ubxSyncDDS(pD); }
         else { chunk= 0; } // give up
      }
      else
      {
         bR+= chunk;
         r= bT - bR;
         if (r < chunk) { chunk= r; }
      }
   } while (chunk > 0);
   return(bR);
} // ubxReadDDS

int ubxTransactDDS (const MemBuff *pMB, const UBXInfoDDS *pD, const U8 msg[], const int msgLen, const int expectPld)
{
   int r= ubxWriteDDS(pD, msg, msgLen);
   if (r >= 0)
   {
      r= ubxGetAvailDDS(pD);
      if (r >= 0) // Persist even if nothing known available (yet)
      {
         return ubxReadDDS(pMB,pD,r,expectPld);
#if 0
         int bT=     iclamp(r, UBX_PKT_MIN, pMB->bytes); // Target
         int chunk=  iclamp(pD->chunk, bT, pMB->bytes);
         U8 *pB=  pMB->p;
         int bR=  0;   // Result

         int t= pD->retry;
         do // Repeated chunk read (no register update necessary)
         {
            pB[bR]= UBXM8_DSB_INVALID;  // set guard byte
            r= lxi2cRead(pD->pI2C, pD->busAddr, pB+bR, chunk);
            if ((r < 0) || (UBXM8_DSB_INVALID == pB[bR]))
            {
               if (t-- > 0) { ubxSyncDDS(pD); }
               else { chunk= 0; } // give up
            }
            else
            {
               bR+= chunk;
               r= bT - bR;
               if (r < chunk) { chunk= r; }
            }
         } while (chunk > 0);
         return(bR);
#endif
      }
   }
   return(r);
} // ubxTransactDDS

int ubxGetInfo (FragBuff16 *pFB, const UBXCtx *pUC)
{
   int r=-1;
   if (pUC->dds.pI2C && validMemBuff(&(pUC->mb),1<<10)) // NB info result potentially large
   {
      U8 msg[9], n= ubxSetFrameHeader(msg, UBXM8_CL_CFG, UBXM8_ID_INF, 1);
      msg[n++]= 0x00; // UBX protocol
      n+= ubxChecksum(msg+n, msg+2, n-2);
      r= ubxTransactDDS(&(pUC->mb), &(pUC->dds), msg, n, sizeof(UBXCfgInf));
      if (pFB && (r > 0)) { pFB->offset= 0; pFB->len= r; } // ?????
   }
   return(r);
} // ubxGetInfo

int ubxSetRate (const U8 cl, const U8 id, const U8 rate, const UBXCtx *pUC)
{  // NB - no register address specified
   int r=-1;
   U8 msg[11], n= ubxSetFrameHeader(msg, UBXM8_CL_CFG, UBXM8_ID_MSG, 3);

   msg[n++]= cl; // Mask lo
   msg[n++]= id; // Mask hi
   msg[n++]= rate;
   n+= ubxChecksum(msg+n, msg+2, n-2);
   //
   if (pUC->dds.pI2C)
   {
      r= lxi2cWriteRB(pUC->dds.pI2C, pUC->dds.busAddr, msg, n);
      LOG_CALL("() - I2C-Write CFG-MSG[%d] r=%d\n", n, r);
      ubxSyncDDS(&(pUC->dds));
   }
   return(r);
} // ubxSetRate

int ubxReset (const U8 resetID, const UBXCtx *pUC)
{  // NB - no register address specified
   int r=-1;
   U8 msg[13], n= ubxSetFrameHeader(msg, UBXM8_CL_CFG, UBXM8_ID_RST, 4);

   msg[n++]= 0x00; // Mask lo
   msg[n++]= 0x00; // Mask hi
   msg[n++]= resetID;
   msg[n++]= 0x00; // reserved
   n+= ubxChecksum(msg+n, msg+2, n-2);
   //
   if (pUC->dds.pI2C)
   {
      r= lxi2cWriteRB(pUC->dds.pI2C, pUC->dds.busAddr, msg, n);
      LOG_CALL("() - I2C-Write CFG-RST[%d] r=%d\n", n, r);
      sleep(1);
   }
   return(r);
} // ubxReset

int ubxRequestPortConfig (const U8 portID, const UBXCtx *pUC)
{  // NB - no register address specified
   int r=-1;
   U8 rqb[9], n= ubxSetFrameHeader(rqb, UBXM8_CL_CFG, UBXM8_ID_PRT, sizeof(portID));

   rqb[n++]= portID; // UART/I2C
   n+= ubxChecksum(rqb+n, rqb+2, n-2);
   //
   if (pUC->dds.pI2C)
   {
      r= lxi2cWriteRB(pUC->dds.pI2C, pUC->dds.busAddr, rqb, n);
      LOG_CALL("() - I2C-Write CFG-PRT [%d] r=%d\n", n, r);
      ubxSyncDDS(&(pUC->dds));
   }
   return(r);
} // ubxRequestPortConfig

int ubxGetVersion (const U8 portID, const UBXCtx *pUC)
{
   int r=-1;
   return(r);
} // ubxRequestPortConfig

void *ubxGetHeadPtr (U8 *pB, const FragBuff16 *pF, const U8 lvl)
{
   // if ((pF->offset+pF->len) > pB->bytes) { return(NULL); } // super paranoid
   pB+= pF->offset;
   switch(lvl)
   {
      case 0 : return(pB);
      case 1 : if (pF->offset >= sizeof(UBXHeader)) { return(pB-sizeof(UBXHeader)); } break;
      case 2 : if (pF->offset >= sizeof(UBXFrameHeader)) { return(pB-sizeof(UBXFrameHeader)); } break;
   }
   return(NULL);
} // ubxGetHeadPtr

int ubxUpdatePortConfig (U8 *pB, const FragBuff16 fb[], const int n, const UBXCtx *pUC)
{
   UBXFrameHeader *pF;
   int r= -1;

   for (int i=0; i<n; i++)
   {
      if (fb[i].len > 2) // skip ACKS (2 byte cmd hdr pld)
      {
         if (fb[i].len != sizeof(UBXPort)) { ERROR_CALL("() - bad frag [%d] %d != %d\n", i, fb[i].len, sizeof(UBXPort)); return(-1); }
         pF= ubxGetHeadPtr(pB, fb+i, 2);
         //LOG("fb[%d] : %d,%+d %02X%02X\n",i, fb[i].offset, fb[i].len, pF->header.classID[0], pF->header.classID[1]);
         if (pF && (0x3 == ubxHeaderMatch(&(pF->header), UBXM8_CL_CFG, UBXM8_ID_PRT)))
         {
            UBXPort *pP= (void*)(pF+1);
            switch(pP->id)
            {
               case UBX_PORT_ID_DDS :
                  wrI16LE(pP->inProtoM, UBX_PORT_PROTO_UBX);
                  wrI16LE(pP->outProtoM, UBX_PORT_PROTO_UBX);
                  break;
               case UBX_PORT_ID_UART :
                  writeBytesLE(pP->uart.baud, 0, 4, 115200);
                  wrI16LE(pP->inProtoM, UBX_PORT_PROTO_NMEA);
                  wrI16LE(pP->outProtoM, UBX_PORT_PROTO_NMEA);
                  break;
               default :
                  WARN_CALL("() - unsupported port? %02X", pP->id);
                  pF= NULL;
                  break;
            }
            if (pF)
            {
               U8 n= sizeof(pF->header)+sizeof(*pP);
               n+= ubxChecksum((U8*)(pP+1), (U8*)&(pF->header), n);
               n+= sizeof(pF->preamble); // preamble bytes = sizeof(*pF) - sizeof(pF->header)
               if (pUC->dds.pI2C)
               {
                  const char *ids[]={"DDS","UART"};
                  r= lxi2cWriteRB(pUC->dds.pI2C, pUC->dds.busAddr, pF->preamble, n);
                  LOG_CALL("() - I2C-Write CFG-PRT [%d] %s r=%d\n", n, ids[pP->id], r);
                  ubxSyncDDS(&(pUC->dds));
               }
            }
         }
      }
   }
   return(r);
} // ubxUpdatePortConfig

#ifdef UBX_TEST

#define FB_COUNT 8

static int endFrag (const FragBuff16 *pF, const int ext) { return(pF->offset + pF->len + ext); }

int ubxProcessPayloads (const MemBuff *pMB, const FragBuff16 *pFB)
{
   FragBuff16 fb[FB_COUNT];
   const U8 *pM= (void*)(pMB->w+pFB->offset);
   int r, nFB= ubxScanPayloads(fb, FB_COUNT, pM, pFB->len);
   LOG("\t - %d payloads\n", nFB);
   ubxDumpPayloads(pM, fb, nFB, DBG_MODE_RAW);
   r= endFrag(fb+nFB-1, sizeof(UBXFrameFooter));
   if ((r < pFB->len) && (UBXM8_DSB_INVALID == pM[r]))
   {
      const int r0= r;
      do { ++r; } while ((r < pFB->len) && (UBXM8_DSB_INVALID == pM[r]));
      LOG("\t - %d trailing junk bytes\n", r-r0);
   }
   return(r);
} // ubxProcessPayloads

int ubxTest (const LXI2CBusCtx *pC, const U8 busAddr)
{
   UBXCtx ctx={0,};
   FragBuff16 fb[FB_COUNT];
   //U8 *pM;
   int nFB=0, nEFB=0, t, n, m, r=-1;

   //LOG("UBXNavPVT:%d\n", sizeof(UBXNavPVT));
   memset(fb,-1,sizeof(fb));
   initCtx(&ctx, pC, NULL, busAddr, 16<<10);

   r= ubxGetInfo(fb, &ctx);
   LOG("ubxGetInfo() - %d bytes\n", r);
   if (r > 0)
   {
      r= ubxProcessPayloads(&(ctx.mb), fb);
      fb[0].offset+= r;
      fb[0].len-= r;
      if (fb[0].len > 0)
      {
         void *pM= (void*)(ctx.mb.w+fb[0].offset);
         LOG("residual %d %d\n", fb[0].offset, fb[0].len);
         reportBytes(OUT,pM,fb[0].len);
      }
      //r= ubxReset(UBX_RESET_ID_SW_FULL, &ctx);

      r= ubxRequestPortConfig(UBX_PORT_ID_DDS, &ctx);
      //r= ubxRequestPortConfig(UBX_PORT_ID_UART, &ctx);
      r= ubxSetRate(UBXM8_CL_NAV, UBXM8_ID_PVT, 1, &ctx);
      r= -1;
      n= 0; m= 5;
      do
      {
         LOG("I%d/%d\n",n,m);
         t= ubxReadAvailStream(fb,&ctx);
         LOG("t[%d]=%d\n",n,t);
         if (t > 0)
         {
            U8 *pM= (U8*)(ctx.mb.p) + fb[0].offset;
            nFB= ubxScanPayloads(fb+1, FB_COUNT-1, pM, fb[0].len);
            for (int i=nFB+1; i<FB_COUNT; i++)
            {
               if ((fb[i].offset + fb[i].len) < fb[0].len)
               {
                  char sf[16], *pS= (void*)(pM+fb[i].offset);
                  if (snprintf(sf, sizeof(sf)-1, "%s", pS) > 0)
                  {
                     LOG("%d %+d[%d]=%s\n", i, fb[i].offset, fb[i].len, sf);
                  }
                  nEFB++;
               }
            }
            LOG("\t- %d bytes transferred, %d skipped, payloads found: %d UBX ,%d other. r=%d\n", t, fb[0].offset, nFB, nEFB, r);
            if (nFB > 0)
            {
               ubxDumpPayloads(pM, fb+1, nFB, DBG_MODE_RAW);
               if (r < 0)
               {
                  r= ubxUpdatePortConfig(pM, fb+1, nFB, &ctx);
                  //ubxDumpPayloads(pM, fb+1, nFB, DBG_MODE_RAW);
                  sleep(1);
               }
            }
         }
         usleep(500000);
      } while (n++ < m);
      r= ubxSetRate(UBXM8_CL_NAV, UBXM8_ID_PVT, 0, &ctx);
   }
   releaseCtx(&ctx);
   return(r);
} // ubxTest

#endif // UBX_TEST

#ifdef UBX_MAIN

typedef struct
{
   char devPath[14]; // host device path
   U8 busAddr;
   U8 flags, pad;
} UBXArgs;

static UBXArgs gArgs= { "/dev/i2c-1", 0x42, 0, 0 };

void usageMsg (const char name[])
{
static const char optCh[]="advh";
static const char argCh[]="##  ";
static const char *desc[]=
{
   "I2C bus address: 2digit hex (no prefix)",
   "device index (-> path /dev/i2c-# )",
   "verbose diagnostic messages",
   "help (display this text)",
};
   const int n= sizeof(desc)/sizeof(desc[0]);
   report(OUT,"Usage : %s [-%s]\n", name, optCh);
   for (int i= 0; i<n; i++)
   {
      report(OUT,"\t%c %c - %s\n", optCh[i], argCh[i], desc[i]);
   }
} // usageMsg

void argDump (const UBXArgs *pA)
{
   report(OUT,"Device: devPath=%s, busAddr=%02X\n", pA->devPath, pA->busAddr);
   report(OUT,"\tflags=%02X\n", pA->flags);
} // argDump

#define ARG_AUTO    (1<<2)
#define ARG_HELP    (1<<1)
#define ARG_VERBOSE (1<<0)

void argTrans (UBXArgs *pA, int argc, char *argv[])
{
   int c, t;
   do
   {
      c= getopt(argc,argv,"a:d:vh");
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
         case 'h' :
            pA->flags|= ARG_HELP;
            break;
         case 'v' :
            pA->flags|= ARG_VERBOSE;
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
      ubxTest(&gBusCtx, gArgs.busAddr);
      lxi2cClose(&gBusCtx);
   }

   return(r);
} // main

#endif // UBX_MAIN
