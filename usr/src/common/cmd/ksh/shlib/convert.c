/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ksh:shlib/convert.c	1.1.6.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/ksh/shlib/convert.c,v 1.1 91/02/28 17:41:49 ccs Exp $"

/*
 *   CONVERT.C
 *
 *
 *   LTOU (STR1, STR2)
 *        Copy STR1 to STR2, changing lower case to upper case.
 *
 *   UTOL (STR1, STR2)
 *        Copy STR1 to STR2, changing upper case to lower case.
 */

#include	<ctype.h>

/* 
 *   LTOU (STR1, STR2)
 *        unsigned char *STR1;
 *        unsigned char *STR2;
 *
 *   Copy STR1 to STR2, converting uppercase alphabetics to
 *   lowercase.  STR2 should be big enough to hold STR1.
 *
 *   STR1 and STR2 may point to the same place.
 *
 */

void ltou(str1,str2)
register unsigned char *str1,*str2;
{
	for(; *str1; str1++,str2++)
	{
		if(islower(*str1))
			*str2 = toupper(*str1);
		else
			*str2 = *str1;
	}
	*str2 = 0;
}


/*
 *   UTOL (STR1, STR2)
 *        unsigned char *STR1;
 *        unsigned char *STR2;
 *
 *   Copy STR1 to STR2, converting lowercase alphabetics to
 *   uppercase.  STR2 should be big enough to hold STR1.
 *
 *   STR1 and STR2 may point to the same place.
 *
 */

void utol(str1,str2)
register unsigned char *str1,*str2;
{
	for(; *str1; str1++,str2++)
	{
		if(isupper(*str1))
			*str2 = tolower(*str1);
		else
			*str2 = *str1;
	}
	*str2 = 0;
}

