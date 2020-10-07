// Common/MBD/lumissil.h - Minimal definitions for Lumissil/ISSI LED driver chip IS31FL3731
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct. 2020

#ifndef LUMISSIL_H
#define LUMISSIL_H

// IS31FL3731 is a 400kHz I2C interface PWM charlieplexed LED
// driver with 8 frame animation and audio signal display capability.
// Popular (?) breakout boards are the AdaFruit 16x9 and Pimoroni PiGlow
// & LEDshim. These vendors provide library code (Arduino, Circuit & regular
// Python) to drive their boards. Here an alternative C programming language
// implementation is provided. The AdaFruit board is a pretty straightforward
// 16x9 monochrome LED device. The Pimoroni boards however drive RGB LEDs
// from which arises some extra complexity due to the charlieplexed chip outputs.

// The IS31FL3731 implements two charlieplex matrices, each with 9 line tristate
// drive allowing a maximum: 2*(9^2-9) = 2*72 = 144 (16*9) LED channels to be
// driven. Charlieplexing requires time division multiplexing to provide the
// illusion of fully independant control (no more than one member of an opposing
// bias pair can be driven at any given instant) and this is elegantly solved
// by the built-in PWM controller :)

#define LMSL_ELEM_COUNT (2*8*9)
#define LMSL_FLAG_BYTES (LMSL_ELEM_COUNT>>3)
#define LMSL_PWM_BYTES LMSL_ELEM_COUNT

#define LMSL_FRAME_PAGE_COUNT (8)

// Full page structure for a frame (image)
typedef struct
{
   U8 addr[1];//={0x00};
   U8 enable[LMSL_FLAG_BYTES]; // On/off flags
   //U8 addr[1];//={0x12};
   U8 blink[LMSL_FLAG_BYTES];  // Flashing (?)
   //U8 addr[1];//={0x24};
   U8 pwm[LMSL_PWM_BYTES]; // Brightness
} FramePage;

// Enable or blink mask sub-section of a page
// Presumably smaller sub-sections could be updated
typedef struct
{
   U8 addr[1];// 0x00 / 0x12
   U8 pwm[LMSL_FLAG_BYTES];
} FrameMask;

//
typedef struct
{
   U8 addr[1]; // 0x24
   U8 pwm[LMSL_PWM_BYTES];
} FramePWM;

// The single control page containing all display mode control registers
typedef struct
{
   U8 addr[1];//={0x00};
   U8 reg[13]; // NB: 0x7 read-only
} ControlPage;

enum LMSLReg
{
   LMSL_REG_CONFIG=0x00,   // Mode, Autoplay start frame
   LMSL_REG_PICTURE,   // Static display
   LMSL_REG_AUTOPLAY1,  // Loop, frames
   LMSL_REG_AUTOPLAY2=0x03,   // Rate
   // 0x04 unused - scratch
   LMSL_REG_DISPOPT=0x05,  // Intensity, Blink
   LMSL_REG_AUDSYNC,   //0x06
   LMSL_REG_FRAMESTAT, //0x07 read-only
   LMSL_REG_BREATHE1,  //0x08
   LMSL_REG_BREATHE2,  //0x09
   LMSL_REG_SHUTDOWN,  //0x0A
   LMSL_REG_AUDGAIN,   //0x0B
   LMSL_REG_AUDRATE=0x0C,
   // End of control page registers
   LMSL_REG_PAGE_SEL=0xFD  // Special page control register (available on every page, obviously...)
};

enum LMSLPageID
{  // Named to match reference document: data pages labelled from one onward...
   LMSL_DATA_PAGE1=0x00,
   // etc...
   LMSL_DATA_PAGE8=0x07,
// 0x08,09,0A unused
   LMSL_CTRL_PAGE=0x0B
};

#endif // LUMISSIL_H
