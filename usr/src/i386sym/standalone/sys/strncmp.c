/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ident	"@(#)stand:i386sym/standalone/sys/strncmp.c	1.1"

/*
 * int
 * strncmp(char *, char *)
 * 	Compare two strings (at most n bytes) and return an indication 
 *	of which is greater.  
 *
 * Calling/Exit State:
 *	The two strings are NUL-terminated, but not necessarily the
 *	same length.
 *
 *	Returns zero if they are identical up to the n-th byte.  
 *	If a difference is found in the first n-bytes, return a 
 *	negative value if the differing character from s1 < s2, 
 *	and a positive value otherwise.
 */
int
strncmp(char *s1, char *s2, int n)
{

	while (--n >= 0 && *s1 == *s2++)
		if (*s1++ == '\0')
			return(0);
	return(n<0 ? 0 : *s1 - *--s2);
}
