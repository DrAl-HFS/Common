#include "ledMatrix.h"

// IS31FL3731

#define ELEMENT_COUNT (2*8*9)
#define FLAG_BYTES (ELEMENT_COUNT>>3)
#define PWM_BYTES ELEMENT_COUNT
typedef struct
{
   U8 addr[1];//={0x00};
   U8 enable[FLAG_BYTES];
   //U8 addr[1];//={0x12};
   U8 blink[FLAG_BYTES];
   //U8 addr[1];//={0x24};
   U8 pwm[PWM_BYTES];
} FramePage;

typedef struct
{
   U8 addr[1];//={0x00};
   U8 reg[13]; // NB: 0x7 read-only
} ControlPage;

#define REG_PAGE_SEL 0xFD
#define CTRL_PAGE 0x0B

void setByMask (U8 r[], const int n, const U8 m[], const U8 v[2])
{
   int i, j;
   for (i= 0; i<n; i++)
   {
      j= (m[i>>3] >> (i & 0x7)) & 0x01; // bit-endianess
      r[i]= v[j];
   }
} // setByMask

int ledMatHack (const LXI2CBusCtx *pC, const U8 busAddr)
{
static const U8 ctrlMask[]={   // byte-interleaved control matrices
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
   U8 pageSel[2]={REG_PAGE_SEL,CTRL_PAGE};

   LOG("ctrlMask[%d]=\n", FLAG_BYTES);
   for (int i=0; i<FLAG_BYTES; i+= 2) LOG("%d : %02X %02X\n", i, ctrlMask[i], ctrlMask[i+1]);
   memset(frames, 0x00, sizeof(frames));
   // All pages start at 0x00
   //for (int i=0; i<8; i++) { frames[i].addr[0]= 0x00; }
   cp.addr[0]= 0x00;
   //memset(cp.reg, 0xFF, sizeof(cp.reg));
   //for (int i=0; i<13; i++) printf("%d : %02X\n", i, cp.reg[i]);
   r= lxi2cReadRB(pC, busAddr, pageSel, sizeof(pageSel));
   if (CTRL_PAGE != pageSel[1])
   {
      printf("got page %x\n", pageSel[1]);
      pageSel[1]= CTRL_PAGE;
      r= lxi2cWriteRB(pC, busAddr, pageSel, sizeof(pageSel));
   }

   r= lxi2cReadRB(pC, busAddr, cp.addr, sizeof(ControlPage));
   LOG("Read Control Page - r=%d\n", r);
   if (r >= 0)
   {
      for (int i=0; i<13; i++) LOG("%d : %02X\n", i, cp.reg[i]);
   }
   pageSel[1]= 0x00; // Frame 0
   r= lxi2cWriteRB(pC, busAddr, pageSel, sizeof(pageSel));
   r= lxi2cReadRB(pC, busAddr, frames[0].addr, sizeof(FramePage));
   LOG("Read Frame Page - r=%d\n", r);
   if (r >= 0)
   {
      for (int i=0; i<FLAG_BYTES; i+= 2) LOG("%d : %02X %02X\n", i, frames[0].enable[i], frames[0].enable[i+1]);
      r= 0;
      memcpy(frames[0].enable, ctrlMask, FLAG_BYTES);
      memset(frames[0].blink, 0x00, FLAG_BYTES);
      memset(frames[0].pwm, 0x00, PWM_BYTES);
      U8 v[2]= {0x00, 0x1F};
      setByMask(frames[0].pwm, 40, ctrlMask, v); // PWM_BYTES
      r= lxi2cWriteRB(pC, busAddr, frames[0].addr, sizeof(FramePage));
      LOG("Write Frame Page - r=%d\n", r);
   }
   if (r > 0) { r= 0; }
   return(r);
} // ledMatHack
