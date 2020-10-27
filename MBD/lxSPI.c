// Common/MBD/lxSPI.h - Linux SPI utils
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
//#include <linux/spidev.h> (in header)

#include "lxSPI.h"


/***/

// Kernel driver arg type compatibility.
typedef unsigned long UL;
typedef unsigned long long ULL;


/***/

Bool32 lxSPIOpen (LXSPICtx *pSC, const char devPath[], U32 clock)
{
   struct stat st;
   int r= -1;
   if ((0 == stat(devPath, &st)) && S_ISCHR(st.st_mode)) // ensure device exists
   {
      pSC->fd= open(devPath, O_RDWR);
      if (pSC->fd >= 0)
      {
         if (clock < 1) { clock= 100000; } // standard/default I2C clock rate
         else if (clock < 10000) { clock*= 1000; } // assume kHz
         //else { pBC->clk= clk; }  // assume Hz
         pSC->port.clock= clock;
         pSC->port.delay= 0;
         pSC->port.bits=  8;
         pSC->port.flags= 0;
/*
// Mode tokens ?
SPI_CPHA
SPI_CPOL

SPI_MODE_0		(0|0)
SPI_MODE_1		(0|SPI_CPHA)
SPI_MODE_2		(SPI_CPOL|0)
SPI_MODE_3		(SPI_CPOL|SPI_CPHA)

SPI_CS_HIGH
SPI_LSB_FIRST
SPI_3WIRE
SPI_LOOP
SPI_NO_CS
SPI_READY
SPI_TX_DUAL
SPI_TX_QUAD
SPI_RX_DUAL
SPI_RX_QUAD
         r= ioctl(pBC->fd, SPI_IOC_RD_MODE, SPI_MODE_0); 0..3
         r= ioctl(pBC->fd, SPI_IOC_WR_MODE, ???);
// other ioctl tokens..??
SPI_IOC_RD_LSB_FIRST
SPI_IOC_WR_LSB_FIRST
SPI_IOC_RD_BITS_PER_WORD
SPI_IOC_WR_BITS_PER_WORD
SPI_IOC_RD_MAX_SPEED_HZ
SPI_IOC_WR_MAX_SPEED_HZ
SPI_IOC_RD_MODE32
SPI_IOC_WR_MODE32
*/
         return(TRUE);
      }
   }
   if (r < 0) { ERROR_CALL("(.. %s) - %d\n", devPath, r); }
   return(r >= 0);
} // lxSPIOpen

void lxSPIClose (LXSPICtx *pSC)
{
   if (pSC->fd >= 0)
   {
      close(pSC->fd);
      pSC->fd= -1;
   }
} // lxSPIClose

int lxSPIReadWrite (LXSPICtx *pSC, U8 r[], const U8 w[], int n)
{
/* __u8		cs_change;
	__u8		tx_nbits;
	__u8		rx_nbits;
	__u16		pad;*/
   struct spi_ioc_transfer m={
      .rx_buf= (UL)r, .tx_buf= (UL)w, .len= n,
      .delay_usecs=pSC->port.delay,
      .speed_hz= pSC->port.clock,
      .bits_per_word=pSC->port.bits
   };
   //return reportBytes(OUT, (void*)&m, sizeof(m));
   return ioctl(pSC->fd, SPI_IOC_MESSAGE(1), &m);
} // lxSPIReadWrite

#ifdef LX_SPI_MAIN
#define ARG_ACTION 0xF0  // Mask
#define ARG_HACK   (1<<4)

#define ARG_OPTION   0x0F  // Mask
#define ARG_HELP    (1<<1)
#define ARG_VERBOSE (1<<0)

typedef struct
{
   char devPath[15]; // host device path e.g. /dev/spidev0.0
   U8 flags;
} LXSPIArgs;

static LXSPIArgs gArgs={"/dev/spidev0.0", 0};
static LXSPICtx gBusCtx;

void usageMsg (const char name[])
{
static const char optCh[]="dvh";
static const char argCh[]="#  ";
static const char *desc[]=
{
   "device index (-> path /dev/spidev0.# )",
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

void argDump (const LXSPIArgs *pA)
{
   report(OUT,"Device: devPath=%s\n", pA->devPath);
   report(OUT,"\tflags=%02X\n", pA->flags);
} // argDump

void argTrans (LXSPIArgs *pA, int argc, char *argv[])
{
   int c;
   do
   {
      c= getopt(argc,argv,"a:d:vh");
      switch(c)
      {
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

   if ((gArgs.flags & ARG_ACTION) && lxSPIOpen(&gBusCtx, gArgs.devPath, 100))
   {
      lxSPIReadWrite(&gBusCtx,NULL,NULL,0);

      lxSPIClose(&gBusCtx);
   }
   return(r);
} // main

#endif
