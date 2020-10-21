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
   // CFG class
   UBXM8_ID_PRT=0x00,
   UBXM8_ID_MSG=0x01,
   UBXM8_ID_INF=0x02,
   UBXM8_ID_CFG=0x09,
};

/***/


#ifdef __cplusplus
} // extern "C"
#endif

#endif // UBX_M8_H
