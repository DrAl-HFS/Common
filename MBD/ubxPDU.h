// Common/MBD/ubxPDU.h - u-blox Protocol Data Unit descriptors
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#ifndef UBX_PDU_H
#define UBX_PDU_H

#include "ubxM8.h" // ???
#include "util.h"
#include "mbdUtil.h"


/***/

#ifdef __cplusplus
extern "C" {
#endif


typedef struct { U8 classID[2], lengthLE[2]; } UBXHeader;

typedef struct // beware structure padding on non-PO4/8/16 data: depending on compiler/options
{
   const U8   preamble[2]; // { 0xB5,0x62 }
   UBXHeader header;
} UBXFrameHeader;
// U8 payload[1+]
typedef struct { U8 checksum[2]; } UBXFrameFooter;


#define UBX_PORT_PROTO_UBX     (1<<0)
#define UBX_PORT_PROTO_NMEA    (1<<1)
#define UBX_PORT_PROTO_RTCM2   (1<<2)
#define UBX_PORT_PROTO_RTCM3   (1<<5)

typedef struct
{
   U8 id, rvd1[1], txRdy[2];
   union
   {
      struct { U8 mode[4], baud[4];  } uart;
      struct { U8 rvd2[8]; } usb;
      struct { U8 mode[4], rvd2[4]; } spi;
      struct { U8 mode[4], rvd2[4]; } dds;
   };
   U8 inProtoM[2], outProtoM[2], flags[2], rvd3[2];
} UBXPort;


/***/

#ifndef INLINE
extern int ubxHeaderMatch (const UBXHeader *pH, const U8 c, const U8 id);
#else
INLINE int ubxHeaderMatch (const UBXHeader *pH, const U8 c, const U8 id) { return((pH->classID[0]==c) << 1) | (pH->classID[1] == id); }
#endif // INLINE

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UBX_PDU_H
