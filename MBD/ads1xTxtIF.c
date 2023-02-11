// Common/MBD/ads1xTxtIF.c - text conversion interfaces for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: AGPL3
// (c) Project Contributors Feb 2022

#include "ads1xTxtIF.h"


/***/

// Displace to generic multiplexor code where ?
U8 getMuxChanID (const char c, const char max) // ='3')
{
   if ((c >= '0') && (c <= max)) { return(c-'0'); } // else
   if ('G' == c) { return(0xF); } // else
   return(0xFF);
} // getMuxChanID

U8 scanMap (const U8 id, const U8 map[], const int max)
{
   int i=0;
   while ((id != map[i]) && (i < max)) { i++; }
   return(i);
} // scanMap

/***/

int muxMapFromA (U8 m[], int mMax, const char *s, const U8 hwID)
{
static const U8 map[ADS1X_MUX_N]={0x01,0x03,0x13,0x23,0x0F,0x1F,0x2F,0x3F};
   const U8 chanChMax= '3'; // '1' for hwID 1x10 to 1x14 ?
   int r= 0, l= -1, i= 0;

   //report(OUT,"muxMap(%s)",s);
   while (s[i] && (l != i) && (r < mMax))
   {
      l= i;
      U8 mc1= getMuxChanID(s[i],chanChMax);
      if ((mc1 <= 3) && ('/' == s[i+1]))
      {
         U8 mc2= getMuxChanID(s[i+2],chanChMax);
         if ((mc2 <= 0xF) && (mc1 != mc2))
         {
            U8 j= (mc1 << 4) | mc2;
            if (hwID >= 0)
            {
               j= scanMap(j, map, ADS1X_MUX_N);
               if (j < ADS1X_MUX_N) { m[r++]= j; }
            } else { m[r++]= j; }
            //report(OUT,"*** 0x%x,0x%x -> %d\n",mc1,mc2,j);
        }
         i+= 2;
         i+= (0 != s[i]);
      }
      i+= (',' == s[i]);
   }
   return(r);
} // muxMap

