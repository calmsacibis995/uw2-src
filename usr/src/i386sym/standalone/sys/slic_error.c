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

#ident	"@(#)stand:i386sym/standalone/sys/slic_error.c	1.1"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/slic.h>
#include <sys/saio.h>

extern int printf(const char *, ...);

/*
 * void
 * slicerror(int, unchar, unchar, unchar, unchar)
 * 	Handle and display information about slic errors.
 *
 * Calling/Exit State:
 *	The caller has detected a SLIC error and passed
 *	its attributes to here in order to have a human
 *	readable summary of the error dumped to the console.
 *
 *	No return value.
 */
void
slicerror(int type, unchar com, unchar dest, unchar val, unchar stat)
{
	char *msg;

	switch (type) {
	case SL_PERROR:
		msg = "parity";
		break;
	case SL_DERROR:
		msg = "destination";
		break;
	case SL_NOTOK:
		msg = "command";
		break;
	}
	printf("\nSLIC %s err: com %x dest %x val %x stat %x\n",
		msg, com, dest, val, stat);
}
