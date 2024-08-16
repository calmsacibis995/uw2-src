/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CLASS_FPRI_H	/* wrapper symbol for kernel use */
#define _PROC_CLASS_FPRI_H	/* subject to change without notice */

#ident	"@(#)kern:proc/class/fpri.h	1.8"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <proc/priocntl.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/priocntl.h>

#endif /* _KERNEL_HEADERS */

/*
 * Fixed-priority dispatcher parameter table entry
 */
typedef struct	fpdpent {
	int	fp_globpri;	/* global (class independent) priority */
	clock_t	fp_quantum;	/* default quantum associated with this level */
} fpdpent_t;


/*
 * Fixed-priority class specific lwp structure
 */
typedef struct fplwp {
	clock_t		fp_pquantum;	/* time quantum given to this lwp */
	clock_t		fp_timeleft;	/* time remaining in lwp's quantum */
	short		fp_pri;		/* priority within fp class */
	ushort		fp_flags;	/* flags defined below */
	struct lwp	*fp_lwpp;	/* pointer to lwp struct */
	char		*fp_lstatp;	/* pointer to l_stat */
	int		*fp_lprip;	/* pointer to l_pri */
	uint		*fp_lflagp;	/* pointer to l_flag */
	uint		fp_prmptoffset;	/* offset of user-mode flag for preemption-inhibit */
	qpcparms_t 	fp_qpcparms;	/* buffer to store queued parameters */
} fplwp_t;


/* Flags */
#define FPBACKQ	0x0002		/* lwp goes to back of disp q when preempted */


#ifdef _KERNEL
/*
 * Kernel version of fixed-priority class specific parameter structure
 */
typedef struct	fpkparms {
	short	fp_pri;
	clock_t	fp_tqntm;	/* expressed in ticks */
} fpkparms_t;
#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_CLASS_FPRI_H */
