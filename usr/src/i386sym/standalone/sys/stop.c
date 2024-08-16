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

#ident	"@(#)stand:i386sym/standalone/sys/stop.c	1.1"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/saio.h>

extern void ssm_rtofw(void);
extern int printf(const char *, ...);

/*
 * void
 * _stop(char *)
 *	Halt the program that is executing, returning to the firmware monitor.
 *
 * Calling/Exit State:
 *	Closes all files listed as open in the file table.
 *
 *	Displays the massage passed in on the console if non-NULL.
 *
 *	Never returns from its call to ssm_rtofw().
 */
void
_stop(char *s)
{
	int i;

	for (i = 0; i < NFILES; i++)
		if (iob[i].i_flgs != 0)
			close(i);
	if (s) 
		printf("%s\n", s);
	ssm_rtofw();
}
