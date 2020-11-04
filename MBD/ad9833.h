// Common/MBD/ad9833.h - basic cross-platform definitions for
// Analog Devices AD9833 signal generator with SPI/3-wire compatible interface.
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors October-November 2020

// HW Ref:  http://www.analog.com/media/en/technical-documentation/data-sheets/AD9833.pdf

#ifndef AD9833_H
#define AD9833_H

//#include <>


/***/

#ifdef __cplusplus
extern "C" {
#endif

enum AD9833RegAddr
{  // Register address : upper 2 or 3 bits of 16b word
   AD9833_REG_CTRL=  0b00,
   AD9833_REG_FREQ0= 0b01,
   AD9833_REG_FREQ1= 0b10,
   AD9833_REG_PHASE0= 0b110,
   AD9833_REG_PHASE1= 0b111,
};

// Byte oriented control register interface
enum AD9833Shift0
{
   AD9833_SH0_TRI=1,
   AD9833_SH0_DCLK=3,
   AD9833_SH0_CLK=5,
   AD9833_SH0_SLP1=6,
   AD9833_SH0_SLP2=7
};
enum AD9833Shift1
{
   AD9833_SH1_RST=0,
   AD9833_SH1_PSEL=2,
   AD9833_SH1_FSEL=3,
   AD9833_SH1_HLB=4,
   AD9833_SH1_B28=5
};
// Mask values for flags
enum AD9833Flag0
{
   AD9833_FL0_TRI=  1<<AD9833_SH0_TRI,
   AD9833_FL0_DCLK= 1<<AD9833_SH0_DCLK,
   AD9833_FL0_CLK=  1<<AD9833_SH0_CLK,
   AD9833_FL0_SLP1= 1<<AD9833_SH0_SLP1,
   AD9833_FL0_SLP2= 1<<AD9833_SH0_SLP2
};
enum AD9833Flag1
{
   AD9833_FL1_RST=  1<<AD9833_SH1_RST,
   AD9833_FL1_PSEL= 1<<AD9833_SH1_PSEL,
   AD9833_FL1_FSEL= 1<<AD9833_SH1_FSEL,
   AD9833_FL1_HLB=  1<<AD9833_SH1_HLB,
   AD9833_FL1_B28=  1<<AD9833_SH1_B28
};

// DEPRECATE : prefer 8bit control register access
enum AD9833Ctrl
{
   AD_F_B28=   1<<13,   // 28bit (vs. 14bit) frequency data load
   AD_F_HLB=   1<<12,   // Hi vs. Lo 14bit data load when B28 clear
   AD_F_FSEL=  1<<11,   // Frequency register select
   AD_F_PSEL=  1<<10,   // Phase register select
   // bit9 reserved
   AD_F_RST=   1<<8,    // "reset" = zero output, does not clean registers.
   AD_F_SLP1=  1<<7,    // MCLK disable => constant DAC output
   AD_F_SLP2=  1<<6,    // DAC disable - 0V output ?
   AD_F_SQR=   1<<5,    // DAC bypass, square wave from acc msb or ...
   // bit4 reserved
   AD_F_SQRD=  1<<3,    // ... acc msb-1 output by DAC bypass (double rate)
   // bit 2 reserved
   AD_F_TRI=   1<<1     // Triangle wave output, versus sine
   // bit 0 reserved
};

// Frequency scale 16b mask
#define AD_FS_MASK ((1<<14)-1)
#define AD_PS_MASK ((1<<13)-1)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // AD9833_H
