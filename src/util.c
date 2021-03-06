// util.c - miscellaneous utility code
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Feb 2018 - June 2019

#include "util.h"

#define GETTIME(a) gettimeofday(a,NULL)
#define USEC(t1,t2) (((t2).tv_sec-(t1).tv_sec)*1000000+((t2).tv_usec-(t1).tv_usec))

static const char gUMultChar[]=" KMGTEP";
#define MULT_CHAR_MAX (sizeof(gUMultChar)-2) // discount leading space and trailing nul

// Not sure how portable __WORDSIZE is...
#define SIZET_BITS __WORDSIZE


/***/

B32 validMemBuff (const MemBuff *pB, const size_t bytes)
{
   return(pB && pB->p && (pB->bytes >= bytes));
} // validMemBuff

B32 allocMemBuff (MemBuff *pB, const size_t bytes)
{
   if (pB)
   {
      void *p= malloc(bytes);
      if (p)
      {
         if (NULL != pB->p) { WARN_CALL("() - overwriting %p\n", pB->p);}
         pB->p= p; pB->bytes= bytes;
         return(TRUE);
      }
   }
   return(FALSE);
} // allocMemBuff

void releaseMemBuff (MemBuff *pB)
{
   if (pB && pB->p) { free(pB->p); pB->p= NULL; }
} // releaseMemBuff

B32 adjustMemBuff (MemBuff *pR, const MemBuff *pB, size_t skipS, size_t skipE)
{
   const size_t s= skipS + skipE;
   if (s < pB->bytes)
   {
      pR->w=      pB->w + skipS;
      pR->bytes=  pB->bytes - s;
   }
   return(s < pB->bytes);
} // adjustMemBuff

const char *stripPath (const char *path)
{
   if (path && *path)
   {
      const char *t= path, *l= path;
      do
      {
         path= t;
         while (('\\' == *t) || ('/' == *t) || (':' == *t)) { ++t; }
         if (t > path) { l= t; }
         t += (0 != *t);
      } while (*t);
      return(l);
   }
   return(NULL);
} // stripPath

// const char *extractPath (char s[], int m, const char *pfe) {} // extractPath

size_t fileSize (const char * const path)
{
   if (path && path[0])
   {
      struct stat sb={0};
      int r= stat(path, &sb);
      if (0 == r) { return(sb.st_size); }
   }
   return(0);
} // fileSize

size_t loadBuff (void * const pB, const char * const path, const size_t bytes)
{
   FILE *hF= fopen(path,"r");
   size_t r= 0;
   if (hF)
   {
      r= fread(pB, 1, bytes, hF);
      fclose(hF);
      if (bytes != r) { WARN_CALL("(%s,%zu) - %zu\n", path, bytes, r); }
   }
   return(r);
} // loadBuff

size_t saveBuff (const void * const pB, const char * const path, const size_t bytes)
{
   FILE *hF= fopen(path,"w");
   size_t r= 0;
   if (hF)
   {
      r= fwrite(pB, 1, bytes, hF);
      fclose(hF);
      if (bytes != r) { WARN_CALL("(%s,%zu) - %zu\n", path, bytes, r); }
   }
   return(r);
} // saveBuff

MBVal readBytesLE (const U8 * const pB, const size_t idx, const int nB)
{
   if (nB > 0)
   {
      MBVal v= pB[idx];
      U8 s= 8;
      for (int i=1; i<nB; i++) { v|= pB[idx+i] << s; s+= 8; }
      return(v);
   }
   return(0);
} // readBytesLE

MBVal writeBytesLE (U8 * const pB, const size_t idx, const int nB, const MBVal v)
{
   U8 s= 0;
   for (int i=0; i<nB; i++) { pB[idx+i]= v >> s; s+= 8; }
   return(v);
} // writeBytesLE

MBVal readBytesBE (const U8 * const pB, const size_t idx, const int nB)
{
   MBVal v=0;
   for (int i=0; i<nB; i++) { v|= pB[idx+i]; v<<= 8; }
   return(v);
} // readBytesBE

MBVal writeBytesBE (U8 * const pB, const size_t idx, const int nB, const MBVal v)
{
   U8 s= 0;
   for (int i=nB-1; i>=0; i--) { pB[idx+i]= v >> s; s+= 8; }
   return(v);
} // writeBytesBE

SMVal deltaT (void)
{
   static struct timeval t[2]={{0,},};
   static int i= 0;
   SMVal dt;
   GETTIME(t+i);
   dt= 1E-6 * USEC(t[i^1],t[i]);
   i^= 1;
   return(dt);
} // deltaT

void statMom1Add (StatMomD1R2 * const pS, const SMVal v)
{
   pS->m[0]+= 1;
   pS->m[1]+= v;
   pS->m[2]+= v * v;
} // statMom1Add

void statMom1AddW (StatMomD1R2 * const pS, const SMVal v, const SMVal w)
{
   pS->m[0]+= w;
   pS->m[1]+= v * w;
   pS->m[2]+= v * v * w;
} // statMom1AddW

void statMom3AddW (StatMomD3R2 * const pS, const SMVal x, const SMVal y, const SMVal z, const SMVal w)
{
   pS->m0+= w;
   pS->m1[0]+= x * w;
   pS->m1[1]+= y * w;
   pS->m1[2]+= z * w;
   pS->m2[0]+= x * x * w;
   pS->m2[1]+= y * y * w;
   pS->m2[2]+= z * z * w;
   pS->m2[3]+= x * y * w;
   pS->m2[4]+= x * z * w;
   pS->m2[5]+= y * x * w;
} // statMom3AddW

U32 statMom1Res1 (StatResD1R2 * const pR, const StatMomD1R2 * const pS, const SMVal dof)
{
   StatResD1R2 r={ 0, 0 };
   U32 o= 0;
   if (pS && (pS->m[0] > 0))
   {
      r.m= pS->m[1] / pS->m[0];
      ++o;
      if (pS->m[0] != dof)
      {
         //SMVal ess= (pS->m[1] * pS->m[1]) / pS->m[0];
         r.v= ( pS->m[2] - (pS->m[1] * r.m) ) / (pS->m[0] - dof);
         ++o;
      }
   }
   if (pR) { *pR= r; }
   return(o);
} // statMom1Res1

U32 statMom3Res1 (StatResD1R2 r[3], const StatMomD3R2 * const pS, const SMVal dof)
{
   U32 o= 0;
   if (pS && (pS->m0 > 0))
   {
      for (int i= 0; i < 3; i++) { r[i].m= pS->m1[i] / pS->m0; }
      ++o;
      if (pS->m0 != dof)
      {
         //SMVal ess= (pS->m[1] * pS->m[1]) / pS->m[0];
         for (int i= 0; i < 3; i++)
         {
            r[i].v= ( pS->m2[i] - (pS->m1[i] * r[i].m) ) / (pS->m0 - dof);
         }
         ++o;
      }
   }
   return(o);
} // statMom3Res1

float binSizeZ (char *pCh, const size_t s)
{
   int i=0;
   while ((i < MULT_CHAR_MAX) && (s > ((size_t)1 << (10 * (i+1))))) { ++i; }
   *pCh= gUMultChar[i];
   return( (float)s / (1 << (10 * i)) );
} // binSizeZ

float decSizeZ (char *pCh, size_t s)
{
   size_t m=1;
   int i=0;
   while ((i < MULT_CHAR_MAX) && (s > (1000 * m))) { ++i; m*= 1000; }
   *pCh= gUMultChar[i];
   return( (float)s / m );
} // decSizeZ

U32 bitCountZ (size_t u)
{
static const U8 n4b8[]=
{
   0, 1, 1, 2, // 0000 0001 0010 0011
   1, 2, 2, 3, // 0100 0101 0110 0111
   1, 2, 2, 3, // 1000 1001 1010 1011
   2, 3, 3, 4  // 1100 1101 1110 1111
};
//static const U64 n4b4=0x4332322132212110; // c+= 0xF & (n4b4 >> ((u & 0xF)<<2) );
   U32   c=0;

   do
   {
      c+= n4b8[ u & 0xF ] + n4b8[ (u >> 4) & 0xF ] + n4b8[ (u >> 8) & 0xF ] + n4b8[ (u >> 12) & 0xF ];
      u>>=  16;
   } while (u > 0);
   return(c);
} // bitCountZ

I32 bitNumHiZ (size_t u)
{
static const I8 h4b8[]=
{  // NB: bit numbers biased +1
   0, 1, 2, 2, // 0000 0001 0010 0011
   3, 3, 3, 3, // 0100 0101 0110 0111
   4, 4, 4, 4, // 1000 1001 1010 1011
   4, 4, 4, 4  // 1100 1101 1110 1111
};
   I32   h=-1;

   if (0 != u)
   {
      register size_t t;

      //if (sizeof(t) >= 8) {
#if SIZET_BITS >= 64
      if (0 != (t= (u >> 32))) { h+= 32; u= t; }
#endif // SIZET_BITS
      if (0 != (t= (u >> 16))) { h+= 16; u= t; }
      if (0 != (t= (u >> 8))) { h+= 8; u= t; }
      if (0 != (t= (u >> 4))) { h+= 4; u= t; }
      h+= h4b8[u];
   }
   return(h);
} // bitNumHiZ

U32 bitsReqI32 (I32 i)
{
   I32 w;
   if (i < 0) { w= bitNumHiZ(-i); }
   else { w= bitNumHiZ(i); }
   return MAX(1,1+w);
} // bitsReqI32

size_t bitCountNU32 (U32 u[], const int n)
{
   size_t t= 0;
   for (int i= 0; i<n; i++) { t+= BIT_COUNT_Z(u[i]); }
   return(t);
} // bitCountNU32

extern int strFmtNSMV (char s[], const int maxS, const char *fmt, const SMVal v[], const int n)
{
   int i=0, nS=0;
   while ((nS < maxS) && (i < n)) { nS+= snprintf(s+nS, maxS-nS, fmt, v[i]); ++i; }
   return(nS);
} // strFmtNSMV

extern SMVal sumNSMV (const SMVal v[], const size_t n)
{
   SMVal s=0;
   for (size_t i= 0; i<n; i++) { s+= v[i]; }
   return(s);
} // sumNSSMV

extern SMVal meanNSMV (const SMVal v[], const size_t n)
{
   if (n > 0) return( sumNSMV(v, n) / n );
   //else
   return(0);
} // meanNSMV

F64 lcombF64 (const F64 x0, const F64 x1, const F64 r0, const F64 r1) { return(x0 * r0 + x1 * r1); }
F64 lerpF64 (const F64 x0, const F64 x1, const F64 r) { return(x0 + r * (x1 - x0)); } // lcombF64(x0, x1, 1.0-r, r); }

U8 bitsValZ (const size_t v)
{
   const float a= 1.0 - ( 1.0 / ((size_t)1 << (SIZET_BITS-1)) );
   //float f= log2f(v);
   //U8 r= f + a;
   //DBG_CALL("() - log2(%u)=%G -> %G -> %u\n", v, f, f+a, r);
   return(log2f(v) + a);
} // bitsValZ

//ifdef UTIL_TEST

int utilSanityCheck (void)
{
#if 0
   char c=0;
   short s= 0;
   int i= 0;
   long l= 0;
   long long ll=0;
   LOG_CALL("() - sizeof: char=%d short=%d int=%d long=%d long long=%d\n",sizeof(c),sizeof(s),sizeof(i),sizeof(l),sizeof(ll));
#endif
#if 0
   size_t t=0;
   printf("utilTest() - bit*:\n");
   for (int i=0; i<32; i++)
   {
      LOG("%d 0x%x %d %u %u\n", i, t, bitNumHiZ(t), bitsReqI32(t), bitsReqI32(-t));
      t= 1<<i;
   }
#endif
   union { long i; U8 u[4]; } sz;

   sz.u[0]= (sizeof(short) << 4) | sizeof(char);
   sz.u[1]= (sizeof(long) << 4) | sizeof(int);
   sz.u[2]= (sizeof(long long) << 4) | sizeof(size_t);
   LOG_CALL("() - sizeof(ll,sz,l,i,s,c) -> 0x%X\n", sz.i);
   return(sz.i);
} // utilTest

//endif

#ifdef UTIL_MAIN

int main (int argc, char *argv[])
{
   utilSanityCheck();
   return
} // utilTest

#endif
