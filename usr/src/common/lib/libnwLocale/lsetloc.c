/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:lsetloc.c	1.2"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lsetloc.c,v 1.3 1994/09/26 17:20:56 rebekah Exp $"
/*===========================================================================*
   Source File ... lsetloc.c
   Author(s) ..... Phil Karren, Allan Neill, Dale Gambill
   Inital Date ... 21-Jun-89 (date of Phil's original)

   Abstract ...... For a description of this function, see the document,
                   'Novell Enabling APIs Description'.  This document is
                   in file name, "aus-rd\sys:\nwlocale\doc\nwlocale.txt".

*===========================================================================*/
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

#define NWL_EXCLUDE_TIME 1
#define NWL_EXCLUDE_FILE 1

#if (defined N_PLAT_NLM || defined N_PLAT_UNIX || defined WIN32)
#define _fstrcpy strcpy
#endif

#include <stdlib.h>                           /* needed for 'itoa' */
#include <string.h>

#if defined (N_PLAT_OS2)
# include <os2def.h>
#define INCL_WIN
#define INCL_WINSHELLDATA
#define INCL_BASE
# include <pmwin.h>
#endif

#if defined N_PLAT_MSW
# include <windows.h>
#endif

#include "ntypes.h"

#ifdef N_PLAT_UNIX
#include "libnwlocale_mt.h"
#include "locale.h"
#endif

#include "nwlocale.h"
#include "enable.h"

extern unsigned char upperCaseTable[256];

/*============================================================================
   prototypes for functions in this module
============================================================================*/
#if defined N_PLAT_MSW && defined N_ARCH_16

static int  NEAR _NWSetCType(NWEXTENDED_COUNTRY N_FAR *countryInfo);
static void NEAR _NWSetMonetary(NWEXTENDED_COUNTRY N_FAR *countryInfo);
static void NEAR _NWSetNumeric(NWEXTENDED_COUNTRY N_FAR *countryInfo);
static void NEAR _NWSetTime(NWEXTENDED_COUNTRY N_FAR *countryInfo);

#elif (defined N_PLAT_UNIX || defined WIN32)

/* UNIX and NT both use the new paradigm of having _NWGetCountryInfo stuff */
/* the localeconv structure directly */

static void N_API _NWSetCollate( void );
static int  N_API   _NWSetCType( LCONV *countryInfo );
static void N_API _NWSetMonetary( LCONV *countryInfo );
static void N_API _NWSetNumeric( LCONV *countryInfo );
static void N_API _NWSetTime( LCONV *countryInfo );

#else

static void N_API _NWSetCollate( void );
static int  N_API _NWSetCType( NWEXTENDED_COUNTRY N_FAR *countryInfo );
static void N_API _NWSetMonetary( NWEXTENDED_COUNTRY N_FAR *countryInfo );
static void N_API _NWSetNumeric( NWEXTENDED_COUNTRY N_FAR *countryInfo );
static void N_API _NWSetTime( NWEXTENDED_COUNTRY N_FAR *countryInfo );

#endif

/*============================================================================
   Lsetlocale
============================================================================*/
char N_FAR * N_API NWLsetlocale(
   int category,
   const char N_FAR * locale)
{
   static char countryIDstring[4];

#if (defined N_PLAT_UNIX || defined WIN32)
	LCONV	countryInfo;
#else
	NWEXTENDED_COUNTRY  countryInfo;
#endif
   int        ccode;
#if defined (N_PLAT_UNIX)
	/*
	**  make setting _DBCSVector, localeInfo,
	**  and countryIDstring an atomic operation.
	*/
	MUTEX_LOCK(&_libnwlocale_lsetloc_lock);
#endif

   _DBCSVector[0].lowValue = 0;
   _DBCSVector[0].highValue = 0;

   /* If the locale is NULL, set nothing and treat this function as a
   query. Return the country code string associated with the current locale.
   The specific strings are not defined in the ANSI specs so the DOS
   country code is used, which is based on the international phone prefix
   for that country. */

   if (locale == (char N_FAR *) NULL)
   {
      ccode = _NWGetCountryInfo(&countryInfo);
   }
   else
   {
      /* If the locale is "", then set the locale to the default.  The
         default for DOS and OS/2 is what was specified in the
         COUNTRY=statement in CONFIG.SYS.   Lsetlocale(NLC_ALL,"") will
         be typical.  Again, return the country code string associated
         with the current locale. */

      if (locale[0] == '\0')
      {
         if ((ccode = _NWGetCountryInfo(&countryInfo)) == 0)
         {
#if (defined N_PLAT_UNIX || defined WIN32)
			localeInfo._countryID = countryInfo.country_id;
#else
			localeInfo._countryID = countryInfo.countryID;
#endif
            switch (category)
            {
#ifdef N_PLAT_UNIX
               case LC_ALL:           /* Set all locale sensitive variables */
#else
               case NLC_ALL:           /* Set all locale sensitive variables */
#endif
#if !defined N_PLAT_MSW 
                  _NWSetCollate();
#endif
                  if ((ccode = _NWSetCType(&countryInfo)) == 0)
                  {
                     _NWSetMonetary(&countryInfo);
                     _NWSetNumeric(&countryInfo);
                     _NWSetTime(&countryInfo);
                  }
                  break;

#ifdef N_PLAT_UNIX
               case LC_COLLATE:         /* Only strcoll and strxfrm affected*/
#else
               case NLC_COLLATE:         /* Only strcoll and strxfrm affected*/
#endif
#if !defined N_PLAT_MSW
                  _NWSetCollate();
#endif
                  break;

#ifdef N_PLAT_UNIX
               case LC_CTYPE:    /* specifies which codepage and the ranges */
#else
               case NLC_CTYPE:    /* specifies which codepage and the ranges */
#endif
                  ccode = _NWSetCType(&countryInfo);
                  break;

#ifdef N_PLAT_UNIX
               case LC_MONETARY:                          /* Monetary units */
#else
               case NLC_MONETARY:                          /* Monetary units */
#endif
                  _NWSetMonetary(&countryInfo);
                  break;

#ifdef N_PLAT_UNIX
               case LC_NUMERIC:                 /* Numeric editing affected */
#else
               case NLC_NUMERIC:                 /* Numeric editing affected */
#endif
                   _NWSetNumeric(&countryInfo);
                   break;

#ifdef N_PLAT_UNIX
               case LC_TIME:                       /* This affects strftime */
#else
               case NLC_TIME:                       /* This affects strftime */
#endif
                  _NWSetTime(&countryInfo);
                  break;
            }
         }
      }

      /* The case where the user has specified a particular locale to set is
      not currently supported.  In this case a NULL pointer is returned as an
      error code. */

      else ccode = -1;
   }

	countryIDstring[3] = 0;
   if (localeInfo._countryID < 10)
   {
      countryIDstring[0] = 0x30;
      countryIDstring[1] = 0x30;
      countryIDstring[2] = (char) (localeInfo._countryID + '0');
   }
   else if (localeInfo._countryID < 100)
   {
      countryIDstring[0] = 0x30;
      countryIDstring[1] = (char) ((localeInfo._countryID / 10) + '0');
      countryIDstring[2] = (char) ((localeInfo._countryID % 10) + '0');
   }
   else
   {
      countryIDstring[0] = (char) ((localeInfo._countryID / 100) + '0');
      countryIDstring[1] = (char) (((localeInfo._countryID / 10) % 10) + '0');
      countryIDstring[2] = (char) ((localeInfo._countryID % 10) + '0');
   }

#if defined(N_PLAT_UNIX)
	MUTEX_UNLOCK(&_libnwlocale_lsetloc_lock);
#endif
   return ( (char N_FAR *) ( (ccode < 0) ? NULL : countryIDstring ) );
}


#if !defined N_PLAT_MSW
/*============================================================================
   _NWSetCollate
============================================================================*/
static void N_API _NWSetCollate()
{
   NWGetCollateTable( localeInfo._collateTable,
                       sizeof(localeInfo._collateTable));
}
#endif

/*============================================================================
   _NWSetCType
============================================================================*/
#if defined N_PLAT_MSW && defined N_ARCH_16
static int NEAR _NWSetCType(NWEXTENDED_COUNTRY N_FAR *countryInfo)
#elif (defined N_PLAT_UNIX || WIN32)
int N_API _NWSetCType(LCONV *countryInfo)
#else
int N_API _NWSetCType(NWEXTENDED_COUNTRY N_FAR *countryInfo)
#endif
{
   int rcode;

   /* Multiple-byte character support is part of NLC_CTYPE locale. */
   rcode = _NWGetDBCSVector(_DBCSVector, NUMBER_VECTORS * sizeof(VECTOR));
   if (!rcode)
   {
#if (defined N_PLAT_UNIX || defined WIN32)
		localeInfo._codePage = (int)countryInfo->code_page;
#else
		localeInfo._codePage = (int)countryInfo->codePage;
#endif

#if defined(N_PLAT_DOS)
      _NWGetUpperCaseTable(upperCaseTable);
#endif
   }
   return(rcode);
}

/*============================================================================
   _NWSetMonetary
============================================================================*/
#if defined N_PLAT_MSW && defined N_ARCH_16
static void NEAR _NWSetMonetary(NWEXTENDED_COUNTRY N_FAR *countryInfo)
{
   _fstrcpy(localeInfo._currencySymbol,countryInfo->currencySymbol);
   _fstrcpy(localeInfo._thousandSeparator,countryInfo->thousandSeparator);
   _fstrcpy(localeInfo._decimalSeparator,countryInfo->decimalSeparator);
   localeInfo._currencyFormatFlags = countryInfo->currencyFormatFlags;
   localeInfo._digitsInCurrency = countryInfo->digitsInCurrency;
}
#elif (defined N_PLAT_UNIX || defined WIN32)
void N_API _NWSetMonetary(LCONV *countryInfo)
{
   strcpy(localeInfo._currencySymbol,countryInfo->currency_symbol);
   _fstrcpy(localeInfo._thousandSeparator,countryInfo->thousands_sep);
   _fstrcpy(localeInfo._decimalSeparator,countryInfo->decimal_point);

	/* Fix this when I get to rearchitect DOS */
/*	localeInfo._currencyFormatFlags = countryInfo->currencyFormatFlags; */
/* localeInfo._digitsInCurrency = countryInfo->digitsInCurrency; */
}
#else
void N_API _NWSetMonetary(NWEXTENDED_COUNTRY N_FAR *countryInfo)
{
   _fstrcpy(localeInfo._currencySymbol,countryInfo->currencySymbol);
   _fstrcpy(localeInfo._thousandSeparator,countryInfo->thousandSeparator);
   _fstrcpy(localeInfo._decimalSeparator,countryInfo->decimalSeparator);
   localeInfo._currencyFormatFlags = countryInfo->currencyFormatFlags;
   localeInfo._digitsInCurrency = countryInfo->digitsInCurrency;
}
#endif

/*============================================================================
   _NWSetNumeric
============================================================================*/
#if defined N_PLAT_MSW && defined N_ARCH_16
static void NEAR _NWSetNumeric(NWEXTENDED_COUNTRY N_FAR *countryInfo)
{
   _fstrcpy(localeInfo._thousandSeparator,countryInfo->thousandSeparator);
   _fstrcpy(localeInfo._decimalSeparator,countryInfo->decimalSeparator);
   _fstrcpy(localeInfo._dataListSeparator,countryInfo->dataListSeparator);
}
#elif (defined N_PLAT_UNIX || defined WIN32)
void N_API _NWSetNumeric(LCONV *countryInfo)
{
   strcpy(localeInfo._thousandSeparator,countryInfo->thousands_sep);
   strcpy(localeInfo._decimalSeparator,countryInfo->decimal_point);
   strcpy(localeInfo._dataListSeparator,countryInfo->data_list_separator);
}
#else
void N_API _NWSetNumeric(NWEXTENDED_COUNTRY N_FAR *countryInfo)

{
   _fstrcpy(localeInfo._thousandSeparator,countryInfo->thousandSeparator);
   _fstrcpy(localeInfo._decimalSeparator,countryInfo->decimalSeparator);
   _fstrcpy(localeInfo._dataListSeparator,countryInfo->dataListSeparator);
}
#endif

/*============================================================================
   _NWSetTime
============================================================================*/
#if defined N_PLAT_MSW && defined N_ARCH_16
static void NEAR _NWSetTime(NWEXTENDED_COUNTRY N_FAR *countryInfo)
#elif (defined N_PLAT_UNIX || defined WIN32)
void N_API _NWSetTime(LCONV *countryInfo)
#else
void N_API _NWSetTime(NWEXTENDED_COUNTRY N_FAR *countryInfo)
#endif
{
#if defined(N_PLAT_OS2)
/*   HAB hAb; */
#endif

#if (defined N_PLAT_UNIX || defined WIN32)
	localeInfo._dateFormat = countryInfo->date_format;
   _fstrcpy(localeInfo._dateSeparator,countryInfo->date_separator);
   _fstrcpy(localeInfo._timeSeparator,countryInfo->time_separator);
   localeInfo._timeFormat = countryInfo->time_format;

#else
	localeInfo._dateFormat = countryInfo->dateFormat;
   _fstrcpy(localeInfo._dateSeparator,countryInfo->dateSeparator);
   _fstrcpy(localeInfo._timeSeparator,countryInfo->timeSeparator);
   localeInfo._timeFormat = countryInfo->timeFormat;
#endif

#if defined N_PLAT_MSW && defined N_ARCH_16

   GetProfileString((LPSTR)"intl", (LPSTR)"s1159", (LPSTR) localeInfo._am,
                     (LPSTR)localeInfo._am, MERIDLEN);
   GetProfileString((LPSTR)"intl", (LPSTR)"s2359", (LPSTR) localeInfo._pm,
                     (LPSTR)localeInfo._pm, MERIDLEN);

#elif defined(N_PLAT_OS2)

/*   hAb = WinInitialize(0);

   WinQueryProfileString(hAb, "PM_National", "s1159", localeInfo._am,
                         localeInfo._am, MERIDLEN);

   WinQueryProfileString(hAb, "PM_National", "s2359", localeInfo._pm,
                         localeInfo._pm, MERIDLEN);

   WinTerminate(hAb);*/
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lsetloc.c,v 1.3 1994/09/26 17:20:56 rebekah Exp $
*/
