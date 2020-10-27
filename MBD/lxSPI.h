// Common/MBD/lxUART.h - Linux SPI utils
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

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
   U32 clock;
   U16 delay;
   U8 bits, flags;
} PortSPI;

typedef struct
{
   int  fd;
   PortSPI port;
} LXSPICtx;



/***/

extern Bool32 lxSPIOpen (LXSPICtx *pSC, const char devPath[], U32 clock);

extern int lxSPIReadWrite (LXSPICtx *pSC, U8 r[], const U8 w[], int n);

extern void lxSPIClose (LXSPICtx *pSC);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LX_SPI_H
