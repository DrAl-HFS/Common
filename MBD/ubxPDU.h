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

#define UBX_PKT_MIN (sizeof(UBXFrameHeader) + 1 + sizeof(UBXFrameFooter))


#define UBX_PORT_ID_DDS  0x00
#define UBX_PORT_ID_UART 0x01
// #define UBX_PORT_ID_UART2 0x02 ???
#define UBX_PORT_ID_USB  0x03
#define UBX_PORT_ID_SPI  0x04

#define UBX_PORT_PROTO_UBX     (1<<0)
#define UBX_PORT_PROTO_NMEA    (1<<1)
#define UBX_PORT_PROTO_RTCM2   (1<<2)
#define UBX_PORT_PROTO_RTCM3   (1<<5)

#define UBX_RESET_ID_HW_IMMED    0x00
#define UBX_RESET_ID_SW_FULL     0x01
#define UBX_RESET_ID_SW_GNSS     0x02
#define UBX_RESET_ID_HW_DEFER    0x04
#define UBX_RESET_ID_GNSS_STOP   0x08
#define UBX_RESET_ID_GNSS_START  0x09

typedef struct
{
   U8 id, rvd1[1], txRdy[2];
   union
   {
      struct { U8 mode[4], baud[4];  } uart;
      struct { U8 rvd2[8]; } usb;
      struct { U8 mode[4], rvd2[4]; } spi;
      struct { U8 mode[4], rvd2[4]; } dds; // i2c
   };
   U8 inProtoM[2], outProtoM[2], flags[2], rvd3[2];
} UBXPort;

typedef struct { U8 year[2], month, day, hour, min, sec; } UBXDateTime;
typedef struct
{
   U8 iTOW[4];
   UBXDateTime tUTC;
   U8 valid;
   U8 accuT[4], nsUTC[4];   // ns
   U8 fixType, flags, flags2, nSat;
   U8 lon[4], lat[4]; // 1E-7 degree
   U8 h[4], hMSL[4]; // height above: ellipsoid (geodetic?) &  mean sea level mm
   U8 accuH[4], accuV[4]; // mm
   U8 velNED[3][4]; // North, East Down mm/s
   U8 groundSpeed[4]; // mm/s
   U8 heading[4]; // 1E-5 degree
   U8 accuGS[4], accuHdg[4]; // mm/s , 1E-5 degree
   U8 pdop[2], flags3, rvd1[5];
   U8 headV[4], magDecl[2], accMag[2];
} UBXNavPVT;

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
