// Common/MBD/ledMatrix.h - Hacks for IS31FL3731 LEDshim
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct. 2020

#include "ledMatrix.h"
#include "lumissil.h"
#include "ledMapRGB.h"


/***/

// Logical to perceptual mapping for [0,1] interval
F32 gammaNorm (F32 x)
{
   if (x < 0.995) { return(1.00800 * powf(x,1.69500)); }
   //else
   return(1);
} // gammaNorm

void dumpReg (const ControlPage *pCP)
{
   U8 reg; // temp for brevity

   reg= pCP->reg[LMSL_REG_CONFIG];
   LOG("CF: M=%d FS=%d\n", (reg >> 3) & 0x3, reg & 0x7);
   LOG("PD: PFS=%d\n", pCP->reg[LMSL_REG_PICTURE] & 0x7);
   reg= pCP->reg[LMSL_REG_AUTOPLAY1];
   LOG("A1: CNS=%d FNS=%d\n", (reg >> 4) & 0x7, reg & 0x7);
   reg= pCP->reg[LMSL_REG_AUTOPLAY2];
   LOG("A2: D=%d\n", reg & 0x3F);
   reg= pCP->reg[LMSL_REG_DISPOPT];
   LOG("DO: IC=%d B=%d BP=%d\n", (reg >> 5) & 0x1, (reg >> 3) & 0x1, reg & 0x7);
   LOG("AS: AE=%d\n", pCP->reg[LMSL_REG_AUDSYNC] & 0x1);
   reg= pCP->reg[LMSL_REG_FRAMESTAT];
   LOG("FS: IN=%d CF=%d\n", (reg >> 4) & 0x1, reg & 0x7);
   reg= pCP->reg[LMSL_REG_BREATHE1];
   LOG("B1: FO=%d FI=%d\n", (reg >> 4) & 0x7, reg & 0x7);
   reg= pCP->reg[LMSL_REG_BREATHE2];
   LOG("B2: EN=%d EX=%d\n", (reg >> 4) & 0x1, reg & 0x7);
   LOG("SD: OP=%d\n", pCP->reg[LMSL_REG_SHUTDOWN] & 0x1);
   reg= pCP->reg[LMSL_REG_AUDGAIN];
   LOG("AG: F=%d E=%d L=%d\n", (reg >> 4) & 0x1, (reg >> 3) & 0x1, reg & 0x7);
   LOG("AR: %d\n", pCP->reg[LMSL_REG_AUDRATE]);
   //for (; i<13; i++) LOG("%d : %02X\n", i, pCP->reg[i]);
} // dumpReg

// Fade (aka "breath" in document) gives a pulsing transition effect
// Values are power-of-two exponents so timing varies as 1,2 .. 64,128 (*26ms)
void setFade (U8 r8[2], U8 enable, U8 ex2FadeOutT, U8 ex2OffT, U8 ex2FadeInT)
{
   r8[0]= ((ex2FadeOutT & 0x7) << 4) | (ex2FadeInT & 0x7);
   r8[1]= ex2OffT & 0x7;
   if (enable) { r8[1]|= 1<<4; }
} // setFade

int ledMatHack (const LXI2CBusCtx *pC, const U8 busAddr)
{
   //LOG_CALL("() - %d bytes, %d bits set\n", sizeof(b), bitCountNU8(b, sizeof(b)));
   int r;
   FramePage   frames[8];
   ControlPage cp;
   U8 i, n, t, pageSel[2]={LMSL_REG_PAGE_SEL,LMSL_CTRL_PAGE};

   memset(frames, 0x00, sizeof(frames));
   // All pages start at 0x00
   //for (int i=0; i<8; i++) { frames[i].addr[0]= 0x00; }
   cp.addr[0]= 0x00;
   //memset(cp.reg, 0xFF, sizeof(cp.reg));
   //for (int i=0; i<13; i++) printf("%d : %02X\n", i, cp.reg[i]);
   r= lxi2cReadRB(pC, busAddr, pageSel, sizeof(pageSel));
   if (LMSL_CTRL_PAGE != pageSel[1])
   {
      printf("got page %x\n", pageSel[1]);
      pageSel[1]= LMSL_CTRL_PAGE;
      r= lxi2cWriteRB(pC, busAddr, pageSel, sizeof(pageSel));
   }

   r= lxi2cReadRB(pC, busAddr, cp.addr, sizeof(ControlPage));
   LOG("Read Control Page - r=%d\n", r);
   if (r >= 0) { dumpReg(&cp); }

   //pageSel[1]= LMSL_DATA_PAGE1; // Frame index 0
   //r= lxi2cWriteRB(pC, busAddr, pageSel, sizeof(pageSel));
   U8 pwmI[SHIM_LED_COUNT];
   const F32 bias= 7.0 / 0xFF;
   const F32 delta= (1.0 - bias) / (SHIM_LED_COUNT-1);
   for (int i=0; i < SHIM_LED_COUNT; i++)
   {
      F32 r= i * delta + bias;
      F32 g= gammaNorm(r);
      U8 gu= 0.5 + 0xFF * g;
      LOG("pwmI[%d] %G -> %G (%G -> %u)\n", i, r, g, 0xFF * r, gu);
      pwmI[i]= gu;
   }

   for (int iPage=0; iPage<LMSL_FRAME_PAGE_COUNT; iPage++)
   {
      pageSel[1]= iPage;
      r= lxi2cWriteRB(pC, busAddr, pageSel, sizeof(pageSel));
      if (0)
      {
         r= lxi2cReadRB(pC, busAddr, frames[iPage].addr, sizeof(FramePage));
         LOG("Read Frame Page - r=%d\n", r);
      }
      else if (iPage > 0) { memcpy(frames+iPage, frames+0, sizeof(frames[0])); }
      else
      {
         r= ledMapSetBits(frames[iPage].enable, LMSL_FLAG_BYTES);
         memcpy(frames[iPage].blink, frames[iPage].enable, LMSL_FLAG_BYTES);
         r= ledMapSetBits(frames[iPage].blink, LMSL_FLAG_BYTES);
         //memset(frames[iPage].blink, 0x00, LMSL_FLAG_BYTES);
         //memset(frames[iPage].pwm, 0x00, LMSL_PWM_BYTES);
      }
      //for (int i=0; i<LMSL_FLAG_BYTES; i+= 2) LOG("%d : %02X %02X\n", i, frames[iPage].enable[i], frames[iPage].enable[i+1]);

      if (r >= 0)
      {
         //U8 v[SHIM_LED_COUNT]= {0x00, }; //0x1F, 0xFF, 0x1F, 0x00, 0x1F};
         //for (int i=0; i <= iPage; i++) { v[SHIM_LED_COUNT-(1+i)]= 0x0F; }
static const U8 cMap[]={0x0,0x1,0x2,0x4,0x6,0x5,0x3,0x7}; // B R G B C M Y W
         n= cMap[iPage];
         //U8 m= SHIM_LED_COUNT / 2, w= iPage+1;
         //if (iPage > 1) { for (int i=(m-w); i < (m+w); i++) { v[i]= 0x7F; } }

         if (n & 0x1) { ledMapChanPWM(frames[iPage].pwm, pwmI, SHIM_LED_COUNT, -1, gMapLED.red); }
         if (n & 0x2) { ledMapChanPWM(frames[iPage].pwm, pwmI, SHIM_LED_COUNT, -1, gMapLED.green); }
         if (n & 0x4) { ledMapChanPWM(frames[iPage].pwm, pwmI, SHIM_LED_COUNT, -1, gMapLED.blue); }

         n= sizeof(FramePage);
         r= lxi2cWriteRB(pC, busAddr, frames[iPage].addr, n);
         LOG("Write Frame Page [%d] - %d Bytes r=%d\n", iPage, n, r);
      }
   }

   if (LMSL_CTRL_PAGE != pageSel[1])
   {
      t= pageSel[1];
      pageSel[1]= LMSL_CTRL_PAGE;
      r= lxi2cWriteRB(pC, busAddr, pageSel, sizeof(pageSel));
      LOG("Control Page Select - r=%d\n", r);
      if (r < 0) { pageSel[1]= t; }
   }
   if (LMSL_CTRL_PAGE == pageSel[1])
   {
      cp.reg[LMSL_REG_CONFIG]= (1<<3); // autoplay, start at [0]
      cp.reg[LMSL_REG_PICTURE]= 0x00;  // Picture [0]
      cp.reg[LMSL_REG_AUTOPLAY1]= 0;   // infinite
      cp.reg[LMSL_REG_AUTOPLAY2]= 0 & 0x3F;   // Frame duration (0->64) *11ms
      n= 2 + LMSL_REG_AUTOPLAY2;
      if (1) //blink)
      {  // no blink with autoplay ?
         cp.reg[LMSL_REG_DISPOPT]= 0; // off |= (1<<3) | 1;
         n= 2+LMSL_REG_DISPOPT;
      }
      r= lxi2cWriteRB(pC, busAddr, cp.addr, n);

      // Split because FS reg 0x7 is read only
      if (1) // 0 == (cp.reg[LMSL_REG_SHUTDOWN] & 0x1))
      {
         i= LMSL_REG_FRAMESTAT; // hack - use framestat as reg address temp
         t= cp.reg[i]; // save
         cp.reg[i]= LMSL_REG_BREATHE1;
         setFade(cp.reg+LMSL_REG_BREATHE1, 0, 5, 0, 0);
         cp.reg[LMSL_REG_SHUTDOWN]= 0x01; // enable

         r= lxi2cWriteRB(pC, busAddr, cp.reg+i, 1+LMSL_REG_SHUTDOWN-i);
         cp.reg[i]= t; // restore
      }
   }

   sleep(1);
   r= lxi2cReadRB(pC, busAddr, cp.addr, sizeof(ControlPage));
   LOG("Read Control Page - r=%d\n", r);
   if (r >= 0) { dumpReg(&cp); }

   if (r > 0) { r= 0; }
   return(r);
} // ledMatHack
