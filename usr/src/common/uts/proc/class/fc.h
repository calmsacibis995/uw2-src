/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CLASS_FC_H	/* wrapper symbol for kernel use */
#define _PROC_CLASS_FC_H	/* subject to change without notice */

#ident	"@(#)kern:proc/class/fc.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <proc/priocntl.h>
#include <util/list.h>
#include <util/ksynch.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/priocntl.h>
#include <sys/list.h>
#include <sys/ksynch.h>

#endif /* _KERNEL_HEADERS */

/*
 * fixed class dispatcher parameter table entry
 */
typedef struct fcdpent {
	int	fc_globpri;	/* global (class independent) priority */
	long	fc_quantum;	/* time quantum given to lwp's at this level */
} fcdpent_t;


#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * fixed class specific lwp structure
 */
typedef struct fclwp {
	list_t	fc_qlink;	/* forward/backward links on hash list */
#define fc_flink fc_qlink.flink	/* forward link */
#define fc_rlink fc_qlink.rlink	/* reverse link */
	long	fc_timeleft;	/* time remaining in lwp's quantum */
	short	fc_cpupri;	/* system controlled component of fc_umdpri */
	short	fc_uprilim;	/* user priority limit */
	short	fc_upri;	/* user priority */
	short	fc_umdpri;	/* user mode priority within fc class */
	char	fc_nice;	/* nice value for compatibility */
	unsigned char fc_flags;	/* flags defined below */
				/*   of quantum (not reset upon preemption) */
	struct lwp *fc_lwpp;	/* pointer to class independent lwp */
	lwpstat_t *fc_lstatp;	/* pointer to l_stat */
	int	*fc_lprip;	/* pointer to l_pri */
	uint	*fc_lflagp;	/* pointer to l_flag */
	qpcparms_t fc_qpcparms;	/* buffer to store queued parameters */
} fclwp_t;


/* flags */
#define	FCKPRI	0x01		/* lwp at kernel mode priority */
#define	FCBACKQ	0x02		/* lwp goes to back of disp q when preempted */
#define FCFORK	0x04		/* lwp has forked, do not restore full quantum */


/*
 * fixed class lwp hash list
 */
typedef struct fchash {
	list_t		fh_qlink;	/* forward/backward links */
#define fh_first	fh_qlink.flink	/* forward link */
#define fh_last		fh_qlink.rlink	/* backward link */
	lock_t		fh_lock;	/* hash bucket spin lock */
} fchash_t;
 
#define	FCHASHSZ	64 

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_CLASS_FC_H */
