// Common/MBD/ads1x.h - Definitions (& utils?) for TI I2C ADC devices (ADS1xxx series)
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

// Alignment shifts within bytes

enum ADS1xShift0
{  // Shifts within byte 0 (Big-Endian 16b MSB)
   ADS1X_SH0_OS=   7, // 1b see ADS1xFlag0 below
   ADS1X_SH0_MUX=  4, // 3b see ADS1xMux below
   ADS1X_SH0_PGA=  1, // 3b see ADS1xGain below
   ADS1X_SH0_MODE= 0 // 1b see ADS1xFlag0 below
}; // ADS1xShift0

enum ADS1xShift1
{  // Shifts within byte 1 (Big-Endian 16b LSB)
   ADS1X_SH1_DR= 5, // 3b (Data Rate) see ADS10SampleRate below
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
   ADS1X_M01=0, // default
   ADS1X_M03,  // Double ended (differential)
   ADS1X_M13,
   ADS1X_M23,
   ADS1X_M0G,  // Single ended (Ground relative)
   ADS1X_M1G,
   ADS1X_M2G,
   ADS1X_M3G=7,
   ADS1X_M_M= 0x7   // mask
}; // ADS1xMux, ADS10Mux, ADS11Mux

enum ADS1xGain
{  // Gain specified as Volts Full Scale (based on internal reference, independant of supply ?)
   ADS1X_GFS_6V144=0,
   ADS1X_GFS_4V096,
   ADS1X_GFS_2V048, // default= 2.048V
   ADS1X_GFS_1V024,
   ADS1X_GFS_0V512,
   ADS1X_GFS_0V256=5,
   //ADS1X_GFS_0V256=6,7
   ADS1X_GFS_M= 0x7   // mask
}; // ADS1xGain, ADS10Gain, ADS11Gain

enum ADS1xCompare
{  // Compare signal assertion modes
   ADS1X_CMP_1=0, // 1 conversion
   ADS1X_CMP_2,
   ADS1X_CMP_4,
   ADS1X_CMP_DISABLE=3,  // default
   ADS1X_CMP_M= 0x3   // mask
}; // ADS1xCompare, ADS10Compare, ADS11Compare

// NB: 12b & 16b precision sub-families have distict sample rates

enum ADS10Rate
{  // Sample rates (samples per second)
   ADS10_R128=0,
   ADS10_R250,
   ADS10_R490,
   ADS10_R920,
   ADS10_R1600, // default
   ADS10_R2400,
   ADS10_R3300=6,
   //,ADS10_S3300=7
   ADS10_R_M= 0x7   // mask
}; // ADS10Rate

enum ADS11Rate
{  // Sample rates (samples per second)
   ADS11_R8=0,
   ADS11_R16,
   ADS11_R32,
   ADS11_R64,
   ADS11_R128, // default
   ADS11_R250,
   ADS11_R475,
   ADS11_R860=7,
   ADS11_R_M= 0x7   // mask
}; // ADS11Rate

// Map (endian converted) ADC reading to Volts
//extern float ads1xRVF (int r, enum ADS1xGain g, float fsrs); // fsrs= 1.0/ADS10_FSR or 1.0/ADS11_FSR
//static const float gainV[]={6.144, 4.096, 2.048, 1.024, 0.512, 0.256};
//return(r * gainV[g] * fsrs);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ADS1X_H
