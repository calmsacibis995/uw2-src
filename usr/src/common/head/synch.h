/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#if !defined(_SYNCH_H)
#define _SYNCH_H
#ident	"@(#)head.usr:synch.h	1.5"
#ident	"$Header: $"
/*
 * This file defines user level synchronization objects,
 * functions, and macros
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <thread.h>
#include <lwpsynch.h>


/*
 * The following are used for static allocation
 */
#define DEFAULTMUTEX    {{0},USYNC_THREAD, {0},{0,0},0}
#define SHAREDMUTEX     {{0},USYNC_PROCESS,{0},{0,0},0}
#define DEFAULTCV 	{{0},USYNC_THREAD, 0, 0}
#define SHAREDCV        {{0},USYNC_PROCESS,0, 0}
#define DEFAULTSEMA     {0,USYNC_THREAD, 0,{0},{0}}
#define SHAREDSEMA 	{0,USYNC_PROCESS,0,{0},{0}}

/*
 * Tests on lock states.
 */
#define SEMA_HELD(x)            ((x)->s_count <= 0)
#define MUTEX_HELD(x)		(LOCK_HELD(&(x)->m_lmutex))


/*
 * The following values are assigned to the type field of all 
 * synchronization primitives.  
 */
#define	USYNC_THREAD	0	/* known only to threads in a process */
#define	USYNC_PROCESS	1	/* shared by different processes      */
#define USYNC_DESTROYED	2	/* the object has been "destroyed" */

typedef struct thrq_elt thrq_elt_t;

struct thrq_elt {
	thrq_elt_t *thrq_next;
	thrq_elt_t *thrq_prev;
};

/*
 * Mutexes
 */
#ifdef __STDC__
typedef volatile struct {
#else	/* __STDC__ */
typedef struct {
#endif	/* __STDC__ */
	lwp_mutex_t	m_lmutex;     /* lwp mutex lock */
	long		m_type;       /* type of mutex */
	lwp_mutex_t	m_sync_lock;  /* synchronization lock for sleep */
	thrq_elt_t 	m_sleepq;     /* sleep queue */
	long		filler;       /* not currently used */
} mutex_t;

/*
 * Condition variables
 */
#ifdef __STDC__
typedef volatile struct {
#else	/* __STDC__ */
typedef struct {
#endif	/* __STDC__ */
	lwp_cond_t	c_lcond;      /* lwp condition variable */
	long		c_type;       /* type of condition variable */
	thrq_elt_t 	*c_syncq;     /* sleep queue */
	lwp_mutex_t	c_sync_lock;  /* synchronization lock for sleep */
} cond_t;

/*
 * Semaphores
 */
#ifdef __STDC__
typedef volatile struct {
#else	/* __STDC__ */
typedef struct {
#endif	/* __STDC__ */
	mutex_t		s_mutex;
	cond_t		s_cond;
	short		s_count;
	short		s_wakecnt;
	int		s_type;
} sema_t;

/*
 * Read-write lock
 */
/*
 * The rwcv_t structure is used when the rwlock_t is of type USYNC_THREAD.
 * The rwcv_t structure is dynamically allocated, one per writer that has to 
 * wait, or one per group of readers that have to wait. This is a FIFO list 
 * pointed to at head by rw_cvqhead and at tail by rw_cvqtail. rwcv_rw 
 * indicates if rwcv_cond is used by a writer or by a group of readers. 
 * rwcv_wakeup indicates a rw_unlock() has been called that unblocks waiters of
 * this condvar. The waiters, when awakened, go back to sleep unless this flag 
 * is set. It may wake up by a signal or for other reasons, which are all 
 * ignored. When a writer is awakened, it deletes its rwcv_t from the list 
 * and deallocates it.  When a reader is awakened, it decrements 
 * rwcv_readerwanted and only the last such reader frees the rwcv_t structure.
 */
#ifdef __STDC__
typedef volatile struct rwcv rwcv_t;
#else /* __STDC__ */
typedef struct rwcv rwcv_t;
#endif /* __STDC__ */

struct rwcv {
	cond_t	rwcv_cond;		/* for waiters to block on */
	rwcv_t	*rwcv_next;		/* to rwcv_t used by next waiter */
	char	rwcv_rw;		/* type of request, reader or writer */
#define	RW_READER	(0)
#define	RW_WRITER	(1)
	char	rwcv_wakeup;		/* set by rw_unlock */
	short	rwcv_readerwanted;	/* number of readers blocked */
} ;

/*
 * read-write locks
 */
#ifdef __STDC__
typedef volatile struct rwlock rwlock_t;
#else /* __STDC__ */
typedef struct rwlock rwlock_t;
#endif /* __STDC__ */

/*
 * rwlock_t is defined for both types, USYNC_THREAD or USYNC_PROCESS.
 * Those members only for USYNC_PROCESS are: rw_lwpcond, rw_wrwakeup,
 * and rw_rdwakecnt; those only for USYNC_THREAD are: rw_cvqhead,
 * and rw_cvqtail.
 */
struct rwlock {
	mutex_t		rw_mutex;	/* controls access to this rwlock */
	lwp_cond_t	rw_lwpcond;	/* for USYNC_PROCESS only */
	int		rw_type;	/* USYNC_THREAD or USYNC_PROCESS */
	short		rw_readers;	/* count of readers holding the lock */
	char		rw_writer;	/* = 1 if held by a writer, or = 0 */
	char		rw_wrwakeup;	/* a wakeup is called for a writer */
	short		rw_writerwanted;/* writers waiting, USYNC_THREAD */
	short		rw_rdwakecnt;	/* readers awakened, USYNC_PROCESS */
	rwcv_t		*rw_cvqhead;	/* FIFO, need only head and tail */
	rwcv_t		*rw_cvqtail;
	long		pad[4];		/* pad out to 64 bytes */
} ;

/*
 * Recursive Mutexes
 */
#ifdef __STDC__
typedef volatile struct {
#else /* __STDC__ */
typedef struct {
#endif /* __STDC__ */
	mutex_t		rm_mutex;	/* mutex lock                        */
	pid_t		rm_pid;		/* pid of owner -- for USYNC_PROCESS */
	thread_t	rm_owner;	/* thread ID of owner of rmutex      */
	int		rm_depth;	/* number of lock calls made by owner*/
	long		filler;		/* not used for now                  */
} rmutex_t;

/*
 * spin locks - the spin_t structure is identical to mutex structure to
 * allow future interoperability.
 */

typedef mutex_t spin_t;

/*
 * synchronization barriers
 */
#ifdef __STDC__
typedef volatile struct barrier barrier_t;
typedef volatile struct barrier_spin barrier_spin_t;
#else /* __STDC__ */
typedef struct barrier barrier_t;
typedef struct barrier_spin barrier_spin_t;
#endif /* __STDC__ */

struct barrier {
	mutex_t		b_lock;		/* controls access to this barrier */
	int		b_type;		/* USYNC_PROCESS or USYNC_THREAD */
	unsigned int	b_count;	/* number of threads to synchronize  */
	unsigned int	b_waiting;	/* number of thread currently waiting */
	unsigned int	b_generation;	/* so we wake up for the right reason */
	cond_t		b_cond;		/* for waiting threads   */
} ;

struct barrier_spin {
	spin_t		bs_lock;	/* controls access to this barrier */
	long		bs_type;	/* USYNC_DESTROYED when destroyed    */
	unsigned int	bs_count;	/* number of threads to synchronize  */
	unsigned int	bs_waiting;	/* number of thread currently waiting*/
	unsigned int	bs_generation;	/* so we wake up for the right reason */
} ;

int             mutex_init(mutex_t *, int, void *);
int             mutex_lock(mutex_t *);
int             mutex_unlock(mutex_t *);
int             mutex_trylock(mutex_t *);
int             mutex_destroy(mutex_t *);
int             cond_init(cond_t *, int, void *);
int             cond_signal(cond_t *);
int             cond_broadcast(cond_t *);
int             cond_wait(cond_t *,mutex_t *);
int             cond_timedwait(cond_t *,mutex_t *,timestruc_t *);
int             cond_destroy(cond_t *);
int             sema_init(sema_t *, int, int, void *);
int             sema_wait(sema_t *);
int             sema_trywait(sema_t *);
int             sema_post(sema_t *);
int             sema_destroy(sema_t *);
int		rwlock_init(rwlock_t *, int, void *);
int		rw_rdlock(rwlock_t *);
int		rw_wrlock(rwlock_t *);
int		rw_unlock(rwlock_t *);
int		rw_tryrdlock(rwlock_t *);
int		rw_trywrlock(rwlock_t *);
int		rwlock_destroy(rwlock_t *);
int		rmutex_init(rmutex_t *, int, void *);
int		rmutex_lock(rmutex_t *);
int		rmutex_unlock(rmutex_t *);
int		rmutex_trylock(rmutex_t *);
int		rmutex_destroy(rmutex_t *);
int		_spin_init(spin_t *, void *);
void		_spin_unlock(spin_t *);
void		_spin_lock(spin_t *);
int		_spin_trylock(spin_t *);
int		_spin_destroy(spin_t *);
int		barrier_init(barrier_t *, int, int, void *);
int		barrier_wait(barrier_t *);
int		barrier_destroy(barrier_t *);
int		_barrier_spin_init(barrier_spin_t *, int, void *);
int		_barrier_spin(barrier_spin_t *);
int		_barrier_spin_destroy(barrier_spin_t *);

#ifdef __cplusplus
}
#endif

#endif /* _SYNCH_H */
