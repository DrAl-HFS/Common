// Common/MBD/ad9833Util.h - low level utility code for
// Analog Devices AD9833 signal generator with SPI/3-wire compatible interface.
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors October-November 2020

#ifndef AD9833_UTIL_H
#define AD9833_UTIL_H

#include "ad9833.h"
#include "mbdUtil.h"


/***/

#ifdef __cplusplus
extern "C" {
#endif

// NB: data stored in host native (LE assumed) order
// - conversion for (msb-first/BE word) transmission is handled within SPI routines

// Frequency & phase register grouped together here for software
// simplification, whereas they are each independantly selected by
// a respective control bit. Registers are independantly addressable
// with the exception of frequency low:high words which must be sent
// in that order, despite communication otherwise being big-endian...
typedef struct
{  // frequency 28bits in lo,hi word order (same 2bit ADR each b01 / b10), phase offset 13bits (3bit ADR, b110 / b111)
   // The full 28bits is written in lo,hi order when ctrl:B28=1
   // Otherwise ctrl:HLB defines whether the hi or lo word is assigned
   UU16 fr[2], pr;
} AD9833FreqPhaseReg;

typedef union
{  // Top 2 or 3 bits of each word form the register address
   struct
   {
      UU16 ctrl;  // ADR<<14 = b00
      AD9833FreqPhaseReg fpr[2];
   };
   U8 b[14]; // retained as convenience for development, deprecate later?
} AD9833Reg;

/***/

// void ad9833SetFreq (UU16 fr[2], const float f); // ??

#ifdef __cplusplus
} // extern "C"
#endif

#endif // AD9833_H
