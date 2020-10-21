// Common/MBD/lxUART.c - Linux UART utils
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#include <fcntl.h>
#include <errno.h>

#include "lxUART.h"


/***/

#if 1

//#include <asm/termbits.h>
#include <stropts.h>
#include <asm/termios.h>
//#include <asm/ioctls.h>

int newTerm (const LXUARTCtx *pUC, int rate) //
{
   struct termios2 st;

   int r= ioctl(pUC->fd, TCGETS2, &st);
   if (r >= 0)
   {
      LOG_CALL("() - %d / %d baud\n", st.c_ispeed, st.c_ospeed);

      if (rate > 0)
      {
         st.c_cflag&= ~CBAUD;
         st.c_cflag|= BOTHER;
         st.c_ispeed= rate;
         st.c_ospeed= rate;
         r= ioctl(pUC->fd, TCSETS2, &st);
         if (r > 0) { r= rate; }
      }
   }
   return(r);
} // newTerm

//#undef struct termios
//#undef struct termio
//#undef struct winsize

#else

#include <termios.h>
#include <sys/ioctl.h>

static I32 stdBaud (I32 r, I8 i)
{
static const U8 sbdL6[]={50,75,110,134,150,200};
static const U16 sbdM10[]={300,600,1200,1800,2400,4800,9600,19200,38400,57600};
static const U32 sbdH2[]={76800,115200};
   if (i <= 0) { r= 0; }
   else
   {
      i-= 1;
      if (i<6) { r= sbdL6[i]; }
      else
      {
         i-= 6;
         if (i<10) { r= sbdM10[i]; }
         else
         {
            i-= 10;
            if (i < 2) { r= sbdH2[i]; }
            else r= -1;
         }
      }
   }
   return(r);
} // stdBaud

int oldTerm (const LXUARTCtx *pUC)
{
   struct termios st;
   //int r= ioctl(pUC->fd, TCGETS, &st);
   int r= tcgetattr(pUC->fd, &st);
   if (r >= 0)
   {
      I8 ib= st.c_cflag & CBAUD;
      r= stdBaud(0,ib);
      //int s= cfgetspeed(st);
   }
   return(r);
} // oldTerm
#endif

/***/

Bool32 lxUARTOpen (LXUARTCtx *pUC, const char devPath[])
{
   struct stat st;
   int r= -1;
   if ((0 == stat(devPath, &st)) && S_ISCHR(st.st_mode)) // ensure device exists
   {
      pUC->fd= open(devPath, O_RDWR|O_NOCTTY|O_NDELAY); // ignore controls & HW signals e.g. DCD
      if (pUC->fd >= 0)
      {
         pUC->baud= newTerm(pUC, 230400);
         LOG_CALL("(..%s) - %d baud\n", devPath, pUC->baud);

         /*
         if ((r < 0) || (0 == (pBC->flags & I2C_FUNC_I2C)))
         {
            ERROR_CALL("(.. %s) - ioctl(.. I2C_FUNCS ..) -> %d\n", path, r);
            //report(ERR1,"(non I2C device ?) ERR#%d -> %s\n", errno, strerror(errno));
            if (r > 0) { r= -1; }
         }
         if (gCtx.flags & LX_I2C_FLAG_TRACE)
         {
            LX_TRC0("(%s) -> %d\n", path, pBC->fd);
            LX_TRC1("ioctl(.. I2C_FUNCS ..) -> %d\n", r);
            LX_TRC1("flags=0x%0X (%dbytes)\n", pBC->flags, sizeof(pBC->flags));
         }
         if (clk < 1) { pBC->clk= 100000; } // standard/default I2C clock rate
         else if (clk < 10000) { pBC->clk= clk*1000; } // assume kHz
         else { pBC->clk= clk; }  // assume Hz
         */
      }
      else { ERROR_CALL("(.. %s)\n", devPath); }
   }
   return(r >= 0);
}

void lxUARTClose (LXUARTCtx *pUC)
{
   if (pUC->fd >= 0)
   {
      close(pUC->fd);
      pUC->fd= -1;
   }
} // lxUARTClose


//lxUARTRead()
//lxUARTWrite()

