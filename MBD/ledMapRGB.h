// Common/MBD/ledMapRGB.h - Mapping for RGB LEDs controlled via IS31FL3731 on Pimoroni LEDshim
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct. 2020

#ifndef LED_MAP_RGB_H
#define LED_MAP_RGB_H

#include "mbdUtil.h"


/***/

#define SHIM_LED_COUNT (28)
#define CHAN_MODE_REVERSE (1<<7) // reverse order
#define CHAN_MODE_BLUE    (1<<6)
#define CHAN_MODE_GREEN   (1<<5)
#define CHAN_MODE_RED     (1<<4)
#define CHAN_MODE_RGB     (0x7<<4)
#define CHAN_MODE_X       (1<<3) // unused
#define CHAN_MODE_NIMASK  (0x7) // number of bits for value index mask: 0..4 -> 0=single, 0x1,3,7,F=periodic (group 2,4,8,16), >=5= fully individual

typedef struct
{
   U8 red[SHIM_LED_COUNT], green[SHIM_LED_COUNT], blue[SHIM_LED_COUNT];
} ChanMap;

typedef union
{  // Charlieplex RGB LED connectivity map, organised by channel
   struct { U8 red[SHIM_LED_COUNT], green[SHIM_LED_COUNT], blue[SHIM_LED_COUNT]; };
   U8 chanRGB[3][SHIM_LED_COUNT];
} ChanMapU;

extern const ChanMap gMapLED;


/***/

// pwm[LMSL_PWM_BYTES]
// Retain or deprecate ? Marginally more efficient for single channel...
extern void ledMapChanPWM
(
   U8 pwm[], // Destination
   const U8 v[],  // PWM value / pattern
   const int n,     // Number of values to store <= SHIM_LED_COUNT
   const U8 chan[], // Channel map eg. gMapLED.red
   const U8 modes
);

extern int ledMapMultiChanPWM (U8 pwm[], const U8 v[], const int n, const U8 modes);

// bm[LMSL_FLAG_BYTES]
extern int ledMapSetBits (U8 bm[], const int n);

#endif // LED_MAP_RGB_H
