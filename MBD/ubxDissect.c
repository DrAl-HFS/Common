// Common/MBD/ubxDissect.h - u-blox protocol dissection tools
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#include "ubxDissect.h"


/***/

void ubxDissectCfgPrt (const UBXPort *pP, const int n)
{
   if (n >= sizeof(*pP))
   {
      U32 mode, baud;
      U16 inM=  rdU16LE(pP->inProtoM);
      U16 outM= rdU16LE(pP->outProtoM);
      U16 f=    rdU16LE(pP->flags);
      U8 addr;

      switch(pP->id)
      {
         case UBX_PORT_ID_DDS :
            addr= pP->dds.mode[0];
            report(OUT,"DDS 0x%02X (HWx%02X) IN,OUT,FLG=%04X,%04X,%04X\n", addr>>1, addr, inM, outM, f);
            break;
         case UBX_PORT_ID_UART :
            mode= readBytesLE(pP->uart.mode, 0, 4);
            baud= readBytesLE(pP->uart.baud, 0, 4);
            report(OUT,"UART MD,BD=0x%08X,%d IN,OUT,FLG=%04X,%04X,%04X\n", mode, baud, inM, outM, f);
            break;
         //case UBX_PORT_ID_USB :
         //case UBX_PORT_ID_SPI :
      }
   }
} // ubxDissectCfgPrt

int stringifyDT (char c[], const UBXDateTime *pT, const U8 *pNS4)
{
   float fracSec= 0;
   if (pNS4) { fracSec= readBytesLE(pNS4, 0, 4) * 1E-9; }
   return sprintf(c, "%d/%d/%d %d:%d:%d (%+f) UTC", rdU16LE(pT->year), pT->month, pT->day, pT->hour, pT->min, pT->sec, fracSec);
} // stringifyDT

void ubxDissectNavPVT (const UBXNavPVT *pP, const int n)
{
   if (n >= sizeof(*pP))
   {
      int i;
      char dt[32];

      stringifyDT(dt, &(pP->tUTC), pP->nsUTC);
      report(OUT,"%s\n", dt);
      if (0x01 & pP->valid) {}
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
         sc[0]= readBytesLE(pP->lon, 0, 4) * 1E-7;
         sc[1]= readBytesLE(pP->lat, 0, 4) * 1E-7;
         h[0]= readBytesLE(pP->h, 0, 4) * 0.001;
         h[1]= readBytesLE(pP->hMSL, 0, 4) * 0.001;
         report(OUT,"lon,lat: %f, %f \t height G,S: %f, %f\n", sc[0], sc[1], h[0], h[1]);
      }
      if (0 == (pP->flags & (1<<5)))
      {
         float v[3], s, b;
         for (i=0; i<3; i++) { v[i]= readBytesLE(pP->velNED[i], 0, 4) * 0.001; }
         s= readBytesLE(pP->groundSpeed, 0, 4) * 0.001;
         b= readBytesLE(pP->heading, 0, 4) * 1E-5;
         report(OUT,"vNED (%f, %f, %f) \t speed %fm/s head %fdeg\n", v[0], v[1], v[2], s, b);
      }
   }
} // ubxDissectNavPVT
