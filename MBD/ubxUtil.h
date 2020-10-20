// Common/MBD/ubxUtil.h - utility code for UBlox M8 GPS module
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#ifndef UBX_UTIL_H
#define UBX_UTIL_H

//include "ubxM8.h"
//#include "util.h"
#include "mbdUtil.h"
#include "lxI2C.h" // temp ? for debug / flexibility

/***/


#ifdef __cplusplus
extern "C" {
#endif

/***/

// Calculate checksum for given message buffer
// Returns 2 (bytes) if calculated, zero otherwise
extern int ubxChecksum (U8 cs[2], const U8 b[], const int n);

// Validate message (requires class, ID, length, payload & checksum)
// optionally setting buffer fragment to payload (if pFB != NULL).
// Returns validated length or zero on failure.
// The preamble (2 sync. chars: 0xB5,0x62) is assumed to have
// been stripped previously.
extern int ubxGetPayload (FragBuff16 *pFB, const U8 b[], const int n);

// Validate multiple messages (using preamble "sync. chars") in buffer, optionally storing
// buffer fragments up to max specified. If fragments can be stored (fb != NULL), scanning
// stops when no further fragments can be stored. Otherwise entire buffer is scanned.
// Returns number of valid messages found.
extern int ubxScanPayloads (FragBuff16 fb[], const int maxFB, const U8 b[], const int n);

//extern int test (const LXI2CBusCtx *pC, const U8 dev[2], const U8 maxIter);
extern int ubloxHack (const LXI2CBusCtx *pC, const U8 busAddr);

//#endif // UBX_UTIL_H

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UBX_UTIL_H
