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
      U16 inM= (U16)rdI16LE(pP->inProtoM);
      U16 outM= (U16)rdI16LE(pP->outProtoM);
      U16 f= (U16)rdI16LE(pP->flags);
      LOG("IN,OUT,F : %04X,%04X,%04X\n", inM, outM, f);
/*      switch(pP->id)
      {
         case 0 :
         case 1 :
         case 2 :
         case 3 :
      }*/
   }
} // ubxDissectCfgPrt
