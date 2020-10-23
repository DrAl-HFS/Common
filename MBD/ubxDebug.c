// Common/MBD/ubxDebug.c - u-blox protocol debug support
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors Oct 2020

#include "ubxDebug.h"


/***/


/***/

static const char *ubxClassStr (U8 c)
{
   switch(c)
   {
      case UBXM8_CL_NAV : return("NAV");
      case UBXM8_CL_RXM : return("RXM");
      case UBXM8_CL_INF : return("INF");
      case UBXM8_CL_ACK : return("ACK");
      case UBXM8_CL_CFG : return("CFG");
      case UBXM8_CL_UPD : return("UPD");
      case UBXM8_CL_MON : return("MON");
      case UBXM8_CL_AID : return("AID");
      case UBXM8_CL_TIM : return("TIM");
      case UBXM8_CL_ESF : return("ESF");
      case UBXM8_CL_MGA : return("MGA");
      case UBXM8_CL_LOG : return("LOG");
      case UBXM8_CL_SEC : return("SEC");
      case UBXM8_CL_HNR : return("HNR");
      case UBXM8_CL_NMEA : return("NMEA");
      case UBXM8_CL_PUBX : return("PUBX");
   }
   return(NULL);
} // ubxClassStr

static const char *ubxIDStr (U8 id)
{
   switch(id)
   {
      case UBXM8_ID_PRT : return("PRT");
      case UBXM8_ID_MSG : return("MSG");
      case UBXM8_ID_INF : return("INF");
      case UBXM8_ID_CFG : return("CFG");
   }
   return(NULL);
} // ubxIDStr

/***/

// Generate string table describing message
int ubxClassIDStrTab
(
   const char *r[],
   int maxR,
   char ch[],
   int maxCh,
   const U8 classID[2],
   const U8 *pP,
   const U16 lenP
)
{
   const char *s= ubxClassStr(classID[0]);
   int n=0;
   if (s && (n < maxR))
   {
      r[n++]= s;
      s= NULL;
      if (UBXM8_CL_CFG == classID[0]) { s= ubxIDStr(classID[1]); } // far from complete...
      else if (UBXM8_CL_ACK == classID[0])
      {
         if ((maxCh > 2) && (n < maxR))
         {
            ch[0]= ',';
            ch[1]= classID[1] +'0';
            ch[2]= 0;
            r[n++]= ch;
         }
         if (pP && (lenP > 0))
         {  // Ack payload is class-id header of command
            s= ubxClassStr(pP[0]);
            if (s && ((maxR - n) > 1)) { r[n++]= " : "; r[n++]= s; }
            s= NULL;
            if (lenP > 1) { s= ubxIDStr(pP[1]); }
         }
      }
      if (s)
      {
         if ((maxR-n) > 1) { r[n++]= ","; r[n++]= s; }
      }
      if ((n < maxR) && (n <= 3)) { r[n++]= " : "; }
   }
   return(n);
} // ubxClassIDStrTab

#define UBX_DUMP_STR_MAX 8
void ubxDumpPayloads (const U8 b[], FragBuff16 fb[], int nFB)
{
   for (int i=0; i<nFB; i++)
   {
      const U8 *pP= b + fb[i].offset;
      const UBXHeader *pH= (void*)(pP - sizeof(*pH));
      const char *s[UBX_DUMP_STR_MAX];
      char ch[8];
      int nS= ubxClassIDStrTab(s, UBX_DUMP_STR_MAX, ch, sizeof(ch), pH->classID, pP, fb[i].len);
      if (nS <= 0)
      {
         LOG("0x%02X,%02X : ", pH->classID[0], pH->classID[1]);
         reportBytes(OUT, pP, fb[i].len);
      }
      else
      {
         for (int i=0; i<nS; i++) { LOG("%s", s[i]); }
         if (0x3 == ubxHeaderMatch(pH, UBXM8_CL_CFG, UBXM8_ID_PRT))
         {
            ubxDissectCfgPrt((void*)pP, fb[i].len);
         }
         else if (nS <= 4) { reportBytes(OUT, pP, fb[i].len); } else { LOG("%s", "\n"); }
      }
   }
} // ubxDumpPayloads

