// Common/MBD/ledMatrix.h - Hacks for IS31FL3731 LEDshim
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct. 2020

#include "ledMatrix.h"
#include "lumissil.h"
#include "ledMapRGB.h"


/***/

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
   if (r >= 0)
   {
      for (int i=0; i<13; i++) LOG("%d : %02X\n", i, cp.reg[i]);
   }
   pageSel[1]= LMSL_DATA_PAGE1; // Frame index 0
   r= lxi2cWriteRB(pC, busAddr, pageSel, sizeof(pageSel));

   if (0)
   {
      r= lxi2cReadRB(pC, busAddr, frames[0].addr, sizeof(FramePage));
      LOG("Read Frame Page - r=%d\n", r);
   }
   else
   {
      r= ledMapSetBits(frames[0].enable, LMSL_FLAG_BYTES);
      memset(frames[0].blink, 0x00, LMSL_FLAG_BYTES);
      memset(frames[0].pwm, 0x00, LMSL_PWM_BYTES);
   }
   //for (int i=0; i<LMSL_FLAG_BYTES; i+= 2) LOG("%d : %02X %02X\n", i, frames[0].enable[i], frames[0].enable[i+1]);

   if (r >= 0)
   {
      U8 v[]= {0x00, 0x1F, 0xFF, 0x1F};
      ledMapChanPWM(frames[0].pwm, v, SHIM_LED_COUNT, 0x03, gMapLED.red); // red);
      //ledMapChanPWM(frames[0].pwm, v, SHIM_LED_COUNT-5, 0x03, gMapLED.green+5); // red);
      ledMapChanPWM(frames[0].pwm, v, SHIM_LED_COUNT, 0x03, gMapLED.blue); // red);
      //ledMapIdxChanPWM(frames[0].pwm, v, 0x03, 0);

      r= lxi2cWriteRB(pC, busAddr, frames[0].addr, sizeof(FramePage));
      LOG("Write Frame Page - r=%d\n", r);
   }
   if (r > 0) { r= 0; }
   return(r);
} // ledMatHack
