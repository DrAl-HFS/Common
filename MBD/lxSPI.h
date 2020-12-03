// Common/MBD/lxUART.h - Linux SPI utils
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct-Dec 2020

#ifndef LX_SPI_H
#define LX_SPI_H

#include "util.h"
#include <linux/spi/spidev.h>


/***/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
   U32 kdmf, clk;    // kernel driver mode flags, transaction clock rate
   U16 delay;
   U8  bpw, flags;    // bits per word, misc user flags (unused)
   //U8  csID, pad;   // chip select defined in device tree only ?
} SPIProfile;

typedef struct
{
   int  fd;
   U32   maxClk;
   SPIProfile currProf;
} LXSPICtx;



/***/

extern Bool32 lxSPIOpen (LXSPICtx *pSC, const char devPath[], const SPIProfile *pP);

extern int lxSPIReadWrite (LXSPICtx *pSC, U8 r[], const U8 w[], int n);

extern void lxSPIClose (LXSPICtx *pSC);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LX_SPI_H
