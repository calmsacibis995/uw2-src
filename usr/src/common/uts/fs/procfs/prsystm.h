/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_PROCFS_PRSYSTM_H	/* wrapper symbol for kernel use */
#define _FS_PROCFS_PRSYSTM_H	/* subject to change without notice */

#ident	"@(#)kern:fs/procfs/prsystm.h	1.15"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <proc/regset.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/procfs.h>		/* SVR4.0COMPAT */
#include <sys/regset.h>		/* REQUIRED */

#else

#include <sys/procfs.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

struct lwp;
struct proc;
struct vnode;
struct psinfo;
struct user;
struct as;

extern void prinvalidate(struct proc *);
extern void prexit(struct proc *);
extern void prstopped(struct lwp *);
extern void prwakeup(struct vnode *);
extern void prdestroy(struct vnode *);
extern void prrdwrwait(struct vnode *);
extern void prgetpsinfo(struct proc *, struct psinfo *);
extern void prgetregs(struct user const *, gregset_t);
extern int prsetregs(struct user *, gregset_t);
extern void prgetfpregs(struct user const *, fpregset_t *);
extern int prsetfpregs(struct user *, fpregset_t const *);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_PROCFS_PRSYSTM_H */
