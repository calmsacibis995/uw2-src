/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_GHIER_H	/* wrapper symbol for kernel use */
#define _UTIL_GHIER_H	/* subject to change without notice */

#ident	"@(#)kern:util/ghier.h	1.18"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ipl.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * This header file has all the global hierarchy information. This file
 * exports the namespace to be used by the various subsystems. This 
 * file also has the hierarchy values for all the "global" locks. Global
 * locks are those that can be unconditionally contested while holding locks
 * from other subsystem.
 */

#ifdef _KERNEL

/*
 * Following are the base hierarchy values to be used by the various 
 * subsystems.
 */

/*
 * Base hierarchy value for the base kernel (excluding drivers).
 */
#define KERNEL_HIER_BASE	33

/*
 * Base hierarchy value  used by the process management subsystem.
 */
#define PROC_HIER_BASE	50	

/*
 * Base hierarchy value  used by VM subsystem.
 */

#define VM_HIER_BASE	150

/*
 * Base hierarchy value  used by FS subsystem.
 */

#define FS_HIER_BASE	150

/*
 * Base hierarchy of locks in the STREAMS subsystem
 */
#define STR_HIER_BASE	42

/*
 * Base hierarchy of locks in the AUDIT subsystem
 */
#define ADT_HIER_BASE   43


/*
 * Hierarchy values and minipls of "global" locks.
 */

#define ENG_MUT_HIER	34	/* engine table mutex */
				/* must be less then PROC_HIER_BASE, as
				   processor_bind/processor_exbind hold
				   engine_tbl_mutex while calling prfind */
#define	ENG_MINIPL	PL6	/* so we can call prfind while holding
				   eng_tbl_mutex */

#define	HIER_RMALLOC	230	/* map structure (rmalloc) lock */

#define INPUT_HIER	234	/* input lock used in "dumb" console driver */

#define OUTPUT_HIER	235	/* output lock used by "dumb" console driver */

#define LBOLT_HIER	235	/* lbolt lock */

#define FLAVOR_HIER	235	/* flavors lock */

#define LWP_HIER	240	/* l_mutex lock */
#define LWP_MINIPL	PLHI

#define PR_SZHIER	(LWP_HIER + 1)	/* p_seize_mutex */
#define PR_SZMINIPL	PLHI

/*
 * Base hierarchy of locks in the MAC subsystem
 */
#define MAC_HIER_BASE 	(LWP_HIER + 1)

#define TSHASH_MUT_HIER	250	/* TS hash bucket spin lock */
#define TSHASH_MUT_MINIPL	PLHI

#define PANIC_HIER	251	/* panic lock */

#define CMNERR_HIER	252	/* common error lock */

#define PUTBUF_HIER	253	/* putbuf lock */

#define KDB_HIER	254	/* db_master lock : KDB */

#define XCALL_HIER	255	/* xcall mutex */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_GHIER_H */
