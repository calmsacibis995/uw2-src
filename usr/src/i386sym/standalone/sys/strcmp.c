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

#ident	"@(#)stand:i386sym/standalone/sys/strcmp.c	1.1"

/*
 * int
 * strcmp(char *, char *)
 * 	Compare two strings and return an indication of which is greater.  
 *
 * Calling/Exit State:
 *	The two strings are NUL-terminated, but not necessarily the
 *	same length.
 *
 *	Returns zero if they are identical,  If a difference is 
 *	found, return a negative value if the differing character
 *	from s1 < s2, and vice-versa.
 */
int
strcmp(char *s1, char *s2)
{

	while (*s1 == *s2++)
		if (*s1++ == '\0')
			return(0);
	return(*s1 - *--s2);
}