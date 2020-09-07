// Common/MBD/ads1x.h - Minimal register definitions for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Aug 2020

// HW Ref:  http://www.ti.com/lit/gpn/ads1015
//          https://www.ti.com/lit/gpn/ads1115

#ifndef ADS1X_H
#define ADS1X_H

#ifdef __cplusplus
extern "C" {
#endif

// Transaction clock cycles: (ADR+REG+2*B) * (8b + 1 ack) + 1 stop
// Used along with bus clock rate to determine wait duration
#define ADS1X_TRANS_NCLK (1 + 4 * (8+1))

// Full scale positive readings (ignoring 2's complement sign)
// as obtained from conversion result register
#define ADS10_FSR (((1<<11)-1)<<4) // NB: 12b "left aligned" to 16b
#define ADS11_FSR ((1<<15)-1)

enum ADS1xReg
{  // Register address (in packet header)
   ADS1X_REG_RES=0, // Result
   ADS1X_REG_CFG,   // Config
   ADS1X_REG_CLO,   // Low and
   ADS1X_REG_CHI=3, //  high compare thresholds
}; // ADS1xReg, ADS10Reg, ADS11Reg

/*
* Config register layout and value definitions.
* Rather than performing 16b endian conversion, the bitfields
* are manipulated as 2 distinct bytes - no field crosses a
* byte boundary :)
*/

// TODO : address naming inconsistencies between shifts, masks and values

// Alignment shifts within bytes

enum ADS1xShift0
{  // Shifts (bit number) within byte 0 (Big-Endian 16b MSB)
   ADS1X_SH0_OS=   7, // 1b see ADS1xFlag0 below
   ADS1X_SH0_MUX=  4, // 3b see ADS1xMux below
   ADS1X_SH0_GAIN=  1, // 3b see ADS1xGain below
   ADS1X_SH0_MODE= 0 // 1b see ADS1xFlag0 below
}; // ADS1xShift0

enum ADS1xShift1
{  // Shifts (bit number) within byte 1 (Big-Endian 16b LSB)
   ADS1X_SH1_DR= 5, // 3b (Data Rate) see ADS1*Rate below
   ADS1X_SH1_CM= 4, // 1b see ADS1xFlag1 below
   ADS1X_SH1_CP= 3, // "
   ADS1X_SH1_CL= 2, // "
   ADS1X_SH1_CQ= 0  // 2b see ADS1xCompare below
}; // ADS1xShift1

// Mask values for flags
enum ADS1xFlag0
{
   ADS1X_FL0_OS=   1<<ADS1X_SH0_OS,  // Operational Status (busy) flag
   ADS1X_FL0_MODE= 1<<ADS1X_SH0_MODE // Mode flag (continuous / single shot)
}; // ADS1xFlag0

enum ADS1xFlag1
{
   ADS1X_FL1_CM= 1<<ADS1X_SH1_CM, // Compare Mode (window vs. simple)
   ADS1X_FL1_CP= 1<<ADS1X_SH1_CP, //  " (alert signal) Polarity
   ADS1X_FL1_CL= 1<<ADS1X_SH1_CL  //  " (alert signal) Latch
}; // ADS1xFlag1

// Ordinal values within bit fields (mostly generic across device family)

enum ADS1xMux
{  // Input multiplexing, 4 double + 4 single ended modes
   ADS1X_MUX01=0, // default
   ADS1X_MUX03,  // Double ended (differential)
   ADS1X_MUX13,
   ADS1X_MUX23,
   ADS1X_MUX0G,  // Single ended (Ground relative)
   ADS1X_MUX1G,
   ADS1X_MUX2G,
   ADS1X_MUX3G=7,
   ADS1X_MUX_M= 0x7   // mask
}; // ADS1xMux, ADS10Mux, ADS11Mux

enum ADS1xGain
{  // Gain specified as Volts Full Scale (based on internal reference, independant of supply ?)
   ADS1X_GAIN_6V144=0,
   ADS1X_GAIN_4V096,
   ADS1X_GAIN_2V048, // default= 2.048V
   ADS1X_GAIN_1V024,
   ADS1X_GAIN_0V512,
   ADS1X_GAIN_0V256=5,
   //ADS1X_GFS_0V256=6,7
   ADS1X_GAIN_M= 0x7   // mask
}; // ADS1xGain, ADS10Gain, ADS11Gain

enum ADS1xCompare // see also ADS1xFlag1 above
{  // Compare signal assertion modes
   ADS1X_CMP_1=0, // 1 conversion
   ADS1X_CMP_2,
   ADS1X_CMP_4,
   ADS1X_CMP_DISABLE=3,  // default
   ADS1X_CMP_M= 0x3   // mask
}; // ADS1xCompare, ADS10Compare, ADS11Compare

// NB: 12b & 16b precision sub-families have distict sample rates
enum ADS1xRate
{  // Data (sample) rates (samples per second)
   ADS10_DR128=0,
   ADS10_DR250,
   ADS10_DR490,
   ADS10_DR920,
   ADS10_DR1600, // default
   ADS10_DR2400,
   ADS10_DR3300=6,
   //ADS10_DR3300=7

   ADS11_DR8=0,
   ADS11_DR16,
   ADS11_DR32,
   ADS11_DR64,
   ADS11_DR128, // default
   ADS11_DR250,
   ADS11_DR475,
   ADS11_DR860=7,

   ADS1X_DR_M= 0x7   // mask
}; // ADS1xRate
/*
enum ADS10Rate
{  // Data (sample) rates (samples per second)
   ADS10_DR128=0,
   ADS10_DR250,
   ADS10_DR490,
   ADS10_DR920,
   ADS10_DR1600, // default
   ADS10_DR2400,
   ADS10_DR3300=6,
   ADS10_DR_M= 0x7   // mask
}; // ADS10Rate

enum ADS11Rate
{  // Data (sample) rates (samples per second)
   ADS11_DR8=0,
   ADS11_DR16,
   ADS11_DR32,
   ADS11_DR64,
   ADS11_DR128, // default
   ADS11_DR250,
   ADS11_DR475,
   ADS11_DR860=7,
   ADS11_DR_M= 0x7   // mask
}; // ADS11Rate
*/

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ADS1X_H
