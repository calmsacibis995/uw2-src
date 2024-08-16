/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:fs/profs/profs_mdep.c	1.1"
#ident	"$Header: $"

#include <fs/profs/prosrfs.h>
#include <svc/cpu.h>
#include <svc/fp.h>
#include <util/plocal.h>

/*
 * void
 * pro_get_chip_fpu(struct plocal *plp, procfile_t *procfilep)
 *
 *	Get the chip type and fpu kind.
 *
 * Calling/Exit State:
 *
 *	This function is called from the VOP proread().
 *	The calling LWP must hold the inode's rwlock in at least
 *	*shared* mode on entry; rwlock remains held on exit.
 *
 * Description:
 *
 *	This function gets the chip type and the fpu type
 *	and populates the appropriate members of the
 *	struct procfile, whose pointer is passed in as an
 *	argument.
 */
void
pro_get_chip_fpu(struct plocal *plp, procfile_t *procfilep)
{
	switch (plp->cpu_id) {
	case CPU_386:
		procfilep->chip = I_386; /* i386 */
		break;
	case CPU_486:
		procfilep->chip = I_486; /* i486 */
		break;
	case CPU_P5:
		procfilep->chip = I_586; /* P5 (Pentium) */
		break;
	default:
		procfilep->chip = 0;
		break;
	}

	switch (fp_kind) {
	case FP_287:
		procfilep->fpu = FPU_287;	/* 287 present */
		break;
	case FP_387:
		procfilep->fpu = FPU_387;	/* 387 present */
		break;
	default:
		procfilep->fpu = FPU_NONE;	/* no FPU present */
		break;
	}
}
