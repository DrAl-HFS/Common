// Common/MBD/ledMapRGB.c - Mapping for RGB LEDs controlled via IS31FL3731 on Pimoroni LEDshim
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct. 2020

#include "ledMapRGB.h"

/***/

#define MAP_MAX_VAL 134

const ChanMap gMapLED=
{
   {
      {  118, 117, 116, 115, 114, 113, 112,
         134, 133, 132, 131, 130, 129, 128,
         127, 121, 122, 123, 124, 125, 126,
         15, 8, 9, 10, 11, 12, 13 },
      {  69, 68, 84, 83, 82, 81, 80,
         21, 20, 19, 18, 17, 33, 32,
         47, 41, 25, 26, 27, 28, 29,
         95, 89, 90, 91, 92, 76, 77 },
      {  85, 101, 100, 99, 98, 97, 96,
         37, 36, 35, 34, 50, 49, 48,
         63, 57, 58, 42, 43, 44, 45,
         111, 105, 106, 107, 108, 109, 93 }
   }
};
#if 1
// Test hacks
static const U8 hackMap[SHIM_LED_COUNT][3]=
{
   {118, 69, 85},
   {117, 68, 101},
   {116, 84, 100},
   {115, 83, 99},
   {114, 82, 98},
   {113, 81, 97},
   {112, 80, 96},

   {134, 21, 37},
   {133, 20, 36},
   {132, 19, 35},
   {131, 18, 34},
   {130, 17, 50},
   {129, 33, 49},
   {128, 32, 48},

   {127, 47, 63},
   {121, 41, 57},
   {122, 25, 58},
   {123, 26, 42},
   {124, 27, 43},
   {125, 28, 44},
   {126, 29, 45},

   {15, 95, 111},
   {8, 89, 105},
   {9, 90, 106},
   {10, 91, 107},
   {11, 92, 108},
   {12, 76, 109},
   {13, 77, 93}
};
static const U8 ctrlMask[]={   // byte-interleaved 8by9 control matrices
0b00000000, 0b10111111, 0b00111110, 0b00111110,
0b00111111, 0b10111110, 0b00000111, 0b10000110,
0b00110000, 0b00110000, 0b00111111, 0b10111110,
0b00111111, 0b10111110, 0b01111111, 0b11111110,
0b01111111, 0b00000000};
/*

void setByMask (U8 r[], const int n, const U8 m[], const U8 v[2])
{
   int i, j;
   for (i= 0; i<n; i++)
   {
      j= (m[i>>3] >> (i & 0x7)) & 0x01; // bit-endianess
      r[i]= v[j];
   }
} // setByMask

   LOG("ctrlMask[%d]=\n", LMSL_FLAG_BYTES);
   for (int i=0; i<LMSL_FLAG_BYTES; i+= 2) LOG("%d : %02X %02X\n", i, ctrlMask[i], ctrlMask[i+1]);

void ()
{      //setByMask(frames[0].pwm, LMSL_PWM_BYTES/2, ctrlMask, v); // LMSL_PWM_BYTES

   if (0 == memcmp(frames[0].enable, ctrlMask, LMSL_FLAG_BYTES)) { LOG("map successful %d\n", r); } else
   {
      ERROR("bad map %d\n", r);
      memcpy(frames[0].enable, ctrlMask, LMSL_FLAG_BYTES);
   }
}
*/

#include <string.h>

// Displace ????
void setBitsU8 (U8 bm[], const U8 idx[], const int n)
{
   for (int i=0; i<n; i++)
   {
      int iB= idx[i] >> 3;
      int m= 1<<(idx[i] & 0x7); // endianess !
      bm[iB]|= m;
   }
} // setBitsU8

#include "lumissil.h"
#include "report.h"

void validateBits (U8 bm[])
{
   if (0 == memcmp(bm, ctrlMask, LMSL_FLAG_BYTES)) { LOG_CALL("(%p) - OK\n", bm); } else
   {
      ERROR_CALL("(%p) - mismatch\n", bm);
      memcpy(bm, ctrlMask, LMSL_FLAG_BYTES);
   }
} // validateBits

#endif // Test hacks

/***/

// pwm[LMSL_PWM_BYTES]
void ledMapChanPWM (U8 pwm[], const U8 v[], const int n, const U8 vIdxMask, const U8 chan[])
{
   for (int i=0; i<n; i++)
   {
      pwm[ chan[i] ]= v[i & vIdxMask];
   }
} // ledMapChanPWM

int ledMapIdxChanPWM (U8 pwm[], const U8 v[], const U8 vIdxMask, const U8 idxChan)
{
   if (idxChan >= 3) { return(0); }
   const U8 *pChan= gMapLED.chanRGB[idxChan];

   for (int i=0; i<SHIM_LED_COUNT; i++)
   {
      if (hackMap[i][idxChan] != pChan[i]) { ERROR("bad map [%d]\n", i); }
      pwm[ pChan[i] ]= v[i & vIdxMask];
   }
   return(SHIM_LED_COUNT);
} // ledMapIdxChanPWM

// bm[LMSL_FLAG_BYTES]
int ledMapSetBits (U8 bm[], const int n)
{
   if ((n<<3) >= MAP_MAX_VAL)
   {
      memset(bm, 0, n);
      setBitsU8(bm, hackMap[0], SHIM_LED_COUNT*3);
      validateBits(bm);
      return(n);
   }
   ERROR_CALL("(%d) - bad map\n", n);
   return(0);
} // ledMapSetBits

