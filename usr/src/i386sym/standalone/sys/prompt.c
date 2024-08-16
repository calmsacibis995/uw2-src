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

#ident	"@(#)stand:i386sym/standalone/sys/prompt.c	1.1"

extern int printf(const char *, ...);
extern char *gets(char *);

/*
 * char *
 * prompt(char *)
 *	Prompt the console operator for a line of input.
 *
 * Calling/Exit State:
 *	"msg" contains a null-terminated string to be displayed 
 *	prior to retrieving input from the console.
 *
 *	Invokes gets() to get input from the console into the
 *	local buffer "buf".
 *
 *	Returns the address of "buf" to the caller, which contains
 *	the null-terminated string of input retrieved from the
 *	console, less tty line editing which may have occurred.
 *
 * Remarks:
 *	Always uses/returns the same buffer with new input.
 *	Therefore, the caller cannot save the address of the
 *	input and assume it will never change; they must be
 *	done with it or have copied it prior to invoking
 *	this function again.
 */	
char *
prompt(char *msg)
{
	static char buf[132];

	printf("%s", msg);
	(void)gets(buf);
	return (buf);
}
