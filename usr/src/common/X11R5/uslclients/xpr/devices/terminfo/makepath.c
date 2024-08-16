#ident	"@(#)xpr:devices/terminfo/makepath.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "varargs.h"
#include "string.h"
#include "errno.h"

#ifndef MEMUTIL
extern char		*malloc();
#endif /* MEMUTIL */

/**
 ** makepath() - CREATE PATHNAME FROM COMPONENTS
 **/

/*VARARGS0*/
char			*makepath (va_alist)
	va_dcl
{
	register va_list	ap;

	register char		*component,
				*p,
				*q;

	register int		len;

	char			*ret;

	va_start (ap);
	for (len = 0; (component = va_arg(ap, char *)); )
		len += strlen(component) + 1;
	va_end (ap);

	if (!len) {
		errno = 0;
		return (0);
	}

	if (!(ret = malloc(len))) {
		errno = ENOMEM;
		return (0);
	}

	va_start (ap);
	for (p = ret; (component = va_arg(ap, char *)); ) {
		for (q = component; *q; )
			*p++ = *q++;
		*p++ = '/';
	}
	p[-1] = 0;
	va_end (ap);

	return (ret);
}
