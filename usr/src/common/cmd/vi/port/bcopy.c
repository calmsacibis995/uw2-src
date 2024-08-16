/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)vi:port/bcopy.c	1.8.1.4"
#ident  "$Header: bcopy.c 1.2 91/06/27 $"
bcopy(from, to, count)
	register unsigned char *from, *to;
	register int count;
{
	while ((count--) > 0)
		*to++ = *from++;
}
