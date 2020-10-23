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
      U32 mode= readBytesLE(pP->uart.mode, 0, 4);
      U32 baud= readBytesLE(pP->uart.baud, 0, 4);
      U16 inM= (U16)rdI16LE(pP->inProtoM);
      U16 outM= (U16)rdI16LE(pP->outProtoM);
      U16 f= (U16)rdI16LE(pP->flags);
      LOG("ID%02X, MD,BD= %08X,%d; IN,OUT,FLG=%04X,%04X,%04X\n", mode, baud, pP->id, inM, outM, f);
/*      switch(pP->id)
      {
         case 0 :
         case 1 :
         case 2 :
         case 3 :
      }*/
   }
} // ubxDissectCfgPrt
