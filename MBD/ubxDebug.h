// Common/MBD/ubxDebug.h - u-blox protocol debug support
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#ifndef UBX_DEBUG_H
#define UBX_DEBUG_H

#include "ubxDissect.h"


/***/


#ifdef __cplusplus
extern "C" {
#endif

/***/

/*
extern int ubxClassIDStrTab
(
   const char *r[],
   int maxR,
   char ch[],
   int maxCh,
   const U8 classID[2],
   const U8 *pP,
   const U16 lenP
);
*/
extern void ubxDumpPayloads (const U8 b[], FragBuff16 fb[], int nFB);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // UBX_DEBUG_H
