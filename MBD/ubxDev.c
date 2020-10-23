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
   const LXI2CBusCtx *pDDS;
   U8 busAddr, retry;
   U16   chunk;   // i2c-bus stream transaction granularity
   // Working buffers / storage
   MemBuff  mb;
   // waste of time...
   //FragBuff16  ws[2]; //, res[FRAG_RES_MAX]
   LXUARTCtx uart;
} UBXCtx;

// Definitions ?

/***/

// Consider: abstract read method for multi-interface compatibility ?
int ubxReadStream (FragBuff16 *pFB, U8 b[], const int max, const UBXCtx *pC)
{
   int r, retry= 3, l= 0, t= 0, chunk= pC->chunk;
   if (max > 1)
   {
      if (chunk <= 0) { chunk= MIN(32,max); }
      b[0]= UBXM8_RG_DATASTREAM;
      r= lxi2cReadRB(pC->pDDS, pC->busAddr, b, chunk);
      if (r >= 0)
      {
         t+= chunk;
         if (UBXM8_DSB_INVALID == b[1]) { chunk= 0; } // invalid data, quit?
         else { l= t-1; r= max-t; if (r < chunk) { chunk= r; } } // update chunk

         while (chunk > 0)
         {  // Continue reading, no register update necessary
            r= lxi2cRead(pC->pDDS, pC->busAddr, b+t, chunk);
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

int ubxReadAvailStream (FragBuff16 *pFB, const UBXCtx *pC)
{
   //U8 * const pOut= (U8*)(pC->mb.p)+pC->ws[0].offset;
   //U8 * const pIn= (U8*)(pC->mb.p)+pC->ws[1].offset;
   U8 out[4];
   int n= 0, t= pC->retry;

   out[0]= UBXM8_RG_NBYTE_HI;
   do
   {
      int r= lxi2cReadRB(pC->pDDS, pC->busAddr, out, 3);
      if (r >= 0) { n= (U16)rdI16LE(out+1); }
      if (n <= 0) { usleep(2000); }
   } while ((n <= 0) && (--t >= 0));
   if (n < 0) { n= pC->chunk; }
   return ubxReadStream(pFB, pC->mb.p, MIN(n, pC->mb.bytes), pC);
} // ubxReadAvailStream

void initCtx (UBXCtx *pC, const LXI2CBusCtx *pDDS, const LXUARTCtx *pU, const U8 busAddr, const U16 chunk, const int bytes)
{
   //lxUARTOpen(&(pC->uart), "/dev/ttyS0");
   if (allocMemBuff(&(pC->mb), bytes))
   {
      pC->pDDS= pDDS;
      pC->busAddr= busAddr;
      pC->chunk= chunk;
      pC->retry= 3;
      //pC->ws[0].offset= 0;
      //pC->ws[0].len=    32;
      //pC->ws[1].offset= pC->ws[0].len;
      //pC->ws[0].len=    pC->mb.bytes - pC->ws[0].len;
   }
} // initCtx

void releaseCtx (UBXCtx *pUC)
{
   lxUARTClose(&(pUC->uart));
   // pDDS ??
   releaseMemBuff(&(pUC->mb));
} // releaseCtx

int ubxRequestPortConfig (const U8 portID, const UBXCtx *pUC)
{
   int r=-1;
   U8 rqb[9], n=0;
   // NB - no register address specified
   rqb[n++]= 0xB5;
   rqb[n++]= 0x62;
   rqb[n++]= UBXM8_CL_CFG;
   rqb[n++]= UBXM8_ID_PRT;
   n+= wrI16LE(rqb+n, sizeof(portID));   // 1byte => get
   rqb[n++]= portID; // I2C
   n+= ubxChecksum(rqb+n, rqb+2, n-2);
   //
   if (pUC->pDDS)
   {
      r= lxi2cWriteRB(pUC->pDDS, pUC->busAddr, rqb, n);
      LOG_CALL("() - I2C-Write CFG-PRT [%d] r=%d\n", n, r);
      usleep(2000);
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

int ubxSetFrame (UBXFrameHeader *pF, U8 cl, U8 id)
{
   if ((pF->preamble[0] != 0xB5) || (pF->preamble[1] != 0x62))
   {
      ERROR_CALL("() - bad preamble: %02X%02X\n", pF->preamble[0], pF->preamble[1]);
      return(0);
   }
   pF->header.classID[0]= cl;
   pF->header.classID[1]= id;
   return(2);
} // ubxSetFrame

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
         LOG("fb[%d] : %d,%+d %02X%02X\n",i, fb[i].offset, fb[i].len, pF->header.classID[0], pF->header.classID[1]);
         if (pF && (0x3 == ubxHeaderMatch(&(pF->header), UBXM8_CL_CFG, UBXM8_ID_PRT)))
         {
            UBXPort *pP= (void*)(pF+1);
            switch(pP->id)
            {
               case UBX_PORT_ID_DDS : LOG("%s\n","DDS");
                  wrI16LE(pP->inProtoM, UBX_PORT_PROTO_UBX);
                  wrI16LE(pP->outProtoM, UBX_PORT_PROTO_UBX);
                  break;
               case UBX_PORT_ID_UART : LOG("%s\n","UART");
                  writeBytesLE(pP->uart.baud, 0, 4, 19200);
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
               //ubxSetFrame(pF, UBXM8_CL_CFG, UBXM8_ID_PRT);
               n+= ubxChecksum((U8*)(pP+1), (U8*)&(pF->header), n);
               n+= sizeof(pF->preamble); // preamble bytes = sizeof(*pF) - sizeof(pF->header)
               if (pUC->pDDS)
               {
                  r= lxi2cWriteRB(pUC->pDDS, pUC->busAddr, pF->preamble, n);
                  LOG_CALL("() - I2C-Write CFG-PRT [%d] ID%02X r=%d\n", n, pP->id, r);
                  usleep(20000);
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
   int nFB=0, t, n, r=-1;

   initCtx(&ctx, pC, NULL, busAddr, 128, 16<<10);

   r= ubxRequestPortConfig(UBX_PORT_ID_DDS,&ctx);
   r= ubxRequestPortConfig(UBX_PORT_ID_UART,&ctx);

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
         LOG("\t- %d bytes transferred, %d skipped, %d payloads found, r=%d\n", t, fb[0].offset, nFB, r);
         if (nFB > 0)
         {
            ubxDumpPayloads(pM, fb+1, nFB);
            if (0) //r < 0)
            {
               r= ubxUpdatePortConfig(pM, fb+1, nFB, &ctx);
               ubxDumpPayloads(pM, fb+1, nFB);
            }
         }
      }
   } while (n-- > 0);

   releaseCtx(&ctx);
   return(r);
} // ubloxHack

//extern int test (const LXI2CBusCtx *pC, const U8 dev[2], const U8 maxIter);

//#endif // UBX_UTIL_H

