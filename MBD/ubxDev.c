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
   FragBuff16  ws[2]; //, res[FRAG_RES_MAX]
   // int uartFD;
   LXUARTCtx uart;
} UBXCtx;

// Definitions ?

/***/




/***/

// TODO: Better param. org.
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
   U8 * const pIn= (U8*)(pC->mb.p)+pC->ws[1].offset;
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
   return ubxReadStream(pFB, pIn, MIN(n, pC->ws[1].len), pC);
} // ubxReadAvailStream

void initCtx (UBXCtx *pC, const LXI2CBusCtx *pDDS, const U8 busAddr, const U16 chunk, const int bytes)
{
   lxUARTOpen(&(pC->uart), "/dev/ttyS0");
   lxUARTClose(&(pC->uart));
   if (allocMemBuff(&(pC->mb), bytes))
   {
      pC->pDDS= pDDS;
      pC->busAddr= busAddr;
      pC->chunk= chunk;
      pC->retry= 3;
      pC->ws[0].offset= 0;
      pC->ws[0].len=    32;
      pC->ws[1].offset= pC->ws[0].len;
      pC->ws[0].len=    pC->mb.bytes - pC->ws[0].len;
   }
} // initCtx

int ubloxHack (const LXI2CBusCtx *pC, const U8 busAddr)
{
   UBXCtx ctx={0,};
   U8 rb[9], *pB=NULL;
   FragBuff16 fb[8];
   int nFB=0, m, n=0, t, tries= 10, r=-1;

   initCtx(&ctx, pC, busAddr, 128, 4<<10);

   rb[n++]= 0xB5;
   rb[n++]= 0x62;
   rb[n++]= UBXM8_CL_CFG;
   rb[n++]= UBXM8_ID_PRT;
   n+= wrI16LE(rb+n, 1);   // 1byte => get
   rb[n++]= 0x00; // I2C
   n+= ubxChecksum(rb+n, rb+2, n-2);
   r= lxi2cWriteRB(pC, busAddr, rb, n);
   LOG("Write CFG-PRT [%d] r=%d\n", n, r);

   n= 0;
   rb[0]= UBXM8_RG_NBYTE_HI;
   do
   {
      r= lxi2cReadRB(pC, busAddr, rb, 3);
      if (r >= 0) { n= (U16)rdI16LE(rb+1); }
      if (n <= 0) { usleep(2000); }
   } while ((n <= 0) && (--tries > 0));

   if (n > 0)
   {
      LOG_CALL("() - %u bytes avail\n", n);
      m= n+2;
      pB= malloc(m);
      if (pB)
      {
         pB[m-1]= 0;
         t= ubxReadStream(fb, pB, m-1, &ctx);
         if (t > 1)
         {
            const U8 *pM= pB + fb[0].offset;
            nFB= ubxScanPayloads(fb, 8, pM, fb[0].len);
            LOG("\t- %d bytes transferred, %d skipped, %d payloads found, r=%d\n", t, fb[0].offset, nFB, r);
            if (nFB > 0) { ubxDumpPayloads(pM,fb,nFB); }
            else
            {
               reportBytes(OUT, pM, fb[0].len);
               if (fb[0].len < m)
               {
                  pB[fb[0].len]= 0;
                  LOG("%s\n", "---");
                  LOG("%s", pM);
                  LOG("\n%s\n", "---");
               }
            }
         }
         free(pB);
         return(r);
      }
   }
   LOG_CALL("() - pB=%p r=%d\n", pB, r);
   releaseMemBuff(&(ctx.mb));
   return(r);
} // ubloxHack

//extern int test (const LXI2CBusCtx *pC, const U8 dev[2], const U8 maxIter);

//#endif // UBX_UTIL_H

