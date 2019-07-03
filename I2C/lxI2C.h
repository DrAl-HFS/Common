#ifndef LX_I2C_H
#define LX_I2C_H

#include "util.h"
#include <linux/i2c.h>
//#include <linux/i2c-dev.h>

#if 0
typedef int8_t   I8;
typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint32_t Bool32;
#endif

#define I2C_M_WR  (0x0)   // Dummy for code readability

typedef unsigned long UL;

typedef struct
{
   UL   flags;
   int  fd;
} LXI2CBusCtx;


/***/

extern Bool32 lxOpenI2C (LXI2CBusCtx *pBC, const char *path);

extern int lxTransactI2C (const LXI2CBusCtx *pBC, const U16 dev, const U16 f, U16 nB, U8 *pB, U8 reg);

extern void lxSleepm (U32 ms);

extern void lxDumpI2C (const LXI2CBusCtx *pC, U16 dev, U8 bytes, U8 addr);

//extern int lxOpenSMBUS (const char *path, I8 devID);

extern void lxClose (LXI2CBusCtx *pC);

#endif // LX_I2C_H
