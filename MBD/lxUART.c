// Common/MBD/lxUART.c - Linux UART utils
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#include <fcntl.h>
#include <errno.h>

#include "lxUART.h"

/*
Basic checks for RPi -
enable_uart=1 (also disable serial console in cmdline.txt, easier using raspi-config)
init_uart_baud=115200 ???no effect???

RPi-W (BT) options:
1) Free up PL011 (becomes Primary i.e. /dev/ttyS0)
a) No BT
dtoverlay=disable-bt
sudo systemctl disable hciuart
b) Mini-UART BT (reliability untested)
dtoverlay=miniuart-bt
core_freq=250 ( OR force_turbo=1 ???)

2) RPi4 extra PL011 devices
cat /boot/overlays/README
dtoverlay=uart#,<param>=<val>
*/
// DIY power-of-hundred exponent & mantissa encoding
#define UEX_SHIFT   14
#define UEX_MASK_M ((1<<UEX_SHIFT)-1)
//#define UEX_E0 (0<<14)
#define UEX_ET2 (1<<14) // 10^2
#define UEX_ET4 (2<<14) // 10^4
#define UEX_ET6 (3<<14) // 10^6

typedef U16 UEX16;
#define REF_BAUD_ULO  (7)  // Communicating with objects nearing the Oort cloud?
#define REF_BAUD_LO  (13)  // Geriatric equipment / very noisy environments
#define REF_BAUD_STD (19)  // Widely supported (assume good quality wire & connectors)
#define REF_BAUD_HI  (21)  // Special installations: low-capacitance/screened structured cable
static const UEX16 refBaud[]=
{
   50,75,110,134,150,200,300,       // 7 ultra-low
   600,1200,1800,2400,4800,9600,    // +6 low
   UEX_ET2|192, UEX_ET2|384, UEX_ET2|576, UEX_ET2|768, UEX_ET2|1152, UEX_ET2|2304, // +6 standard
   UEX_ET2|4608, UEX_ET2|9216  // +2 high
};


/***/

// mantissa, base & exponent
static U32 scalePowU (const U32 m, const U32 b, const U8 e)
{
   switch (e)
   {
      case 0 : return(m);
      case 1 : return(m * b);
      default :
      {
         U32 s= b * b;
         for (U8 i=3; i<=e; i++) { s*= b; }
         return(m * s);
      }
   }
} // scalePowU

static U32 unpackUEX16 (UEX16 x)
{
   return scalePowU(UEX_MASK_M & x, 100, x>>UEX_SHIFT);
} // unpackUEX16

U32 adiff (U32 a, U32 b) { if (a > b) return(a - b); else return (b-a); }

int matchUEX16 (U32 *pR, const U32 x, const UEX16 ref[], int n)
{
   int iB= -1;
   if (n > 0)
   {
      U32 rB=0, dB=-1, r=-1, d=-1, ld;
      do
      {
         ld= d;
         r= unpackUEX16(ref[--n]);
         d= adiff(x,r);
         if (d < dB) { iB= n; rB= r; dB= d; }
      } while ((n > 0) && (d < ld));
      if (pR) { *pR= rB; }
   }
   return(iB);
} // matchUEX16

int matchBaudRate (int *pR, const int r, const U8 range)
{
   if (r > 0) { return matchUEX16((unsigned*)pR, r, refBaud, MIN(REF_BAUD_HI,range)); }
   return(0);
} // matchBaudRate

void testUEX (void)
{
   for (int i=0; i<REF_BAUD_HI; i++) { LOG("S%d : %u\n", i, unpackUEX16(refBaud[i])); }
   //n= sizeof(extBaud)/sizeof(extBaud[0]);
   //for (int i=0; i<n; i++) { LOG("E%d : %u\n", i, unpackUEX16(extBaud[i])); }
   for (int i=100; i<100000000; i*= 1.5)
   {
      I32 r;
      matchBaudRate(&r, i, REF_BAUD_HI);
      LOG("%d -> %d\n", i, r);
   }
} // testUEX

#ifndef LXUART_TERMIOS_OLD

//#include <asm/termbits.h>
//#include <asm/ioctls.h>
#include <stropts.h>
#include <asm/termios.h>

void dump (const int f)
{
   if (0 != f)
   {
      const char *s[32]={NULL,};
      int n= 0;
      if (f & EXTA)	s[n++]= "External rate clock A";
      if (f & EXTB)	s[n++]= "External rate clock B";
      if (f & CSIZE)	s[n++]= "Bit mask for data bits";
      if (f & CS5)	s[n++]= "5 data bits";
      if (f & CS6)	s[n++]= "6 data bits";
      if (f & CS7)	s[n++]= "7 data bits";
      if (f & CS8)	s[n++]= "8 data bits";
      if (f & CSTOPB)	s[n++]= "2 stop bits (1 otherwise)";
      if (f & CREAD)	s[n++]= "Enable receiver";
      if (f & PARENB)	s[n++]= "Enable parity bit";
      if (f & PARODD)	s[n++]= "Use odd parity instead of even";
      if (f & HUPCL)	s[n++]= "HUP";
      if (f & CLOCAL)	s[n++]= "Local";
      //if (f & LOBLK)	s[n++]= "Block job control output";
      //if (f & CNEW_RTSCTS)	s[n++]= "CNEW_RTSCTS";
      if (f & CRTSCTS)	s[n++]= "CRTSCTS";
      for (int i=0; i<n; i++) { LOG("%s\n",s[i]); }
   }
} // dump

void setTermiosFlagCh (char c[4], const int f)
{
   int i= 0;
   if (f & CS8) { c[i]= '8'; }   // assume 8bit mode...
   else if (f & CS7) { c[i]= '7'; }
   else if (f & CS6) { c[i]= '6'; }
   else if (f & CS5) { c[i]= '5'; }
   ++i;
   if (0 == (f & PARENB)) { c[i]= 'N'; }
   else if (f & PARODD) { c[i]= 'O'; } else { c[i]= 'E'; }
   ++i;
   if (f & CSTOPB) { c[i]= '2'; } else { c[i]= '1'; }
   c[++i]= 0;
} // setTermiosFlagCh

int interrogatePort (const int fd, const PortUART *pS) //
{
   struct termios2 st;

   int r= ioctl(fd, TCGETS2, &st);
   if (r >= 0)
   {
      char fc[4];
      setTermiosFlagCh(fc, st.c_cflag);
      LOG_CALL("() - %d / %d baud %s\n", st.c_ispeed, st.c_ospeed, fc);
      //dump(st.c_cflag);

      int b= MIN(st.c_ispeed, st.c_ospeed);
      if (pS && (pS->baud > 0) &&
         ((pS->baud != st.c_ispeed) || (pS->baud != st.c_ospeed)))
      {
         st.c_cflag&= ~CBAUD;
         st.c_cflag|= BOTHER;
         st.c_ispeed= pS->baud;
         st.c_ospeed= pS->baud;
         r= ioctl(fd, TCSETS2, &st);
         if (r >= 0) { b= pS->baud; }
      }
      return(b);
   }
   return(r);
} // interrogatePort

#else // LXUART_TERMIOS_OLD

// Older functionality - standard baud rates only
#include <termios.h>
#include <sys/ioctl.h>

#define REF_BAUD_TERMIOS_IMAX (REF_BAUD_LO+2)
static I32 termiosBaudIdx (int i)
{
   if (i > 0)
   {
      if ((i-1) < REF_BAUD_TERMIOS_IMAX) { return unpackUEX16(refBaud[i-1]); }
      switch (i)
      {
         case B57600  : return(57600);
         case B115200 : return(115200);
         case B230400 : return(230400);
      }
   }
   return(0);
} // termiosBaudIdx

static int termiosMatchBaudRate (int *pR, const int r)
{
   int i= matchBaudRate(pR,r,REF_BAUD_STD);
   if (i < REF_BAUD_TERMIOS_IMAX) { return(i+1); }
   static const U16 t[]={ B57600, B115200, B115200, B230400 }; // Aaargh! no 76800 !!??
   return( t[i-REF_BAUD_TERMIOS_IMAX] );
} // termiosMatchBaudRate

int interrogatePort (const int fd, const PortUART *pS)
{
   struct termios st;
   //int r= ioctl(pUC->fd, TCGETS, &st);
   int r= tcgetattr(fd, &st);
   if (r >= 0)
   {
      int iB= st.c_cflag & CBAUD;
      int rB= termiosBaudIdx(iB);
      int s[2];
      s[0]= cfgetispeed(&st);
      s[1]= cfgetospeed(&st);
      LOG_CALL("() - %d / %d (%d?) baud\n", s[0], s[1], rB);

      if (pS && ((pS->baud > 0) || (pS->tbiv != 0)))
      {
         int rBS= termiosBaudIdx(pS->tbiv);
         if (0 != rBS) { iB= pS->tbiv; } // Assume rate is termios standard index value e.g. "B9600"
         else
         {  // Find index of best match, apply +1 shift (0 -> B0, 1 -> B50)
            iB= termiosMatchBaudRate(&rBS, pS->baud);
         }

         cfsetspeed(&st, iB);
         r= tcsetattr(fd, 0, &st);
         if (r >= 0) { return(rBS); } // newly set value
      }
      return(rB); // existing value
   }
   return(r); // error
} // interrogatePort

#endif // LXUART_TERMIOS_OLD


/***/

Bool32 lxUARTOpen (LXUARTCtx *pUC, const char devPath[])
{
   struct stat st;
   int r= -1;

   //testUEX();

   if ((0 == stat(devPath, &st)) && S_ISCHR(st.st_mode)) // ensure device exists
   {
      pUC->fd= open(devPath, O_RDWR|O_NOCTTY|O_NDELAY); // ignore controls & HW signals e.g. DCD
      if (pUC->fd >= 0)
      {
         pUC->port.baud= interrogatePort(pUC->fd, NULL); // 230400);
         LOG_CALL("(..%s) - %d baud\n", devPath, pUC->port.baud);

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

