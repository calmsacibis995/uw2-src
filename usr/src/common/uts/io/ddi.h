/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DDI_H	/* wrapper symbol for kernel use */
#define _IO_DDI_H	/* subject to change without notice */

#ident	"@(#)kern:io/ddi.h	1.70"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * ddi.h -- the flag and function definitions needed by DDI-conforming
 * drivers.  This header file contains #undefs to undefine macros that
 * drivers would otherwise pick up in order that function definitions
 * may be used. Programmers should place the include of this header file
 * after any header files that define the macros #undef'ed or the code
 * may compile incorrectly.
 */

/*
 * Define _DDI so that interfaces defined in other header files, but not
 * available to drivers, can be hidden.
 */
#if !defined(_DDI) && !defined(_DDI_C)
#define _DDI
#endif

#ifdef _KERNEL_HEADERS

#include <fs/buf.h>		/* SVR4.0COMPAT */
#include <io/ddi_f.h>		/* PORTABILITY */
#include <io/uio.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#include <io/f_ddi.h>		/* SVR4.0COMPAT */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/buf.h>		/* SVR4.0COMPAT */
#include <sys/ddi_f.h>		/* PORTABILITY */
#include <sys/uio.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#include <sys/f_ddi.h>		/* SVR4.0COMPAT */

#else

#include <sys/buf.h>		/* SVR4.0COMPAT */
#include <sys/uio.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */


#ifdef _KERNEL

/*
 * Old sleep priorities for use with sleep().
 */

#define	PSWP	0
#define	PINOD	10
#define PSNDD	PINOD
#define PRIBIO	20
#define	PZERO	25
#define PMEM	0
#define	PPIPE	26
#define PVFS	27
#define	PWAIT	30
#define	PSLEP	39
#define	PUSER	60
#define	PIDLE	127
#define	PCATCH	0400

/*
 * The following macros designate a kernel parameter for drv_getparm
 * and drv_setparm. Implementation-specific parameter defines should
 * start at 100.
 */

#define TIME    	1
#define UPROCP  	2
#define PPGRP   	3
#define LBOLT   	4
#define SYSRINT 	5
#define SYSXINT 	6
#define SYSMINT 	7
#define SYSRAWC 	8
#define SYSCANC 	9
#define SYSOUTC 	10
#define PPID    	11
#define PSID    	12
#define UCRED   	13
#define DRV_MAXBIOSIZE	14
#define STRMSGSIZE	15

/*
 * The following declarations take the place of macros in param.h.
 */

#undef ptob
#undef btop
#undef btopr
#ifndef _DDI_C
#undef _PTOB
#undef _BTOP
#undef _BTOPR
#endif

#ifdef __STDC__
extern ulong_t ptob(ulong_t);
extern ulong_t btop(ulong_t);
extern ulong_t btopr(ulong_t);
#else
extern ulong_t ptob();
extern ulong_t btop();
extern ulong_t btopr();
#endif

/*
 * The following declarations take the place of macros in buf.h
 */

#undef bioreset
#undef bioerror
#undef geterror

#if defined(__STDC__)
extern void bioreset(struct buf *);
extern void bioerror(struct buf *, int);
extern int geterror(struct buf *);
#else
extern void bioreset();
extern void bioerror();
extern int geterror();
#endif

/*
 * The following declarations take the place of macros in sysmacros.h.
 * The undefs are for any case where a driver includes sysmacros.h,
 * even though DDI conforming drivers should not.
 */

#undef getemajor
#undef geteminor
#undef getmajor
#undef getminor
#undef makedevice
#ifndef _DDI_C
#undef _GETEMAJOR
#undef _GETEMINOR
#undef _GETMAJOR
#undef _GETMINOR
#undef _MAKEDEVICE
#endif

#ifdef __STDC__
extern major_t getemajor(dev_t);
extern minor_t geteminor(dev_t);
extern major_t getmajor(dev_t);
extern minor_t getminor(dev_t);
extern dev_t makedevice(major_t, minor_t);
extern int etoimajor(major_t);
extern int itoemajor(major_t, int);
#else
extern major_t getemajor();
extern minor_t geteminor();
extern major_t getmajor();
extern minor_t getminor();
extern dev_t makedevice();
extern int etoimajor();
extern int itoemajor();
#endif

/*
 * The following declarations take the place of macros in uio.h.
 */

#undef uiomove

#ifdef __STDC__
extern int uiomove(void *kernbuf, long n, uio_rw_t rw, uio_t *uiop);
#else
extern int uiomove();
#endif

/*
 * Declarations for DDI/DKI functions defined either in ddi.c or elsewhere.
 * Some of these duplicate declarations from other header files; they are
 * included here since drivers aren't supposed to #include those other files.
 */

#ifdef __STDC__
struct buf;
extern struct pollhead *phalloc(int);
extern int physiock(void (*)(), struct buf *, dev_t, int, daddr_t, uio_t *);
extern int drv_getparm(ulong_t parm, void *value_p);
extern int drv_priv(void *credp);
extern int drv_setparm(ulong_t, ulong_t);
extern clock_t drv_hztousec(clock_t);
extern clock_t drv_usectohz(clock_t);
extern void drv_usecwait(clock_t);
extern paddr_t vtop(caddr_t vaddr, void *procp);
struct page;
extern paddr_t pptophys(const struct page *);
extern ppid_t phystoppid(paddr_t);
extern ppid_t kvtoppid(caddr_t vaddr);
extern caddr_t physmap(paddr_t, ulong_t, uint_t);
extern void physmap_free(caddr_t, ulong_t, uint_t);
extern int bcmp(const void *, const void *, size_t);
extern void bcopy(const void *from, void *to, size_t bcount);
extern void bzero(void *buf, size_t bcount);
extern int copyin(const void *, void *, size_t);
extern int copyout(const void *, void *, size_t);
extern void delay(long);
extern toid_t dtimeout(void (*)(), void *, long, pl_t, processorid_t);
extern toid_t itimeout(void (*)(), void *, long, pl_t);
extern void untimeout(toid_t);
extern uchar_t inb(int);
extern ushort_t inw(int);
extern ulong_t inl(int);
extern void outb(int, uchar_t);
extern void outw(int, ushort_t);
extern void outl(int, ulong_t);
extern void ovbcopy(void *, void *, size_t);
extern void repinsb(int, uchar_t *, int);
extern void repinsw(int, ushort_t *, int);
extern void repinsd(int, ulong_t *, int);
extern void repoutsb(int, uchar_t *, int);
extern void repoutsw(int, ushort_t *, int);
extern void repoutsd(int, ulong_t *, int);
extern void *proc_ref(void);
extern boolean_t proc_valid(void *);
extern void proc_unref(void *);
extern int proc_signal(void *, int);
extern char *strcpy(char *, const char *);
extern char *strncpy(char *, const char *, size_t);
extern char *strcat(char *, const char *);
extern char *strncat(char *, const char *, size_t);
extern int strcmp(const char *, const char *);
extern int strlen(const char *);
extern int strncmp(const char *, const char *, size_t);
extern int sleep(caddr_t, int);
extern void wakeup(caddr_t);
#else
extern struct pollhead *phalloc();
extern int physiock();
extern int drv_getparm();
extern int drv_priv();
extern int drv_setparm();
extern clock_t drv_hztousec();
extern clock_t drv_usectohz();
extern void drv_usecwait();
extern paddr_t vtop();
extern paddr_t pptophys();
extern ppid_t phystoppid();
extern ppid_t kvtoppid();
extern caddr_t physmap();
extern void physmap_free();
extern int bcmp();
extern void bcopy();
extern void bzero();
extern int copyin();
extern int copyout();
extern void delay();
extern toid_t dtimeout();
extern toid_t itimeout();
extern void untimeout();
extern uchar_t inb();
extern ushort_t inw();
extern ulong_t inl();
extern void outb();
extern void outw();
extern void outl();
extern void ovbcopy();
extern void repinsb();
extern void repinsw();
extern void repinsd();
extern void repoutsb();
extern void repoutsw();
extern void repoutsd();
extern void *proc_ref();
extern void proc_unref();
extern int proc_signal();
extern char *strcpy();
extern char *strncpy();
extern char *strcat();
extern char *strncat();
extern int strcmp();
extern int strlen();
extern int strncmp();
extern int sleep();
extern void wakeup();
#endif

/*
 * int min(int, int);
 * int max(int, int);
 *
 * Define min() and max() as macros so that drivers will not pick up the
 * function versions used by the kernel, since they do unsigned comparisons.
 */
#define min(a, b)	((int)(a) < (int)(b) ? (a) : (b))
#define max(a, b)	((int)(a) < (int)(b) ? (b) : (a))


/*
 * For compatibility with older DDI/DKI versions, provide bool_t,
 * which is equivalent to boolean_t.
 */
typedef boolean_t bool_t;
#ifndef TRUE
#define TRUE	B_TRUE
#endif
#ifndef FALSE
#define FALSE	B_FALSE
#endif

/*
 * If DEBUG and SPINDEBUG are not defined we want the kernel to use
 * inline versions for regular spin locks and fast spin locks. However,
 * the drivers always must get function entry points.
 */

#if (! defined DEBUG && ! defined SPINDEBUG)

#undef lock_nodbg
#undef trylock_nodbg
#undef unlock_nodbg

#endif /* ! DEBUG && ! SPINDEBUG */

#ifdef DEBUG

#undef SLEEP_LOCKAVAIL
#undef SLEEP_LOCKOWNED

#ifdef __STDC__
extern boolean_t SLEEP_LOCKAVAIL(sleep_t *);
extern boolean_t SLEEP_LOCKOWNED(sleep_t *);
#else
extern boolean_t SLEEP_LOCKAVAIL();
extern boolean_t SLEEP_LOCKOWNED();
#endif

#endif /* DEBUG */

#undef KS_HOLD0LOCKS

#ifdef __STDC__
extern boolean_t KS_HOLD0LOCKS(void);
#else
extern boolean_t KS_HOLD0LOCKS();
#endif

#undef ATOMIC_INT_INIT
#undef ATOMIC_INT_READ
#undef ATOMIC_INT_WRITE
#undef ATOMIC_INT_INCR
#undef ATOMIC_INT_DECR
#undef ATOMIC_INT_ADD
#undef ATOMIC_INT_SUB
#ifndef _DDI_C
#undef _ATOMIC_INT_INIT
#undef _ATOMIC_INT_READ
#undef _ATOMIC_INT_WRITE
#undef _ATOMIC_INT_INCR
#undef _ATOMIC_INT_DECR
#undef _ATOMIC_INT_ADD
#undef _ATOMIC_INT_SUB
#endif

#ifdef __STDC__
extern void ATOMIC_INT_INIT(atomic_int_t *, int);
extern int ATOMIC_INT_READ(atomic_int_t *);
extern void ATOMIC_INT_WRITE(atomic_int_t *, int);
extern void ATOMIC_INT_INCR(atomic_int_t *);
extern boolean_t ATOMIC_INT_DECR(atomic_int_t *);
extern void ATOMIC_INT_ADD(atomic_int_t *, int);
extern void ATOMIC_INT_SUB(atomic_int_t *, int);
#else
extern void ATOMIC_INT_INIT();
extern int ATOMIC_INT_READ();
extern void ATOMIC_INT_WRITE();
extern void ATOMIC_INT_INCR();
extern boolean_t ATOMIC_INT_DECR();
extern void ATOMIC_INT_ADD();
extern void ATOMIC_INT_SUB();
#endif

#ifndef _DDI_C

/*
 * Kernel uses UNIPROC-optimized lock interfaces.
 * Driver must always use function entry points.
 */
#ifdef UNIPROC

#undef LOCK_ALLOC
#undef LOCK_DEALLOC
#undef RW_ALLOC
#undef RW_DEALLOC
#undef LOCK
#undef LOCK_OWNED
#undef TRYLOCK
#undef UNLOCK
#undef RW_RDLOCK
#undef RW_WRLOCK
#undef RW_TRYRDLOCK
#undef RW_TRYWRLOCK
#undef RW_UNLOCK
#undef RW_OWNED
#undef KSVFLAG

#if defined DEBUG || defined SPINDEBUG
#define LOCK_ALLOC(h, i, p, s)	lock_alloc_dbg(h, i, p, s, KSFLAGS)
#define LOCK_DEALLOC	lock_dealloc_dbg
#define LOCK_OWNED	lock_owned_dbg
#define RW_ALLOC(h, i, p, s)	rw_alloc_dbg(h, i, p, s, KSFLAGS)
#define RW_DEALLOC	rw_dealloc_dbg
#define LOCK(lockp, pl)	lock_dbg(lockp, pl, B_FALSE)
#define TRYLOCK		trylock_dbg
#define UNLOCK		unlock_dbg
#define RW_RDLOCK(lockp, pl)	rw_rdlock_dbg(lockp, pl, B_FALSE)
#define RW_WRLOCK(lockp, pl)	rw_wrlock_dbg(lockp, pl, B_FALSE)
#define RW_TRYRDLOCK	rw_tryrdlock_dbg
#define RW_TRYWRLOCK	rw_trywrlock_dbg
#define RW_UNLOCK	rw_unlock_dbg
#define RW_OWNED	rw_owned_dbg
#define KSVFLAG		KSVMPDEBUG
extern rwlock_t *rw_alloc_dbg(uchar_t, pl_t, lkinfo_t *, int, int);
extern lock_t *lock_alloc_dbg(uchar_t, pl_t, lkinfo_t *, int, int);
extern pl_t lock_dbg(lock_t *, pl_t, boolean_t);
extern pl_t rw_wrlock_dbg(rwlock_t *, pl_t, boolean_t);
extern pl_t rw_rdlock_dbg(rwlock_t *, pl_t, boolean_t);
#else /* DEBUG || SPINDEBUG */
#define LOCK_ALLOC	lock_alloc
#define LOCK_DEALLOC	lock_dealloc
#define LOCK_OWNED	lock_owned
#define RW_ALLOC	rw_alloc
#define RW_DEALLOC	rw_dealloc
#define LOCK		lock_nodbg
#define TRYLOCK		trylock_nodbg
#define UNLOCK		unlock_nodbg
#define RW_RDLOCK	rw_rdlock
#define RW_WRLOCK	rw_wrlock
#define RW_TRYRDLOCK	rw_tryrdlock
#define RW_TRYWRLOCK	rw_trywrlock
#define RW_UNLOCK	rw_unlock
#define RW_OWNED	rw_owned
#define KSVFLAG		KSVMPNODEBUG
extern lock_t *lock_alloc(uchar_t h, pl_t min, lkinfo_t *lk, int flags);
extern rwlock_t *rw_alloc(uchar_t h, pl_t min, lkinfo_t *lk, int flags);
extern pl_t lock_nodbg(lock_t *, pl_t);
extern pl_t rw_wrlock(rwlock_t *, pl_t);
extern pl_t rw_rdlock(rwlock_t *, pl_t);
#endif /* DEBUG || SPINDEBUG */

extern void LOCK_DEALLOC(lock_t *);
extern void RW_DEALLOC(rwlock_t *);
extern pl_t TRYLOCK(lock_t *, pl_t);
extern void UNLOCK(lock_t *, pl_t);
extern pl_t RW_TRYRDLOCK(rwlock_t *, pl_t);
extern pl_t RW_TRYWRLOCK(rwlock_t *, pl_t);
extern void RW_UNLOCK(rwlock_t *, pl_t);

#endif /* UNIPROC */

/* defined in ksynch.h */
#undef splx

/*
 * Note: the following is not a DDI/DKI interface, but it's defined as a
 * macro in ksynch.h, so we make sure to undef it here, just in case.
 */
#undef spl

/*
 * Note: the following are not DDI/DKI interfaces, but they're defined as
 * macros in ksynch.h, so we make sure to undef them here, just in case.
 */
#undef SLEEP_INIT
#undef SLEEP_DEINIT
#undef SLEEP_LOCKBLKD
#undef SLEEP_DISOWN
#undef SLEEP_LOCK_PRIVATE
#undef SLEEP_LOCK_RELLOCK
#undef SLEEP_UNSLEEP
#undef RWSLEEP_ALLOC
#undef RWSLEEP_INIT
#undef RWSLEEP_DEINIT
#undef RWSLEEP_DEALLOC
#undef RWSLEEP_RDLOCK
#undef RWSLEEP_RDLOCK_RELLOCK
#undef RWSLEEP_WRLOCK
#undef RWSLEEP_WRLOCK_RELLOCK
#undef RWSLEEP_TRYRDLOCK
#undef RWSLEEP_TRYWRLOCK
#undef RWSLEEP_UNLOCK
#undef RWSLEEP_RDAVAIL
#undef SV_BLKD
#undef SV_UNSLEEP
#undef IS_LOCKED
#undef KS_HOLD1LOCK
#undef FSPIN_INIT
#undef FSPIN_LOCK
#undef FSPIN_TRYLOCK
#undef FSPIN_UNLOCK
#undef FSPIN_OWNED
#undef LOCK_INIT
#undef LOCK_DEINIT
#undef LOCK_SH
#undef RW_INIT
#undef RW_DEINIT
#undef RW_RDLOCK_SH
#undef RW_WRLOCK_SH
#undef READ_SYNC
#undef WRITE_SYNC
#undef LOCK_PLMIN
#undef UNLOCK_PLMIN
#undef TRYLOCK_PLMIN
#undef LOCK_SH_PLMIN
#undef RW_RDLOCK_PLMIN
#undef RW_WRLOCK_PLMIN
#undef RW_UNLOCK_PLMIN

/*
 * Undefine IPL symbols from ipl.h so driver is forced to use ipl variables.
 */
#undef PL0
#undef PL1
#undef PL2
#undef PL3
#undef PL4
#undef PL5
#undef PL6
#undef PL7
#undef PLHI
#undef PLTTY
#undef PLSTR
#undef PLDISK
#undef PLTIMEOUT
#undef PLBASE
#undef PLMIN
#undef PLXCALL
#undef PLMAX
#undef INVPL

/*
 * Undefine priority symbols from param.h so driver is forced to use
 * priority variables.
 */
#undef PRIMEM
#undef PRINOD
#undef PRIBUF
#undef PRIMED
#undef PRIPIPE
#undef PRIVFS
#undef PRIWAIT
#undef PRIREMOTE
#undef PRISLEP
#undef PRIZERO
#undef PRIDLE

#endif /* !_DDI_C */

/* spl routines */

#ifdef __STDC__
extern void splx(pl_t);
extern pl_t spltimeout(void);
extern pl_t spldisk(void);
extern pl_t splstr(void);
extern pl_t spltty(void);
extern pl_t splhi(void);
extern pl_t spl0(void);
extern pl_t spl7(void);

#else

extern pl_t spltimeout();
extern pl_t spldisk();
extern pl_t splstr();
extern pl_t spltty();
extern pl_t splhi();
extern pl_t spl0();
extern pl_t spl7();
extern void splx();
#endif

#define splbase spl0

/*
 * Basic constants
 */
#ifndef NBPSCTR
#define NBPSCTR	512
#endif
#ifndef NULL
#define NULL	0
#endif
#ifndef NOPAGE
#define NOPAGE	((ppid_t)-1)
#endif

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_DDI_H */
