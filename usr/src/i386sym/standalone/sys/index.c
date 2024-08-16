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

#ident	"@(#)stand:i386sym/standalone/sys/index.c	1.1"

#include <sys/types.h>
#include <sys/param.h>

/* 
 * char *
 * index(char *, char)
 *	Find an occurrance of the specified character in the given string.
 *
 * Calling/Exit State:
 *	Assumes that the string argument is NULL terminated.
 *
 *	Returns NULL if the specified character is not found.
 *	Otherwise, return the address of the first occurance
 *	of that byte value within the string.
 */
char *
index(char *sp, char c)
{
	do {
		if (*sp == c)
			return(sp);
	} while (*sp++);
	return(NULL);
}
