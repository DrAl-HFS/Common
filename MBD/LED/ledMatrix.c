// Common/MBD/ledMatrix.h - Hacks for IS31FL3731 LEDshim
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct. 2020

#include "ledMatrix.h"
#include "lumissil.h"
#include "ledMapRGB.h"


/***/

// Logical to perceptual mapping for [0,1] interval
F32 normGammaCorrectF32 (F32 x)
{
   if (x < 0.995) { return(1.008 * powf(x,1.695)); }
   //else
   return(1);
} // normGammaCorrectF32

// Interval [0,1] normalised unit conversion of HSV to RGB
// Lightness (v) scale is arbitrary but Hue and Saturation
// must be normalised for correct operation
void normHSV2RGBF32 (F32 rgbN[3], const F32 hN, const F32 sN, const F32 v)
{
// r=v*(1+s*(cos(h)-1))
// g=v*(1+s*(cos(h-2.09439)-1))
// b=v*(1+s*(cos(h+2.09439)-1))
   //if (h > 1) { h-= floor(h); }
   //if (v < 0) { for (int i= 0; i<3; i++) { rgbN[i]= 0; } return; }
   const F32 hRad= hN * 2 * M_PI; // Norm -> radians
   F32 cosRad[3];
   cosRad[0]= cosf(hRad);
   cosRad[1]= cosf(hRad - 2.09439); // - 2/3 PI
   cosRad[2]= cosf(hRad + 2.09439); // + 2/3 PI
   // Vectorisable iteration
   for (int i= 0; i<3; i++) { rgbN[i]= v * (1 + sN * (cosRad[i] - 1)); }
} // normHSV2RGBF32

void normHSV2RGBU8 (U8 rgbU8[3], const F32 hsvNF32[3])
{
   F32 rgbF32[3];
   normHSV2RGBF32(rgbF32, hsvNF32[0], hsvNF32[1], normGammaCorrectF32(hsvNF32[2]) * 0xFF);
   for (int i= 0; i<3; i++) { rgbU8[i]= rgbF32[i]+0.5; }
} // normHSV2RGBU8

void lerpHSV2RGBU8 (U8 rgbU8[], const F32 hsvA[3], const F32 hsvB[3], const int n)
{
   if (n > 0)
   {
      normHSV2RGBU8(rgbU8, hsvA);
      if (n > 1)
      {
         int m= n-1;
         normHSV2RGBU8(rgbU8+3*m, hsvB);
         if (n > 2)
         {
            F32 tHSV[3], dHSV[3];
            F32 r= 1.0 / m;
            for (int i= 0; i<3; i++) { dHSV[i]= r * (hsvB[i] - hsvA[i]); }
            for (int t= 1; t<m; t++)
            {
               for (int i= 0; i<3; i++) { tHSV[i]= hsvA[i] + t * dHSV[i]; }
               normHSV2RGBU8(rgbU8+3*t, tHSV);
               LOG("t=%d\n",t);
            }
         }
      }
   }
} // lerpHSV2RGBU8

void tstH (const int n)
{
#if 0
   F32 rgb[3], h=0, d=1;

   if (n > 1) { d/= n; }
   LOG("H [%d] ->\tR\tG\tB\n", n);
   for (int i=0; i <= n; i++)
   {
      normHSV2RGBF32(rgb, h, 0.5, 0.5);
      LOG("%6.4G -> %5.3G, %5.3G, %5.3G\n", h, rgb[0], rgb[1], rgb[2]);
      h+= d;
   }
#else
   F32 hsv[2][3]= { {0.1,1,0.125}, {0.9,0.5,0.75} };
   U8 rgb[3*10];
   if (n > 10) return;
   memset(rgb, -1, sizeof(rgb));
   lerpHSV2RGBU8(rgb, hsv[0], hsv[1], n);
   for (int i=0; i < n; i++)
   {
      int j= 3*i;
      LOG("[%d] : ( %u, %u, %u )\n", i, rgb[j+0], rgb[j+1], rgb[j+2]);
   }
#endif
   LOG("%s\n","---");
} // tstH

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

int chanTestSetData (FramePage *pFP, const U8 v[], const U8 i, const U8 testID, U8 chanMode)
{
static const U8 cMap[]={0x0,0x1,0x2,0x4,0x6,0x5,0x3,0x7}; // B R G B C M Y W
static const int strideI[2]={0,1};
static const int strideRGB[2]={1,3};
   chanMode|= cMap[i]<<4;
   switch (testID)
   {
      case 3 :
         ledMapMultiChanPWM(pFP->pwm, v, SHIM_LED_COUNT, strideRGB, chanMode|CHAN_MODE_RGB);
         break;
      case 2 :
         ledMapMultiChanPWM(pFP->pwm, v, SHIM_LED_COUNT, strideI, chanMode);
         break;
      case 1 :
         ledMap1NChanPWM(pFP->pwm, v, SHIM_LED_COUNT, chanMode);
         break;
      default :
      {
         const int iC= (i % 3) * SHIM_LED_COUNT;
         ledMapChanPWM(pFP->pwm, v, SHIM_LED_COUNT, gMapLED.chan + iC, chanMode);
      }
   }
   return(sizeof(FramePage));
} // chanTestSetData

void genVI (U8 v[], const int n)
{
   const F32 bias= 6.5 / 0xFF;
   const F32 delta= (1.0 - bias) / (n-1);
   for (int i=0; i < n; i++)
   {
      F32 r= i * delta + bias;
      F32 g= normGammaCorrectF32(r);
      U8 gu= 0.5 + 0xFF * g;
      //U8 ru= 0.5 + 0xFF * r;
      //LOG("pwmI[%d] %G -> %G (%u -> %u)\n", i, r, g, ru, gu);
      v[i]= gu;
   }
} // genVI

void genVRGB (U8 v[], const int n)
{
   F32 hsv[2][3]= { {-0.01, 1.0, 0.75}, {1.01, 1.0, 0.75} };

   lerpHSV2RGBU8(v, hsv[0], hsv[1], n);
} // genVRGB

int ledMatHack (const LXI2CBusCtx *pC, const U8 busAddr, const U8 modeFlags)
{
   int r;
   FramePage   frames[8];
   ControlPage cp;
   U8 i, n, t, pageSel[2]={LMSL_REG_PAGE_SEL,LMSL_CTRL_PAGE};

   tstH(8);

   r= lxi2cReadRB(pC, busAddr, pageSel, sizeof(pageSel));
   if (LMSL_CTRL_PAGE != pageSel[1])
   {
      printf("got page %x\n", pageSel[1]);
      pageSel[1]= LMSL_CTRL_PAGE;
      r= lxi2cWriteRB(pC, busAddr, pageSel, sizeof(pageSel));
   }

   cp.addr[0]= 0x00;
   r= lxi2cReadRB(pC, busAddr, cp.addr, sizeof(ControlPage));
   LOG("Read Control Page - r=%d\n", r);
   if (r >= 0) { dumpReg(&cp); }

   // Setup intensity table
   U8 pwmI[SHIM_LED_COUNT];
   genVI(pwmI, SHIM_LED_COUNT);

   U8 pwmRGB[3*SHIM_LED_COUNT];
   genVRGB(pwmRGB, SHIM_LED_COUNT);


   // Initialise first frame
   frames[0].addr[0]= 0x00;
   r= ledMapSetBits(frames[0].enable, LMSL_FLAG_BYTES, SHIM_LED_COUNT, 0, CHAN_MODE_RGB);
   memcpy(frames[0].blink, frames[0].enable, LMSL_FLAG_BYTES);
   memset(frames[0].pwm, 0x00, LMSL_PWM_BYTES);


   // Setup each page and send to device
   //for (int iPage=0; iPage<LMSL_FRAME_PAGE_COUNT; iPage++)
   int iPage= LMSL_FRAME_PAGE_COUNT;
   while (--iPage >= 0)
   {
      pageSel[1]= iPage;
      r= lxi2cWriteRB(pC, busAddr, pageSel, sizeof(pageSel));
      if (0)
      {  // copy existing from device
         r= lxi2cReadRB(pC, busAddr, frames[iPage].addr, sizeof(FramePage));
         LOG("Read Frame Page - r=%d\n", r);
      }
      else if (iPage > 0) { memcpy(frames+iPage, frames+0, sizeof(frames[0])); }

      n= chanTestSetData(frames+iPage, pwmRGB, iPage, 3, 0x85);
      if (n > 0)
      {
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
      if (1) //blink
      {
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

   if (MODE_SHUTDOWN & modeFlags)
   {
      sleep(10);
      if (LMSL_CTRL_PAGE == pageSel[1])
      {
         U8 sd[2]= {LMSL_REG_SHUTDOWN, 0x00}; // shutdown
         r= lxi2cWriteRB(pC, busAddr, sd, 2);
      }
   }

   if (r > 0) { r= 0; }
   return(r);
} // ledMatHack

#ifdef LED_MAIN

#define ARG_ACTION 0xF0  // Mask
#define ARG_HACK   (1<<4)

#define ARG_OPTION   0x0F  // Mask
#define ARG_HELP    (1<<1)
#define ARG_VERBOSE (1<<0)

typedef struct
{
   char devPath[14]; // host device path
   U8 busAddr; // bus device address
   U8 flags;
} LEDArgs;

static LEDArgs gArgs={"/dev/i2c-1", 0x75, 0};
static LXI2CBusCtx gBusCtx;

void usageMsg (const char name[])
{
static const char optCh[]="advh";
static const char argCh[]="##  ";
static const char *desc[]=
{
   "I2C bus address: 2digit hex (no prefix)",
   "device index (-> path /dev/i2c-# )",
   "verbose diagnostic messages",
   "help (display this text)",
};
   const int n= sizeof(desc)/sizeof(desc[0]);
   report(OUT,"Usage : %s [-%s]\n", name, optCh);
   for (int i= 0; i<n; i++)
   {
      report(OUT,"\t%c %c - %s\n", optCh[i], argCh[i], desc[i]);
   }
} // usageMsg

void argDump (const LEDArgs *pA)
{
   report(OUT,"Device: devPath=%s, busAddr=%02X, Flags=%02X\n", pA->devPath, pA->busAddr, pA->flags);
} // argDump

void argTrans (LEDArgs *pA, int argc, char *argv[])
{
   int c, t;
   do
   {
      c= getopt(argc,argv,"a:d:vh");
      switch(c)
      {
         case 'a' :
            sscanf(optarg, "%x", &t);
            if ((t & 0x7F) == t) { pA->busAddr= t; }
            break;
         case 'd' :
         {
            char ch= optarg[0];
            if ((ch > '0') && (ch <= '9')) { pA->devPath[13]= ch; }
            break;
         }
         case 'h' :
            pA->flags|= ARG_HELP;
            break;
         case 'v' :
            pA->flags|= ARG_VERBOSE;
            break;
     }
   } while (c > 0);
   if (pA->flags & ARG_HELP) { usageMsg(argv[0]); }
   if (pA->flags & ARG_VERBOSE) { argDump(pA); }
} // argTrans

int main (int argc, char *argv[])
{
   int r= -1;

   argTrans(&gArgs, argc, argv);

   //if ((gArgs.flags & ARG_ACTION) &&
   if (lxi2cOpen(&gBusCtx, gArgs.devPath, 400))
   {
      r= ledMatHack(&gBusCtx, gArgs.busAddr, MODE_SHUTDOWN);

      lxi2cClose(&gBusCtx);
   }
   return(r);
} // main

#endif // LED_MAIN
