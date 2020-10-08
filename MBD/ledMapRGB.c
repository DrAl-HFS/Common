// Common/MBD/ledMapRGB.c - Mapping for RGB LEDs controlled via IS31FL3731 on Pimoroni LEDshim
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct. 2020

#include "ledMapRGB.h"

/***/

#define MAP_MIN_VAL 8
#define MAP_MAX_VAL 134

const ChanMapU gMapLED=
{  // NB: extra braces required for union
#if 1
 { // First union member is Flattened array
   118, 117, 116, 115, 114, 113, 112,  134, 133, 132, 131, 130, 129, 128,
   127, 121, 122, 123, 124, 125, 126,  15, 8, 9, 10, 11, 12, 13,

   69, 68, 84, 83, 82, 81, 80,      21, 20, 19, 18, 17, 33, 32,
   47, 41, 25, 26, 27, 28, 29,      95, 89, 90, 91, 92, 76, 77,

   85, 101, 100, 99, 98, 97, 96,    37, 36, 35, 34, 50, 49, 48,
   63, 57, 58, 42, 43, 44, 45,      111, 105, 106, 107, 108, 109, 93
 }
#else
 { // First union member is struct of arrays
   {  118, 117, 116, 115, 114, 113, 112,  134, 133, 132, 131, 130, 129, 128,  127, 121, 122, 123, 124, 125, 126,  15, 8, 9, 10, 11, 12, 13 },
   {  69, 68, 84, 83, 82, 81, 80, 21,  20, 19, 18, 17, 33, 32, 47,   41, 25, 26, 27, 28, 29, 95,   89, 90, 91, 92, 76, 77 },
   {  85, 101, 100, 99, 98, 97, 96,    37, 36, 35, 34, 50, 49, 48,   63, 57, 58, 42, 43, 44, 45,   111, 105, 106, 107, 108, 109, 93 }
 }
#endif
}; // gMapLED


/***/

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

void mapCheck (U8 idxChan)
{
   const U8 *pChan= gMapLED.red;

   for (int i=0; i<SHIM_LED_COUNT; i++)
   {
      if (hackMap[i][0] != pChan[i]) { ERROR_CALL("bad [%d]\n", i); }
   }
} // mapCheck

#endif // Test hacks

U8 getVIM (U8 m)
{
   I8 n= m & CHAN_MODE_NIMASK;
   if (n > 4) { return 0xFF; }
   //else
   return((1 << n) - 1);
} // getVIM

U8 getChanPtrs (const U8 *p[3], U8 m)
{
   U8 n=0;
   //if (0 == (m & CHAN_MODE_RGB)) { m|= CHAN_MODE_RGB }
   if (m & CHAN_MODE_RED) { p[n++]= gMapLED.red; }
   if (m & CHAN_MODE_GREEN) { p[n++]= gMapLED.green; }
   if (m & CHAN_MODE_BLUE) { p[n++]= gMapLED.blue; }
   return(n);
} // getChanPtrs

/***/

void ledMapMultiChanPWM
(
   U8 pwm[], // Destination
   const U8 vxxx[],  // tuples
   const int n,     // Number of values to store <= SHIM_LED_COUNT
   const int stride[2],
   const U8 modes
)
{
   const U8 *pChan[3], nC= getChanPtrs(pChan, modes);
   if (nC > 0)
   {
      const U8 vIdxMask= getVIM(modes);

      for (int i=0; i<n; i++)
      {
         int j= (i & vIdxMask) * stride[1];
         U8 k= 0;
         if (modes & CHAN_MODE_REVERSE)
         {
            do { pwm[ pChan[k][n-i] ]= vxxx[j]; j+= stride[0]; } while (++k < nC);
         }
         else
         {
            do { pwm[ pChan[k][i] ]= vxxx[j]; j+= stride[0]; } while (++k < nC);
         }
      }
   }
} // ledMapMultiChanPWM

int ledMap1NChanPWM (U8 pwm[], const U8 v[], const int n, const U8 modes)
{
   const U8 *pChan[3], nC= getChanPtrs(pChan, modes);

   if (nC > 0)
   {
      const U8 vIdxMask= getVIM(modes);

      if (modes & CHAN_MODE_REVERSE)
      {
         for (int i=0; i<n; i++)
         {
            const U8 u= v[i & vIdxMask];
            U8 k= 0;
            do { pwm[ pChan[k][n-i] ]= u; } while (++k < nC);
         }
      }
      else
      {
         for (int i=0; i<n; i++)
         {
            const U8 u= v[i & vIdxMask];
            U8 k= 0;
            do { pwm[ pChan[k][i] ]= u; } while (++k < nC);
         }
      }
      return(n);
   }
   return(0);
} // ledMap1NChanPWM

// (pwm[LMSL_PWM_BYTES], v[n], gMapLED.red, CHAN_MODE_REVERSE|3)
void ledMapChanPWM (U8 pwm[], const U8 v[], const int n, const U8 chan[], const U8 modes)
{
   const U8 vIdxMask= getVIM(modes);
   if (modes & CHAN_MODE_REVERSE)
   {  // reverse order
      for (int i=0; i<n; i++) { pwm[ chan[n-i] ]= v[i & vIdxMask]; }
   }
   else
   {
      for (int i=0; i<n; i++) { pwm[ chan[i] ]= v[i & vIdxMask]; }
   }
} // ledMapChanPWM

// bm[LMSL_FLAG_BYTES]
int ledMapSetBits (U8 bm[], const int nBM, int count, int offset, const U8 modes)
{
   if ((nBM<<3) >= MAP_MAX_VAL)
   {
      memset(bm, 0, nBM);
      if ((0 == offset) && (SHIM_LED_COUNT == count) && (CHAN_MODE_RGB == (modes & CHAN_MODE_RGB)))
      {
         setBitsU8(bm, gMapLED.chan, SHIM_LED_COUNT*3);
         //validateBits(bm);
         return(nBM);
      }
      else
      {
         if ((offset < 0) && (count + offset > 0)) { count+= offset; offset= 0; }
         if (offset < SHIM_LED_COUNT)
         {
            if ((count <= 0) || (count+offset > SHIM_LED_COUNT)) { count= SHIM_LED_COUNT-offset; }
            if (modes & CHAN_MODE_RED)   { setBitsU8(bm, gMapLED.red+offset, count); }
            if (modes & CHAN_MODE_GREEN) { setBitsU8(bm, gMapLED.green+offset, count); }
            if (modes & CHAN_MODE_BLUE)  { setBitsU8(bm, gMapLED.blue+offset, count); }
            return(nBM);
         }
      }
   }
   ERROR_CALL("(%d) - bad map\n", nBM);
   return(0);
} // ledMapSetBits

