// Common/MBD/ads1x.h - Definitions & utils for TI I2C ADC devices (ADS1xxx series)
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
   ADS1X_RR=0, // Result
   ADS1X_RC,   // Config
   ADS1X_RL,   // Low and
   ADS1X_RH=3, //  high compare thresholds
}; // ADS1xReg, ADS10Reg, ADS11Reg

// Config register bit field definitions
enum ADS1xMux
{  // Input multiplexing, 4 double + 4 single ended modes
   ADS1X_M01=0, // default
   ADS1X_M03,  // Double ended (differential)
   ADS1X_M13,
   ADS1X_M23,
   ADS1X_M0G,  // Single ended (Ground relative)
   ADS1X_M1G,
   ADS1X_M2G,
   ADS1X_M3G=7
}; // ADS1xMux, ADS10Mux, ADS11Mux

enum ADS1xGain
{  // Gain setting in terms of Volts Full Scale (assumes 5V supply ?)
   ADS1X_G6_144=0,
   ADS1X_G4_096,
   ADS1X_G2_048, // default= 2.048V
   ADS1X_G1_024,
   ADS1X_G0_512,
   ADS1X_G0_256=5
   //ADS1X_G0_256=6,7
}; // ADS1xGain, ADS10Gain, ADS11Gain

enum ADS10SampleRate
{  // Sample rates (samples per second)
   ADS10_S128=0,
   ADS10_S250,
   ADS10_S490,
   ADS10_S920,
   ADS10_S1600, // default
   ADS10_S2400,
   ADS10_S3300=6
   //,ADS10_S3300=7
}; // ADS10SampleRate

enum ADS11SampleRate
{  // Sample rates (samples per second)
   ADS11_S8=0,
   ADS11_S16,
   ADS11_S32,
   ADS11_S64,
   ADS11_S128, // default
   ADS11_S250,
   ADS11_S475,
   ADS11_S860=7
}; // ADS11SampleRate

enum ADS1xCompare
{  // Compare signal assertion modes
   ADS1X_C1=0, // 1 conversion
   ADS1X_C2,
   ADS1X_C4,
   ADS1X_CD=3  // default = disable
}; // ADS1xCompare, ADS10Compare, ADS11Compare


#ifdef __cplusplus
} // extern "C"
#endif

#endif // ADS1X_H
