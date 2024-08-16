/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_loc_data.c	1.14"

#include "synonyms.h"
#include <locale.h>
#include "_locale.h"

const char _str_nlcolsp[] = "\n: ";

const char _str_uxlibc[] = "uxlibc";

const char _str_uxsyserr[] = "uxsyserr";

const char _str_no_msg[] = "Message not found!!\n";

const char _str_lc_all[] = "LC_ALL";

const char _str_lang[] = "LANG";

const char _str_c[] = "C";

const char *const _str_catname[LC_ALL] =
{
	"LC_CTYPE",	"LC_NUMERIC",	"LC_TIME",
	"LC_COLLATE",	"LC_MONETARY",	"LC_MESSAGES"
};

char *_locale[LC_ALL] =
{
	(char *)_str_c,	(char *)_str_c,	(char *)_str_c,
	(char *)_str_c,	(char *)_str_c,	(char *)_str_c
};

unsigned char _numeric[SZ_NUMERIC + SZ_GROUPING] = {'.'};

unsigned char *_grouping = &_numeric[SZ_NUMERIC];
