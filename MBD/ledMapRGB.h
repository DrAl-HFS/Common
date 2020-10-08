// Common/MBD/ledMapRGB.h - Mapping for RGB LEDs controlled via IS31FL3731 on Pimoroni LEDshim
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct. 2020

#ifndef LED_MAP_RGB_H
#define LED_MAP_RGB_H

#include "mbdUtil.h"

// Notes: routines here are (lightly) coupled to Pimoroni LEDshim charlieplexing structure.
// May factor this out (if necessary) at some point i.e. if another charlieplexed device
// looks useful...

/***/

#define SHIM_LED_COUNT (28)

#define CHAN_MODE_REVERSE (1<<7) // reverse order
#define CHAN_MODE_BLUE    (1<<6)
#define CHAN_MODE_GREEN   (1<<5)
#define CHAN_MODE_RED     (1<<4)
#define CHAN_MODE_RGB     (0x7<<4)
#define CHAN_MODE_X       (1<<3) // unused
#define CHAN_MODE_NIMASK  (0x7) // number of bits for value index mask: 0..4 -> 0=single, 0x1,3,7,F=periodic (group 2,4,8,16), >=5= fully individual


typedef union
{  // Charlieplex RGB LED connectivity map, organised by channel
   U8 chan[3*SHIM_LED_COUNT];
   struct { U8 red[SHIM_LED_COUNT], green[SHIM_LED_COUNT], blue[SHIM_LED_COUNT]; };
} ChanMapU;

extern const ChanMapU gMapLED;


/***/

// Map multiple input patterns to multiple pwm destination channels
// (Caveat: R,G,B ordering required)
// Stride allows flexible organisation
// e.g. (1,3) for interleaved RGB, (n,1) for planar
extern void ledMapMultiChanPWM
(
   U8 pwm[], // Destination
   const U8 vxxx[], // tuples of dimension <= stride[1]
   const int n,    // Number of values to store <= SHIM_LED_COUNT
   const int stride[2], // [0] for element, [1] for tuple
   const U8 modes
);

// DEPRECATE: supersceded by preceding routin
// Single input pattern to multiple pwm destination channels
extern int ledMap1NChanPWM (U8 pwm[], const U8 v[], const int n, const U8 modes);

// Single input pattern to single pwm destination channel
// Permits offsetting channel map to store contiguous
// (physical space on device) subset (if required ?)
extern void ledMapChanPWM
(
   U8 pwm[], // Destination
   const U8 v[],  // PWM value / pattern
   const int n,     // Number of values to store <= SHIM_LED_COUNT
   const U8 chan[], // Channel map eg. gMapLED.red
   const U8 modes
);

// Default: (bm[LMSL_FLAG_BYTES], LMSL_FLAG_BYTES, SHIM_LED_COUNT, 0)
extern int ledMapSetBits (U8 bm[], const int nBM, int count, int offset, const U8 modes);

#endif // LED_MAP_RGB_H
