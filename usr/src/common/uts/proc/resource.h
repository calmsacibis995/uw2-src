/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_RESOURCE_H	/* wrapper symbol for kernel use */
#define _PROC_RESOURCE_H	/* subject to change without notice */

#ident	"@(#)kern:proc/resource.h	1.10"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * Process priority specifications
 */

#define	PRIO_PROCESS	0
#define	PRIO_PGRP	1
#define	PRIO_USER	2


/*
 * Resource limits
 */

#define	RLIMIT_CPU	0		/* cpu time in milliseconds */
#define	RLIMIT_FSIZE	1		/* maximum file size */
#define	RLIMIT_DATA	2		/* data size */
#define	RLIMIT_STACK	3		/* stack size */
#define	RLIMIT_CORE	4		/* core file size */
#define RLIMIT_NOFILE	5		/* file descriptors */
#define RLIMIT_VMEM	6		/* maximum mapped memory */
#define RLIMIT_AS	RLIMIT_VMEM

#define	RLIM_NLIMITS	7		/* number of resource limits */

#define	RLIM_INFINITY	0x7fffffff

typedef unsigned long rlim_t;

/*
 * Structure used by the get/setrlimit(2) system calls.
 */
struct rlimit {
	rlim_t  rlim_cur;	/* current limit */
	rlim_t  rlim_max;	/* maximum value for rlim_cur */
};

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */
#include <util/ksynch.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */
#include <sys/ksynch.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Resource limits object:
 */
typedef struct rlimits {
	fspin_t	rl_mutex;			/* state lock for rl_ref */
	u_long	rl_ref;				/* reference count */
	struct	rlimit rl_limits[RLIM_NLIMITS];	/* resource limits */
} rlimits_t;

extern struct rlimits *sys_rlimits;	/* system resource limits */

extern rlimits_t *rlget(void);		/* get a new rlimits object */
extern void rlholdn(rlimits_t *, u_int);/* get n references to rlimits object */
extern void rlfreen(rlimits_t *, u_int);/* release n references */
extern void rlinstall(rlimits_t *);	/* install new rlimits for process */
extern int rlimit(int, rlim_t, rlim_t);	/* change the given resource limit */

#define	rlhold(rlimitsp) rlholdn((rlimitsp), 1)	/* hold one reference */
#define	rlfree(rlimitsp) rlfreen((rlimitsp), 1)	/* release one reference */

#else /* ! _KERNEL && ! _KMEMUSER */

#ifdef __STDC__
extern int getrlimit(int, struct rlimit *);
extern int setrlimit(int, const struct rlimit *);
#else
extern int getrlimit();
extern int setrlimit();
#endif

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_RESOURCE_H */
