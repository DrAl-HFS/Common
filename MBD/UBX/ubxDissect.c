// Common/MBD/ubxDissect.h - u-blox protocol dissection tools
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#include "ubxDissect.h"


/***/

static int stringifyDT (char *s, const int max, const UBXDateTime *pT, const U8 *pNS4)
{
   float fracSec= 0;
   if (pNS4) { fracSec= readBytesLE(pNS4, 0, 4) * 1E-9; }
   return snprintf(s, max, "%d/%d/%d %d:%d:%d (%+f) UTC", rdU16LE(pT->year), pT->month, pT->day, pT->hour, pT->min, pT->sec, fracSec);
} // stringifyDT

static const char *portIDStr (U8 id)
{
   const char *s[]={"DDS","UART","UART2","USB","SPI"};
   if (id < 5) { return(s[id]); }
   return "???";
} // portIDStr

// Return ASCII for low 4 bits (nybble)
static char hexCharL4 (U8 u)
{
   u&= 0xF;
   if (u <= 9) { return(u+'0'); } else { return(u-9+'a'); }
} // hexCharL4

static int hex2ChU8 (char c[2], U8 u)
{
   c[1]= hexCharL4(u);
   c[0]= hexCharL4(u>>4);
   return(2);
} // hex2ChU8

/***/

void ubxDissectNavPVT (const UBXNavPVT *pP, const int n)
{
   if (n >= sizeof(*pP))
   {
      int i;
      char sdt[64];

      stringifyDT(sdt, sizeof(sdt)-1, &(pP->tUTC), pP->nsUTC);
      report(OUT,"%s\n", sdt);
      //if (0x01 & pP->valid)
      {
static const char fixCh[]={'N','D','2','3','G','T','?'};
         i= pP->fixType;
         if (i >= sizeof(fixCh)) { i= sizeof(fixCh)-1; }
         report(OUT,"Fix:%c nSat=%d valid=%02X\n", fixCh[i], pP->nSat, pP->valid);
         //pP->flags, flags2
      }
      if (0 == (pP->flags3 & (1<<0)))
      {
         float sc[2], h[2];
         sc[0]= rdI32LE(pP->lon) * 1E-7;
         sc[1]= rdI32LE(pP->lat) * 1E-7;
         h[0]=  rdI32LE(pP->h) * 0.001;
         h[1]=  rdI32LE(pP->hMSL) * 0.001;
         report(OUT,"lon,lat: %f, %f \t height G,S: %f, %f\n", sc[0], sc[1], h[0], h[1]);
      }
      if (0 == (pP->flags & (1<<5)))
      {
         float v[3], s, b;
         for (i=0; i<3; i++) { v[i]= rdI32LE(pP->velNED[i]) * 0.001; }
         //s= readBytesLE(pP->groundSpeed, 0, 4) * 0.001;
         s= rdI32LE(pP->groundSpeed) * 0.001;
         b= rdI32LE(pP->heading) * 1E-5;
         report(OUT,"vNED (%f, %f, %f) \t speed %fm/s head %fdeg\n", v[0], v[1], v[2], s, b);
      }
   }
} // ubxDissectNavPVT

void ubxDissectCfgPrt (const UBXPort *pP, const int n)
{
   if (n >= sizeof(*pP))
   {
      U32 mode, baud;
      U16 inM=  rdU16LE(pP->inProtoM);
      U16 outM= rdU16LE(pP->outProtoM);
      U16 f=    rdU16LE(pP->flags);
      U8 addr;

      report(OUT,"%s ", portIDStr(pP->id));
      switch(pP->id)
      {
         case UBX_PORT_ID_DDS :
            addr= pP->dds.mode[0];
            report(OUT,"0x%02X (HWx%02X)\n", addr>>1, addr);
            break;
         case UBX_PORT_ID_UART :
            mode= readBytesLE(pP->uart.mode, 0, 4);
            baud= readBytesLE(pP->uart.baud, 0, 4);
            report(OUT,"MD,BD=0x%08X,%d\n", mode, baud);
            break;
         //case UBX_PORT_ID_USB :
         //case UBX_PORT_ID_SPI :
         default :
            mode= readBytesLE(pP->spi.mode, 0, 4);
            report(OUT,"MD=0x%08X", mode);
            break;
      }
      report(OUT," IN,OUT,FLG=%04X,%04X,%04X\n", inM, outM, f);
   }
} // ubxDissectCfgPrt

void ubxDissectCfgInf (const UBXCfgInf *pI, const int n)
{
   if (n >= sizeof(*pI))
   {
      int i= 0, k=0;
      char fs[16];
      do
      {
         for (int j= 0; j<6; j++) { k+= hex2ChU8(fs+k, pI->inf[j]); }
         fs[k]= 0;
         report(OUT,"%s %s\n", portIDStr(pI->id), fs);
      } while ((++i * sizeof(*pI)) <= n);
   }
} // ubxDissectCfgInf
