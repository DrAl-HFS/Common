// Common/MBD/ads1xTxtIF.h - text conversion interfaces for TI I2C ADC devices (ADS1xxx series)
// https://github.com/DrAl-HFS/Common.git
// Licence: AGPL3
// (c) Project Contributors Feb 2022

#ifndef ADS1X_TXT_IF_H
#define ADS1X_TXT_IF_H

#include "ads1xUtil.h"


/***/

int muxMapFromA (U8 m[], int mMax, const char *s, const U8 hwID);


#endif // ADS1X_TXT_IF_H
