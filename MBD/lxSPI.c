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


/***/

static int strMode (char s[], const int m, const U32 kdmf)
{
static const char hlm[3]={'H','L','M'};
static const char cd[2]={'-',':'};
   U8 c;
   int n= 0;
   n+= snprintf(s+n, m-n, "MODE%u", kdmf & SPI_MODE_3);
   //if (kdmf & SPI_CPHA) { n+= snprintf(s+n, m-n, "CPHA "); }
   //if (kdmf & SPI_CPOL) { n+= snprintf(s+n, m-n, "CPOL "); }

   c= (0 == (kdmf & SPI_NO_CS));
   n+= snprintf(s+n, m-n, " CS%c",cd[c]);
   if (c)
   {
      c= (0 == (kdmf & SPI_CS_HIGH));
      n+= snprintf(s+n, m-n, "%c->%c", hlm[c], hlm[c^1]);
   }
   c= hlm[ 1+(0 == (kdmf & SPI_LSB_FIRST)) ];
   n+= snprintf(s+n, m-n, " %cSB", c);
   if (kdmf & SPI_3WIRE) { n+= snprintf(s+n, m-n, " 3WR"); }
   if (kdmf & SPI_READY) { n+= snprintf(s+n, m-n, " RDY"); }
// SPI_LOOP SPI_TX_DUAL SPI_TX_QUAD SPI_RX_DUAL SPI_RX_QUAD OCTA....
   return(n);
} // strMode

static int strProf (char s[], const int m, const SPIProfile *pP)
{
   int n= snprintf(s, m, "clk= %G%cHz, delay= %uus, bpw= %u\n ", sciFmtSetF(s, pP->clk), s[0], pP->delay, pP->bpw);
   n+= snprintf(s+n, m-n, "\tmode= 0x%X : ", pP->kdmf);
   strMode(s+n, m-n, pP->kdmf);
   return(n);
} // strProf

static int getProf (int fd, SPIProfile *pP)
{
   int r, m=0;
   r= ioctl(fd, SPI_IOC_RD_MODE32, &(pP->kdmf)); m|= (r>= 0);
   r= ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &(pP->bpw)); m|= (r>= 0)<<1;
   return(m);
} // getInfo

static int getDefaultProf (int fd, SPIProfile *pP)
{
   pP->clk= 8E6; // 8MHz sensible default
   pP->delay= 0;
   return getProf(fd,pP);
} // getDefaultProf

static int setProf (int fd, const SPIProfile *pP)
{
   int r, m=0;
   r= ioctl(fd, SPI_IOC_WR_MODE32, &(pP->kdmf)); m|= (r>= 0);
   r= ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &(pP->bpw)); m|= (r>= 0)<<1;
   return(m);
} // setProf

static void setTransProf (struct spi_ioc_transfer *pT, const SPIProfile *pP)
{
   memset(pT, 0, sizeof(*pT));
   pT->speed_hz=        pP->clk;
   pT->delay_usecs=     pP->delay;
   pT->bits_per_word=   pP->bpw;
   pT->cs_change= 1; // ??? no change for part of ongoing transaction ???
} // setTransProf

/***/

Bool32 lxSPIOpen (LXSPICtx *pSC, const char devPath[], const SPIProfile *pP)
{
   struct stat st;
   int r= -1;
   if ((0 == stat(devPath, &st)) && S_ISCHR(st.st_mode)) // ensure device exists
   {
      pSC->fd= open(devPath, O_RDWR);
      if (pSC->fd >= 0)
      {
         r= ioctl(pSC->fd, SPI_IOC_RD_MAX_SPEED_HZ, &(pSC->maxClk));
         if (pP)
         {
            r= setProf(pSC->fd, pP);
            if (0x3 == r) { pSC->currProf= *pP; }
            else
            {
               WARN_CALL("(... %p) - profile fail (0x%X)\n", pP, r);
               getProf(pSC->fd, &(pSC->currProf));
            }
         }
         else { getDefaultProf(pSC->fd, &(pSC->currProf)); }
/*
         if (SPI_MODE_3 != (SPI_MODE_3 & inf.m32)) { inf.m32|= SPI_MODE_3; ++nc; }
         if (SPI_CS_HIGH == (SPI_CS_HIGH & inf.m32)) { inf.m32&= ~SPI_CS_HIGH; ++nc; }
         if (16 != inf.bpw) { inf.bpw= 16; ++nc; }
         if (nc > 0) { nc= setInfo(pSC->fd, &inf); LOG("set() - %d\n", nc); }
*/
         char s[96];
         LOG_CALL("() - maxClk= %G%cHz\n", sciFmtSetF(s, pSC->maxClk), s[0]);
         strProf(s, sizeof(s)-1, &(pSC->currProf)); LOG("\t%s\n", s);
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
   struct spi_ioc_transfer m;
   setTransProf(&m,&(pSC->currProf));
   m.rx_buf= (UL)r;
   m.tx_buf= (UL)w;
   m.len=   n;
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
   SPIProfile prof;
   int r= -1;

   argTrans(&gArgs, argc, argv);

   prof.kdmf= SPI_MODE_3 | SPI_CS_HIGH; // SPI_MODE_3=SPI_CPOL|SPI_CPHA
   prof.clk= 488E3;    // kernel driver mode flags, transaction clock rate
   prof.delay= 0;
   prof.bpw= 8;
   //if ((gArgs.flags & ARG_ACTION) &&
   if (lxSPIOpen(&gBusCtx, gArgs.devPath, &prof))
   {
      U8 rd[2]={0,0}, wr[2]={0xFA,0xFA};
      r= lxSPIReadWrite(&gBusCtx,rd,wr,2);
      LOG("lxSPIReadWrite() - %d : %X,%X\n", r, rd[0], rd[1]);

      lxSPIClose(&gBusCtx);
   }
   return(r);
} // main

#endif // LX_SPI_MAIN
