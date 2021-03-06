// Common/MBD/ad9833.h - basic cross-platform definitions for
// Analog Devices AD9833 signal generator with SPI/3-wire compatible interface.
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3A
// (c) Project Contributors Oct 2020 - Jan 2021

// HW Ref:  http://www.analog.com/media/en/technical-documentation/data-sheets/AD9833.pdf

#ifndef AD9833_H
#define AD9833_H

//#include <>


/***/

#ifdef __cplusplus
extern "C" {
#endif

enum AD9833RegID
{  // Register address : upper 2 or 3 bits of 16b word
   AD9833_REG_CTRL=  0b00,
   AD9833_REG_FREQ0= 0b01,
   AD9833_REG_FREQ1= 0b10,
   AD9833_REG_PHASE0= 0b110,
   AD9833_REG_PHASE1= 0b111,
};

// Byte oriented control register interface seems simplest
// for AVR target, despite 16b endianess issue (ad9833 hardware
// registers must be updated using 16b msb-first (BE) transfers
// only). Cryptic flag names assigned by manufacturer are replaced
// with better (native language bias!) mnemonics here.
enum AD9833Shift0
{
   AD9833_SH0_TRI=1,     // "MODE" triangle waveform +0.6V pp
   AD9833_SH0_FCLK=3,    // "DIV2" full rate (matching sin/tri freq.) clock output +Vcc (datasheet incorrect)
   AD9833_SH0_OCLK=5,    // "OPBITEN" half rate clock waveform output +Vcc
   AD9833_SH0_SLP_DAC=6, // "SLEEP2" DAC disabled (clock output only)
   AD9833_SH0_SLP_MCLK=7 // "SLEEP1" MCLK disabled (DAC output held)
};
enum AD9833Shift1
{
   AD9833_SH1_RST=0,    // Reset
   AD9833_SH1_PSEL=2,   // Phase register output select
   AD9833_SH1_FSEL=3,   // Frequency register output select
   AD9833_SH1_HLB=4,    // Hi!Lo frequency register write select
   AD9833_SH1_B28=5     // Full frequency register pair write mode select
};
// Mask values for flags
enum AD9833Flag0
{
   AD9833_FL0_TRI=  1<<AD9833_SH0_TRI,
   AD9833_FL0_FCLK= 1<<AD9833_SH0_FCLK,
   AD9833_FL0_OCLK= 1<<AD9833_SH0_OCLK,
   AD9833_FL0_SLP_DAC=  1<<AD9833_SH0_SLP_DAC,
   AD9833_FL0_SLP_MCLK= 1<<AD9833_SH0_SLP_MCLK
};
enum AD9833Flag1
{
   AD9833_FL1_RST=  1<<AD9833_SH1_RST,
   AD9833_FL1_PSEL= 1<<AD9833_SH1_PSEL,
   AD9833_FL1_FSEL= 1<<AD9833_SH1_FSEL,
   AD9833_FL1_HLB=  1<<AD9833_SH1_HLB,
   AD9833_FL1_B28=  1<<AD9833_SH1_B28
};

// 16b mask values for Frequency scale, Phase offset
#define AD9833_FSR_MASK ((1<<14)-1)
#define AD9833_FSR_MASK_8H 0x3F // ((1<<6)-1) most significant 8bits
#define AD9833_PSR_MASK ((1<<13)-1)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // AD9833_H
