// Common/MBD/lxSPI.h - Linux SPI utils
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct-Dec 2020

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
//#include <linux/spidev.h> (in header)

#include "lxSPI.h"
#include "sciFmt.h"


/***/

// Kernel driver arg type compatibility.
typedef unsigned long UL;
typedef unsigned long long ULL;

typedef struct
{
   uint32_t m32, maxClk;
   uint8_t  bpw, pad[3];
} SPIInfo;


/***/

static int modeStr (char s[], const int max, const uint32_t m)
{
   int n= 0;
   if (m & SPI_CPHA) { n+= snprintf(s+n, max-n, "CPHA "); }
   if (m & SPI_CPOL) { n+= snprintf(s+n, max-n, "CPOL "); }
   if (m & SPI_NO_CS) { n+= snprintf(s+n, max-n, "NCS "); }
   if (m & SPI_CS_HIGH) { n+= snprintf(s+n, max-n, "HCS "); }
   if (m & SPI_LSB_FIRST) { n+= snprintf(s+n, max-n, "LSB "); }
   if (m & SPI_3WIRE) { n+= snprintf(s+n, max-n, "3WR "); }
   if (m & SPI_READY) { n+= snprintf(s+n, max-n, "RDY "); }
// SPI_LOOP
/*
SPI_TX_DUAL
SPI_TX_QUAD
SPI_RX_DUAL
SPI_RX_QUAD
*/
   return(n);
} // modeStr

static int getInfo (int fd, SPIInfo *pI)
{
   int r, n=0;

   r= ioctl(fd, SPI_IOC_RD_MODE32, &(pI->m32)); n+= (r>= 0);
   r= ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &(pI->maxClk)); n+= (r>= 0);
   r= ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &(pI->bpw)); n+= (r>= 0);
   //r= ioctl(fd, SPI_IOC_RD_LSB_FIRST, &(pI->lsb)); n+= (r>= 0); // redundant

   return(n);
} // getInfo

static int setInfo (int fd, const SPIInfo *pI)
{
   int r, n=0;

   r= ioctl(fd, SPI_IOC_WR_MODE32, &(pI->m32)); n+= (r>= 0);
   //???  r= ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &(pI->maxClk)); n+= (r>= 0); // what point ?
   r= ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &(pI->bpw)); n+= (r>= 0);
   //r= ioctl(fd, SPI_IOC_WR_LSB_FIRST, &(pI->lsb)); n+= (r>= 0);

   return(n);
} // setInfo

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
         SPIInfo inf;
         char s[64], nc=0;

         getInfo(pSC->fd, &inf);

         if (SPI_MODE_3 != (SPI_MODE_3 & inf.m32)) { inf.m32|= SPI_MODE_3; ++nc; }
         if (SPI_CS_HIGH == (SPI_CS_HIGH & inf.m32)) { inf.m32&= ~SPI_CS_HIGH; ++nc; }
         if (16 != inf.bpw) { inf.bpw= 16; ++nc; }
         if (nc > 0) { nc= setInfo(pSC->fd, &inf); LOG("set() - %d\n", nc); }

         LOG_CALL("() - %G%cHz\n", sciFmtSetF(s, inf.maxClk), s[0]);
         LOG("\t%ubpw, %ulsb, 0x%08X modes:\n", inf.bpw, inf.lsb, inf.m32);
         modeStr(s, sizeof(s)-1, inf.m32); LOG("\t%s\n", s);

         if (clock < 1) { clock= 8000000; } // 8MHz default SPI clock rate
         else if (clock < 10000) { clock*= 1000; } // assume kHz

         pSC->port.clock= MIN(clock, inf.maxClk);
         pSC->port.delay= 0;
         pSC->port.bits=  inf.bpw;
         pSC->port.flags= 0;

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
   struct spi_ioc_transfer m={
      .rx_buf= (UL)r, .tx_buf= (UL)w, .len= n, .speed_hz= pSC->port.clock,
      .delay_usecs= pSC->port.delay, .bits_per_word= pSC->port.bits,
      .cs_change= 1, // toggle CS
      // .tx_nbits= 0, .rxnbits= 0 // multi-rate transfer unused here  (.pad= 0 )
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
      c= getopt(argc,argv,"d:vh");
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

   //if ((gArgs.flags & ARG_ACTION) &&
   if (lxSPIOpen(&gBusCtx, gArgs.devPath, 100))
   {
      lxSPIReadWrite(&gBusCtx,NULL,NULL,0);

      lxSPIClose(&gBusCtx);
   }
   return(r);
} // main

#endif // LX_SPI_MAIN
