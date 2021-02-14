// Common/MBD/IMU/bnoHack.h - Hacks for BNO085 IMU
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Feb. 2021

// ref: https://github.com/adafruit/Adafruit_CircuitPython_BNO08x/blob/master/adafruit_bno08x/i2c.py

#include "bnoHack.h"
#include "lxTiming.h"

#define BNO_PLD_MAX  ((1<<15)-5)
#define BNO_PLD_STD  128

// "Sensor Hub Transport Protocol" - Hillcrest labs proprietary
typedef union
{
   U8 b[4];
   struct { U16 len;  // (LE)
      U8 chan, seq; }; // anon
} BNOHdr;

typedef struct
{
   U8 id, rvd[1];
} PldReqID;

typedef struct
{
   U8 id, rst;
   U8 ver[2];
   U32 part;
   U32 build;
   U16 patch;
   U16 rvd;
} PldID;

typedef struct
{
   BNOHdr hdr;
   PldReqID pld;
} PktReqID;

typedef struct
{
   BNOHdr hdr;
   U8 pld[BNO_PLD_STD];
} PktAny;

#ifndef I2C_BLK_BYTES
#define I2C_BLK_BYTES   (1<<9)
#endif

/***/

int payloadBytes (const U8 id[])
{
static const U8 bytesFx[]=
{
   0, // ???
   16, 12,  // Command res, req
   0, 0, 0, 0, 0, // FRS
   16, 2,   // ID res, req
   0, 0,  // ???
   16, 16, 16,  // Feature res,cmd,req
   0
};
   switch(id[0] & 0xF0)
   {
      case 0xF0: return( bytesFx[ (id[0] & 0x0F) ] );
   }
   return(0);
} // payloadBytes

int validID (const PldID *pP) { return(0xF8 == pP->id); }

int dumpID (const PldID *pP)
{
   if (validID(pP))
   {
      LOG("V%d.%d, Part%d, Build%d, Patch%d\n", pP->ver[0], pP->ver[1], pP->part, pP->build, pP->patch);
      return(16);
   }
   return(0);
} // dumpID

U16 dumpHdr (const BNOHdr *pH, int i)
{
   char dir[]={'<','>'};
   U16 pld= pH->len & 0x7FFF; // mask out continuation flag
   if (pld > 4) { pld-= 4; }
   LOG("%c %d (0x%02X), %d, %d:\n", dir[i&0x1], pld, pH->len, pH->chan, pH->seq);
   return(pld);
} // dumpHdr

int dumpRead (const LXI2CBusCtx *pC, const U8 busAddr, PktAny *pA)
{
   const int max= MIN(sizeof(*pA), I2C_BLK_BYTES);
   int l=0, r=0;
   r= lxi2cReadStream(pC, busAddr, pA->hdr.b, sizeof(pA->hdr));
   if (r >= 0)
   {
      int s= dumpHdr(&(pA->hdr),0);
      if (s > 0)
      {
         int i=0, t=0;
         do
         {
            int b= MIN(max, s-t);
            r= lxi2cReadStream(pC, busAddr, pA->hdr.b, b+4);
            if (r >= 0)
            {
               l= dumpHdr(&(pA->hdr),0);
               b= MIN(b, l);
               reportBytes(0, pA->pld, b);
               t+= b; i++;
            }
         } while ((r >= 0) && (t < s));
         LOG(" : [%d] (%d)\n", i, t);
         r= l;
      }
   }
   return(r);
} // dumpRead

int bnoReset (const LXI2CBusCtx *pC, const U8 busAddr, int syncDelay)
{
   PktAny any;
   int r;
   any.hdr.len= 5;
   any.hdr.chan= 1;  // "exe"
   any.hdr.seq= 0;
   any.pld[0]= 0x01; // reset
   r= lxi2cWriteRB(pC, busAddr, any.hdr.b, any.hdr.len);
   if ((r >= 0) && (syncDelay >= 0)) { timeSpinSleep(syncDelay * 1000000); }
   return(r);
} // bnoReset

int bnoHack (const LXI2CBusCtx *pC, const U8 busAddr, const U8 modeFlags)
{
   PktReqID pkt;
   PktAny any;
   int r=0, n=0, tries=3;

   bnoReset(pC, busAddr, 10);
   dumpRead(pC,busAddr,&any); // flush any junk

   pkt.hdr.len= sizeof(pkt);
   if (6 != pkt.hdr.len) { WARN_CALL("() - align pad! (%d)\n", pkt.hdr.len); pkt.hdr.len= 6; }
   pkt.hdr.chan= 2; // "shtp"    0; // "cmd"
   pkt.hdr.seq= 0;
   pkt.pld.id= 0xF9;
   pkt.pld.rvd[0]= 0xFF;

   do
   {
      r= lxi2cWriteRB(pC, busAddr, pkt.hdr.b, pkt.hdr.len);
      if (r >= 0)
      {
         dumpHdr(&(pkt.hdr),1);
         pkt.hdr.seq++;
         int i=0, s=1, l= dumpRead(pC,busAddr,&any);
         while ((i < l) && (s > 0))
         {
            s= dumpID((void*)(any.pld+i));
            if (0 == s) { s= payloadBytes(any.pld+i); }
            else { n+= s; }
            i+= s;
         }
         if (n > 0) { tries= 0; } // correct response
      }
   } while (--tries > 0);
   return(r);
} // bnoHack

