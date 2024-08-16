/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:util/calldemon.c	1.1"

#include <util/kdb/xdebug.h>

/*
 * The in_demon flag indicates whether the system is currently in a
 * kernel debugger.
 */
int in_demon = 0;

/*
 * void
 * call_demon(void)
 *
 * Calling/Exit State:
 *	Invokes the kernel debugger.
 *
 * Remarks:
 *	For the i386 implementation, in_demon is set/cleared by the debugger,
 *	rather than by call_demon().
 */
void
call_demon(void)
{
#ifndef NODEBUGGER
	extern int demon_call_type;

	if (cdebugger != nullsys) {
		/*
		 * Set a flag and generate a trap into the debugger.
		 * This is done, rather than calling the debugger directly,
		 * to get a trap frame saved.
		 */
		demon_call_type = DR_OTHER;
		asm(" int $3"); /* Force a debug trap */
	}
#endif
}
