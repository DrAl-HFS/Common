#include "ledMatrix.h"

// IS31FL3731

typedef struct
{
   U8 addr[1];//={0x00};
   U8 enable[18];
   //U8 addr[1];//={0x12};
   U8 blink[18];
   //U8 addr[1];//={0x24};
   U8 pwm[8*18];
} FramePage;

typedef struct
{
   U8 addr[1];//={0x00};
   U8 reg[13]; // NB: 0x7 read-only
} ControlPage;

#define CTRL_PAGE 0x0B

int ledMatHack (const LXI2CBusCtx *pC)
{
static const U8 b[]={   // byte-interleaved control matrices
0b00000000, 0b10111111, 0b00111110, 0b00111110,
0b00111111, 0b10111110, 0b00000111, 0b10000110,
0b00110000, 0b00110000, 0b00111111, 0b10111110,
0b00111111, 0b10111110, 0b01111111, 0b11111110,
0b01111111, 0b00000000};
/* Patterns ????
0000000000        1 0 111111 00
11111    000      11111 0
111111   00000    1 0 11111 0
111      00       1 0000 11 0
11       000000   00 11 0000
111111   00       1 0 11111 0
111111   0        1 0 11111 0
1111111  0        1111111 0,
1111111           00000000
*/
   //LOG_CALL("() - %d bytes, %d bits set\n", sizeof(b), bitCountNU8(b, sizeof(b)));
   int r;
   FramePage   frames[8];
   ControlPage cp;
   U8 pageSel[2]={0xFD,};

   cp.addr[0]= 0x00;
   //memset(cp.reg, 0xFF, sizeof(cp.reg));
   //for (int i=0; i<13; i++) printf("%d : %02X\n", i, cp.reg[i]);
   r= lxi2cReadRB(pC, 0x75, pageSel, sizeof(pageSel));
   if (CTRL_PAGE != pageSel[1])
   {
      printf("got page %x\n", pageSel[1]);
      pageSel[1]= CTRL_PAGE;
      r= lxi2cWriteRB(pC, 0x75, pageSel, sizeof(pageSel));
   }

   r= lxi2cReadRB(pC, 0x75, cp.addr, sizeof(ControlPage));
   LOG("Read Control Page - r=%d\n", r);
   if (r >= 0)
   {
      for (int i=0; i<13; i++) printf("%d : %02X\n", i, cp.reg[i]);
   }
   pageSel[1]= 0x00; // Frame 0
   r= lxi2cWriteRB(pC, 0x75, pageSel, sizeof(pageSel));
   r= lxi2cReadRB(pC, 0x75, frames[0].addr, sizeof(FramePage));
   LOG("Read Frame Page - r=%d\n", r);
   if (r >= 0)
   {
      for (int i=0; i<18; i++) printf("%d : %02X\n", i, frames[0].enable[i]);
      r= 0;
   }

   return(r);
} // ledMatHack
