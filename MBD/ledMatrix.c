// Common/MBD/ledMatrix.h - Hacks for IS31FL3731 LEDshim
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct. 2020

#include "ledMatrix.h"
#include "lumissil.h"
#include "ledMapRGB.h"


/***/

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
   LOG("DO: IC=%d B=%d BP=%d\n", (reg >> 4) & 0x1, (reg >> 3) & 0x1, reg & 0x7);
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

int ledMatHack (const LXI2CBusCtx *pC, const U8 busAddr)
{
   //LOG_CALL("() - %d bytes, %d bits set\n", sizeof(b), bitCountNU8(b, sizeof(b)));
   int r;
   FramePage   frames[8];
   ControlPage cp;
   U8 pageSel[2]={LMSL_REG_PAGE_SEL,LMSL_CTRL_PAGE};

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

   for (int iPage=0; iPage<LMSL_FRAME_PAGE_COUNT; iPage++)
   {
      pageSel[1]= iPage;
      r= lxi2cWriteRB(pC, busAddr, pageSel, sizeof(pageSel));
      if (0)
      {
         r= lxi2cReadRB(pC, busAddr, frames[iPage].addr, sizeof(FramePage));
         LOG("Read Frame Page - r=%d\n", r);
      }
      else
      {
         r= ledMapSetBits(frames[iPage].enable, LMSL_FLAG_BYTES);
         memset(frames[iPage].blink, 0x00, LMSL_FLAG_BYTES);
         memset(frames[iPage].pwm, 0x00, LMSL_PWM_BYTES);
      }
      //for (int i=0; i<LMSL_FLAG_BYTES; i+= 2) LOG("%d : %02X %02X\n", i, frames[iPage].enable[i], frames[iPage].enable[i+1]);

      if (r >= 0)
      {
         U8 v[SHIM_LED_COUNT]= {0x00, }; //0x1F, 0xFF, 0x1F, 0x00, 0x1F};
         U8 m= SHIM_LED_COUNT / 2, w= iPage+1;
         if (iPage > 1) { for (int i=(m-w); i < (m+w); i++) { v[i]= 0x7F; } }
         //ledMapChanPWM(frames[iPage].pwm, v, SHIM_LED_COUNT, -1, gMapLED.red); // red);
         //ledMapChanPWM(frames[iPage].pwm, v, SHIM_LED_COUNT-5, 0x03, gMapLED.green+5); // red);
         ledMapChanPWM(frames[iPage].pwm, v, SHIM_LED_COUNT, -1, gMapLED.blue); // red);
         //ledMapIdxChanPWM(frames[iPage].pwm, v, 0x03, 0);

         r= lxi2cWriteRB(pC, busAddr, frames[iPage].addr, sizeof(FramePage));
         LOG("Write Frame Page [%d] - r=%d\n", iPage, r);
      }
   }

   if (LMSL_CTRL_PAGE != pageSel[1])
   {
      pageSel[1]= LMSL_CTRL_PAGE;
      r= lxi2cWriteRB(pC, busAddr, pageSel, sizeof(pageSel));
      if (r > 0)
      {
         LOG("Control Page Select - r=%d\n", r);
         cp.reg[LMSL_REG_CONFIG]= (1<<3); // autoplay
         cp.reg[LMSL_REG_PICTURE]= 0x07;  // P F 8
         cp.reg[LMSL_REG_AUTOPLAY1]= 0;
         cp.reg[LMSL_REG_AUTOPLAY2]= 1;
         cp.reg[LMSL_REG_BREATHE1]= (5<<4) | 5; // out, in
         cp.reg[LMSL_REG_BREATHE2]= (1<<4) | 1; // enable, ext
         r= lxi2cWriteRB(pC, busAddr, cp.addr, 2+LMSL_REG_BREATHE2);
      }
   }
   if (0 == (cp.reg[LMSL_REG_SHUTDOWN] & 0x1))
   {
      U8 pwr[2]={LMSL_REG_SHUTDOWN,0x01};
      r= lxi2cWriteRB(pC, busAddr, pwr, sizeof(pwr));
   }

   sleep(3);
   r= lxi2cReadRB(pC, busAddr, cp.addr, sizeof(ControlPage));
   LOG("Read Control Page - r=%d\n", r);
   if (r >= 0) { dumpReg(&cp); }

   if (r > 0) { r= 0; }
   return(r);
} // ledMatHack
