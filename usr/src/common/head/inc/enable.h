/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/enable.h	1.3"
#ifndef __ENABLE_H__
#define __ENABLE_H__


/*
    INCLUDE FILES
*/

# include <stdarg.h>

#ifndef INT8
#define INT8  char
#define INT16 int
#define INT32 long
#endif

#ifndef UINT8
#define UINT8  unsigned char
#define UINT16 unsigned short
#define UINT32 unsigned long
#endif

#include "npackon.h"

typedef struct _VALUE
{
   unsigned char mantissa[8];
   unsigned int exponent : 15;
   unsigned int sign : 1;
} VALUE;

#define MAX_NUMBER  64
#define NUMBER_VECTORS 4

/*====================================================================
   typedef VECTOR
====================================================================*/

typedef struct
{
   char lowValue;
   char highValue;
} VECTOR;

/*====================================================================
   typedef NWDOS_EXT_COUNTRY_TABLE
====================================================================*/

typedef struct
{
   unsigned char  infoID;
#if (defined N_PLAT_UNIX || defined WIN32)
   char *countryTable;
#else
	char far *countryTable;
#endif
} NWDOS_EXT_COUNTRY_TABLE;

/*====================================================================
   typedef NWEXTENDED_COUNTRY
====================================================================*/

typedef struct
{
   unsigned char infoID;
   unsigned int  size;
   /* The following is common to DOS 3.3 and OS/2  */
   unsigned int  countryID;
   unsigned int  codePage;
   /* The next four fields are valid for DOS 3.0 and above */
   /* The next four fields are the only fields valid for DOS 2.1 */
   unsigned int  dateFormat;
   char    currencySymbol[5];
   char    thousandSeparator[2];
   char    decimalSeparator[2];
   /* The following fields are valid for DOS 3.0 and above */
   char    dateSeparator[2];
   char    timeSeparator[2];
   char    currencyFormatFlags;
   char    digitsInCurrency;
   char    timeFormat;
#if (defined N_PLAT_UNIX || defined WIN32)
   void    (*UpperCase)();
#else
   void    (far *UpperCase)();
#endif
   char    dataListSeparator[2];
   char    PAD[10];
} NWEXTENDED_COUNTRY;

/*===================================================================
   *typedef NWLOCALE_DEFS*
====================================================================*/

typedef struct
{
   char    _collateTable[256];
   int     _countryID;
   int     _codePage;
   int     _dateFormat;
   char    _currencySymbol[5];
   char    _thousandSeparator[2];
   char    _decimalSeparator[2];
   char    _dateSeparator[2];
   char    _timeSeparator[2];
   char    _currencyFormatFlags;
   char    _digitsInCurrency;
   int     _timeFormat;
   char    _dataListSeparator[2];
   char    _am[MERIDLEN];
   char    _pm[MERIDLEN];
} NWLOCALE_DEFS;

extern NWLOCALE_DEFS localeInfo;

/*===================================================================
** DATA
===================================================================*/

#if !defined LOCALE_DATA_FILE
#   if defined (NWDOS) || defined (NWWIN)
    extern unsigned int  DosMajorVersion;
    extern unsigned int  DosMinorVersion;
#   endif
    extern VECTOR _DBCSVector[NUMBER_VECTORS];
#endif

/*
** MACROS
*/
#define IF_DOUBLE_BYTE  _DBCSVector[0].lowValue
#define L_MB_CUR_MAX    (_DBCSVector[0].lowValue == 0) ? 1 : 2

#define _NWMBCompare(c1,c2) \
   (int) ((NWCharType(*c1) == NWDOUBLE_BYTE && *c1 == *c2) ? \
         *(c1+1) - *(c2+1) : *c1-*c2)

#define _NWCompareWeight(c1,c2) \
   (localeInfo._collateTable[c1]) - (localeInfo._collateTable[c2])

#define _NWIncrement(s,x) ((_DBCSVector[0].lowValue == 0) ? \
             (s + x) : NWIncrement(s,x))

/*****************************************************************************
 * INTERNAL PROTOTYPES
 ****************************************************************************/

N_GLOBAL_LIBRARY(int) _NWGetDBCSVector ( VECTOR N_FAR *vector,
                              size_t maxsize );

N_GLOBAL_LIBRARY(int) _NWGetUpperCaseTable ( unsigned char N_FAR *upperCaseTable );

N_GLOBAL_LIBRARY(int) _mantissa_to_decimal (char N_FAR *buffer, VALUE tmp );

N_GLOBAL_LIBRARY(void) _NWDoubleToExtended (void *buffer, void *src);

#if (defined N_PLAT_UNIX || defined WIN32)
int N_API _NWGetCountryInfo ( LCONV *countryInfo );
#else
N_GLOBAL_LIBRARY(int) _NWGetCountryInfo ( NWEXTENDED_COUNTRY N_FAR *countryInfo );
#endif

char N_FAR * N_API _Increment ( char N_FAR *string,
                                size_t numChars );

LCONV N_FAR * N_API _Llocaleconv (void);

void ReorderPrintfParameters(
         char N_FAR * N_FAR *format,
	 char *newFormat,
         unsigned int N_FAR *parms);

void WReorderPrintfParameters(
         char N_FAR * N_FAR *format,
	 char *newFormat,
         unsigned int N_FAR *parms);

void PrintDigit(nuint32 c, nuint8 N_FAR *buffer);

nuint32 PrintNumber(nuint32 number, nuint32 base,
	nuint8 N_FAR *buffer);

void ProcessFieldInfo(nuint8 N_FAR *N_FAR *format, nuint32 N_FAR *width,
	nuint32 N_FAR *precision, nuint32 N_FAR *flags);

nuint32 PercentFormat(nuint8 N_FAR *format, va_list N_FAR *parms,
   void (N_FAR *output)(char ch, nuint8 N_FAR * N_FAR * outparms, int N_FAR *count),
	void N_FAR *outparms,
   int N_FAR *count);

/**************************** Assembly Routines *************************/

#if (defined(NWDOS) || defined(NWWIN))
int far _NWGetMSDOSCollateTable ( UINT8 far *collateTable, int size );

int far _NWGetMSDOSUpperTable (NWDOS_EXT_COUNTRY_TABLE far *upperCaseTable);

int far _NWGetCountryInfo38 ( void far *countryInfo,
                              unsigned int far *countryID );

int far _NWGetCountryInfo65 ( NWEXTENDED_COUNTRY far *countryInfo );

int far _NWGetDOSVersion ( unsigned int far *major,
                           unsigned int far *minor );
#endif

#include "npackoff.h"

#endif /* __ENABLE_H__ */
