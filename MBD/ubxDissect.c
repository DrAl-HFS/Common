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
      char dt[32];
      stringifyDT(dt, &(pP->tUTC), pP->nsUTC);
      report(OUT,"%s\n", dt);
   }
} // ubxDissectNavPVT
