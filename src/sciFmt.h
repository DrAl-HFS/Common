// sciFmt.h - Scientific number format handling.
// https://github.com/DrAl-HFS/Common.git
// Licence: GPL V3
// (c) Project Contributors April 2019

#ifndef SCI_FMT_H
#define SCI_FMT_H

//define PERMIT_ELEC_FMT

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef double (*SciFmtFuncPtr) (char*,const double);


/***/

// Raw exponential notation units
extern double sciFmtDummyF (char *pCh, const double f); // { *pCh= ' '; return(s); }

// Convert positive floating point to standard scientific multiplier units.
// Generates single multiplier char e.g. 'm' (milli), 'G' (giga)
extern double sciFmtSetF (char *pCh, const double uf);
// Sign test & convert wrapper function
extern double sciFmtSetNegF (char *pCh, const double nf);

extern int sciFmtScanF64 (double *pF, const char s[], const int max);
extern int sciFmtScanF32 (float *pF, const char s[], const int max);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SCI_FMT_H
