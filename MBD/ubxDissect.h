// Common/MBD/ubxDissect.h - u-blox protocol dissection tools
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#ifndef UBX_DISSECT_H
#define UBX_DISSECT_H

#include "ubxPDU.h" // ???
#include "mbdUtil.h"


/***/


#ifdef __cplusplus
extern "C" {
#endif

/***/


extern void ubxDissectCfgPrt (const UBXPort *pP, const int n);

extern void ubxDissectNavPVT (const UBXNavPVT *pP, const int n);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // UBX_DISSECT_H
