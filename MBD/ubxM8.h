// Common/MBD/ubxM8.h - definitions for u-blox M8 GPS module
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#ifndef UBX_M8_H
#define UBX_M8_H

//#include "mbdUtil.h"


/***/

#ifdef __cplusplus
extern "C" {
#endif

// Data stream invalid byte
#define UBXM8_DSB_INVALID 0xFF

// DDS (I2C) interface
enum UBXM8Reg // : U8
{  // Only three registers: all read only
   UBXM8_RG_NBYTE_HI=0xFD,
   UBXM8_RG_NBYTE_LO, //=0xFE
   UBXM8_RG_DATASTREAM=0xFF
};

// Message class ID
enum UBXM8Class
{
   UBXM8_CL_NAV=0x01,
   UBXM8_CL_RXM=0x02,
   UBXM8_CL_INF=0x04,
   UBXM8_CL_ACK=0x05,
   UBXM8_CL_CFG=0x06,
   UBXM8_CL_UPD=0x09,
   UBXM8_CL_MON=0x0A,
   UBXM8_CL_AID=0x0B,
   UBXM8_CL_TIM=0x0D,
   UBXM8_CL_ESF=0x10,
   UBXM8_CL_MGA=0x13,
   UBXM8_CL_LOG=0x21,
   UBXM8_CL_SEC=0x27,
   UBXM8_CL_HNR=0x28,
   UBXM8_CL_NMEA=0xF0,  // NMEA config over UBX protocol
   UBXM8_CL_PUBX=0xF1   // u-blox NMEA extensions
};

// Message endpoint/function ID
enum UBXM8ID
{  // ACK class
   UBXM8_ID_NACK=0x00,
   UBXM8_ID_ACK=0x01,
   // AID
   UBXM8_ID_INI=0x01,   // !!!
   UBXM8_ID_HUI=0x02,   // !!!
   UBXM8_ID_ALM=0x30,
   UBXM8_ID_EPH=0x31,
   UBXM8_ID_AOP=0x33,
   // CFG class
   UBXM8_ID_PRT=0x00,   // !!!
   UBXM8_ID_MSG=0x01,   // !!!
   UBXM8_ID_INF=0x02,   // !!!
   UBXM8_ID_RST=0x04,
   UBXM8_ID_DAT=0x06,
   UBXM8_ID_RATE=0x08,
   UBXM8_ID_CFG=0x09,
   UBXM8_ID_RXM=0x11,
   UBXM8_ID_ANT=0x13,
   UBXM8_ID_ODO=0x1E,
   UBXM8_ID_SBAS=0x16,
   UBXM8_ID_NMEA=0x17,
   UBXM8_ID_USB=0x1B,
   UBXM8_ID_NAVX5=0x23,
   UBXM8_ID_NAV5=0x24,
   UBXM8_ID_TP5=0x31,   // !!!
   UBXM8_ID_RINV=0x34,
   UBXM8_ID_ITFM=0x39,
   UBXM8_ID_TMODE2=0x3D,
   UBXM8_ID_PWR=0x57,
   UBXM8_ID_PM2=0x3B,
   UBXM8_ID_GNSS=0x3E,
   UBXM8_ID_LOGFILTER=0x47,
   UBXM8_ID_TXSLOT=0x53,
   UBXM8_ID_HNR=0x5C,
   UBXM8_ID_ESRC=0x60,
   UBXM8_ID_DOSC=0x61,
   UBXM8_ID_SMGR=0x62,
   UBXM8_ID_GEOFENCE=0x69,
   UBXM8_ID_DGNSS=0x70,
   UBXM8_ID_TMODE3=0x71,
   UBXM8_ID_PMS=0x86,
   UBXM8_ID_SLAS=0x8D,
   UBXM8_ID_BATCH=0x93,
   // ESF HNR ignored
   // INF
   UBXM8_ID_ERROR=0x00, // !!!
   UBXM8_ID_WARNING=0x01, // !!!
   UBXM8_ID_NOTICE=0x02, // !!!
   UBXM8_ID_TEST=0x03, // !!!
   UBXM8_ID_DEBUG=0x04, // !!!
   // LOG MGA MON ???
   // NAV
   UBXM8_ID_PVT=0x07 // Full? navigation solution
};

/***/


#ifdef __cplusplus
} // extern "C"
#endif

#endif // UBX_M8_H
