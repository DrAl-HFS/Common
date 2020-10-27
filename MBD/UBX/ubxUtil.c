// Common/MBD/ubxUtil.c - utility code for u-blox GPS module
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#include "ubxUtil.h"


/***/

// Definitions ?

/***/

static int skipASCII (const signed char a[], int max, const signed char min)
{
   int i=0;
   while ((i < max) && (a[i] >= min)) { i++; }
   return(i);
} // skipASCII

int ubxSetFrameHeader (U8 b[], const U8 cl, const U8 id, const U16 nPB)
{
   b[0]= 0xB5;
   b[1]= 0x62;
   b[2]= cl;
   b[3]= id;
   wrI16LE(b+4, nPB);
   return(6);
} // ubxSetFrameHeader

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
   if (n > 6)  // min 7 bytes
   {  // NB preamble not considered
      const U16 len= rdI16LE(b+2);
      if (n >= (4+len))
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
         } //else { LOG("CS{%02X,%02X}{%02X,%02X}\n", cs[0], cs[1], b[len+4], b[len+5]); return(-2); }
      } else return(-1);
   }
   return(0);
} // ubxGetPayload

int ubxScanPayloads (FragBuff16 fb[], const int maxFB, const U8 b[], const int n)
{
   FragBuff16 *pFB=NULL;
   const int m= n-2;
   int nFB=0, nH=0, nBad=0, i=0, r;

   if (maxFB > 0) { pFB= fb; }
   while (i < m)
   {
      if ((0xB5 == b[i+0]) && (0x62 == b[i+1]))
      {
         i+= 2;
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
      else
      {
         r= skipASCII((void*)(b+i), n-i, 0);
         if (r > 0)
         {
            if (pFB && ((nFB+nH) < maxFB))
            {
               pFB[maxFB-nH].offset= i;
               pFB[maxFB-nH].len=    r;
               nH++;
            }
            i+= r;
         }
      }
      i+= (r <= 0);
   }
   if (nBad > 0) { LOG_CALL("() - nBad=%d\n", nBad); }
   return(nFB);
} // ubxScanPayloads




