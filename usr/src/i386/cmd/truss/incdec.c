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

#ident	"@(#)truss:i386/cmd/truss/incdec.c	1.1.2.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/truss/incdec.c,v 1.1 91/02/28 20:15:15 ccs Exp $"

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include "pcontrol.h"
#include "ramdata.h"
#include "proto.h"

/*
 * Atomic operations for shared memory:
 * Increment or decrement a word in memory with one machine instruction.
 *
 * Although the window is small, encountering a forced process switch
 * in the middle of a read-alter-rewrite sequence could mess up the
 * multi-process coordination this supports.
 */

/*ARGSUSED*/
void
increment(p)
volatile int * p;
{
#if u3b2 || u3b5 || u3b15 || u3b1500
	asm("MOVW    (%ap),%r0");
	asm("INCW    (%r0)");
#elif mc68k
	asm("mov.l	8(%fp),%a0");
	asm("add.l	&1,(%a0)");
#elif i386
	asm("movl	8(%esp), %ecx");
	asm("lock");
	asm("incl	(%ecx)");
#else
	(*p)++;
#endif
}

/*ARGSUSED*/
void
decrement(p)
volatile int * p;
{
#if u3b2 || u3b5 || u3b15 || u3b1500
	asm("MOVW    (%ap),%r0");
	asm("DECW    (%r0)");
#elif mc68k
	asm("mov.l	8(%fp),%a0");
	asm("sub.l	&1,(%a0)");
#elif i386
	asm("movl	8(%esp), %ecx");
	asm("lock");
	asm("decl	(%ecx)");
#else
	--(*p);
#endif
}
