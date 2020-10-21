// Common/MBD/lxUART.h - Linux UART utils
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#ifndef LX_UART_H
#define LX_UART_H

#include "util.h"


/***/


#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
   //UL   flags;
   int  fd;
   int  baud;
} LXUARTCtx;



/***/

extern Bool32 lxUARTOpen (LXUARTCtx *pUC, const char devPath[]);
//lxUARTRead()
//lxUARTWrite()
extern void lxUARTClose (LXUARTCtx *pUC);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LX_UART_H
