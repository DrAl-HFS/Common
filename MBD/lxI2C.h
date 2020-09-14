// Common/MBD/lxI2C.h - I2C bus utils for Linux
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Feb 2018 - Aug 2020

#ifndef LX_I2C_H
#define LX_I2C_H

#include "util.h"
#include <linux/i2c.h>
//#include <linux/i2c-dev.h>

#ifdef __cplusplus
extern "C" {
#endif

// I2C clock cycles for a address bits plus b payload bits
// assumes single ACK following address, thereafter one ACK
// per 8 bits plus final NACK to end transaction.
#define I2C_ADDR_BITS_NCLK(a,b) ((a)+1+(b)+(((b)+7)>>3) + 1)
// I2C clock cycles for byte addressed n byte message
#define I2C_BYTES_NCLK(n) I2C_ADDR_BITS_NCLK(8,(n<<3))

#define I2C_M_WR  (0x0)   // Dummy for code readability

typedef unsigned long UL;

typedef struct
{
   UL   flags;
   int  fd;
   int  clk; // bus clock rate used for transaction timing estimation
} LXI2CBusCtx;

typedef struct
{
   U8    nB, b[3];
   U32   maxIter, maxErr, ivl_us;
} LXI2CPing;



/***/

extern Bool32 lxi2cOpen (LXI2CBusCtx *pBC, const char *path, const int clk);

extern int lxi2cReadRB (const LXI2CBusCtx *pBC, const U8 dev, U8 regBytes[], const U8 nRB);
extern int lxi2cWriteRB (const LXI2CBusCtx *pBC, const U8 dev, const U8 regBytes[], const U8 nRB);

// Multi-message-block transfer extensions for efficiency (?) and convenience
// when numerous register blocks are to be read or written on a given device.
// NB - these support uniform block size only and a single device address only
// within a "batch" of messages. More specific stuff is best achieved by using
// the "i2c-dev" API directly.
// CONSIDER: is there a more general abstraction that yields greater elegance
// without sacrificing efficiency? What about devRegBytes[]={dev,reg,b0..bn} ???
extern int lxi2cReadMultiRB
(
   const LXI2CBusCtx *pBC, // bus info
   const MemBuff *pWS,  // Optional workspace (for linux kernel I2C structs)
   const U8 dev,        // device id
   U8 regBytes[], // array of data blocks
   const U8 nRB,  // bytes per message block (must be uniform)
   const U8 nM    // Number of message blocks
);
extern int lxi2cWriteMultiRB  // ditto
(
   const LXI2CBusCtx *pBC,
   const MemBuff *pWS,
   const U8 dev,
   const U8 regBytes[],
   const U8 nRB,
   const U8 nM
);

// DEPRECATE
extern int lxi2cTrans (const LXI2CBusCtx *pBC, const U16 dev, const U16 f, U16 nB, U8 *pB, U8 reg);

// millisecond sleep for compatibility with "black box" driver code eg. Bosch Sensortech
extern void lxi2cSleepm (U32 ms);

extern void lxi2cDumpDevAddr (const LXI2CBusCtx *pC, U16 dev, U8 bytes, U8 addr);

extern Bool32 lxi2cOpenSMBUS (LXI2CBusCtx *pBC, const char *path, I8 devID);
// f= I2C_SMBUS_READ / I2C_SMBUS_WRITE
extern int lxi2cTransSMBUS (const LXI2CBusCtx *pBC, const U16 f, U16 nB, U8 *pB, U8 reg);

extern void lxi2cClose (LXI2CBusCtx *pC);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LX_I2C_H
