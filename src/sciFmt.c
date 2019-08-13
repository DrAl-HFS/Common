// sciFmt.c - Scientific number format handling.
// https://github.com/DrAl-HFS/Common.git
// (c) Project Contributors April 2019

#include "sciFmt.h"

// Enable GNU extensions if needed (exp10 etc. not avilable by default)
#ifdef __GNUC__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#undef _MATH_H
#include <math.h>
#endif // _GNU_SOURCE
#endif // __GNUC_

static const char gSFMultChar[]="yzafpnum kMGTPEZY";
#define MULT_CHAR_IDX0 8   // central space
#define MULT_CHAR_MAX (sizeof(gSFMultChar)-2) // discount central space and trailing nul

// NB: 'E' for exa interpreted as exponential by library routines so not
// supported in scanning routines - could support using (case independant)
// spelled versions or use integer library routines and assemble floating point explicitly...
#define MC_SCAN_MAX  (MULT_CHAR_MAX-3) // discount central space and trailing nul

//#define PERMIT_ELEC_FMT
// Allow 'K' (non-SI) as synonym for 'k'
#define PERMIT_K


/***/

int findIdxChN (const char c, const char s[], const int max)
{
   int i= 0;
   while ((c != s[i]) && (i < max)) { ++i; }
   return(i);
} // findIdxChN

double sciFmtDummyF (char *pCh, const double f) { *pCh= ' '; return(f); }

double sciFmtSetF (char *pCh, const double uf)
{
   float m=1;
   int i=MULT_CHAR_IDX0;
   if (uf > 1)
   {
      if (uf < 1E30)
      {
         while ((i < MULT_CHAR_MAX) && (uf > (1000 * m))) { ++i; m*= 1000; }
      }
   }
   else if (uf > 0)
   {
      if (uf > 1E-30)
      {
         while ((i > 0) && (uf < (m * 0.099))) { --i; m*= 0.001; }
      }
   }
   *pCh= gSFMultChar[i];
   return( uf / m );
} // sciFmtSetF

double sciFmtSetNegF (char *pCh, const double nf)
{
   if (nf < 0) { return -sciFmtSetF(pCh, -nf); }
   //else
   return sciFmtSetF(pCh, nf);
} // sciFmtSetNegF

int sciFmtScanF64 (double *pF, const char s[], const int max)
{
   int n= 0;

   if (max > 0) // && (NULL != s))
   {
      const char *pE= s;
      double f= strtod(s, (char**)&pE);

      n= pE - s;
      if ((n > 0) && (n < max))
      {
         int i;
#ifdef PERMIT_K
         if ('K' == s[n]) { i= MULT_CHAR_IDX0+1; } else
#endif
         { i= findIdxChN(s[n], gSFMultChar, 1+MC_SCAN_MAX); }
         if ((i <= MC_SCAN_MAX) && (MULT_CHAR_IDX0 != i))
         {
#ifdef PERMIT_ELEC_FMT
            register int m= n++;
            if (findIdxChN('.', s, m) >= m)
            { // no decimal point scanned, allow "10k4" etc.
               int n1;
               double f1;

               pE= s + n;
               f1= strtod(s+n, (char**)&pE);
               n1= pE - (s + n);
               if ((n1 > 0) && (f1 > 1) && (findIdxChN('.', s+n, n1) >= n1))
               {
                  n+= n1;
                  f1*= exp10(-n1);
                  if (f >= 0) { f+= f1; } else { f-= f1; }
               }
            }
#endif // PERMIT_ELEC_FMT
            f*= exp10(3*(i-MULT_CHAR_IDX0));
         }
         if (pF) { *pF= f; }
      }
   }
   return(n);
} // sciFmtScanF64

int sciFmtScanF32 (float *pF32, const char s[], const int max)
{
   double f64=0;
   int n= sciFmtScanF64(&f64, s, max);
   if (pF32 && (n > 0)) { *pF32= f64; }
   return(n);
} // sciFmtScanF32

