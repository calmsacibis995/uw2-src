/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_HIER_H	/* wrapper symbol for kernel use */
#define _PROC_HIER_H	/* subject to change without notice */

#ident	"@(#)kern:proc/proc_hier.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ghier.h>		/* REQUIRED */
#include <util/ipl.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * This header file has all the hierarchy and minipl information 
 * pertaining to the process subsystem.  Note that all lock hierarchies in 
 * this file are expressed as a positive offset from a base hierarchy value
 * that is associated with the process subsystem.  Clearly, locks that can be 
 * held across subsystem boundaries need to be dealt with separately.
 * These "global" locks have their hierarchy values defined in <util/ghier.h>.
 *
 * The hierarchy values are checked amongst locks that have identical
 * minipl and hence the hierarchy namespace can be shared among locks that
 * have different minipls.
 *
 * The order of presentation in this file is based on <minipl, hierarchy>
 * pairs.
 * IPL values for locks with the lowest minipl are defined first.  Then
 * within each minipl, the lock hierarchy values are given in ascending
 * order.  This is a natural hierarchical presentation, with locks which
 * which need to be acquired first appearing first in this file.
 *
 * Generally, locks which are at the same minipl and the same hierarchy
 * value are disjoint.
 */

#ifdef _KERNEL

/*
 * Locks with minipl of PLMIN.
 */
#define	FD_HIER		PROC_HIER_BASE		/* fdt_mutex */
#define	FD_MINIPL	PLMIN

/*
 * Hierarchy values for System V shared memory IPC.
 */
#define SHM_PRI  	PRIMED   		/* sleep priority */
#define SHMDIR_HIER	PROC_HIER_BASE		/* shmdir_lck */
#define SHMID_HIER	(PROC_HIER_BASE + 1)	/* kshm_lck */

/*
 * Hierarchy values for System V message queue IPC.
 */
#define	MSGDIR_HIER	PROC_HIER_BASE		/* msgdir_mutex */
#define	MSGDS_HIER	(PROC_HIER_BASE + 1)	/* kmsq_mutex (per msqid_ds) */
#define	MSGRESV_HIER	(PROC_HIER_BASE + 2)	/* msgresv_mutex */

/*
 * Hierarchy values for System V semaphore IPC.
 */
#define	SEMDIR_HIER	PROC_HIER_BASE		/* semdir_mutex */
#define	SEMDS_HIER	(PROC_HIER_BASE + 1)	/* ksem_mutex (per semid_ds) */
#define	SEMUNP_HIER	(PROC_HIER_BASE + 2)	/* semunp_mutex */

/*
 * Locks acquired at PL6.  Currently PL_SESS is set to
 * PL6. XXX Have to revisit this file once we finalize PL_SESS.
 */
#define PL_SESS		PL6

#define PR_PSSHIER	PROC_HIER_BASE		/* p_sess_mutex */
#define PR_SSHIER	(PROC_HIER_BASE + 1)	/* s_mutex */
#define PR_SSMINIPL	PL_SESS

#define PIDHASH_HIER	(PROC_HIER_BASE + 2)	/* pidhash_mutex */

#define PROCLIST_HIER   (PROC_HIER_BASE + 2) 	/* proclist_mutex */
#define PROCLIST_MINIPL PL6

/*
 * Locks for /proc file system.
 */

/* prcommon structure. */
#define	PRC_HIER	(PROC_HIER_BASE + 7)
#define	PRC_MINIPL	PLHI

/* prnode. */
#define	PRN_HIER	(PROC_HIER_BASE + 8)
#define	PRN_MINIPL	PLHI


/* 
 * Locks with minipl of PLHI.
 */
#define PR_PHIER	PROC_HIER_BASE		/* p_mutex */
#define PR_PMINIPL	PLHI

#define	FILE_HIER	(PROC_HIER_BASE + 1)	/* f_mutex */
#define	FILE_MINIPL	PLHI

#define PR_CDHIER	(PROC_HIER_BASE + 1)	/* p_cdir_mutex */
#define PR_CDMINIPL	PLHI

#define UID_HIER	(PROC_HIER_BASE + 1)	/* ui_mutex */
#define UID_MINIPL	PLHI

#define	SQ_HIER		(PROC_HIER_BASE + 10)	/* Sync queue values. */
#define	SQ_MINIPL	PLHI

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_HIER_H */
