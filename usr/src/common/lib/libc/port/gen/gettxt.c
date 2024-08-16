/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/gettxt.c	1.14"

#include "synonyms.h"
#include <ctype.h>
#include <string.h>
#include "_locale.h"
#include "pfmtm.h"

#ifdef __STDC__
	#pragma weak gettxt = _gettxt
#endif

char *
#ifdef __STDC__
gettxt(const char *ident, const char *defmsg)
#else
gettxt(ident, defmsg)const char *ident, *defmsg;
#endif
{
	char *p, cat[LC_NAMELEN];
	char *loc;
	int msgno;

	/*
	* Separate ident into catalog and number.
	* If anything is wrong with its shape, return _str_no_msg.
	*/
	p = &cat[0];
	while ((*p = *ident) != ':')
	{
		if (p >= &cat[LC_NAMELEN - 1] || *p == '\0')
			return (char *)_str_no_msg;
		p++;
		ident++;
	}
	*p = '\0';
	msgno = 0;
	while (isdigit(*++ident))	/* ignores overflows */
	{
		msgno *= 10;
		msgno += *ident - '0';
	}
	if (*ident != '\0')
	{
		if (*ident != ':')
			return (char *)_str_no_msg;
		defmsg = ident + 1;	/* permits pfmt-like packing */
	}
	return (char *)__gtxt(cat, msgno, defmsg);
}
