/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/ctype.c	1.13"

#ifdef __STDC__
	#pragma weak setchrclass = _setchrclass
#endif
#include "synonyms.h"
#include <locale.h>

int
setchrclass(ccname)
const char *ccname;
{
	if (ccname == 0)
		ccname = "";
	if (setlocale(LC_CTYPE, ccname) == NULL)
		return(-1);
	return(0);
}
