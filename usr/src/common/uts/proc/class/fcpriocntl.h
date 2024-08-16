/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CLASS_FCPRIOCNTL_H	/* wrapper symbol for kernel use */
#define _PROC_CLASS_FCPRIOCNTL_H	/* subject to change without notice */

#ident	"@(#)kern:proc/class/fcpriocntl.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Fixed class specific structures for the priocntl system call.
 */

typedef struct fcparms {
	short	fc_uprilim;	/* user priority limit */
	short	fc_upri;	/* user priority */
	long	fc_timeleft;	/* time-left for this lwp */
	short	fc_cpupri;	/* the assigned cpu priority */
	short	fc_umdpri;	/* the computed user mode priority */
} fcparms_t;


typedef struct fcinfo {
	short	fc_maxupri;	/* configured limits of user priority range */
} fcinfo_t;

#define	FC_NOCHANGE	-1

/*
 * The following is used by the dispadmin(1M) command for
 * scheduler administration and is not for general use.
 */

typedef struct fcadmin {
	struct fcdpent	*fc_dpents;
	short		fc_ndpents;
	short		fc_cmd;
} fcadmin_t;

#define	FC_GETDPSIZE	1
#define	FC_GETDPTBL	2
#define	FC_SETDPTBL	3


#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_CLASS_FCPRIOCNTL_H */
