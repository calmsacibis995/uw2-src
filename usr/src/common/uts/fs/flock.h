/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_FLOCK_H	/* wrapper symbol for kernel use */
#define _FS_FLOCK_H	/* subject to change without notice */

#ident	"@(#)kern:fs/flock.h	1.9"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <fs/fcntl.h>	/* REQUIRED */
#include <fs/vnode.h>	/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/fcntl.h>	/* REQUIRED */
#include <sys/vnode.h>	/* REQUIRED */
#include <sys/ksynch.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define	INOFLCK		1	/* Vnode is locked when reclock() is called. */
#define	SETFLCK		2	/* Set a file lock. */
#define	SLPFLCK		4	/* Wait if blocked. */
#define	RCMDLCK		8	/* RGETLK/RSETLK/RSETLKW specified */

#define IGN_PID		(-1)	/* ignore epid when cleaning locks */

/* file locking structure (connected to vnode) */

#define l_end 		l_len
#define MAXEND  	017777777777

#ifdef _KERNEL
extern	lkinfo_t	sleeplcks_lkinfo;
#endif	/* _KERNEL */

typedef struct filock {
	struct	flock set;	/* contains type, start, and end */
	union	{
		int wakeflg;	/* for locks sleeping on this one */
		struct {
			long sysid;
			pid_t pid;
		} blk;			/* for sleeping locks only */
	}	stat;
#ifdef _KERNEL
	sv_t	frlock_sv;
#endif	/* _KERNEL */
	struct	filock *prev;
	struct	filock *next;
} filock_t;

#if defined(__STDC__)
int	reclock(vnode_t *, flock_t *, int, int, off_t, off_t);
int	chklock(vnode_t *, int, off_t, int, int, off_t);
void	cleanlocks(vnode_t *, pid_t, sysid_t);
void	frlck_init(void);
#else
int	reclock();
int	chklock();
void	cleanlocks();
void	frlck_init();
#endif

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_FLOCK_H */
