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
   int   baud;
   short tbiv; // termios baud index (mask) value
   char  mode[6];
} PortUART;

typedef struct
{
   int  fd;
   PortUART port;
} LXUARTCtx;



/***/

extern Bool32 lxUARTOpen (LXUARTCtx *pUC, const char devPath[]);
//lxUARTRead() -> read(fd...
//lxUARTWrite() -> write(fd...
extern void lxUARTClose (LXUARTCtx *pUC);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LX_UART_H
