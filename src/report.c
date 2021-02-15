// report.c - Filtered error/debug/log reporting.
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors  Feb 2018 - June 2019


#include "report.h"
#include <stdarg.h>
#include <errno.h>

/***/
// NB ensure power-of-two
#define REPORT_WIDTH_BYTES (1<<4)

/***/

typedef struct
{
   U16 filterMask[4];
} ReportCtx;

static ReportCtx gRC={ {0xFFFF,0xFFFF,0xFFFF,0xFFFF} };


/***/

static B32 notMasked (const U8 fcid, const U8 lnid)
{
   return ((fcid & FRCD) || (gRC.filterMask[fcid & ERR0] & (1 << lnid)));
} // masked

static B32 displayable (const U8 id)
{
   return((id >= DBG) || notMasked((id & REPORT_FCID_MASK) >> REPORT_FCID_SHIFT, (id & REPORT_LNID_MASK) >> REPORT_LNID_SHIFT));
} // displayable

static const char * getPrefix (const U8 fcid, const U8 lnid)
{
static const char *pfx[]=
{
   NULL,
   "TRACE: ",
   "WARNING: ",
   "ERROR: "
};
   if (fcid & NDNT) { return("\t"); }
   //else
   return( pfx[ fcid & ERR0 ] );
} // getPrefix

int report (const U8 id, const char fmt[], ...)
{
   int r=0;
   va_list args;
   if (id >= DBG)
   {  // NB: stdout & stderr are supposed to be macros (Cxx spec.) and so might
      // create safety/portability issues if not handled in the obvious way...
      va_start(args,fmt);
      if (OUT == id) { r+= vfprintf(stdout, fmt, args); }
      else { r+= vfprintf(stderr, fmt, args); }
      va_end(args);
   }
   else
   {
      const U8 fcid= (id & REPORT_FCID_MASK) >> REPORT_FCID_SHIFT;
      const U8 lnid= (id & REPORT_LNID_MASK) >> REPORT_LNID_SHIFT;
      if (notMasked(fcid,lnid))
      {
         const char *p= getPrefix(fcid,lnid);
         if (p) { r+= fprintf(stderr, "%s", p); }
         va_start(args,fmt);
         r+= vfprintf(stderr, fmt, args);
         va_end(args);
         if ((ERR0 == (fcid & ERR0)) && (0 != errno)) { fprintf(stderr, "\tERR#%d -> %s\n", errno, strerror(errno)); }
      }
   }
   //else user log/result files?
   return(r);
} // report

//bool FACTOR to where ?
int printable (const char c) { return((c > 0x1F) && (c < 0x7F)); }

int snputnc (char s[], const int m, const char c, int n)
{
   n= MIN(m,n);
   for (int i=0; i<n; i++) { s[i]= c; }
   return(n);
} // snputnc

int contigSet (int x)
{
   int c= 0;
   while (x)
   {
      c+= (0x3 == (x & 0x3));
      x>>= 1;
   }
   return(c);
} // contigSet

int reportBytes (const U8 id, const U8 *pB, int nB)
{
   const int maxl= 16;
   char chBuff[8*REPORT_WIDTH_BYTES];
   const int mCh= sizeof(chBuff)-1;
   int r= 0;
   if (displayable(id) && pB && (nB > 0))
   {
      int mP= 0, iB=0;
      do
      {
         int lB= MIN(nB-iB, REPORT_WIDTH_BYTES);
         int remB= nB - (iB+lB);
         int nCh= 0;
         chBuff[0]= 0;
         mP&= 0x1; // mask off all but last from preceding line
         for (int i= iB; i < (iB+lB); i++)
         {
            mP=  (mP << 1) | printable(pB[i]);
            nCh+= snprintf(chBuff+nCh, mCh-nCh, " %02x", pB[i]);
         }
         if (contigSet(mP))
         {
            int n= (((REPORT_WIDTH_BYTES - lB) * 3 + 7) >> 3); // NB: 8 space tab assumed
            nCh+= snputnc(chBuff+nCh, mCh-nCh, '\t', 1+n);
            for (int i= iB; i < (iB+lB); i++)
            {
               signed char ch= pB[i];
               if (!printable(ch)) { ch='.'; }
               nCh+= snputnc(chBuff+nCh, mCh-nCh, ch, 1);
            }
         }
         iB+= lB;
         if ((maxl > 2) && (remB > (REPORT_WIDTH_BYTES*maxl)))
         {  // head & tail for large dump
            int skip= remB - (2 * REPORT_WIDTH_BYTES);
            iB+= skip;
            nCh+= snprintf(chBuff+nCh, mCh-nCh, "\n... [%d] ...\n", skip);
         }
         else { nCh+= snprintf(chBuff+nCh, mCh-nCh, "\n"); } // if (rem > 0)
         //if (nCh >= mCh) ???
         r+= report(id, "%s", chBuff);
      } while (iB < nB);
   }
   return(r);
} // reportBytes

int reportN (const U8 id, const char *a[], int n, const char start[], const char sep[], const char *end)
{
   int i,r=0;
   if ((id >= DBG) && (n > 0))
   {
      if (OUT == id)
      {
         r= fprintf(stdout, "%s%s", start, a[0]);
         for (i=1; i<n; i++) { r+= fprintf(stdout, "%s%s", sep, a[i]); }
         if (end) { r+= fprintf(stdout, "%s", end); }
      }
      else
      {
         r= fprintf(stderr, "%s%s", start, a[0]);
         for (i=1; i<n; i++) { r+= fprintf(stderr, "%s%s", sep, a[i]); }
         if (end) { r+= fprintf(stderr, "%s", end); }
      }
   }
   return(r);
} // reportN


