/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kbdload:util.c	1.1.1.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/kbdload/util.c,v 1.1 91/02/28 17:38:39 ccs Exp $"

#include <stdio.h>
#include <sys/types.h>
#include <sys/kbd.h>

/*
 * Copy an ALIGNER to destination.
 */

cpalign(where, con)
	ALIGNER *where;
	ALIGNER con;
{
	*where = con;
}

/*
 * Copy a bunch of chars that are really a structure.  The target
 * is ALIGN aligned.
 */

cpchar(where, what, size)
	char *where, *what;
	int size;
{
	while (size--)
		*where++ = *what++;
}
