/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:getcntry.c	1.3"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/getcntry.c,v 1.4 1994/09/26 17:20:31 rebekah Exp $"
/*========================================================================
	==	library	: NetWare Directory Services Platform Library
	==
	==	file		: GETCNTRY.C
	==
	==	routine	: NWGetCountryInfo
	==
	==	author		: Phil Karren
	==
	==	date		: 21 June 1989
	==
	==	comments	: Get Country Information from DOS, OS2, NT and Unix
	==            Internal functions called by NWLsetlocale
	==
	==	modifications	:
	==
	==	dependencies	:
	==
	========================================================================*/
/*--------------------------------------------------------------------------
			(C) Unpublished Copyright of Novell, Inc.	All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

#define NWL_EXCLUDE_TIME 1
#define NWL_EXCLUDE_FILE 1

#if defined(N_PLAT_OS2)
#define CODEPAGE_LIST_TOO_SHORT	473

#define INCL_DOSNLS
#define INCL_WINSHELLDATA
#define INCL_BASE
# include <os2.h>
#endif

#if defined N_PLAT_MSW
# include <windows.h>
#endif

#include <string.h>
#include "ntypes.h"
#ifdef N_PLAT_UNIX
#include <locale.h>
#endif
#include "nwlocale.h"
#include "enable.h"

#ifdef N_PLAT_UNIX
#include <locale.h>
#endif


int N_API _NWGetCountryInfo(
#if	(defined N_PLAT_UNIX || defined WIN32)
		LCONV *countryInfo )
#else
		NWEXTENDED_COUNTRY N_FAR *countryInfo )
#endif
{
		int rcode = 0;

#if defined(N_PLAT_DOS)
		if((rcode = _NWGetDOSVersion(&DosMajorVersion,
				&DosMinorVersion)) != 0)
			return -1;

		if (DosMajorVersion < 2 ||
				(DosMajorVersion == 2 && DosMinorVersion < 10))
			return -1;

		if ((DosMajorVersion == 2 && DosMinorVersion >= 10) ||
				(DosMajorVersion == 3 && DosMinorVersion < 30))
		{
			if ((rcode = _NWGetCountryInfo38(&countryInfo->dateFormat,
					&countryInfo->countryID)) != 0)
				return rcode;

			switch (countryInfo->countryID)
			{
			case JAPAN:
				countryInfo->codePage = 897;
				break;

			case KOREA:
				countryInfo->codePage = 934;
				break;

			case TAIWAN:
				countryInfo->codePage = 938;
				break;

			case PRC:
				countryInfo->codePage = 936;
				break;

			default:
				countryInfo->codePage = 437;
			}
		}
		else
		{
			countryInfo->codePage = 0xFFFF;
			if ((rcode = _NWGetCountryInfo65(countryInfo)) != 0)
				return rcode;

			/* Should only happen for JAPAN */
			if (countryInfo->codePage == 0xFFFF)
			{
				if ((rcode = _NWGetCountryInfo38(&countryInfo->dateFormat,
						&countryInfo->countryID)) != 0)
					return rcode;
  			switch (countryInfo->countryID)
  			{
  			case JAPAN:
  				countryInfo->codePage = 897;
  				break;
  
  			case KOREA:
  				countryInfo->codePage = 934;
  				break;
  
  			case TAIWAN:
  				countryInfo->codePage = 938;
  				break;
  
  			case PRC:
  				countryInfo->codePage = 936;
  				break;
  
  			default:
  				countryInfo->codePage = 437;
  			}
			}
		}

		/*
			-JBI 1/25/93
			This switch caused several code pages to fail.
			In particular Russian failed.  Rather than restrict
			as this one does, Rob Brinner and I thought it
			would be best ot remove this section and leave
			whatever code has been calculated.

		switch ((int) countryInfo->codePage)
		{
			case 437:
			case 850:
			case 862:
			case 863:
			case 865:
			case 897:
			case 932:
			case 934:
			case 936:
			case 938:
					break;

			default:
					countryInfo->countryID = 0;
					countryInfo->codePage = 0;
					break;
		}
		*/

#elif defined N_PLAT_MSW && defined N_ARCH_16
		int		far pascal GetKBCodePage(void);

		countryInfo->countryID = GetProfileInt((LPSTR)"intl",
			(LPSTR)"iCountry", (short) 1);

		switch (countryInfo->countryID)
		{
		case JAPAN:
			countryInfo->codePage = 897;
			break;

		case KOREA:
			countryInfo->codePage = 934;
			break;

		case TAIWAN:
			countryInfo->codePage = 886;
			break;

		case PRC:
			countryInfo->codePage = 936;
			break;

		default:
			countryInfo->codePage = GetKBCodePage();
		}

		countryInfo->timeFormat = (char)GetProfileInt((LPSTR)"intl",
			(LPSTR)"iTime", 0);

		countryInfo->dateFormat = (char)GetProfileInt((LPSTR)"intl",
			(LPSTR)"iDate", 0);

		countryInfo->currencyFormatFlags = (char)GetProfileInt((LPSTR)"intl",
			(LPSTR)"iCurrency", 0);

		countryInfo->digitsInCurrency = (char)GetProfileInt((LPSTR)"intl",
			(LPSTR)"iDigits", 2);

		GetProfileString((LPSTR)"intl", (LPSTR)"sCurrency",
			(LPSTR)"$",
			(LPSTR)countryInfo->currencySymbol, 5);

		GetProfileString((LPSTR)"intl", (LPSTR)"sThousand",
			(LPSTR)",",
			(LPSTR)countryInfo->thousandSeparator, 2);

		GetProfileString((LPSTR)"intl", (LPSTR)"sDecimal",
			(LPSTR)".",
			(LPSTR)countryInfo->decimalSeparator, 2);

		GetProfileString((LPSTR)"intl", (LPSTR)"sDate",
			(LPSTR)"/",
			(LPSTR)countryInfo->dateSeparator, 2);

		GetProfileString((LPSTR)"intl", (LPSTR)"sTime",
			(LPSTR)":",
			(LPSTR)countryInfo->timeSeparator, 2);

		GetProfileString((LPSTR)"intl", (LPSTR)"sList",
			(LPSTR)",",
			(LPSTR)countryInfo->dataListSeparator, 2);

#elif defined(N_PLAT_OS2)
	unsigned short length;

	/*
		During startup, the GetProfile calls will block until the
		shell is started.  To get around this, DosXxx calls are
		being used.  -JBI  1/6/93  SPD#27626
	*/
	COUNTRYINFO		ctryInfo;
	COUNTRYCODE		ctryCode;

	/* use default country and code page */
	ctryCode.country = 0;
	ctryCode.codepage = 0;

	/*
		DosGetCtryInfo() could use the &countryInfo->countryID pointer,
		but I did it this way in case either structure changes.
	*/
	if ((rcode = DosGetCtryInfo(sizeof(COUNTRYINFO),
			&ctryCode, &ctryInfo, &length)) != 0)
		return rcode;

	/* make sure we have enough data to copy over below */
	if (length < sizeof(COUNTRYINFO))
		return -1;

	/* copy from the OS/2 structure to our structure */
	countryInfo->countryID = ctryInfo.country;
	countryInfo->codePage = ctryInfo.codepage;
	countryInfo->dateFormat = ctryInfo.fsDateFmt;
	countryInfo->timeFormat = ctryInfo.fsTimeFmt;
	countryInfo->currencyFormatFlags = ctryInfo.fsCurrencyFmt;
	countryInfo->digitsInCurrency = ctryInfo.cDecimalPlace;

	_fmemcpy(countryInfo->currencySymbol, ctryInfo.szCurrency, 5);
	_fmemcpy(countryInfo->thousandSeparator,
			ctryInfo.szThousandsSeparator, 2);
	_fmemcpy(countryInfo->decimalSeparator, ctryInfo.szDecimal, 2);
	_fmemcpy(countryInfo->dateSeparator, ctryInfo.szDateSeparator, 2);
	_fmemcpy(countryInfo->timeSeparator, ctryInfo.szTimeSeparator, 2);
	_fmemcpy(countryInfo->dataListSeparator, ctryInfo.szDataSeparator,2);

#elif defined(N_PLAT_UNIX)

/* UNIX stuff goes here */


struct lconv *AnsiStructure;
char *UnixLocale;


	/* Query Unix for its locale information */
	UnixLocale = setlocale(LC_ALL, "");
	
	AnsiStructure=localeconv();
	if(AnsiStructure == NULL)
		{
			return ( -1);
		}
	
	strcpy(countryInfo->decimal_point,
		AnsiStructure->decimal_point);

	strcpy(countryInfo->thousands_sep,
		AnsiStructure->thousands_sep);

	strcpy(countryInfo->grouping,
		AnsiStructure->grouping);

	countryInfo->grouping[0] += '0';

	strcpy(countryInfo->int_curr_symbol,
		AnsiStructure->int_curr_symbol);

	strcpy(countryInfo->currency_symbol,
		AnsiStructure->currency_symbol);

	strcpy(countryInfo->mon_decimal_point,
		AnsiStructure->mon_decimal_point);

	strcpy(countryInfo->mon_thousands_sep,
		AnsiStructure->mon_thousands_sep);

	strcpy(countryInfo->mon_grouping,
		AnsiStructure->mon_grouping);

	countryInfo->mon_grouping[0] += '0';

	strcpy(countryInfo->positive_sign,
		AnsiStructure->positive_sign);

	strcpy(countryInfo->negative_sign,
		AnsiStructure->negative_sign);

	countryInfo->int_frac_digits = AnsiStructure->int_frac_digits;

	countryInfo->frac_digits = AnsiStructure->frac_digits;

	countryInfo->p_cs_precedes = AnsiStructure->p_cs_precedes;

	countryInfo->p_sep_by_space = AnsiStructure->p_sep_by_space;

	countryInfo->n_cs_precedes = AnsiStructure->n_cs_precedes;

	countryInfo->n_sep_by_space = AnsiStructure->n_sep_by_space;

	countryInfo->p_sign_posn = AnsiStructure->p_sign_posn;

	countryInfo->n_sign_posn = AnsiStructure->n_sign_posn;

	/* put the Novell additions here */

	if(strcmp(UnixLocale, "en_NZ") == NULL)
		{
			countryInfo->country_id = NEWZEALAND;
			countryInfo->code_page=1;
			strcpy(countryInfo->data_list_separator, " ");
			strcpy(countryInfo->date_separator, "/");
			strcpy(countryInfo->time_separator, ":");
			countryInfo->time_format = 1;
			countryInfo->date_format = 1;
			strcpy(countryInfo->am, "AM");
			strcpy(countryInfo->pm, "PM");
		}
	else
	if(strcmp(UnixLocale, "en_CA") == NULL)
		{
			countryInfo->country_id = CANADA;
			countryInfo->code_page=1;
			strcpy(countryInfo->data_list_separator, " ");
			strcpy(countryInfo->date_separator, "/");
			strcpy(countryInfo->time_separator, ":");
			countryInfo->time_format = 1;
			countryInfo->date_format = 1;
			strcpy(countryInfo->am, "AM");
			strcpy(countryInfo->pm, "PM");
		}
	else
	if(strcmp(UnixLocale, "en") == NULL)
		{
			countryInfo->country_id = UK;
			countryInfo->code_page=1;
			strcpy(countryInfo->data_list_separator, " ");
			strcpy(countryInfo->date_separator, "/");
			strcpy(countryInfo->time_separator, ":");
			countryInfo->time_format = 1;
			countryInfo->date_format = 1;
			strcpy(countryInfo->am, "AM");
			strcpy(countryInfo->pm, "PM");
		}
	else
	if(strcmp(UnixLocale, "en_AU") == NULL)
		{
			countryInfo->country_id = AUSTRALIA;
			countryInfo->code_page=1;
			strcpy(countryInfo->data_list_separator, " ");
			strcpy(countryInfo->date_separator, "/");
			strcpy(countryInfo->time_separator, ":");
			countryInfo->time_format = 1;
			countryInfo->date_format = 1;
			strcpy(countryInfo->am, "AM");
			strcpy(countryInfo->pm, "PM");
		}

	else
	if(strcmp(UnixLocale, "en_IR") == NULL)
		{
			countryInfo->country_id = IRELAND;
			countryInfo->code_page=1;
			strcpy(countryInfo->data_list_separator, " ");
			strcpy(countryInfo->date_separator, "/");
			strcpy(countryInfo->time_separator, ":");
			countryInfo->time_format = 1;
			countryInfo->date_format = 1;
			strcpy(countryInfo->am, "AM");
			strcpy(countryInfo->pm, "PM");
		}
	else
	if(strcmp(UnixLocale, "da") == NULL)
		{
			countryInfo->country_id = DENMARK;
			countryInfo->code_page=1;
			strcpy(countryInfo->data_list_separator, " ");
			strcpy(countryInfo->date_separator, "-");
			strcpy(countryInfo->time_separator, ".");
			countryInfo->time_format = 1;
			countryInfo->date_format = 1;
			strcpy(countryInfo->am, "AM");
			strcpy(countryInfo->pm, "PM");
		}
	else
	if(strcmp(UnixLocale, "fr_FR") == NULL)
		{
			countryInfo->country_id = FRANCE;
			countryInfo->code_page=1;
			strcpy(countryInfo->data_list_separator, " ");
			strcpy(countryInfo->date_separator, ".");
			strcpy(countryInfo->time_separator, ":");
			countryInfo->time_format = 1;
			countryInfo->date_format = 1;
			strcpy(countryInfo->am, "AM");
			strcpy(countryInfo->pm, "PM");
		}

	else
	if(strcmp(UnixLocale, "fr_CA") == NULL)
		{
			countryInfo->country_id = CANADA;
			countryInfo->code_page=1;
			strcpy(countryInfo->data_list_separator, " ");
			strcpy(countryInfo->date_separator, "-");
			strcpy(countryInfo->time_separator, ":");
			countryInfo->time_format = 1;
			countryInfo->date_format = 2;
			strcpy(countryInfo->am, "AM");
			strcpy(countryInfo->pm, "PM");
		}

/*	else
	if(strcmp(UnixLocale, "jp") == NULL)
		{
			countryInfo->country_id = JAPAN;
			countryInfo->code_page=;
			strcpy(countryInfo->data_list_separator, " ");
			strcpy(countryInfo->date_separator, "-");
			strcpy(countryInfo->time_separator, ":");
			countryInfo->time_format = 1;
			countryInfo->date_format = 2;
			strcpy(countryInfo->am, "\0");
			strcpy(countryInfo->pm, "\0");
		}

*/
	else /* default to USA */
		{
			countryInfo->country_id = USA;
			countryInfo->code_page=1;
			strcpy(countryInfo->data_list_separator, " ");
			strcpy(countryInfo->date_separator, "-");
			strcpy(countryInfo->time_separator, ":");
			countryInfo->time_format = 0;
			countryInfo->date_format = 0;
			strcpy(countryInfo->am, "AM");
			strcpy(countryInfo->pm, "PM");
		}

#elif defined (WIN32)

/* NT Stuff goes here */

#include "unicode.h"

	LCID		MyLocale;
	nstr16	ustr1[25];
	nstr8		pstr1[25];
	void		*utolHandle;
	size_t	i;

	utolHandle = (void *) ustr1; /* Shut up uninitialized variable warnings */
	MyLocale = GetThreadLocale();

	rcode = GetLocaleInfoW(MyLocale, LOCALE_SDECIMAL,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, countryInfo->decimal_point,
		sizeof(countryInfo->decimal_point), ustr1, 'E', &i);

	rcode = GetLocaleInfoW(MyLocale, LOCALE_STHOUSAND,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, countryInfo->thousands_sep,
		sizeof(countryInfo->thousands_sep), ustr1, 'E', &i);

	rcode = GetLocaleInfoW(MyLocale, LOCALE_SGROUPING,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, countryInfo->grouping,
		sizeof(countryInfo->grouping), ustr1, 'E', &i);

	rcode = GetLocaleInfoW(MyLocale, LOCALE_SINTLSYMBOL,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, countryInfo->int_curr_symbol,
		sizeof(countryInfo->int_curr_symbol), ustr1, 'E', &i);

	rcode = GetLocaleInfoW(MyLocale, LOCALE_SCURRENCY,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, countryInfo->currency_symbol,
		sizeof(countryInfo->currency_symbol), ustr1, 'E', &i);

	rcode = GetLocaleInfoW(MyLocale, LOCALE_SMONDECIMALSEP,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, countryInfo->mon_decimal_point,
		sizeof(countryInfo->mon_decimal_point), ustr1, 'E', &i);

	rcode = GetLocaleInfoW(MyLocale, LOCALE_SMONTHOUSANDSEP,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, countryInfo->mon_thousands_sep,
		sizeof(countryInfo->mon_thousands_sep), ustr1, 'E', &i);

	rcode = GetLocaleInfoW(MyLocale, LOCALE_SMONGROUPING,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, countryInfo->mon_grouping,
		sizeof(countryInfo->mon_grouping), ustr1, 'E', &i);

	rcode = GetLocaleInfoW(MyLocale, LOCALE_SPOSITIVESIGN,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, countryInfo->positive_sign,
		sizeof(countryInfo->positive_sign), ustr1, 'E', &i);

	rcode = GetLocaleInfoW(MyLocale, LOCALE_SNEGATIVESIGN,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, countryInfo->negative_sign,
		sizeof(countryInfo->negative_sign), ustr1, 'E', &i);

	rcode = GetLocaleInfoW(MyLocale, LOCALE_ICURRDIGITS,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, pstr1, sizeof(pstr1),
		ustr1, 'E', &i);
	rcode=NWatoi(pstr1);
	countryInfo->int_frac_digits = (char) rcode;

	rcode = GetLocaleInfoW(MyLocale, LOCALE_IDIGITS,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, pstr1, sizeof(pstr1),
		ustr1, 'E', &i);
	rcode=NWatoi(pstr1);
	countryInfo->frac_digits = (char) rcode;

	rcode = GetLocaleInfoW(MyLocale, LOCALE_IPOSSYMPRECEDES,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, pstr1, sizeof(pstr1),
		ustr1, 'E', &i);
	rcode=NWatoi(pstr1);
	countryInfo->p_cs_precedes = (char) rcode;

	rcode = GetLocaleInfoW(MyLocale, LOCALE_IPOSSEPBYSPACE,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, pstr1, sizeof(pstr1),
		ustr1, 'E', &i);
	rcode=NWatoi(pstr1);
	countryInfo->p_sep_by_space = (char) rcode;

	rcode = GetLocaleInfoW(MyLocale, LOCALE_INEGSYMPRECEDES,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, pstr1, sizeof(pstr1),
		ustr1, 'E', &i);
	rcode=NWatoi(pstr1);
	countryInfo->n_cs_precedes = (char) rcode;

	rcode = GetLocaleInfoW(MyLocale, LOCALE_INEGSEPBYSPACE,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, pstr1, sizeof(pstr1),
		ustr1, 'E', &i);
	rcode=NWatoi(pstr1);
	countryInfo->n_sep_by_space = (char) rcode;

	rcode = GetLocaleInfoW(MyLocale, LOCALE_IPOSSIGNPOSN,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, pstr1, sizeof(pstr1),
		ustr1, 'E', &i);
	rcode=NWatoi(pstr1);
	countryInfo->p_sign_posn = (char) rcode;

	rcode = GetLocaleInfoW(MyLocale, LOCALE_INEGSIGNPOSN,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, pstr1, sizeof(pstr1),
		ustr1, 'E', &i);
	rcode=NWatoi(pstr1);
	countryInfo->n_sign_posn = (char) rcode;

	/* Novell additions to the ANSI standard */

	rcode = GetLocaleInfoW(MyLocale, LOCALE_ICOUNTRY,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, pstr1, sizeof(pstr1),
		ustr1, 'E', &i);
	rcode=NWatoi(pstr1);
	countryInfo->country_id = rcode;

	countryInfo->code_page = (int) GetConsoleCP();

	rcode = GetLocaleInfoW(MyLocale, LOCALE_SLIST,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, countryInfo->data_list_separator,
		sizeof(countryInfo->data_list_separator), ustr1, 'E', &i);

	rcode = GetLocaleInfoW(MyLocale, LOCALE_SDATE,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, countryInfo->date_separator,
		sizeof(countryInfo->date_separator), ustr1, 'E', &i);

	rcode = GetLocaleInfoW(MyLocale, LOCALE_STIME,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, countryInfo->time_separator,
		sizeof(countryInfo->time_separator), ustr1, 'E', &i);

	rcode = GetLocaleInfoW(MyLocale, LOCALE_ITIME,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, pstr1, sizeof(pstr1),
		ustr1, 'E', &i);
	rcode=NWatoi(pstr1);
	countryInfo->time_format = (char) rcode;

	rcode = GetLocaleInfoW(MyLocale, LOCALE_IDATE,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, pstr1, sizeof(pstr1),
		ustr1, 'E', &i);
	rcode=NWatoi(pstr1);
	countryInfo->date_format = rcode;

	rcode = GetLocaleInfoW(MyLocale, LOCALE_S1159,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, countryInfo->am,
		sizeof(countryInfo->am), ustr1, 'E', &i);

	rcode = GetLocaleInfoW(MyLocale, LOCALE_S2359,
		ustr1, sizeof(ustr1));
	rcode = NWUnicodeToLocal(utolHandle, countryInfo->pm,
		sizeof(countryInfo->pm), ustr1, 'E', &i);

#endif

return rcode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/getcntry.c,v 1.4 1994/09/26 17:20:31 rebekah Exp $
*/
