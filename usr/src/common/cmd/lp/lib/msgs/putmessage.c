/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/msgs/putmessage.c	1.7.5.3"
#ident	"$Header: $"
/* LINTLIBRARY */

#if	defined(__STDC__)
# include	<stdarg.h>
#else
# include	<varargs.h>
#endif

/* VARARGS */
#if	defined(__STDC__)
int putmessage(char * buf, short type, ... )
#else
int putmessage(buf, type, va_alist)
    char	*buf;
    short	type;
    va_dcl
#endif
{
    int		size;
    va_list	arg;
    int		_putmessage();

#if	defined(__STDC__)
    va_start(arg, type);
#else
    va_start(arg);
#endif

    size = _putmessage(buf, type, arg);
    va_end(arg);
    return(size);
}
