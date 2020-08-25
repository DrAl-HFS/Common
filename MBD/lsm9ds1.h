// Common/MBD/lsm9ds1.h - Definitions for ST 9dof IMU device
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug 2020

// HW Ref:  http://?.?.?/? lsm9ds1.pdf

#ifndef LSM9DS1_H
#define LSM9DS1_H

#ifdef __cplusplus
extern "C" {
#endif

// Register addresses (required in packet headers)
enum LSMAccM16RegID
{  // 16bit accelerometer measures
   LSM_REG_TEMP=0x15, // 2 bytes contiguous
   // angular 3*2 bytes contiguous
   LSM_REG_ANG_X=0x18,
   LSM_REG_ANG_Y=0x1A,
   LSM_REG_ANG_Z=0x1C,
   // linear 3*2 bytes contiguous
   LSM_REG_LIN_X=0x28,
   LSM_REG_LIN_Y=0x2A,
   LSM_REG_LIN_Z=0x2C,
}; // LSMRegAccM16

enum LSMAccCtrlRegID // Control registers (8b)
{
   LSM_REG_ACTRL_ACT1=0x04,
   LSM_REG_ACTRL_ACT9=0x0D,

   LSM_REG_ACTRL_ANG1=0x10,
   LSM_REG_ACTRL_ANG2=0x11,
   LSM_REG_ACTRL_ANG3=0x12,

   LSM_REG_ACTRL04=0x1E, // gyro output, interrupt modes

   LSM_REG_ACTRL_LIN5=0x1F, // output
   //...LSM_REG_ACTRL_LIN6=0x20, // data rate / power mode
   LSM_REG_ACTRL_LIN7=0x21,  // resolution / filter

   LSM_REG_ACTRL08=0x22, // system
   //...LSM_REG_ACTRL09=0x23, // gyro sleep, fifo/interface selection
   LSM_REG_ACTRL10=0x24,  // self test flags

   LSM_REG_ACTRL_FIFO1=0x2E, // fifo control
   LSM_REG_ACTRL_FIFO2=0x2F,

   LSM_REG_ACTRL_INTR1=0x30, // interrupt control
   //...
   LSM_REG_ACTRL_INTR8=0x37,
}; // LSMAccCtrlRegID

enum LSMMagV16RegID
{  // 16bit magnetometer values
   LSM_REG_MAG_OFFSX=0x05, // environmental calibration offsets
   LSM_REG_MAG_OFFSY=0x07,
   LSM_REG_MAG_OFFSZ=0x09,
   // measurements
   LSM_REG_MAG_X=0x28,
   LSM_REG_MAG_Y=0x2A,
   LSM_REG_MAG_Z=0x2C
}; // LSMMagV16RegID

enum LSMMagCtrlRegID // Control registers (8b)
{
   LSM_REG_MCTRL01=0x20, // system
   //...LSM_REG_MCTRL02=0x21, //
   //...LSM_REG_MCTRL03=0x22, //
   //...LSM_REG_MCTRL04=0x23, //
   LSM_REG_MCTRL05=0x24, //

   LSM_REG_MSTAT=0x27, // contiguous with 16b measurement

   LSM_REG_MCTRL_INTR1=0x30,
   LSM_REG_MCTRL_INTR2=0x31
}; // LSMAccCtrlRegID

// Displace structures up to next level of abstraction ?
typedef struct // I2C packet/frames for 16b acc. & temperature measurement
{                // X,Y,Z register tuple can be read as single block
   U8 temp[3]; // 0x15:16 - temperature in +-1/16ths Celcius offset from 25C
   U8 ang[7]; // X,Y,Z -> 0x18:19, 1A:1B, 1C:1D
   U8 lin[7]; // X,Y,Z -> 0x28:29, 2A:2B, 2C:2D
} LSMAccValI16RegFrames;

typedef struct // I2C packet/frames for acc. control registers
{
   U8 actInt[10]; // 0x04..0D activity/interrupt control
   U8 ang[4]; // 0x10..12
   U8 lin[4];  // 0x1F..21
   U8 r8_10[4]; // 0x22..24
   U8 r4[2];   // 0x1E
   U8 fifo[3]; // 0x2E,2F
   U8 intr[9]; // 0x30..37
} LSMAccCtrlRegFrames;

typedef struct // I2C packet/frames for 16b mag. calibration/measurement
{
   U8 offs[7]; // 0x05..
   //U8 statMag[8]; // 0x27combined status byte & mag output?
   U8 mag[7]; // 0x28..
} LSMMagValI16RegFrames;

typedef struct // I2C packet/frames for mag. control registers
{
   U8 r1_5[6]; // 0x20...24
   U8 stat[2]; // 0x27
   U8 intr[4]; // 0x30
} LSMMagCtrlRegFrames;


#ifdef __cplusplus
} // extern "C"
#endif

#endif // LSM9DS1_H
