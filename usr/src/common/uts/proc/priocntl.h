/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_PRIOCNTL_H	/* wrapper symbol for kernel use */
#define _PROC_PRIOCNTL_H	/* subject to change without notice */

#ident	"@(#)kern:proc/priocntl.h	1.16"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <proc/procset.h> /* REQUIRED */

#else

#include <sys/types.h>	/* REQUIRED */
#include <sys/procset.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define	PC_VERSION	1	/* First version of priocntl */

#define priocntl(idtype, id, cmd, arg)\
	__priocntl(PC_VERSION, idtype, id, cmd, arg)

#define priocntlset(psp, cmd, arg)\
	__priocntlset(PC_VERSION, psp, cmd, arg)

#define priocntllist(lwpp, nlwp, cmd, arg)\
	_priocntllist(PC_VERSION, lwpp, nlwp, cmd, arg)

#ifdef __STDC__
extern long	__priocntl(int, idtype_t, id_t, int, void *);
extern long	__priocntlset(int, procset_t *, int, void *);
extern long	_priocntllist(int, lwpid_t *, int, int, void *);
#else
extern long	__priocntl(), __priocntlset(), _priocntllist();
#endif

/*
 * The following are the possible values of the command
 * argument for the priocntl system call.
 */

#define PC_GETCID	0	/* Get class ID */
#define	PC_GETCLINFO	1	/* Get info about a configured class */
#define	PC_SETPARMS	2	/* Set scheduling parameters */
#define	PC_GETPARMS	3	/* Get scheduling parameters */
#define PC_ADMIN	4	/* Scheduler administration (used by     */
				/*   dispadmin(1M), not for general use) */
#define	PC_PRMPTPOLL	5	/* poll for preemption */
#define	PC_YIELD	6	/* yield the cpu */

#define PC_SETAGEPARMS	7	/* Set Aging parameters */
#define PC_GETAGEPARMS	8	/* Get Aging parameters */

#define PC_CLNULL	-1

#define	PC_CLNMSZ	16
#define	PC_CLINFOSZ	(32 / sizeof(long))
#define	PC_CLPARMSZ	(32 / sizeof(long))
#define PRLST_LOCAL	32	/* size of local buffer in priocntllst() */

typedef struct pcinfo {
	id_t	pc_cid;			/* class id */
	char	pc_clname[PC_CLNMSZ];	/* class name */
	long	pc_clinfo[PC_CLINFOSZ];	/* class information */
} pcinfo_t;

typedef struct pcparms {
	id_t	pc_cid;			    /* process class */
	long	pc_clparms[PC_CLPARMSZ];    /* class specific parameters */
} pcparms_t;

typedef struct qpcparms {
	void	*qpc_argp;		/* pointer to class specific pointer */
	pcparms_t qpc_pcparms;		/* queued parameters */
	boolean_t qpc_classchg;		/* true when we need to change classes */
} qpcparms_t;

/*
 * Structure used to pass arguments to the setageparms() function
 * which is called indirectly through linear_search() or one_proc().
 */
typedef struct ageparms {
	size_t maxrss;
	clock_t et_age_interval;
	clock_t init_agequantum;
	clock_t min_agequantum;
	clock_t max_agequantum;
} ageparms_t;


/*
 * The following is used by the dispadmin(1M) command for
 * scheduler administration and is not for general use.
 */

typedef struct pcadmin {
	id_t	pc_cid;
	caddr_t	pc_cladmin;
} pcadmin_t;

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_PRIOCNTL_H */
