// Common/MBD/ledMapRGB.h - Mapping for RGB LEDs controlled via IS31FL3731 on Pimoroni LEDshim
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct. 2020

#ifndef LED_MAP_RGB_H
#define LED_MAP_RGB_H

#include "mbdUtil.h"


/***/

#define SHIM_LED_COUNT (28)

typedef union
{  // Charlieplex RGB LED connectivity map, organised by channel
   struct { U8 red[SHIM_LED_COUNT], green[SHIM_LED_COUNT], blue[SHIM_LED_COUNT]; };
   U8 chanRGB[3][SHIM_LED_COUNT];
} ChanMap;

extern const ChanMap gMapLED;


/***/

// pwm[LMSL_PWM_BYTES]
extern void ledMapChanPWM
(
   U8 pwm[], // Destination
   const U8 v[],  // PWM value / pattern
   const int n,         // Number of values to store <= SHIM_LED_COUNT
   const U8 vIdxMask,   // Value index mask: 0=single, 0x1,3,7,F=periodic (group 2,4,8,16), -1= fully individual
   const U8 chan[]      // Channel map eg. gMapLED.red
);

// DEPRECATE
extern int ledMapIdxChanPWM (U8 pwm[], const U8 v[], const U8 vIdxMask, const U8 idxChan);

// bm[LMSL_FLAG_BYTES]
extern int ledMapSetBits (U8 bm[], const int n);

#endif // LED_MAP_RGB_H
