/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CLASS_TS_H	/* wrapper symbol for kernel use */
#define _PROC_CLASS_TS_H	/* subject to change without notice */

#ident	"@(#)kern:proc/class/ts.h	1.14"
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

#else

#include <sys/list.h>

#endif /* _KERNEL_HEADERS */

/*
 * time-sharing dispatcher parameter table entry
 */
typedef struct tsdpent {
	int	ts_globpri;	/* global (class independent) priority */
	long	ts_quantum;	/* time quantum given to lwp's at this level */
	short	ts_tqexp;	/* ts_umdpri assigned when lwp at this level */
				/*   exceeds its time quantum */
	short	ts_slpret;	/* ts_umdpri assigned when lwp at this level */
				/*  returns to user mode after sleeping */
	short	ts_maxwait;	/* bumped to ts_lwait if more than ts_maxwait */
				/*  secs elapse before receiving full quantum */
	short	ts_lwait;	/* ts_umdpri assigned if ts_dispwait exceeds  */
				/*  ts_maxwait */				
} tsdpent_t;


#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * time-sharing class specific lwp structure
 */
typedef struct tslwp {
	list_t	ts_qlink;	/* forward/backward links on hash list */
#define ts_flink ts_qlink.flink	/* forward link */
#define ts_rlink ts_qlink.rlink	/* reverse link */
	long	ts_timeleft;	/* time remaining in lwp's quantum */
	short	ts_cpupri;	/* system controlled component of ts_umdpri */
	short	ts_uprilim;	/* user priority limit */
	short	ts_upri;	/* user priority */
	short	ts_umdpri;	/* user mode priority within ts class */
	char	ts_nice;	/* nice value for compatibility */
	unsigned char ts_flags;	/* flags defined below */
	short	ts_dispwait;	/* number of wall clock seconds since start */
				/*   of quantum (not reset upon preemption) */
	struct lwp *ts_lwpp;	/* pointer to class independent lwp */
	lwpstat_t *ts_lstatp;	/* pointer to l_stat */
	int	*ts_lprip;	/* pointer to l_pri */
	uint	*ts_lflagp;	/* pointer to l_flag */
	int	ts_sleepwait;	/* accumulated sleep time */
	qpcparms_t ts_qpcparms;	/* buffer to store queued parameters */
} tslwp_t;


/* flags */
#define	TSKPRI	0x01		/* lwp at kernel mode priority */
#define	TSBACKQ	0x02		/* lwp goes to back of disp q when preempted */
#define TSFORK	0x04		/* lwp has forked, do not restore full quantum */


/*
 * time-sharing lwp hash list
 */
typedef struct tshash {
	list_t		th_qlink;	/* forward/backward links */
#define th_first	th_qlink.flink	/* forward link */
#define th_last		th_qlink.rlink	/* backward link */
	lock_t		th_lock;	/* hash bucket spin lock */
} tshash_t;
 
#define	TSHASHSZ	64 

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_CLASS_TS_H */
