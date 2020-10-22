// Common/MBD/lxI2C.h - I2C bus utils for Linux
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Feb 2018 - Sept 2020

#ifndef LX_I2C_H
#define LX_I2C_H

#include "util.h" // necessary ?
#include <linux/i2c.h>
//#include <linux/i2c-dev.h>

/***/

#ifdef __cplusplus
extern "C" {
#endif

// I2C clock cycles for a address bits plus b payload bits
// assumes single START preceding and ACK following address,
// thereafter one ACK per 8 payload bits (or part thereof
// when padding is required) plus final NACK to end transaction.
#define I2C_ADDR_BITS_NCLK(a,b) ((a)+3+9*(((b)+7)>>3))
// I2C clock cycles for byte addressed n byte message
#define I2C_BYTES_NCLK(n) I2C_ADDR_BITS_NCLK(8,(n<<3))

#define I2C_M_WR  (0x0)   // Dummy for code readability

typedef unsigned long UL; // For compatibility with kernel I2C flag def.

// CONSIDER:
typedef struct
{
   int clock;
   //UL flags; ???
} PortI2C;

// Bus context
typedef struct
{
   UL   flags;
   int  fd;
   int  clk; // bus clock rate used for transaction timing estimation
} LXI2CBusCtx;

// Flags
#define I2C_PING_VERBOSE (1<<0)

// "Ping" descriptor
typedef struct
{
   U8    nB, b[7];	// "packet" bytes: count, content
   U32   maxIter, maxErr; // packets to send, error/failure limit
   long  ivlNanoSec;	// interval (nanoseconds)
} LXI2CPing;


/***/

extern Bool32 lxi2cOpen (LXI2CBusCtx *pBC, const char devPath[], const int clk);

// Simple read without write (for stream interface)
extern int lxi2cRead (const LXI2CBusCtx *pBC, const U8 busAddr, U8 b[], const U8 nB);
// extern int lxi2cWrite (const LXI2CBusCtx *pBC, const U8 busAddr, const U8 b[], const U8 nB);

// Register interface
extern int lxi2cReadRB (const LXI2CBusCtx *pBC, const U8 busAddr, U8 regBytes[], const U8 nRB);
extern int lxi2cWriteRB (const LXI2CBusCtx *pBC, const U8 busAddr, const U8 regBytes[], const U8 nRB);

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
   const U8 busAddr,    // device id
   U8 regBytes[], // array of data blocks
   const U8 nRB,  // bytes per message block (must be uniform)
   const U8 nM    // Number of message blocks
);
extern int lxi2cWriteMultiRB  // ditto
(
   const LXI2CBusCtx *pBC,
   const MemBuff *pWS,
   const U8 busAddr,
   const U8 regBytes[],
   const U8 nRB,
   const U8 nM
);

// DEPRECATE : first attempt at flexible read/write wrapper - inefficient for write due to (apparently
// undocumented) single contiguous buffer requirement for writing. (Whereas reading is happily split into
// write+read portions to match the common mode of device operation.)
extern int lxi2cTrans (const LXI2CBusCtx *pBC, const U16 busAddr, const U16 f, U16 nB, U8 *pB, U8 reg);

extern Bool32 lxi2cOpenSMBUS (LXI2CBusCtx *pBC, const char *path, I8 devID);
// f= I2C_SMBUS_READ / I2C_SMBUS_WRITE
extern int lxi2cTransSMBUS (const LXI2CBusCtx *pBC, const U16 f, U16 nB, U8 *pB, U8 reg);

extern void lxi2cClose (LXI2CBusCtx *pC);

/***/

// millisecond sleep for compatibility with "black box" driver code eg. Bosch Sensortech
extern void lxi2cSleepm (U32 milliSec);

// Dump device registers
extern int lxi2cDumpDevAddr (const LXI2CBusCtx *pC, U8 busAddr, U8 bytes, U8 addr);

// Write packets to bus for debug purposes
extern int lxi2cPing (const LXI2CBusCtx *pC, U8 busAddr, const LXI2CPing *pP, U8 modeFlags);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LX_I2C_H
