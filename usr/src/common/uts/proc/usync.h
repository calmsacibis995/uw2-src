/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_PROC_USYNC_H	/* wrapper symbol for kernel use */
#define	_PROC_USYNC_H	/* subject to change without notice */

#ident	"@(#)kern:proc/usync.h	1.15"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/list.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/list.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


#ifdef _KERNEL

#define MAXTIMEOUT	100000000
#define NSEC_PER_HZ	(1000 * 1000 * 1000 / HZ)

#define SQ_NHASH	64	/* number of global hash queues */
#define LSQ_NHASH	16	/* number of local hash queues */

/* Global hash function */
#define SQ_HASHFUNC(vp, off)	(((((int)vp) >> 4) + (((int)off) >> 4)) & (SQ_NHASH - 1))

/* Local hash function */
#define LSQ_HASHFUNC(mutexp)	((((int)mutexp) >> 4) & (LSQ_NHASH - 1))

#define  SEMA_HASHSIZE  19      /* Some prime number */
#define SEMA_HASHFUNC(a,b)      (((u_int)a + (u_int)b) % SEMA_HASHSIZE)

/*
 * Definition of a sync queue.
 */
typedef struct sq {
	list_t	sq_list;		/* must be first */
#define	sq_hnext	sq_list.flink
#define	sq_hprev	sq_list.rlink
	char	sq_flags;		/* flags related to a sync-queue */
	char	sq_uflag;		/* flag indicating whether
					 * some one is writing to user
					 * level flag or not */
	sv_t	sq_sv;			/* synchronization variable */
	int	sq_refcnt;		/* number of lwps blocked or
					 * intending to block */
	void	*sq_key1;		/* (key1, key2) pair is used */
	void	*sq_key2;		/* for hashing */
	vaddr_t	sq_mutexp;		/* virtual address of mutex *
					 * in process-private *
					 * address space */
	struct lwp *sq_head;		/* head of list of sleeping lwp's */
	struct lwp *sq_tail;		/* tail of list of sleeping lwp's */
} sq_t;

/* sq_flags */
#define SQGLOBAL	0x01

typedef struct sqlist {
	struct sq *sql_head;
	struct sq *sql_tail;
} sqlist_t;

struct lwp;
struct proc;

extern boolean_t as_getwchan(vaddr_t, uint_t *, void **, void **);
extern void sq_wakeup(struct lwp *, sq_t *);
extern void sq_delqueue(struct lwp *, sq_t *);
extern void sq_init(void);
extern void sq_exit(struct lwp *);
extern void sq_cancelblock(struct lwp *, sq_t *, int *);
extern int sq_unblock(lwp_t *, sq_t *, int *, int);
extern int sq_block(lwp_t *, sq_t *);
extern int sq_checksigs(struct lwp *, sq_t *);
extern int usync_unsleep(sq_t *, struct lwp *);
extern void sq_hashfree(struct proc *);

#define SQ_ONQUEUE(sq, lwp)	(((lwp)->l_flag & L_ONSQ) && \
				 ((lwp)->l_sq == (sq)))

/*
 * test for sleepers on queues
 */
#define QUEUEISEMPTY(sq)	(sq->sq_head == NULL)  /* true if queue empty */

#else	/* _KERNEL */

#ifdef	__STDC__
extern int prepblock(vaddr_t, char *, int);
extern int block(const timestruc_t *);
extern int unblock(vaddr_t, char *, int);
extern int rdblock(vaddr_t, char *);
extern void cancelblock(void);
#else	/* __STDC__ */
extern int prepblock();
extern int block();
extern int unblock();
extern void cancelblock();
#endif /* __STDC__ */

#endif /* _KERNEL */

typedef struct _lwp_sema {
	int count;
} _lwp_sema_t;

/*
 * Manifest constants used in system calls.
 */
/*
 * Input to unblock.
 */
#define	UNBLOCK_ALL	1	/* broadcast type unblock */
#define	UNBLOCK_READER	2	/* awaken 1 reader */
#define	UNBLOCK_WRITER	3	/* awaken 1 writers */
#define	UNBLOCK_ANY	4	/* awaken 1 of any type */
#define	UNBLOCK_RESET	16	/* reset lwp_mutex_t wanted flag if necessary */

/*
 * Output from the unblock system call stub.
 */
#define	UNBLOCK_OK		0
#define	UNBLOCK_NOTTHERE	1

/*
 * Input to prepblock.
 */
#define	PREPBLOCK_READER UNBLOCK_READER
#define	PREPBLOCK_WRITER UNBLOCK_WRITER

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_USYNC_H */
