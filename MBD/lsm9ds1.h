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
   LSM_REG_CTRL_ANG1=0x10,
   LSM_REG_CTRL_ANG2=0x11,
   LSM_REG_CTRL_ANG3=0x12,

   LSM_REG_CTRL04=0x1E, // gyro output, interrupt modes

   LSM_REG_CTRL_LIN5=0x1F, // output
   LSM_REG_CTRL_LIN6=0x20, // data rate / power mode
   LSM_REG_CTRL_LIN7=0x21,  // resolution / filter

   LSM_REG_CTRL08=0x22, // system
   LSM_REG_CTRL09=0x23, // gyro sleep, fifo/interface selection
   LSM_REG_CTRL10=0x24  // self test flags
};

enum LSMMagV16RegID
{  // 16bit magnetometer values
   LSM_REG_MAG_OFFSX=0x05, // environmental calibration offsets
   LSM_REG_MAG_OFFSY=0x07,
   LSM_REG_MAG_OFFSZ=0x09,
   // measurements
   LSM_REG_MAG_X=0x28,
   LSM_REG_MAG_Y=0x2A,
   LSM_REG_MAG_Z=0x2C
};

// Displace structures up to next level of abstraction ?
typedef struct // I2C packet/frames for 16b measures
{                // X,Y,Z register tuple can be read as single block
   U8 temp[3]; // 0x15:16 - temperature in +-1/16ths Celcius offset from 25C
   U8 ang[7]; // X,Y,Z -> 0x18:19, 1A:1B, 1C:1D
   U8 lin[7]; // X,Y,Z -> 0x28:29, 2A:2B, 2C:2D
} LSMAccValI16RegFrames;

typedef struct
{
   U8 ang[4]; // 0x10..12
   U8 lin[4];  // 0x1F..21
   U8 r8_10[4]; // 0x22...
   U8 r4[2]; // 0x1E
} LSMAccCtrlRegFrames;

typedef struct
{
   U8 offs[7];
   U8 mag[7];
} LSMMagValI16RegFrames;

typedef struct
{
   U8 r1_5[6]; // 0x20...
   U8 r4[2]; // 0x1E
} LSMMagCtrlRegFrames;


#ifdef __cplusplus
} // extern "C"
#endif

#endif // LSM9DS1_H
