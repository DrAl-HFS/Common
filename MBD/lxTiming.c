// Common/MBD/lxTiming.c - timing utils for Linux
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Feb 2018 - Sept 2020

#include <assert.h>
//#include <signal.h>

#include "lxTiming.h"

/***/

#define CLOCK_GRANULARITY_NS (500)  // nano, worst case ? typical <500ns
#define SS_CLOCK_BIAS_NS (50)       // account for typical overheads
#define ITIMER_GRANULARITY (4)      // micro
#define USLEEP_GRANULARITY (2000)   // micro

#define MILLI_TICKS  (1000)
#define MICRO_TICKS  (MILLI_TICKS * 1000)
#define NANO_TICKS   (MICRO_TICKS * 1000)

#define DUSECF(t1,t2) (((t2).tv_sec-(t1).tv_sec) + 1E-6*((t2).tv_usec-(t1).tv_usec))
#define DNSECF(t1,t2) (((t2).tv_sec-(t1).tv_sec) + 1E-9*((t2).tv_nsec-(t1).tv_nsec))

/***/
/*
// DEPRECATED: itimer stuff (high latency)

int gSigCount[2]={0,0};
void alarmHandler (int a) { gSigCount[(SIGALRM==a)]+= 1; }

int initTimer (itimerval *pT, F32 sec)
{
   void *p= signal(SIGALRM,alarmHandler); // set up alarm handler to replace <itimer> default action (SigTerm)
   LOG_CALL("(%G) - %p\n", sec, p);
   if (pT && sec > 0)
   {
      pT->tLast.it_value.tv_sec= sec;  // truncate
      pT->tLast.it_value.tv_usec= 1E6 * (sec - pT->tLast.it_value.tv_sec);
      pT->tLast.it_interval=   pT->tLast.it_value;
      return setitimer(ITIMER_REAL, &(pT->tLast), NULL); // >= 0);
   }
   return(0);
} // initTimer

int usSpinSleep (int us)
{
   if (us >= ITIMER_GRANULARITY)
   {
      struct itimerval ivl= { .it_value.tv_sec= 0, .it_value.tv_usec= us };
      //ivl.it_value.tv_sec= us / 1000000;
      //ivl.it_value.tv_usec= us % 1000000;
      int r= setitimer(ITIMER_REAL, &ivl, NULL);
      if (r >= 0)
      {
         if (us >= USLEEP_GRANULARITY) { usleep(us - 500); } // Minimise wasted clock cycles
         do // spin (system calls)
         {
            r= getitimer(ITIMER_REAL, &ivl);
         } while ((r >= 0) && (ivl.it_value.tv_usec >= ITIMER_GRANULARITY));
         us= ivl.it_value.tv_usec;
      }
   }
   return(us);
} // usSpinSleep

#ifdef MBD_RPI_VC4 // Broadcom VideoCore IV timestamp register(s) mapped into (root-only) process address space
#include "rpiUtil.h"
void rpivcts (void)
{
   uint64_t ts0, ts1;
   if (vcAcquire(-1))
   {
      ts0= vcTimestamp();
      ts1= vcTimestamp();
      printf("dt=%d\n",(int)(ts1-ts0));
   }
} //
#endif // MBD_RPI_VC4

// Evaluate other timing mechanisms that should provide better-than-millisecond accuracy
//#include <sys/time.h>
#if 0
#define USEC(t1,t2) (((t2).tv_sec-(t1).tv_sec)*1000000+((t2).tv_usec-(t1).tv_usec))

 // not functional
{  // Process clock
   // CLOCK_REALTIME CLOCK_MONOTONIC
   timer_t hT=NULL;
   struct sigevent ev={SIGEV_NONE, };
   struct itimerspec ts0={0}, ts1={0};

   r= timer_create(CLOCK_REALTIME, &ev, &hT);
   printf("r=%d %p\n", r, hT);
   if (0 == r)
   {
      timer_gettime(hT, &ts0);
      printf("%ld : %ld (%ld : %ld)\n", ts0.it_value.tv_sec, ts0.it_value.tv_nsec, ts0.it_interval.tv_sec, ts0.it_interval.tv_nsec);
      //timer_settime(t, int flags, const struct itimerspec *new_value, struct itimerspec * old_value);
      timer_gettime(hT, &ts1);
      printf("%ld : %ld (%ld : %ld)\n", ts1.it_value.tv_sec, ts1.it_value.tv_nsec, ts1.it_interval.tv_sec, ts1.it_interval.tv_nsec);
   }
}
*/

F32 timeNow (RawTimeStamp *pNow)
{
   if (clock_gettime(CLOCK_REALTIME, pNow) >= 0)
   {
      return(pNow->tv_sec + 1E-9 * pNow->tv_nsec);
   }
   return(-1);
} // timeNow

F32 timeElapsed (RawTimeStamp *pLast)
{
   RawTimeStamp now;
   F32 dt=-1;
   if (clock_gettime(CLOCK_REALTIME, &now) >= 0)
   {
      dt= DNSECF(*pLast, now);
      *pLast= now;
   }
   return(dt);
} // elapsedTime

long timeSpinSleep (long nanoSec)
{
   assert(nanoSec < (2*NANO_TICKS));  // 32bit long overflow check
   if (nanoSec > CLOCK_GRANULARITY_NS)
   {
      struct timespec ts[2];
      int r= clock_gettime(CLOCK_REALTIME, ts+0);
      if (r >= 0)
      {  // set target
         ts[1].tv_sec= ts[0].tv_sec;
         ts[1].tv_nsec= ts[0].tv_nsec + nanoSec; // SS_CLOCK_BIAS_NS);
         if (ts[1].tv_nsec > NANO_TICKS)
         {  // correction needed
            ts[1].tv_sec+= ts[1].tv_nsec / NANO_TICKS;
            ts[1].tv_nsec %= NANO_TICKS;
         }
         // Minimise wasted system clock cycles.
         // NB: underestimation of sleep interval to minimise variability (significant overruns likely)
         if (nanoSec > (USLEEP_GRANULARITY*1000)) { usleep((nanoSec >> 10) - 500); }

         do // Now spin (system calls) to meet target
         {
            r= clock_gettime(CLOCK_REALTIME, ts+0);
         } while ((r >= 0) && ((ts[1].tv_sec > ts[0].tv_sec) || (ts[1].tv_nsec > ts[0].tv_nsec))); // NB: relying on lazy eval

         if ((r >= 0) && (ts[1].tv_sec == ts[0].tv_sec))
         {
            return(ts[1].tv_nsec - ts[0].tv_nsec); // -ve overrun, or (v.unlikely) +ve time remaining
         }
      }
   }
   return(nanoSec);
} // timeSpinSleep

#ifdef LX_TIMING_MAIN

int sumNI (int v[], const int n) { if (n > 0) { int t= v[0]; for (int i=1; i<n; i++) { t+= v[i]; } return(t); } return(0); }
float rcpI (int x) { if (0 != x) { return(1.0 / (float)x); } else return(0); }
float sumNF (float v[], const int n) { if (n > 0) { float t= v[0]; for (int i=1; i<n; i++) { t+= v[i]; } return(t); } return(0); }
float rcpF (float x) { if (0 != x) { return(1.0 / x); } else return(0); }

void th1 (int us)
{
   int r, dtus[100];
   struct itimerval ivl;

   LOG_CALL("(%dus)\n",us);
   ivl.it_value.tv_sec= 0;
   ivl.it_value.tv_usec= 100000;
   ivl.it_interval= ivl.it_value;
   r= setitimer(ITIMER_REAL, &ivl, NULL);
   if (r >= 0)
   {
      int n=0, last;
      r= getitimer(ITIMER_REAL, &ivl); last= ivl.it_value.tv_usec;
      while ((r >= 0) && (n < 100))
      {
         //spinSleep(us); // can't reuse ITIMER_REAL ...
         r= getitimer(ITIMER_REAL, &ivl);
         dtus[n++]= last - ivl.it_value.tv_usec; last= ivl.it_value.tv_usec;
      }
      LOG("th1: [%d]: mean=%G\n", n, rcpI(n) * sumNI(dtus,n));
      // for (int i=0; i<n; i++) { LOG("\t%d", dtus[i]); }
      LOG("%s\n","---");
   } else { ERROR_CALL(" : setitimer() - %d\n", r); }
} // th1

void th2 (int us)
{
   struct timespec ts[2];
   int r,n=0;
   F32 dt[100];
   r= clock_gettime(CLOCK_REALTIME, ts+1);
   while ((r >= 0) && (n < 100))
   {
      int i= n&1;
      //spinSleep(us);
      r= clock_gettime(CLOCK_REALTIME, ts+i);
      dt[n++]= DNSECF(ts[i^1],ts[i]);
   }
   LOG("th2: [%d]: mean=%G\n", n, rcpF(n) * sumNF(dt,n));
   // for (int i=0; i<n; i++) { LOG("\t%d", dtus[i]); }
   LOG("%s\n","---");
} // th2

int timerTestHacks (int us)
{
   //initTimer(NULL,0);
   RawTimeStamp iot;

   F32 tf[10];
   tf[0]= timeNow(&iot);
   for (int i=1; i<10; i++) { tf[i]= timeElapsed(&iot); }
   LOG("timeNow: %G\n", tf[0]);
   for (int i=1; i<10; i++) { LOG("[%d] : %G\n", i, tf[i]); }

   signal(SIGALRM,SIG_IGN); // <itimer> signal ignore (prevent default <SigTerm>)
   th1(us);
   LOG("Sig=%d,%d\n%s\n",gSigCount[0],gSigCount[1],"***");
   th2(us);

   long tl[10], ns= us*1000;
   LOG("nsSpinSleep(%d) sizeof(long)=%d\n", ns, sizeof(long));
   for (int i=0; i<10; i++) { tl[i]= timeSpinSleep(ns); }
   for (int i=0; i<10; i++) { LOG("[%d] : %d\n", i, tl[i]); }
} // timerTestHacks

int main (int argc, char *argv[])
{
   return timerTestHacks(2500);
} // main

#endif // LX_TIMING_MAIN


