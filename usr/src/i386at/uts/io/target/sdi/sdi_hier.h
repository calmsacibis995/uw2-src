/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_TARGET_SDI_SDI_HIER_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SDI_SDI_HIER_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/sdi/sdi_hier.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define	TARGET_HIER_BASE	 5
#define	SDI_HIER_BASE		15
#define	HBA_HIER_BASE		20

#ifdef PDI_SVR42

#define	LKINFO_DECL(l,s,f)		int l
#define	LOCK_ALLOC(h,i,p,f)		((lock_t *)1)
#define	LOCK(lockp, ipl)		spl5s()
#define	UNLOCK(lockp, ipl)		splx(ipl)

#define	_SDI_LOCKED	0x1
#define	_SDI_SLEEPING	0x2
#define	SLEEP_ALLOC(h,i,f)		(int *)kmem_zalloc(sizeof(int), f)
#define	SLEEP_LOCK(lockp, pri)		{ \
				while ( *(lockp) & _SDI_LOCKED ) { \
					*(lockp) |= _SDI_SLEEPING; \
					sleep(lockp, pri); \
				} \
				*(lockp) |= _SDI_LOCKED; }

#define	SLEEP_UNLOCK(lockp)		{ \
				*(lockp) &= ~_SDI_LOCKED; \
				if (*(lockp) & _SDI_SLEEPING) { \
					*(lockp) &= ~_SDI_SLEEPING; \
					wakeup(lockp); \
				} }

#define	RW_WRLOCK(lockp, ipl)		LOCK(lockp, ipl)
#define	RW_RDLOCK(lockp, ipl)		LOCK(lockp, ipl)
#define	RW_UNLOCK(lockp, ipl)		UNLOCK(lockp, ipl)
#define	TRYLOCK(lockp, ipl)		LOCK(lockp, ipl)
#define	SV_ALLOC(f)			(int *)kmem_zalloc(1, f)

#define	SV_WAIT(svp, pri, lockp)	sleep(svp, pri)
#define	SV_BROADCAST(svp, flags)	wakeup(svp)

#define lock_t		int
#define rwlock_t	int
#define sleep_t		int
#define sv_t		int
#define	pl_t		int

#define	ITIMEOUT(f, a, t, p)	timeout(f, a, t)

#ifndef PLDISK
#define	PLDISK	5
#endif

extern int pldisk;
extern int pridisk;

#else	/* !PDI_SVR42 */
#define	ITIMEOUT(f, a, t, p) itimeout(f, a, t, p)
#endif /* PDI_SVR42 */
#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SDI_SDI_HIER_H */
