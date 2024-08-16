/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:_llocale.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/_llocale.c,v 1.1 1994/09/26 17:20:23 rebekah Exp $"
/*===========================================================================*
   Source File ... _llocalec.c
   Author(s) ..... Phil Karren, Dale Gambill
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

#if defined N_PLAT_MSW && defined N_ARCH_16
#include <windows.h>
#include <windowsx.h>
#include <ver.h>
#endif

#include <string.h>
#include "ntypes.h"
#if defined (N_PLAT_UNIX)
#include "libnwlocale_mt.h"
#endif
#include "nwlocale.h"
#include "enable.h"

LCONV N_FAR * N_API _Llocaleconv()
#if (defined N_PLAT_DOS || (defined N_PLAT_MSW && defined N_ARCH_16) || defined N_PLAT_OS2)
{
   NWEXTENDED_COUNTRY countryInfo;
   extern LCONV __lconvInfo;
   int rcode;

#if defined N_PLAT_MSW && defined N_ARCH_16

	/* These are used to make Windows system calls to get code page info */

    DWORD size;
    DWORD hver;
    LPVOID verptr;
    BOOL result;
    LPWORD lpBuffer;
    UINT langsize;
/*    char buf[80]; */
    char fname[80];

#endif

	rcode = _NWGetCountryInfo(&countryInfo);

   if (rcode == 0)
   {
      _fstrcpy(__lconvInfo.decimal_point, countryInfo.decimalSeparator);
      _fstrcpy(__lconvInfo.thousands_sep, countryInfo.thousandSeparator);
      _fstrcpy(__lconvInfo.currency_symbol,countryInfo.currencySymbol);

      /* Since DOS does not distinguish between monetary and numeric markers
        (separator and decimal) this code makes them both the same.*/

      _fstrcpy(__lconvInfo.mon_decimal_point, countryInfo.decimalSeparator);
      _fstrcpy(__lconvInfo.mon_thousands_sep, countryInfo.thousandSeparator);

      /* Fractional digits are also assumed to be the same for local as
        well as international monetary format */

      __lconvInfo.frac_digits = countryInfo.digitsInCurrency;
      __lconvInfo.int_frac_digits = countryInfo.digitsInCurrency;

     /* Also assume currency symbol in same location for both positive and
        negative numbers */

      switch((int) countryInfo.currencyFormatFlags)
      {
         case 0:    /* Currency sym precedes number, not separated by space */
            __lconvInfo.p_cs_precedes = 1;
            __lconvInfo.n_cs_precedes = 1;
            __lconvInfo.p_sep_by_space = 0;
            __lconvInfo.n_sep_by_space = 0;
            break;

         case 1:     /* Currency sym follows number, not separated by space */
            __lconvInfo.p_cs_precedes = 0;
            __lconvInfo.n_cs_precedes = 0;
            __lconvInfo.p_sep_by_space = 0;
            __lconvInfo.n_sep_by_space = 0;
            break;

         case 2:     /* Currency symbol precedes number, separated by space */
            __lconvInfo.p_cs_precedes = 1;
            __lconvInfo.n_cs_precedes = 1;
            __lconvInfo.p_sep_by_space = 1;
            __lconvInfo.n_sep_by_space = 1;
            break;

         case 3:      /* Currency symbol follows number, separated by space */
            __lconvInfo.p_cs_precedes = 0;
            __lconvInfo.n_cs_precedes = 0;
            __lconvInfo.p_sep_by_space = 1;
            __lconvInfo.n_sep_by_space = 1;
            break;

         case 4:
            __lconvInfo.p_cs_precedes = 0;
            __lconvInfo.n_cs_precedes = 0;
            __lconvInfo.p_sep_by_space = 0;
            __lconvInfo.n_sep_by_space = 0;
            _fstrcpy(__lconvInfo.mon_decimal_point, __lconvInfo.currency_symbol);
            break;
      }

      /* Information not supplied by DOS: */

      /* The positive sign precedes quantity and currency symbols */
      __lconvInfo.p_sign_posn = 1;

      /* The negative sign precedes quantity and currency symbol.  The
          switch statement may change the default, if necessary */
      __lconvInfo.n_sign_posn = 1;
      _fstrcpy(__lconvInfo.positive_sign, "");
      _fstrcpy(__lconvInfo.negative_sign, "-");

      /* Assume all countries group by thousands.  Change later if not. */
      _fstrcpy(__lconvInfo.grouping, "3");
      _fstrcpy(__lconvInfo.mon_grouping, "3");

#endif /* DOS or WIN */

#if defined N_PLAT_MSW && defined N_ARCH_16 /* Changes for Windows code page and country ID.  LJG */

		/* First try to query windows for code page info. */
		_fstrcpy(fname, "SHELL.DLL");
		size = GetFileVersionInfoSize(fname, &hver);
		verptr = GlobalAllocPtr(GMEM_ZEROINIT, size);
   	if (verptr)
		{
			result = GetFileVersionInfo(fname, hver, size, verptr);
			if (result)
			{
	   		result = VerQueryValue(verptr,"\\VarFileInfo\\Translation",
				   &(LPVOID)lpBuffer,
				   &langsize);
			if (result)	/* call succeeded */
				{
					__lconvInfo.code_page=lpBuffer[1];
				}
				else	/* Windows is unable to report code page */
				{
		/* Check win.ini for NWCodePage=
			if it's not there, default to code page of 1252 */
      		__lconvInfo.code_page=GetProfileInt("netware", "NWCodePage", 1252);
				}
			}
//			GlobalFreePtr(verptr);
			GlobalUnlockPtr(verptr);
			GlobalFree(GlobalPtrHandle(verptr));
		}

	switch((int) countryInfo.countryID)
      {

			case ARABIC:
			case WBAHRAIN:
			case WCYPRUS:
			case WEGYPT:
			case WETHIOPIA:
			case WIRAN:
			case WIRAQ:
			case WJORDAN:
			case WKUWAIT:
			case WLIBYA:
			case WMALTA:
			case WMOROCCO:
			case WPAKISTAN:
			case WQATAR:
			case WSAUDI:
			case WTANZANIA:
			case WTUNISIA:
			case WTURKEY:
			case WUAE:
			case WYEMEN:
		      __lconvInfo.country_id = ARABIC;
            _fstrcpy(__lconvInfo.int_curr_symbol, "BEF");
            break;

         case AUSTRALIA:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "???");
            break;

         case BELGIUM:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "BEF");
            break;

         case CANADA_FR:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "CAD");
            break;

         case DENMARK:             /* Negative sign follows currency symbol */
		      __lconvInfo.country_id = countryInfo.countryID;
            __lconvInfo.n_sign_posn = 4;
            _fstrcpy(__lconvInfo.int_curr_symbol, "DKK");
            break;

         case FINLAND:             /* Negative sign follows currency symbol */
		      __lconvInfo.country_id = countryInfo.countryID;
            __lconvInfo.n_sign_posn = 4;
            _fstrcpy(__lconvInfo.int_curr_symbol, "FIM");
            break;

         case FRANCE:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "FRF");
            break;

			case GERMANYE:
         case GERMANY:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "DDM");
            break;

			case HEBREW:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "???");
            break;

         case ITALY:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "ITL");
            break;

         case NETHERLANDS:         /* Negative sign follows currency symbol */
		      __lconvInfo.country_id = countryInfo.countryID;
            __lconvInfo.n_sign_posn = 4;
            _fstrcpy(__lconvInfo.int_curr_symbol, "NLG");
            break;

         case NORWAY:        /* Negative sign follows currency and quantity */
		      __lconvInfo.country_id = countryInfo.countryID;
            __lconvInfo.n_sign_posn = 2;
            _fstrcpy(__lconvInfo.int_curr_symbol, "NOK");
            break;

         case PORTUGAL:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "PTE");
            break;

         case SPAIN:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "ESP");
            break;

         case SWEDEN:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "SEK");
            break;

         case SWITZERLAND:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "SFR");
            break;

         case UK:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "GBP");
            break;

         case USA:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "USD");
            break;

         case JAPAN:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "JPY");
            break;

         case KOREA:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "KRW");
            break;

         case PRC:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "CNY");
            break;

			case TAIWAN:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "TWD");
            break;

			case LATIN_AMERICA:
			case WARGENTINA :
			case WBOLIVIA	 :
			case WCHILE		 :
			case WCOLOMBIA	 :
			case WCOSTARICA :
			case WECUADOR	 :
			case WELSALVADOR:
			case WGUATEMALA :
			case WHONDURAS	 :
			case WMEXICO	 :
			case WNICARAGUA :
			case WPANAMA	 :
			case WPARAGUAY	 :
			case WPERU		 :
			case WURUGUAY	 :
			case WVENEZUELA :
		      __lconvInfo.country_id = LATIN_AMERICA;
            _fstrcpy(__lconvInfo.int_curr_symbol, "???");
				break;

         default:
		      __lconvInfo.country_id = countryInfo.countryID;
            _fstrcpy(__lconvInfo.int_curr_symbol, "???");
            break;
      }
#elif (defined N_PLAT_DOS || defined N_PLAT_OS2)
      __lconvInfo.country_id = countryInfo.countryID;
      __lconvInfo.code_page = countryInfo.codePage;
      switch((int) __lconvInfo.country_id)
      {
         case BELGIUM:
            _fstrcpy(__lconvInfo.int_curr_symbol, "BEF");
            break;

         case CANADA_FR:
            _fstrcpy(__lconvInfo.int_curr_symbol, "CAD");
            break;

         case DENMARK:             /* Negative sign follows currency symbol */
            __lconvInfo.n_sign_posn = 4;
            _fstrcpy(__lconvInfo.int_curr_symbol, "DKK");
            break;

         case FINLAND:             /* Negative sign follows currency symbol */
            __lconvInfo.n_sign_posn = 4;
            _fstrcpy(__lconvInfo.int_curr_symbol, "FIM");
            break;

         case FRANCE:
            _fstrcpy(__lconvInfo.int_curr_symbol, "FRF");
            break;

         case GERMANY:
            _fstrcpy(__lconvInfo.int_curr_symbol, "DDM");
            break;

         case ITALY:
            _fstrcpy(__lconvInfo.int_curr_symbol, "ITL");
            break;

         case NETHERLANDS:         /* Negative sign follows currency symbol */
            __lconvInfo.n_sign_posn = 4;
            _fstrcpy(__lconvInfo.int_curr_symbol, "NLG");
            break;

         case NORWAY:        /* Negative sign follows currency and quantity */
            __lconvInfo.n_sign_posn = 2;
            _fstrcpy(__lconvInfo.int_curr_symbol, "NOK");
            break;

         case PORTUGAL:
            _fstrcpy(__lconvInfo.int_curr_symbol, "PTE");
            break;

         case SPAIN:
            _fstrcpy(__lconvInfo.int_curr_symbol, "ESP");
            break;

         case SWEDEN:
            _fstrcpy(__lconvInfo.int_curr_symbol, "SEK");
            break;

         case SWITZERLAND:
            _fstrcpy(__lconvInfo.int_curr_symbol, "SFR");
            break;

         case UK:
            _fstrcpy(__lconvInfo.int_curr_symbol, "GBP");
            break;

         case USA:
            _fstrcpy(__lconvInfo.int_curr_symbol, "USD");
            break;

         case JAPAN:
            _fstrcpy(__lconvInfo.int_curr_symbol, "JPY");
            break;

         case KOREA:
            _fstrcpy(__lconvInfo.int_curr_symbol, "KRW");
            break;

         case PRC:
            _fstrcpy(__lconvInfo.int_curr_symbol, "CNY");
            break;

         case TAIWAN:
            _fstrcpy(__lconvInfo.int_curr_symbol, "TWD");
            break;

         default:
            _fstrcpy(__lconvInfo.int_curr_symbol, "???");
            break;
      }
#endif /* defined N_PLAT_MSW && defined N_ARCH_16 */

#if (defined N_PLAT_DOS || (defined N_PLAT_MSW && defined N_ARCH_16) || defined N_PLAT_OS2)
		__lconvInfo.time_format = countryInfo.timeFormat;
      __lconvInfo.date_format = countryInfo.dateFormat;
      _fstrcpy(__lconvInfo.data_list_separator, countryInfo.dataListSeparator);
      _fstrcpy(__lconvInfo.date_separator, countryInfo.dateSeparator);
      _fstrcpy(__lconvInfo.time_separator, countryInfo.timeSeparator);
		_fstrcpy(__lconvInfo.am,localeInfo._am);
		_fstrcpy(__lconvInfo.pm,localeInfo._pm);

	} /*  End of 1st if */
#endif

#if (defined N_PLAT_UNIX || defined WIN32)
{
   extern LCONV __lconvInfo;
   int rcode;

	/*
	**  make setting __lconvInfo an atomic operation.
	*/
	MUTEX_LOCK(&_libnwlocale__lconvInfo_lock);

	rcode = _NWGetCountryInfo(&__lconvInfo);

	MUTEX_UNLOCK(&_libnwlocale__lconvInfo_lock);

#endif


    return ((LCONV N_FAR *) &__lconvInfo);

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/_llocale.c,v 1.1 1994/09/26 17:20:23 rebekah Exp $
*/
