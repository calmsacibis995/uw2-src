/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:proc/execseg.c	1.7"
#ident	"$Header: $"

/* XENIX source support */

#include <proc/proc.h>
#include <proc/seg.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/plocal.h>

/*
 * int
 * execseg(char *uap, rval_t *rvp)
 *	XENIX system call
 *
 * Calling/Exit State:
 *	This system call returns a code selector pointing to the 
 *	memory region mapped by the user's data selector. This allows
 *	data segments to be executed.
 */

/* ARGSUSED */
int
execseg(char *uap, rval_t *rvp)
{
	struct segment_desc descr;

	/*
	 * Argument is Returned as a Far Pointer 
	 */
	rvp->r_val1 = 0;
	rvp->r_val2 = CSALIAS_SEL;

	descr = u.u_dt_infop[DT_LDT]->di_table[seltoi(USER_DS)];
	setdscracc1(&descr, UTEXT_ACC1);
	return set_dt_entry(CSALIAS_SEL, &descr);
}

/*
 * int
 * unexecseg(char *uap, rval_t *rvp)
 *	An alias selector is invalidated.
 *
 * Calling/Exit State:
 *
 */

/* ARGSUSED */
int
unexecseg(char *uap, rval_t *rvp)
{
	struct segment_desc descr;

	/* mark segment not-valid */
	setdscracc1(&descr, 0);
	return set_dt_entry(CSALIAS_SEL, &descr);
}
