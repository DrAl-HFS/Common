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
} UBXModesDDS;

typedef struct
{
   MemBuff     mb;
   UBXModesDDS dds;
   // Working buffers / storage
   LXUARTCtx uart;
} UBXCtx;

// Definitions ?

/***/

INLINE void ubxSync (const UBXCtx *pUC) { if (pUC->dds.syncus > 0) { usleep(pUC->dds.syncus); } }

// Consider: abstract read method for multi-interface compatibility ?
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

int ubxGetAvail (const UBXCtx *pUC)
{
   int t= pUC->dds.retry;
   U8 io[3];

   io[0]= UBXM8_RG_NBYTE_HI;
   do
   {
      int r= lxi2cReadRB(pUC->dds.pI2C, pUC->dds.busAddr, io, 3);
      if (r >= 0)
      {
         return rdU16BE(io+1);
      } else ubxSync(pUC);
   } while (t-- > 0);
   return(-1);
} // ubxGetAvail

int ubxReadAvailStream (FragBuff16 *pFB, const UBXCtx *pUC)
{
   int n= ubxGetAvail(pUC);
   if (n < 0) { n= pUC->dds.chunk; }
   n= MIN(1+n, pUC->mb.bytes);
   return ubxReadStream(pFB, pUC->mb.p, n, pUC);
} // ubxReadAvailStream

void initCtx (UBXCtx *pUC, const LXI2CBusCtx *pI2C, const LXUARTCtx *pU, const U8 busAddr, const size_t bytes)
{
   //lxUARTOpen(&(pC->uart), "/dev/ttyS0");
   if (allocMemBuff(&(pUC->mb), bytes))
   {
      pUC->dds.pI2C= pI2C;
      pUC->dds.busAddr= busAddr;
      pUC->dds.retry=   3;
      pUC->dds.chunk=   64;
      pUC->dds.syncus=  2000;
   }
} // initCtx

void releaseCtx (UBXCtx *pUC)
{
   lxUARTClose(&(pUC->uart));
   // pI2C ??
   releaseMemBuff(&(pUC->mb));
} // releaseCtx

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
      ubxSync(pUC);
   }
   return(r);
} // ubxRequestPortConfig

void *ubxGetPtr (U8 *pB, const FragBuff16 *pF, const U8 lvl)
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
} // ubxGetPtr

int ubxUpdatePortConfig (U8 *pB, const FragBuff16 fb[], const int n, const UBXCtx *pUC)
{
   UBXFrameHeader *pF;
   int r= -1;

   for (int i=0; i<n; i++)
   {
      if (fb[i].len > 2) // skip ACKS (2 byte cmd hdr pld)
      {
         if (fb[i].len != sizeof(UBXPort)) { ERROR_CALL("() - bad frag [%d] %d != %d\n", i, fb[i].len, sizeof(UBXPort)); return(-1); }
         pF= ubxGetPtr(pB, fb+i, 2);
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
                  ubxSync(pUC);
               }
            }
         }
      }
   }
   return(r);
} // ubxUpdatePortConfig

#define FB_COUNT 8
int ubloxHack (const LXI2CBusCtx *pC, const U8 busAddr)
{
   UBXCtx ctx={0,};
   FragBuff16 fb[FB_COUNT];
   int nFB=0, nEFB=0, t, n, r=-1;

   memset(fb,-1,sizeof(fb));
   initCtx(&ctx, pC, NULL, busAddr, 16<<10);

   //r= ubxReset(UBX_RESET_ID_SW_FULL, &ctx);

   r= ubxRequestPortConfig(UBX_PORT_ID_DDS, &ctx);
   r= ubxRequestPortConfig(UBX_PORT_ID_UART, &ctx);

   r= -1;
   n= 1;
   do
   {
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
               char sf[8], *pS= (void*)(pM+fb[i].offset);
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
   } while (n-- > 0);

   releaseCtx(&ctx);
   return(r);
} // ubloxHack

//extern int test (const LXI2CBusCtx *pC, const U8 dev[2], const U8 maxIter);

//#endif // UBX_UTIL_H

