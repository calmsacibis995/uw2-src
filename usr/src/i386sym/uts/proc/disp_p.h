/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_DISP_P_H	/* wrapper symbol for kernel use */
#define _PROC_DISP_P_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:proc/disp_p.h	1.9"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_KERNEL)

/*
 * "User" preemption point.
 */
#define	UPREEMPT()	if (l.eventflags & EVT_RUNRUN) { \
				CL_PREEMPT(u.u_lwpp, u.u_lwpp->l_cllwpp); \
			}

#define EVT_UPREEMPT	EVT_RUNRUN

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_DISP_P_H */