// Common/MBD/IMU/bnoHack.h - Hacks for BNO085 IMU
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Feb. 2021

// ref: https://github.com/adafruit/Adafruit_CircuitPython_BNO08x/blob/master/adafruit_bno08x/i2c.py

#include "bnoHack.h"
#include "lxTiming.h"

#define BNO_PLD_MAX  ((1<<15)-5)
#define BNO_PLD_INIT 508
#define BNO_PLD_STD  124

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

typedef struct { BNOHdr hdr; U8 pld[BNO_PLD_MAX]; } PktMax;
typedef struct { BNOHdr hdr; U8 pld[BNO_PLD_INIT]; } PktInit;
typedef struct { BNOHdr hdr; U8 pld[BNO_PLD_STD]; } PktStd;
typedef struct { BNOHdr hdr; U8 pld[]; } PktAny; // NB : zero-sized array extension

typedef struct { U8 t; union { int v; const char *p; }; } TLVDatum;

#ifndef I2C_BLK_BYTES
#define I2C_BLK_BYTES   (1<<9)
#endif

#define TLV_MAX 40

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

int addTLV (TLVDatum *pD, const U8 b[], const int remB)
{
   int s= 0;
   if (remB > 2)
   {
      if (((b[0] > 0) && (b[0] < 10)) || (0x80 == b[0]))
      {
         s= 2 + b[1];
         if (remB >= s)
         {
            pD->t= b[0];
            if (b[1] > 4) { pD->t|= 0x80; }
            if (0x80 & pD->t)
            { // ASCII string
               int e= b[1]-1;
               pD->p= (void*)(b+2);
               if (!(printable(pD->p[0]) && (0 == pD->p[e]))) { return(0); }
            }
            else { pD->v= readBytesLE(b, 2, b[1]); }
         }
         else { s= remB-s; } // shortfall
      }
   }
   return(s);
} // addTLV

int scanTLV (TLVDatum d[], const int maxD, const U8 b[], const int maxB)
{
   int iB=0, nD=0;
   while ((iB < maxB) && (nD < maxD))
   {
      int s= addTLV(d+nD, b+iB, maxB-iB);
      if (s > 0) { iB+= s; nD++; } else { iB++; }
   }
   return(nD);
} // scanTLV

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

int dumpRead (const LXI2CBusCtx *pC, const U8 busAddr, PktAny *pA, const int max)
{
   //const int max= MIN(sizeof(*pA), I2C_BLK_BYTES);
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
               reportBytes(OUT, pA->pld, b);
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
   PktStd any;
   int r;
   any.hdr.len= 5;
   any.hdr.chan= 1;  // "exe"
   any.hdr.seq= 0;
   any.pld[0]= 0x01; // reset
   r= lxi2cWriteRB(pC, busAddr, any.hdr.b, any.hdr.len);
   if ((r >= 0) && (syncDelay >= 0)) { timeSpinSleep(syncDelay * 1000000); }
   return(r);
} // bnoReset

const char *getStrTLV (U8 id)
{
static const char *tlvIDStr[]=
{
   "Ver", "GUID",
   "MaxW", "MaxR", "MaxTW", "MaxTR",
   "NormC", "WakeC",
   "AName", "CName"
};
   if (id < 10) { return(tlvIDStr[id]); }
   //else
   return("?");
} // getStrTLV

int init (const LXI2CBusCtx *pC, const U8 busAddr)
{
   TLVDatum tlv[TLV_MAX];
   PktInit any;
   int r;

   bnoReset(pC, busAddr, 10);
   r= dumpRead(pC, busAddr, (void*)&any, BNO_PLD_INIT+4); // flush any junk
   if (r > 0)
   {
      int n= scanTLV(tlv, TLV_MAX, any.pld, r);
      LOG_CALL("() - found %d TLV\n", n);
      for (int i=0; i<n; i++)
      {
         U8 x= 0xC0 & tlv[i].t;
         LOG("%s : ", getStrTLV(tlv[i].t & 0x3F));
         if (0 == x) { LOG("%d\n", tlv[i].v); }
         else if (0x80 == x) { LOG("%s\n", tlv[i].p); }
         else { LOG("0x%02X?\n", tlv[i].p[0]); }
      }
   }
   return(r);
} // init

int bnoHack (const LXI2CBusCtx *pC, const U8 busAddr, const U8 modeFlags)
{
   PktReqID pkt;
   PktStd any;
   int r=0, n=0, tries=3;

   init(pC,busAddr);

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
         int i=0, s=1, l= dumpRead(pC, busAddr, (void*)&any, BNO_PLD_STD+4);
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

