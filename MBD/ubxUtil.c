// Common/MBD/ubxUtil.c - utility code for UBlox M8 GPS module
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

//include "ubxM8.h"
#include "ubxUtil.h"


/***/

// M8 GPS
enum UBloxReg
{  // Only three registers: all read only
   UBLX_REG_NBYTE_HI=0xFD,
   UBLX_REG_NBYTE_LO, //=0xFE
   UBLX_REG_DATASTREAM=0xFF
};


/***/

int ubxChecksum (U8 cs[2], const U8 b[], const int n)
{
   if (n > 0)
   {
      cs[1]= cs[0]= b[0];
      for (int i= 1; i<n; i++)
      {
         cs[0]+= b[i];
         cs[1]+= cs[0];
      }
      return(2);
   }
   return(0);
} // ubxChecksum

int ubxGetPayload (FragBuff16 *pFB, const U8 b[], const int n)
{
   if (n > 6)
   {
      const U16 len= rdI16LE(b+2);
      if (n >= (6+len))
      {
         U8 cs[2];
         //LOG_CALL("() - len=%d\n", len);
         ubxChecksum(cs, b, len+4);
         if ((cs[0] == b[len+4]) && (cs[1] == b[len+5]))
         {
            if (pFB)
            {
               pFB->offset= 4;
               pFB->len= len;
            }
            return(6+len);
         }
      }
   }
   return(0);
} // ubxGetPayload

int ubxScanPayloads (FragBuff16 fb[], const int maxFB, const U8 b[], const int n)
{
   FragBuff16 *pFB=NULL;
   const int m= n-2;
   int nFB=0, nBad=0, i=0, r;

   if (maxFB > 0) { pFB= fb; }
   while (i < m)
   {
      if ((0xB5 == b[i++]) && (0x62 == b[i++]))
      {
         r= ubxGetPayload(pFB, b+i, n-i);
         if (r > 0)
         {
            ++nFB;
            if (pFB)
            {
               pFB->offset+= i;
               if (nFB >= maxFB) { return(nFB); }
               ++pFB; //= fb+nFB;
            }
            i+= r;
         } else { nBad++; }
      }
   }
   if (nBad > 0) { LOG_CALL("() - nBad=%d\n", nBad); }
   return(nFB);
} // ubxScanPayloads

int ubloxHack (const LXI2CBusCtx *pC, const U8 busAddr)
{
   U8 rb[9];
   FragBuff16 fb[8];
   int nFB=0, n=0, tries= 10, r=-1;

   rb[n++]= 0xB5;
   rb[n++]= 0x62;
   rb[n++]= 0x06; // CFG
   rb[n++]= 0x00; // PRT
   n+= wrI16LE(rb+n, 1);
   rb[n++]= 0x00; // I2C
   n+= ubxChecksum(rb+n, rb+2, n-2);
   r= lxi2cWriteRB(pC, busAddr, rb, n);
   LOG("Write CFG-PRT [%d] r=%d\n", n, r);

   n= 0;
   rb[0]= UBLX_REG_NBYTE_HI;
   do
   {
      r= lxi2cReadRB(pC, busAddr, rb, 3);
      if (r >= 0) { n= (U16)rdI16LE(rb+1); }
      if (n <= 0) { usleep(50000); }
   } while ((n <= 0) && (--tries > 0));

   if (n > 0)
   {
      U8 *pB;

      LOG_CALL("() - %u bytes avail\n", n);
      pB= malloc(n+2);
      if (pB)
      {
         int c, t= 0;

         //memset(buffer+1, 0, sizeof(buffer)-1);
         pB[0]= UBLX_REG_DATASTREAM;
         pB[n+1]= 0x00;
         c= MIN(128,n-t);
         r= lxi2cReadRB(pC, busAddr, pB, c);
         if (r >= 0)
         {
            t+= c;
            c= MIN(128,n-t);
         }
         while ((r >= 0) && (c > 0))
         {
            r= lxi2cRead(pC, busAddr, pB+t, c);
            if (r >= 0)
            {
               t+= c;
               c= MIN(128,n-t);
            }
         }
         if (t > 1)
         {
            const signed char *pMsg= (void*)(pB+1);
            nFB= ubxScanPayloads(fb, 8, pB+1, t);
            if (nFB <= 0) { reportBytes(OUT, pB+1, 50); }
            c= 0;
            while ((c < n) && (pMsg[c] < ' ')) { ++c; }
            if (c < n)
            {
               LOG("%s\n", "---");
               LOG("%s", pMsg+c);
               LOG("\n%s\n", "---");
            }
            LOG("\t- %d of %d bytes read, %d chars skipped, %d payloads found, r=%d\n", t, n, c, nFB, r);
         }
         free(pB);
      }
      return(r);
   }
   LOG_CALL("() - %d\n", r);
   return(r);
} // ubloxHack

//extern int test (const LXI2CBusCtx *pC, const U8 dev[2], const U8 maxIter);

//#endif // UBX_UTIL_H

