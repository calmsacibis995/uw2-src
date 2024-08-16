/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwlocale.h	1.5"
#ifndef __NWLOCALE_H__

#include "ntypes.h"
#include "npackon.h"

/*----------------------------------------------------------------------------*
 *                                                                            *
 *      This include file defines the constants, types, structures,           *
 *      and prototypes for the Novell Enabling APIs.                          *
 *                                                                            *
 *      (c) Copyright. 1989-1991 Novell, Inc.  All rights reserved.           *
 *                                                                            *
 *----------------------------------------------------------------------------*/


#ifndef NWCCODE
#define NWCCODE           unsigned int
#endif

#if (defined WIN32 || defined N_PLAT_UNIX)
#define NUMBER_TYPE double
#else
#define NUMBER_TYPE long double
#endif

#if !defined (_SIZE_T_DEFINED) && !defined (_SIZE_T) && !defined (_SIZE_T_DEFINED_)
typedef unsigned int size_t;
# define _SIZE_T
#endif

/* (in imitation of stdlib.h) */

#define L_MB_LEN_MAX       2   /* (in imitation of limits.h) */
#define MERIDLEN           5
#define NWSINGLE_BYTE      1
#define NWDOUBLE_BYTE      2

#define _UCHAR             (unsigned char)

#ifndef NLC_ALL
# define NLC_ALL            0
#endif
#ifndef LC_ALL
# define LC_ALL NLC_ALL
#endif

#ifndef NLC_COLLATE
# define NLC_COLLATE        1
#endif
#ifndef LC_COLLATE
# define LC_COLLATE NLC_COLLATE
#endif

#ifndef NLC_CTYPE
# define NLC_CTYPE          2
#endif
#ifndef LC_CTYPE
# define LC_CTYPE NLC_CTYPE
#endif

#ifndef NLC_MONETARY
# define NLC_MONETARY       3
#endif
#ifndef LC_MONETARY
# define LC_MONETARY NLC_MONETARY
#endif

#ifndef NLC_NUMERIC
# define NLC_NUMERIC        4
#endif
#ifndef LC_NUMERIC
# define LC_NUMERIC NLC_NUMERIC
#endif

#ifndef NLC_TIME
# define NLC_TIME           5
#endif
#ifndef LC_TIME
# define LC_TIME NLC_TIME
#endif


/*====================================================================
   country definitions
====================================================================*/
#define ARABIC            785
#define WBAHRAIN          973
#define WCYPRUS           357 /* ??? */
#define WEGYPT             20
#define WETHIOPIA         251
#define WIRAN              98
#define WIRAQ             964
#define WJORDAN           962
#define WKUWAIT           965
#define WLIBYA            218
#define WMALTA            356 /* ??? */
#define WMOROCCO          212 /* SHOULD THIS BE FRENCH?? */
#define WPAKISTAN          92
#define WQATAR            974 /* ??? */
#define WSAUDI            966
#define WTANZANIA         255 /* ??? */
#define WTUNISIA          216 /* ??? */
#define WTURKEY            90 /* ??? */
#define WUAE              971
#define WYEMEN            967 /* ??? */
#define AUSTRALIA          61
#define BELGIUM            32
#define CANADA_FR           2
#define CANADA              2
#define DENMARK            45
#define FINLAND           358
#define FRANCE             33
#define GERMANY            49
#define GERMANYE           37
#define HEBREW            972
#define IRELAND			  353
#define ITALY              39
#define LATIN_AMERICA       3
#define WARGENTINA         54
#define WBOLIVIA          591
#define WCHILE             56
#define WCOLOMBIA          57
#define WCOSTARICA        506
#define WECUADOR          593
#define WELSALVADOR       503
#define WGUATEMALA        502
#define WHONDURAS         504
#define WMEXICO            52
#define WNICARAGUA        505
#define WPANAMA           507
#define WPARAGUAY         595
#define WPERU              51
#define WURUGUAY          598
#define WVENEZUELA         58
#define NETHERLANDS        31
#define NORWAY             47
#define PORTUGAL          351
#define SPAIN              34
#define SWEDEN             46
#define SWITZERLAND        41
#define UK                 44
#define USA                 1
#define JAPAN              81
#define KOREA              82
#define PRC                86
#define TAIWAN             886	/* This one for DOS */
#define WTAIWAN				886	/* This one for Windows */
#define ASIAN_ENGLISH      99
#define NEWZEALAND			64

/*====================================================================
   typedef Llconv
====================================================================*/
typedef struct Llconv
   {
   char decimal_point[4];     /* non-monetary decimal point */
   char thousands_sep[4];     /* non-monetary separator for digits
                                   left of the decimal-point */
   char grouping[4];          /* String indicating size of groups
                                   of digits*/
   /*
    The international currency symbol applicable to
    the current locale.  The first three characters
    contain the alphabetic international currency
    symbol in accordance with those specified in ISO
    4217 "codes for the representation of currency
    and funds." The fourth character is the character
    used to separate the international currency
    symbol from the monetary quantity.
   */
   char int_curr_symbol[8];
   char currency_symbol[4];   /* Currency symbol for current locale */
   char mon_decimal_point[4]; /* monetary decimal point */
   char mon_thousands_sep[4]; /* monetary separator for digits left
                                   of the decimal-point */
   char mon_grouping[8];      /* String indicating size of
                                   groups of digits */
   char positive_sign[4];     /* String indicating positive
                                   monetary value */
   char negative_sign[4];     /* String indicating negative
                                   monetary value */
   char int_frac_digits;      /* Num of fractional digits in
                                   monetary display */
   char frac_digits;          /* Num of fractional digits in
                                   non-monetary display*/
   char p_cs_precedes;        /* 1=precede or 0=succeeds
                                   (pos currency symbol)*/
   char p_sep_by_space;       /* 1=space separator or
                                 0=no space separator
                                   (currency symbol) */
   char n_cs_precedes;        /* location of currency_symbol
                                   for neg monetary quantity */
   char n_sep_by_space;       /* separation of currency symbol
                                   in neg monetary quantity */
   char p_sign_posn;          /* value indicating position of
                                   positive_sign for positive
                                   monetary quantity */
   char n_sign_posn;          /* value indicating position of
                                   negative_sign for negative
                                   monetary quantity.*/

   /* Novell Additions to the ANSI definition:*/
   nint16	code_page;
   nint16	country_id;
   char 		data_list_separator[2];
   char		date_separator[2];
   char		time_separator[2];
   char		time_format;
   nint16	date_format;
	char		am[MERIDLEN];
	char		pm[MERIDLEN];
   char		reserved[40];
} LCONV;

/*====================================================================
   *function prototypes*
====================================================================*/
#if defined(__cplusplus)
extern "C" {
#endif

LCONV N_FAR * N_API NWLlocaleconv(
         LCONV N_FAR *lconvPtr);

int N_API NWLmblen(
         char N_FAR *string,
         size_t maxBytes);

char  N_FAR * N_API NWLsetlocale(
         int category,
         const char N_FAR *locale);

char  N_FAR * N_API NWLstrchr(
         char N_FAR *string,
         int find);

/* NWLstrcoll  (see below) */

size_t N_API NWLstrcspn(
         const char N_FAR *string1,
         const char N_FAR *string2);

#ifndef NWL_EXCLUDE_TIME
size_t N_API NWLstrftime(
         char N_FAR *string,
         size_t maxSize,
         char N_FAR *format,
         struct tm N_FAR *timePtr);
#endif

char  N_FAR * N_API NWLstrpbrk(
         char N_FAR *string1,
         const char N_FAR *string2);

char  N_FAR * N_API NWLstrrchr(
         char N_FAR *string,
         int find);

char  N_FAR * N_API NWLstrrev(
         char N_FAR *string1,
         char N_FAR *string2);

size_t N_API NWLstrspn(
         const char N_FAR *string1,
         const char N_FAR *string2);

char  N_FAR * N_API NWLstrstr(
         char N_FAR *string,
         char N_FAR *searchString);

/* NWLstrupr ( see below )*/

char  N_FAR * N_API NWIncrement(
         char N_FAR *string,
         size_t numChars);

char  N_FAR * N_API NWstrImoney(
         char N_FAR *buffer,
         NUMBER_TYPE Value);

char  N_FAR * N_API NWstrmoney(
         char N_FAR *buffer,
         NUMBER_TYPE Value);

int   N_API NWstrncoll(
         char N_FAR *string1,
         char N_FAR *string2,
         size_t maxBytes);

char   N_FAR * N_API NWstrncpy(
         char N_FAR *target_string,
         char N_FAR *source_string,
         int numChars);

char   N_FAR * N_API NWLstrbcpy(
         char N_FAR *target_string,
         char N_FAR *source_string,
         int numBytes);

char  N_FAR * N_API NWstrnum(
         char N_FAR *buffer,
         NUMBER_TYPE Value);

int   N_API NWstrlen(char N_FAR *str);

N_GLOBAL_LIBRARY( nint )
NWLTruncateString
(
   pnchar8 pStr,
   nint  iMaxLen
);

int N_API NWLInsertChar(
	char N_FAR * src,
	char N_FAR *insertableChar);


int  N_API_VARARGS NWprintf(
         char N_FAR *format,
         ...);

#ifndef NWL_EXCLUDE_FILE
int  N_API_VARARGS NWfprintf(
         FILE N_FAR *stream,
         char N_FAR *format,
         ...);
#endif

int  N_API_VARARGS NWsprintf(
         char N_FAR *buffer,
         char N_FAR *format,
         ...);

/* Functions using variable parameter lists have the pointer to the */
/* variable list declared as void instead of va_list to enable the user to */
/* compile without including stdarg.h in every module. */

int N_API NWvprintf(
         char N_FAR *format,
         void N_FAR *arglist);

#ifndef NWL_EXCLUDE_FILE
int N_API NWvfprintf(
         FILE N_FAR *stream,
         char N_FAR *format,
         void N_FAR *arglist);
#endif

int N_API NWvsprintf(
         char N_FAR *buffer,
         char N_FAR *format,
         void N_FAR *arglist);

int  N_API_VARARGS NWwsprintf(
         char N_FAR *buffer,
         char N_FAR *format,
         ...);

int N_API NWatoi(
         char N_FAR *string);

int N_API NWisalpha(
         unsigned int ch);

int N_API NWisalnum(
         unsigned int ch);

int N_API NWisdigit(
         unsigned int ch);

void N_API NWGetNWLOCALEVersion(unsigned char N_FAR *majorVersion,
                                unsigned char N_FAR *minorVersion,
                                unsigned char N_FAR *revisionLevel,
                                unsigned char N_FAR *betaReleaseLevel);

NWCCODE N_API NWGetShortMachineName(char N_FAR *shortMachineName);

/* This call is not needed for Windows */
int N_API NWGetCollateTable ( char N_FAR *retCollateTable,
                               size_t maxLen );

size_t N_API NWLstrxfrm(
         char N_FAR *string1,
         char N_FAR *string2,
         size_t numBytes);

char  N_FAR * N_API NWNextChar(
         char N_FAR *string);

char  N_FAR * N_API NWPrevChar(
         const char N_FAR *string,
         char N_FAR *position);

char  N_FAR * N_API NWLstrupr ( char N_FAR *string );
int  N_API NWCharUpr ( int chr );
int  N_API  NWLstrcoll( char N_FAR *string1, char N_FAR *string2);
/* #endif */

int  N_API NWCharType(unsigned char ch);
int  N_API NWCharVal(char N_FAR *);

#if defined(__cplusplus)
}
#endif

#include "npackoff.h"
#define __NWLOCALE_H__
#endif
