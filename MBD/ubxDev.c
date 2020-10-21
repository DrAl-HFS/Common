// Common/MBD/ubxDev.c - utility code for u-blox GPS module
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#include "ubxDev.h"


/***/

// Definitions ?

/***/

// Factor -> ubxDbg ?
const char *ubxClassStr (U8 c)
{
   switch(c)
   {
      case UBXM8_CL_NAV : return("NAV");
      case UBXM8_CL_RXM : return("RXM");
      case UBXM8_CL_INF : return("INF");
      case UBXM8_CL_ACK : return("ACK");
      case UBXM8_CL_CFG : return("CFG");
      case UBXM8_CL_UPD : return("UPD");
      case UBXM8_CL_MON : return("MON");
      case UBXM8_CL_AID : return("AID");
      case UBXM8_CL_TIM : return("TIM");
      case UBXM8_CL_ESF : return("ESF");
      case UBXM8_CL_MGA : return("MGA");
      case UBXM8_CL_LOG : return("LOG");
      case UBXM8_CL_SEC : return("SEC");
      case UBXM8_CL_HNR : return("HNR");
      case UBXM8_CL_NMEA : return("NMEA");
      case UBXM8_CL_PUBX : return("PUBX");
   }
   return(NULL);
} // ubxClassStr

const char *ubxIDStr (char ch[], const U8 classID[2])
{
   switch(classID[0])
   {
      case UBXM8_CL_ACK : ch[0]= classID[1] +'0'; return(ch);

      case UBXM8_CL_CFG :
         switch(classID[1])
         {
            case UBXM8_ID_PRT : return("PRT");
            case UBXM8_ID_MSG : return("MSG");
            case UBXM8_ID_INF : return("INF");
            case UBXM8_ID_CFG : return("CFG");
         }
         break;
   }
   return("??"); // far from complete...
} // ubxIDStr

int ubxReadStream (FragBuff16 *pFB, U8 b[], const int max, int chunk, const LXI2CBusCtx *pC, const U8 busAddr)
{
   int r, retry= 3, l= 0, t= 0;
   if (max > 1)
   {
      b[0]= UBXM8_RG_DATASTREAM;
      if (chunk <= 0) { chunk= MIN(128,max); }
      r= lxi2cReadRB(pC, busAddr, b, chunk);
      if (r >= 0)
      {
         t+= chunk;
         if (UBXM8_DSB_INVALID == b[1]) { chunk= 0; } // invalid data, quit?
         else { l= t-1; r= max-t; if (r < chunk) { chunk= r; } } // update chunk

         while (chunk > 0)
         {  // Continue reading, no register update necessary
            r= lxi2cRead(pC, busAddr, b+t, chunk);
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

void ubxDumpPayloads (const U8 b[], FragBuff16 fb[], int nFB)
{
   for (int i=0; i<nFB; i++)
   {
      const U8 *pP= b + fb[i].offset;
      const UBXM8Header *pH= (void*)(pP-6); //sizeof(*pH));
      const char *cs= ubxClassStr(pH->classID[0]);
      if (NULL == cs)
      {
         LOG("0x%02X,%02X : ", pH->classID[0], pH->classID[1]);
         reportBytes(OUT, pP, fb[i].len);
      }
      else
      {
         char ch[2];
         ch[1]= 0;
         LOG("%s,%s : ", cs, ubxIDStr(ch,pH->classID));
      }

   }
} // ubxDumpPayloads

int ubloxHack (const LXI2CBusCtx *pC, const U8 busAddr)
{
   U8 rb[9], *pB=NULL;
   FragBuff16 fb[8];
   int nFB=0, m, n=0, t, tries= 10, r=-1;

   LOG("sizeof(UBXM8Header)=%d\n", sizeof(UBXM8Header));

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
         t= ubxReadStream(fb, pB, m-1, 128, pC, busAddr);
         if (t > 1)
         {
            const U8 *pM= pB + fb[0].offset;
            nFB= ubxScanPayloads(fb, 8, pM, fb[0].len);
            LOG("\t- %d bytes transferred, %d skipped, %d payloads found, r=%d\n", t, n, fb[0].offset, nFB, r);
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
   return(r);
} // ubloxHack

//extern int test (const LXI2CBusCtx *pC, const U8 dev[2], const U8 maxIter);

//#endif // UBX_UTIL_H

